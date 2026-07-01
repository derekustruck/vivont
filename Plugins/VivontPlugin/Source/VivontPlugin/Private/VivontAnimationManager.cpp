// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontAnimationManager.h"
#include "Modules/ModuleManager.h" // Added for FModuleManager
#include "VivontPlugin.h" // Added for FVivontPluginModule
#include "VivontAudioManager.h"
#include "SimpleOpenAIClient.h" // Added full include for SimpleOpenAIClient
#include "Containers/Ticker.h" // Added for FTSTicker
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "Sound/SoundWave.h"
#include "AudioDevice.h"
#include "DSP/Dsp.h"
#include "TimerManager.h"
#include "HAL/ThreadSafeCounter.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VivontBlendshapeRunner.h"   // In-process inference (replaces HTTP UVivontAPIClient)
#include "VivontBlendshapeTypes.h"    // FBlendShapeFrame used in delegates/members
#include "VivontAudioManager.h" // Added for UVivontAudioSubsystem
#include "Subsystems/SubsystemCollection.h" // Required for InitializeDependency/GetSubsystem
#include "Templates/SharedPointer.h"
#include "Containers/Ticker.h"
#include "VivontLog.h"
#include "Sound/SoundWaveProcedural.h" // Added for procedural sound wave
#include "Kismet/GameplayStatics.h"
#include "VivontLiveLinkSource.h" // Need this for the TODO in AnimationTick
#include "VivontPlugin.h" // Need this for GetLiveLinkSource() example
#include "Features/IModularFeatures.h" // Required for ILiveLinkClient
#include "ILiveLinkClient.h" // Required for ILiveLinkClient
#include "Misc/Paths.h" // Added for FPaths
#include "HAL/PlatformFileManager.h" // Added for FPlatformFileManager
#include "Misc/FileHelper.h" // Added for FFileHelper

// Removed FDefaultAnimationRunnable class definition
// Removed FAudioRecordingRunnable class definition (obsolete, using AudioSubsystem now)

/* Constructor removed.
   Initialization should be performed in Initialize() instead.
*/

void UVivontAnimationManager::Initialize(FSubsystemCollectionBase& Collection)
{
    // Declare dependencies FIRST to ensure they are initialized before this subsystem
    Collection.InitializeDependency<UVivontBlendshapeRunner>();
    Collection.InitializeDependency<USimpleOpenAIClient>();
    Collection.InitializeDependency<UVivontAudioSubsystem>();

    // Call Super::Initialize AFTER declaring dependencies
    Super::Initialize(Collection);

    // Get settings
    Settings = GetMutableDefault<UVivontSettings>();

    // Retrieve dependent subsystems using GEngine (now safe after InitializeDependency)
    Runner = GEngine->GetEngineSubsystem<UVivontBlendshapeRunner>();
    if (!Runner) // Correct check for TObjectPtr
    {
        VIVONT_ERROR("AnimManager Init", "Failed to retrieve VivontBlendshapeRunner subsystem!");
    }
    else
    {
        VIVONT_LOG(Log, TEXT("Successfully retrieved VivontBlendshapeRunner subsystem."));
    }
    OpenAIAPI = GEngine->GetEngineSubsystem<USimpleOpenAIClient>();
    if (!OpenAIAPI) // Correct check for TObjectPtr
    {
        VIVONT_ERROR("AnimManager Init", "Failed to retrieve SimpleOpenAIClient subsystem!");
    }
    else
    {
        VIVONT_LOG(Log, TEXT("Successfully retrieved SimpleOpenAIClient subsystem."));
    }
    AudioSubsystem = GEngine->GetEngineSubsystem<UVivontAudioSubsystem>();
    if (!AudioSubsystem) // Correct check for TObjectPtr
    {
        VIVONT_ERROR("AnimManager Init", "Failed to retrieve VivontAudioSubsystem!");
    }
    else
    {
        VIVONT_LOG(Log, TEXT("Successfully retrieved VivontAudioSubsystem."));
    }

    // Connect delegates
    VIVONT_LOG(Log, TEXT("Initialize: Attempting to bind Runner delegates..."));
    if (Runner) // Correct check for TObjectPtr
    {
        Runner->OnBlendShapesReceived.AddDynamic(this, &UVivontAnimationManager::OnBlendShapesReceived);
        Runner->OnInferenceError.AddDynamic(this, &UVivontAnimationManager::OnAPIErrorReceived);
    }

    VIVONT_LOG(Log, TEXT("Initialize: Attempting to bind OpenAIAPI delegates..."));
    if (OpenAIAPI) // Correct check for TObjectPtr
    {
        VIVONT_LOG(Log, TEXT("Initialize: OpenAIAPI pointer is VALID (%p). Binding delegates..."), OpenAIAPI.Get());
        OpenAIAPI->OnAudioReceived.AddDynamic(this, &UVivontAnimationManager::OnAudioGenerated);
        OpenAIAPI->OnTextResponse.AddDynamic(this, &UVivontAnimationManager::OnTextResponseReceived);
        OpenAIAPI->OnErrorReceived.AddDynamic(this, &UVivontAnimationManager::OnAPIErrorReceived);
    }
    else
    {
        VIVONT_ERROR("Initialize", "OpenAIAPI pointer is NULL during delegate binding!");
    }

    // Register animation tick
    AnimationTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UVivontAnimationManager::AnimationTick), 1.0f / 60.0f); // Assuming 60fps for animation updates
}

