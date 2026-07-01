// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "HAL/CriticalSection.h"
#include "Templates/SharedPointer.h"
#include "VivontBlendshapeTypes.h"
#include "VivontInferenceManifest.h"
#include "VivontBlendshapeRunner.generated.h"

namespace UE::NNE { class IModelInstanceRunSync; }
class UNNEModelData;

/**
 * In-process audio -> blendshape inference.
 *
 * Replaces the old HTTP UVivontAPIClient. Loads the in-house ONNX model via
 * NNE (GPU/DirectML through NNERuntimeORTDml, CPU fallback NNERuntimeORTCpu) and
 * runs the manifest-described contract: raw mono windows -> per-frame curves. Supports both
 * FastCausalCNN (16kHz, 1 frame/window) and the WavLM family (24kHz, SequenceLen frames/window,
 * center frame used) entirely from the manifest, with no hardcoded schema.
 *
 * Keeps the same OnBlendShapesReceived delegate the AnimationManager already
 * expects, so the orchestration/playback layers are unchanged.
 */
UCLASS()
class VIVONTPLUGIN_API UVivontBlendshapeRunner : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Submit an utterance for inference. Audio is converted to mono float and
     * resampled to the model's rate. Inference runs asynchronously; results are
     * broadcast on the game thread via OnBlendShapesReceived.
     * @param AudioData         WAV or raw 16-bit PCM bytes.
     * @param SourceSampleRate  Sample rate to assume for raw PCM (OpenAI TTS = 24000).
     */
    UFUNCTION(BlueprintCallable, Category = "Vivont|Inference")
    void SubmitAudio(const TArray<uint8>& AudioData, int32 SourceSampleRate = 24000);

    /** True once the model + manifest are loaded and an instance is ready. */
    UFUNCTION(BlueprintPure, Category = "Vivont|Inference")
    bool IsModelReady() const { return bModelReady; }

    /** Curve names in model output order (drives the LiveLink subject). */
    const TArray<FName>& GetCurveNames() const { return Manifest.CurveNames; }

    /** Broadcast with the full inferred sequence for an utterance. */
    UPROPERTY(BlueprintAssignable, Category = "Vivont|Inference")
    FOnBlendShapesReceived OnBlendShapesReceived;

    /** Broadcast on inference / model load failure. */
    UPROPERTY(BlueprintAssignable, Category = "Vivont|Inference")
    FOnInferenceError OnInferenceError;

private:
    /** Lazily load manifest + ONNX and create the NNE model instance. */
    bool EnsureModelLoaded(FString& OutError);

    /** Background inference over windowed audio. Broadcasts on the game thread. */
    void RunInference(TArray<float> MonoAudio);

    /** Broadcast helpers that always fire on the game thread. */
    void BroadcastResultGameThread(TArray<FBlendShapeFrame> Frames);
    void BroadcastErrorGameThread(FString Message);

    FVivontInferenceManifest Manifest;

    /** Kept alive so the runtime's reference to model data remains valid. */
    UPROPERTY(Transient)
    TObjectPtr<UNNEModelData> ModelData;

    TSharedPtr<UE::NNE::IModelInstanceRunSync> ModelInstance;

    bool bModelReady = false;
    bool bLoadAttempted = false;

    /** Serializes model load + inference (one utterance at a time). */
    FCriticalSection InferenceCriticalSection;
};
