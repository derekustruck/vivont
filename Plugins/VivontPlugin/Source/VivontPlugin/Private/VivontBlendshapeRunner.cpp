// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontBlendshapeRunner.h"
#include "VivontAudioUtils.h"
#include "VivontSettings.h"
#include "VivontLog.h"

#include "NNE.h"
#include "NNEModelData.h"
#include "NNERuntimeCPU.h"
#include "NNERuntimeGPU.h"
#include "NNERuntimeRunSync.h"
#include "NNETypes.h"
#include "NNEStatus.h"

#include "Async/Async.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

void UVivontBlendshapeRunner::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogVivont, Log, TEXT("VivontBlendshapeRunner initialized. Model loads lazily on first SubmitAudio."));
}

void UVivontBlendshapeRunner::Deinitialize()
{
    FScopeLock Lock(&InferenceCriticalSection);
    ModelInstance.Reset();
    ModelData = nullptr;
    bModelReady = false;
    Super::Deinitialize();
}

bool UVivontBlendshapeRunner::EnsureModelLoaded(FString& OutError)
{
    if (bModelReady)
    {
        return true;
    }
    if (bLoadAttempted)
    {
        OutError = TEXT("Inference model previously failed to load (see earlier log).");
        return false;
    }
    bLoadAttempted = true;

    const UVivontSettings* Settings = GetDefault<UVivontSettings>();
    FString ManifestPath = Settings ? Settings->InferenceManifestPath : FString();
    if (ManifestPath.IsEmpty())
    {
        OutError = TEXT("InferenceManifestPath is not set in Vivont plugin settings.");
        return false;
    }
    if (FPaths::IsRelative(ManifestPath))
    {
        // Resolve relative to the plugin's own directory so the model ships with
        // the plugin (self-contained), not the host project. Falls back to the
        // project dir if the plugin can't be located.
        FString BaseDir = FPaths::ProjectDir();
        if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("VivontPlugin")))
        {
            BaseDir = Plugin->GetBaseDir();
        }
        ManifestPath = FPaths::Combine(BaseDir, ManifestPath);
    }

    if (!FVivontInferenceManifest::LoadFromFile(ManifestPath, Manifest, OutError))
    {
        return false;
    }

    TArray<uint8> ModelBytes;
    if (!FFileHelper::LoadFileToArray(ModelBytes, *Manifest.ModelFilePath))
    {
        OutError = FString::Printf(TEXT("Could not read ONNX model file: %s"), *Manifest.ModelFilePath);
        return false;
    }

    ModelData = NewObject<UNNEModelData>(this);
    ModelData->Init(TEXT("onnx"), ModelBytes);

    const bool bWantGpu = !Settings || Settings->bUseGPUInference;
    TSharedPtr<UE::NNE::IModelInstanceRunSync> Instance;
    FString Backend;

    if (bWantGpu)
    {
        TWeakInterfacePtr<INNERuntimeGPU> Runtime = UE::NNE::GetRuntime<INNERuntimeGPU>(TEXT("NNERuntimeORTDml"));
        if (Runtime.IsValid())
        {
            if (TSharedPtr<UE::NNE::IModelGPU> Model = Runtime->CreateModelGPU(ModelData))
            {
                Instance = Model->CreateModelInstanceGPU();
                Backend = TEXT("GPU/DirectML (NNERuntimeORTDml)");
            }
        }
        else
        {
            UE_LOG(LogVivont, Warning, TEXT("VivontBlendshapeRunner: GPU runtime NNERuntimeORTDml unavailable; falling back to CPU."));
        }
    }

    if (!Instance)
    {
        TWeakInterfacePtr<INNERuntimeCPU> Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(TEXT("NNERuntimeORTCpu"));
        if (Runtime.IsValid())
        {
            if (TSharedPtr<UE::NNE::IModelCPU> Model = Runtime->CreateModelCPU(ModelData))
            {
                Instance = Model->CreateModelInstanceCPU();
                Backend = TEXT("CPU (NNERuntimeORTCpu)");
            }
        }
    }

    if (!Instance.IsValid())
    {
        OutError = TEXT("No NNE runtime could create the model. Ensure the NNERuntimeORT plugin is enabled.");
        return false;
    }

    // Fixed input shape [1, AudioLen] — raw mono waveform window for all current model families.
    const TArray<uint32> InputDims = { 1u, static_cast<uint32>(Manifest.AudioLen) };
    const UE::NNE::FTensorShape InputShape = UE::NNE::FTensorShape::Make(InputDims);
    if (Instance->SetInputTensorShapes({ InputShape }) != UE::NNE::EResultStatus::Ok)
    {
        OutError = TEXT("NNE SetInputTensorShapes failed for the model input.");
        return false;
    }

    ModelInstance = Instance;
    bModelReady = true;

    UE_LOG(LogVivont, Log, TEXT("VivontBlendshapeRunner: model loaded on %s. AudioLen=%d, SR=%d, fps=%d, curves=%d, seqLen=%d, audioNorm=%s."),
        *Backend, Manifest.AudioLen, Manifest.AudioSampleRate, Manifest.Fps, Manifest.OutDim(),
        Manifest.SequenceLen, *Manifest.AudioNormalization);
    return true;
}