void UVivontAnimationManager::Deinitialize()
{
    // Stop any active animations
    this->StopAnimation(); // Use this-> explicitly for clarity

    // Unregister animation tick
    FTSTicker::GetCoreTicker().RemoveTicker(AnimationTickHandle);

    // Disconnect delegates safely
    if (Runner) // Correct check for TObjectPtr
    {
        if (Runner->OnBlendShapesReceived.IsBound())
        {
             Runner->OnBlendShapesReceived.RemoveDynamic(this, &UVivontAnimationManager::OnBlendShapesReceived);
        }
       if (Runner->OnInferenceError.IsBound())
       {
            Runner->OnInferenceError.RemoveDynamic(this, &UVivontAnimationManager::OnAPIErrorReceived);
       }
    }

    if (OpenAIAPI) // Correct check for TObjectPtr
    {
        if (OpenAIAPI->OnAudioReceived.IsBound())
        {
            OpenAIAPI->OnAudioReceived.RemoveDynamic(this, &UVivontAnimationManager::OnAudioGenerated);
        }
        if (OpenAIAPI->OnTextResponse.IsBound())
        {
            OpenAIAPI->OnTextResponse.RemoveDynamic(this, &UVivontAnimationManager::OnTextResponseReceived);
        }
        if (OpenAIAPI->OnErrorReceived.IsBound())
        {
            OpenAIAPI->OnErrorReceived.RemoveDynamic(this, &UVivontAnimationManager::OnAPIErrorReceived);
        }
    }

    Super::Deinitialize();
}

// Removed InitializeLiveLinkFace method definition
// Removed StartDefaultAnimation method definition
// Removed StopDefaultAnimation method definition

// Corrected Signature: Removed WorldContextObject parameter
void UVivontAnimationManager::ProcessAudioData(const TArray<uint8>& AudioData)
{
    if (AudioData.Num() == 0)
    {
        OnAnimationError.Broadcast(TEXT("ProcessAudioData: Empty audio data received."));
        return;
    }

    // Stop any active animations/audio first
    this->StopAnimation(); // Use this-> explicitly

    // Store audio data temporarily while waiting for blendshapes
    this->PendingAudioData = AudioData; // Use this-> explicitly
    UE_LOG(LogVivont, Log, TEXT("ProcessAudioData: Stored %d bytes of audio data. Requesting blendshapes..."), AudioData.Num());

    // Send audio data to the in-process inference runner for blendshape generation.
    // OpenAI TTS audio is 24 kHz mono 16-bit PCM.
    if (this->Runner) // Use this-> explicitly and correct check
    {
        this->Runner->SubmitAudio(AudioData, 24000);
    }
    else
    {
        VIVONT_ERROR("ProcessAudioData", "Blendshape runner is NOT valid! Cannot generate blendshapes.");
        OnAnimationError.Broadcast(TEXT("Inference runner not available"));
        this->PendingAudioData.Empty(); // Use this-> explicitly
    }
}

// Corrected Signature: Removed WorldContextObject parameter
void UVivontAnimationManager::ProcessTextInput(const FString& TextInput)
{
    if (TextInput.IsEmpty())
    {
        OnAnimationError.Broadcast(TEXT("ProcessTextInput: Empty text input received."));
        return;
    }

    if (!this->OpenAIAPI) // Use this-> explicitly and correct check
    {
        OnAnimationError.Broadcast(TEXT("ProcessTextInput: OpenAI API client not available"));
        return;
    }

    // Stop any active animations before sending new text
    this->StopAnimation(); // Use this-> explicitly

    // Send text to OpenAI API
    UE_LOG(LogVivont, Log, TEXT("ProcessTextInput: Sending text to OpenAI API: %s"), *TextInput);
    this->OpenAIAPI->SendTextMessage(TextInput); // Use this-> explicitly
}

void UVivontAnimationManager::StopAnimation()
{
    // Reset animation state flags and data
    // this->bIsAnimating = false; // REMOVED - State handled by LiveLinkSource
    this->bIsPlayingSyncedAudio = false;
    // this->CurrentFrame = 0; // REMOVED - State handled by LiveLinkSource
    this->CurrentBlendShapes.Empty(); // Still need to clear the stored shapes
    this->PendingAudioData.Empty();
    // this->PlaybackStartTime = 0.0f; // REMOVED - Timing handled by LiveLinkSource

    // Unbind the completion delegate if the source is valid
    if (this->LiveLinkSource.IsValid() && this->LiveLinkSource->OnPlaybackSequenceCompleted.IsBound())
    {
        this->LiveLinkSource->OnPlaybackSequenceCompleted.Unbind();
        UE_LOG(LogVivont, Log, TEXT("StopAnimation: Unbound HandlePlaybackSequenceCompleted from LiveLink source delegate."));
    }
    this->LiveLinkSource.Reset(); // Clear the shared pointer

    // Broadcast that animation ended (or was stopped)
    OnAnimationEnded.Broadcast();
    UE_LOG(LogVivont, Log, TEXT("StopAnimation: Animation stopped and state reset."));

    // If needed, explicitly tell the audio subsystem to stop all sounds it might be playing
    if (this->AudioSubsystem) // Use this-> explicitly and correct check
    {
        // this->AudioSubsystem->StopAllAudio(); // Consider uncommenting if needed
    }
}

float UVivontAnimationManager::GetLastAnimationDuration() const
{
    // Return the stored duration. This is updated in OnBlendShapesReceived.
    // Consider thread safety if accessed from multiple threads, though subsystems are typically game thread.
    return LastAudioDuration;
}

void UVivontAnimationManager::HandlePlaybackSequenceCompleted()
{
    VIVONT_LOG(Log, TEXT("HandlePlaybackSequenceCompleted: Received completion signal from LiveLink source. Broadcasting OnAnimationEnded."));

    // Broadcast the main AnimationEnded delegate for the AnimBP
    OnAnimationEnded.Broadcast();

    // Optionally, reset internal state if needed, though StopAnimation usually handles this
    this->bIsPlayingSyncedAudio = false;

    // Unbind self to prevent potential issues if the delegate somehow fires again before the next sequence
    if (this->LiveLinkSource.IsValid() && this->LiveLinkSource->OnPlaybackSequenceCompleted.IsBound())
    {
        this->LiveLinkSource->OnPlaybackSequenceCompleted.Unbind();
        UE_LOG(LogVivont, Log, TEXT("HandlePlaybackSequenceCompleted: Unbound self from LiveLink source delegate."));
    }
     this->LiveLinkSource.Reset(); // Clear the shared pointer after handling completion
}

// REMOVED: PlayAudioAndAnimateFace function definition.

// Helper function to convert TArray<float> to FBlendShapeFrame
FBlendShapeFrame UVivontAnimationManager::ConvertToBlendShapeFrame(const TArray<float>& Values)
{
    FBlendShapeFrame Frame;
    Frame.SetAllValues(Values);
    return Frame;
}

// Helper function to convert FBlendShapeFrame to TArray<float>
TArray<float> UVivontAnimationManager::ConvertFromBlendShapeFrame(const FBlendShapeFrame& Frame)
{
    return Frame.GetRawValues();
}