void UVivontBlendshapeRunner::SubmitAudio(const TArray<uint8>& AudioData, int32 SourceSampleRate)
{
    if (AudioData.Num() == 0)
    {
        OnInferenceError.Broadcast(TEXT("SubmitAudio: empty audio data."));
        return;
    }

    FString Error;
    if (!EnsureModelLoaded(Error))
    {
        UE_LOG(LogVivont, Error, TEXT("VivontBlendshapeRunner: %s"), *Error);
        OnInferenceError.Broadcast(Error);
        return;
    }

    TArray<float> Mono = FVivontAudioUtils::PrepareMonoFloat(AudioData, SourceSampleRate, Manifest.AudioSampleRate);
    if (Mono.Num() == 0)
    {
        OnInferenceError.Broadcast(TEXT("SubmitAudio: audio decode produced no samples."));
        return;
    }

    TWeakObjectPtr<UVivontBlendshapeRunner> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, MonoAudio = MoveTemp(Mono)]() mutable
    {
        if (UVivontBlendshapeRunner* Self = WeakThis.Get())
        {
            Self->RunInference(MoveTemp(MonoAudio));
        }
    });
}

void UVivontBlendshapeRunner::RunInference(TArray<float> MonoAudio)
{
    FScopeLock Lock(&InferenceCriticalSection);

    if (!ModelInstance.IsValid())
    {
        BroadcastErrorGameThread(TEXT("RunInference: model instance is invalid."));
        return;
    }

    const int32 AudioLen = Manifest.AudioLen;
    const int32 Hop = Manifest.HopSamples();
    const int32 OutDim = Manifest.OutDim();
    const int32 Total = MonoAudio.Num();

    if (Total == 0 || AudioLen <= 0 || OutDim <= 0)
    {
        BroadcastErrorGameThread(TEXT("RunInference: invalid audio or model dimensions."));
        return;
    }

    TArray<float> InputBuffer;
    InputBuffer.SetNumZeroed(AudioLen);
    TArray<float> OutputBuffer;
    OutputBuffer.SetNumZeroed(Manifest.OutputElementCount());

    // The model emits FramesPerWindow chronological frames per inference (output (1, FramesPerWindow,
    // OutDim)). We emit all of them and advance the window by FramesPerWindow frames, so the model runs
    // once per FramesPerWindow output frames instead of once per frame -- ~FramesPerWindow x fewer heavy
    // SSL inferences per utterance. For FramesPerWindow == 1 (legacy FastCausalCNN) this is the original
    // one-frame-per-window behavior. Emitted blocks tile the timeline contiguously (each window's frames
    // are centered on the window center; advancing FramesPerWindow frames makes consecutive blocks abut).
    const int32 FramesPerWindow = Manifest.FramesPerWindow();
    const int32 WindowHop = FMath::Max(1, FramesPerWindow * Hop);

    const UE::NNE::FTensorBindingCPU InputBinding{ InputBuffer.GetData(), static_cast<uint64>(InputBuffer.Num()) * sizeof(float) };
    const UE::NNE::FTensorBindingCPU OutputBinding{ OutputBuffer.GetData(), static_cast<uint64>(OutputBuffer.Num()) * sizeof(float) };

    TArray<FBlendShapeFrame> Frames;
    Frames.Reserve(Total / FMath::Max(1, Hop) + FramesPerWindow);

    for (int32 Start = 0; Start < Total; Start += WindowHop)
    {
        const int32 Count = FMath::Min(AudioLen, Total - Start);

        // Window into a fixed AudioLen buffer (zero-padded) and apply scalar normalization.
        for (int32 i = 0; i < AudioLen; ++i)
        {
            const float Sample = (i < Count) ? MonoAudio[Start + i] : 0.0f;
            InputBuffer[i] = (Sample - Manifest.AudioMean) / Manifest.AudioStd;
        }

        if (ModelInstance->RunSync({ InputBinding }, { OutputBinding }) != UE::NNE::EResultStatus::Ok)
        {
            BroadcastErrorGameThread(TEXT("RunInference: NNE RunSync failed."));
            return;
        }

        // Denormalize each of this window's FramesPerWindow frames (chronological order) into a frame.
        for (int32 f = 0; f < FramesPerWindow; ++f)
        {
            const int32 FrameBase = f * OutDim;
            TArray<float> Values;
            Values.SetNumUninitialized(OutDim);
            for (int32 c = 0; c < OutDim; ++c)
            {
                Values[c] = OutputBuffer[FrameBase + c] * Manifest.BlendStd[c] + Manifest.BlendMean[c];
            }
            Frames.Add(FBlendShapeFrame(MoveTemp(Values)));
        }
    }

    UE_LOG(LogVivont, Log, TEXT("VivontBlendshapeRunner: inferred %d frames from %d samples (%.2fs @ %d Hz)."),
        Frames.Num(), Total, static_cast<float>(Total) / FMath::Max(1, Manifest.AudioSampleRate), Manifest.AudioSampleRate);

    BroadcastResultGameThread(MoveTemp(Frames));
}

void UVivontBlendshapeRunner::BroadcastResultGameThread(TArray<FBlendShapeFrame> Frames)
{
    TWeakObjectPtr<UVivontBlendshapeRunner> WeakThis(this);
    AsyncTask(ENamedThreads::GameThread, [WeakThis, ResultFrames = MoveTemp(Frames)]() mutable
    {
        if (UVivontBlendshapeRunner* Self = WeakThis.Get())
        {
            Self->OnBlendShapesReceived.Broadcast(true, ResultFrames);
        }
    });
}

void UVivontBlendshapeRunner::BroadcastErrorGameThread(FString Message)
{
    TWeakObjectPtr<UVivontBlendshapeRunner> WeakThis(this);
    AsyncTask(ENamedThreads::GameThread, [WeakThis, ErrorMessage = MoveTemp(Message)]() mutable
    {
        if (UVivontBlendshapeRunner* Self = WeakThis.Get())
        {
            UE_LOG(LogVivont, Error, TEXT("VivontBlendshapeRunner: %s"), *ErrorMessage);
            Self->OnInferenceError.Broadcast(ErrorMessage);
        }
    });
}