void UVivontAnimationManager::OnBlendShapesReceived(bool bSuccess, const TArray<FBlendShapeFrame>& BlendShapes)
{
    if (!bSuccess || BlendShapes.Num() == 0)
    {
        UE_LOG(LogVivont, Error, TEXT("OnBlendShapesReceived: Failed to receive valid blend shapes: Success=%d, NumFrames=%d"),
            bSuccess, BlendShapes.Num());
        OnAnimationError.Broadcast(TEXT("Failed to receive valid blend shapes"));
        this->PendingAudioData.Empty(); // Use this-> explicitly
        return;
    }

    UE_LOG(LogVivont, Log, TEXT("OnBlendShapesReceived: Received %d blend shape frames."), BlendShapes.Num());

    // --- Calculate Audio Duration First ---
    float CalculatedAudioDuration = 0.0f;
    bool bHasAudioToPlay = false;
    if (!this->AudioSubsystem) // Use this-> explicitly and correct check
    {
        UE_LOG(LogVivontAudio, Error, TEXT("OnBlendShapesReceived: AudioSubsystem is invalid!"));
        OnAnimationError.Broadcast(TEXT("Internal error: Audio subsystem not available."));
        this->PendingAudioData.Empty(); // Use this-> explicitly
        return;
    }

    if (this->PendingAudioData.Num() > 0) // Use this-> explicitly
    {
        if (this->AudioSubsystem->GetAudioDuration(this->PendingAudioData, CalculatedAudioDuration))
        {
            UE_LOG(LogVivontAudio, Log, TEXT("OnBlendShapesReceived: Calculated audio duration: %.4f seconds."), CalculatedAudioDuration);
            bHasAudioToPlay = true;
        }
        else
        {
            UE_LOG(LogVivontAudio, Warning, TEXT("OnBlendShapesReceived: Failed to calculate audio duration from pending data. Animating without audio."));
            this->PendingAudioData.Empty(); // Clear data if we can't get duration
            bHasAudioToPlay = false;
        }
    }
    else
    {
         UE_LOG(LogVivontAudio, Warning, TEXT("OnBlendShapesReceived: Received blendshapes but PendingAudioData is empty. Animating without audio."));
         bHasAudioToPlay = false;
    }
    this->LastAudioDuration = CalculatedAudioDuration; // Store calculated duration (will be 0 if no audio or failed)

    // --- Store Blendshapes & Send to LiveLink FIRST ---
    this->CurrentBlendShapes = BlendShapes; // Still store shapes locally if needed elsewhere, though LiveLinkSource now owns the active sequence

    // Store the LiveLink source reference for delegate binding/unbinding
    // Note: We get a new SharedPtr here. If the source is destroyed elsewhere, this might become invalid later.
    // Consider if a more robust way to hold the reference is needed, but this matches previous pattern.
    this->LiveLinkSource = FModuleManager::LoadModuleChecked<FVivontPluginModule>("VivontPlugin").GetLiveLinkSource();

    if (this->LiveLinkSource.IsValid()) // Use the member variable now
    {
        // Drive the LiveLink subject's curve set from the model's output schema (data-driven).
        if (this->Runner)
        {
            this->LiveLinkSource->SetCurveNames(this->Runner->GetCurveNames());
        }
        UE_LOG(LogVivont, Log, TEXT("OnBlendShapesReceived: Sending %d blend shape frames and duration %.4f to LiveLink source '%s'."), BlendShapes.Num(), this->LastAudioDuration, *this->LiveLinkSource->GetSourceName().ToString());
        this->LiveLinkSource->ReceiveAnimationData(BlendShapes, this->LastAudioDuration); // Send all frames initially AND the duration

        // Bind the completion delegate
        this->LiveLinkSource->OnPlaybackSequenceCompleted.BindUObject(this, &UVivontAnimationManager::HandlePlaybackSequenceCompleted);
        UE_LOG(LogVivont, Log, TEXT("OnBlendShapesReceived: Bound HandlePlaybackSequenceCompleted to LiveLink source delegate."));
    }
    else
    {
        // If LiveLink isn't ready, we probably shouldn't play audio either, as it won't be synced.
        UE_LOG(LogVivont, Error, TEXT("OnBlendShapesReceived: FVivontLiveLinkSource is not valid. Cannot pass animation data or play synced audio."));
        OnAnimationError.Broadcast(TEXT("LiveLink source not available for animation."));
        this->PendingAudioData.Empty(); // Clear pending audio if we can't animate
        this->CurrentBlendShapes.Empty();
        return; // Exit early
    }

    // --- Play Audio via Subsystem AFTER sending data to LiveLink ---
    if (bHasAudioToPlay)
    {
        // AudioSubsystem validity was already checked when calculating duration
        float PlaybackDuration = 0.0f; // Variable for PlayAudioData output, though we already calculated it
        UE_LOG(LogVivontAudio, Log, TEXT("OnBlendShapesReceived: Requesting audio playback for %d bytes via AudioSubsystem."), this->PendingAudioData.Num());
        this->AudioSubsystem->PlayAudioData(this->PendingAudioData, PlaybackDuration); // Pass OutDuration
        this->bIsPlayingSyncedAudio = true; // Set flag *after* successfully starting playback
        VIVONT_LOG(Log, TEXT("OnBlendShapesReceived: Broadcasting OnAnimationStarted (Audio: Yes)")); // DIAGNOSTIC LOG
        OnAnimationStarted.Broadcast(); // Broadcast animation start *after* both LiveLink data sent and audio started
        UE_LOG(LogVivontAudio, Log, TEXT("OnBlendShapesReceived: Audio playback initiated. Reported duration: %.4f seconds."), PlaybackDuration);
    }
    else
    {
        // If there's no audio, but we successfully sent blendshapes, we might still consider the animation "started"
        this->bIsPlayingSyncedAudio = false;
        VIVONT_LOG(Log, TEXT("OnBlendShapesReceived: Broadcasting OnAnimationStarted (Audio: No)")); // DIAGNOSTIC LOG
        OnAnimationStarted.Broadcast(); // Broadcast animation start even without audio
        UE_LOG(LogVivont, Log, TEXT("OnBlendShapesReceived: Animation started without audio playback."));
    }

    // Clear pending audio data now that it's been used (or deemed unusable)
    this->PendingAudioData.Empty();
}

void UVivontAnimationManager::OnAudioGenerated(const TArray<uint8>& AudioData)
{
    VIVONT_LOG(Log, TEXT("OnAudioGenerated: Entered function."));

    if (AudioData.Num() == 0)
    {
        FString ErrorMsg = TEXT("OnAudioGenerated: Failed to generate audio (empty data)");
        UE_LOG(LogVivontAudio, Error, TEXT("%s"), *ErrorMsg);
        OnAnimationError.Broadcast(ErrorMsg);
        return;
    }

    // Optionally save the raw PCM data for debugging (gated to avoid per-utterance disk churn).
    if (this->Settings && this->Settings->bDebugDump)
    {
        FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        FString DebugFilename = FString::Printf(TEXT("openai_audio_%s.wav"), *Timestamp);
        this->SaveAudioToFile(AudioData, DebugFilename, true); // Use this-> explicitly
        UE_LOG(LogVivontAudio, Log, TEXT("Debug audio saved to %s"), *DebugFilename);
    }

    UE_LOG(LogVivontAudio, Log, TEXT("OnAudioGenerated: Received %d bytes from OpenAI."), AudioData.Num());

    // Store the received audio data temporarily
    this->PendingAudioData = AudioData; // Use this-> explicitly
    UE_LOG(LogVivontAudio, Log, TEXT("OnAudioGenerated: Stored %d bytes in PendingAudioData."), this->PendingAudioData.Num());

    // Now send the audio data to the in-process runner for blendshape generation.
    if (this->Runner) // Use this-> explicitly and correct check
    {
        UE_LOG(LogVivontAudio, Log, TEXT("OnAudioGenerated: Submitting %d bytes to inference runner."), AudioData.Num());
        this->Runner->SubmitAudio(AudioData, 24000);
    }
    else
    {
        VIVONT_ERROR("OnAudioGenerated", "Blendshape runner is NOT valid! Cannot generate blendshapes.");
        OnAnimationError.Broadcast(TEXT("Inference runner not available"));
        this->PendingAudioData.Empty(); // Use this-> explicitly
    }
}

void UVivontAnimationManager::OnTextResponseReceived(const FString& ResponseText)
{
    if (ResponseText.IsEmpty())
    {
        OnAnimationError.Broadcast(TEXT("OnTextResponseReceived: Failed to receive valid text response (empty)"));
        return;
    }
    UE_LOG(LogVivont, Log, TEXT("OnTextResponseReceived: Text response received: %s"), *ResponseText);
    // Currently, we don't trigger animation/audio from just text responses.
}

void UVivontAnimationManager::OnAPIErrorReceived(const FString& ErrorMessage)
{
    UE_LOG(LogVivont, Error, TEXT("OnAPIErrorReceived: API Error: %s"), *ErrorMessage);
    OnAnimationError.Broadcast(ErrorMessage);
    this->StopAnimation(); // Use this-> explicitly
}

// REMOVED: OnAudioFinished function definition.

bool UVivontAnimationManager::AnimationTick(float DeltaTime)
{
    // The AnimationManager no longer handles frame-by-frame timing.
    // This responsibility is now fully within FVivontLiveLinkSource using audio duration.
    // This tick function could be used for other periodic checks if needed in the future.

    // if (this->bIsAnimating && this->CurrentBlendShapes.Num() > 0) // Use this-> explicitly
    // {
    //     this->UpdateAnimationTiming(DeltaTime); // Use this-> explicitly // REMOVED
    // }

    return true; // Continue ticking
}

// REMOVED: UpdateLiveLinkBlendshapes function implementation.
// This logic is now fully handled by FVivontLiveLinkSource internally.

// REMOVED: UpdateAnimationTiming function definition. Timing is handled by LiveLinkSource.
// REMOVED: GetFrameIndexAtTime function definition. Timing is handled by LiveLinkSource.

bool UVivontAnimationManager::SaveAudioToFile(const TArray<uint8>& AudioData, const FString& Filename, bool bAsWav)
{
    // Create debug directory if it doesn't exist
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("Vivont");
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SaveDir))
    {
        if (!PlatformFile.CreateDirectory(*SaveDir))
        {
             UE_LOG(LogVivontAudio, Error, TEXT("Failed to create directory: %s"), *SaveDir);
             return false;
        }
    }

    FString FilePath = SaveDir / Filename;

    // If we want to save as WAV and the data is PCM, create a WAV header
    if (bAsWav && AudioData.Num() > 0 && !(AudioData[0] == 'R' && AudioData[1] == 'I' && AudioData[2] == 'F' && AudioData[3] == 'F'))
    {
        TArray<uint8> WavData;

        // Standard WAV header for 16-bit PCM mono at 24kHz
        const int32 NumChannels = 1;
        const int32 SampleRate = 24000;
        const int32 BitDepth = 16;
        const int32 BytesPerSample = BitDepth / 8;
        const uint32 PCMDataSize = AudioData.Num();
        const uint32 HeaderSize = 44; // Standard WAV header size
        const uint32 ChunkSize = PCMDataSize + HeaderSize - 8;
        const uint32 ByteRate = SampleRate * NumChannels * BytesPerSample;
        const uint16 BlockAlign = NumChannels * BytesPerSample;

        // RIFF header
        WavData.Append({'R', 'I', 'F', 'F'});
        WavData.Append(reinterpret_cast<const uint8*>(&ChunkSize), sizeof(ChunkSize));
        WavData.Append({'W', 'A', 'V', 'E'});

        // fmt subchunk
        WavData.Append({'f', 'm', 't', ' '});
        const uint32 FmtChunkSize = 16; // For PCM
        WavData.Append(reinterpret_cast<const uint8*>(&FmtChunkSize), sizeof(FmtChunkSize));
        const uint16 AudioFormat = 1; // PCM
        WavData.Append(reinterpret_cast<const uint8*>(&AudioFormat), sizeof(AudioFormat));
        const uint16 WavNumChannels = NumChannels;
        WavData.Append(reinterpret_cast<const uint8*>(&WavNumChannels), sizeof(WavNumChannels));
        const uint32 WavSampleRate = SampleRate;
        WavData.Append(reinterpret_cast<const uint8*>(&WavSampleRate), sizeof(WavSampleRate));
        WavData.Append(reinterpret_cast<const uint8*>(&ByteRate), sizeof(ByteRate));
        WavData.Append(reinterpret_cast<const uint8*>(&BlockAlign), sizeof(BlockAlign));
        const uint16 WavBitsPerSample = BitDepth;
        WavData.Append(reinterpret_cast<const uint8*>(&WavBitsPerSample), sizeof(WavBitsPerSample));

        // data subchunk
        WavData.Append({'d', 'a', 't', 'a'});
        WavData.Append(reinterpret_cast<const uint8*>(&PCMDataSize), sizeof(PCMDataSize));

        // Append the PCM data
        WavData.Append(AudioData);

        return FFileHelper::SaveArrayToFile(WavData, *FilePath);
    }
    else
    {
        // Save raw data as-is (or already WAV)
        return FFileHelper::SaveArrayToFile(AudioData, *FilePath);
    }
}
