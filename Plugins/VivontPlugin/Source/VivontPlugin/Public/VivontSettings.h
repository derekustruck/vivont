// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VivontSettings.generated.h"

UENUM(BlueprintType)
enum class EVivontVoiceType : uint8
{
    Ash     UMETA(DisplayName = "Ash"),
    Echo    UMETA(DisplayName = "Echo"),
    Fable   UMETA(DisplayName = "Fable"),
    Onyx    UMETA(DisplayName = "Onyx"),
    Nova    UMETA(DisplayName = "Nova"),
    Shimmer UMETA(DisplayName = "Shimmer")
};

UCLASS(config = Engine, defaultconfig)
class VIVONTPLUGIN_API UVivontSettings : public UObject
{
    GENERATED_BODY()

public:
    UVivontSettings();

    /** OpenAI API Key */
    UPROPERTY(Config, EditAnywhere, Category = "API", meta = (DisplayName = "OpenAI API Key"))
    FString OpenAIApiKey;

    /** Voice to use for TTS */
    UPROPERTY(Config, EditAnywhere, Category = "Voice", meta = (DisplayName = "Voice Type"))
    EVivontVoiceType VoiceType;

    /** OpenAI chat model used to generate the assistant reply. */
    UPROPERTY(Config, EditAnywhere, Category = "LLM", meta = (DisplayName = "LLM Model"))
    FString LLMModel;

    /** OpenAI text-to-speech model used to synthesize the reply audio (e.g. gpt-4o-mini-tts, tts-1). */
    UPROPERTY(Config, EditAnywhere, Category = "Voice", meta = (DisplayName = "TTS Model"))
    FString TTSModel;

    /** Optional delivery/style guidance for steerable TTS models (gpt-4o-mini-tts). Ignored by tts-1. */
    UPROPERTY(Config, EditAnywhere, Category = "Voice", meta = (DisplayName = "TTS Instructions", MultiLine = true))
    FString TTSInstructions;

    /** LLM System Instructions */
    UPROPERTY(Config, EditAnywhere, Category = "LLM", meta = (DisplayName = "System Instructions", MultiLine = true))
    FString SystemInstructions;

    /** Path to the in-house inference manifest JSON (relative to the plugin directory, or absolute). Describes the ONNX model, normalization stats and output curve names. Keep it inside the plugin so the model ships self-contained. */
    UPROPERTY(Config, EditAnywhere, Category = "Inference", meta = (DisplayName = "Inference Manifest Path"))
    FString InferenceManifestPath;

    /** Use GPU (DirectML / NNERuntimeORTDml) for inference; falls back to CPU if unavailable. */
    UPROPERTY(Config, EditAnywhere, Category = "Inference", meta = (DisplayName = "Use GPU Inference"))
    bool bUseGPUInference = true;

    /** Write debug audio / response dumps to Saved/Vivont. Off in normal use. */
    UPROPERTY(Config, EditAnywhere, Category = "Diagnostics", meta = (DisplayName = "Debug Dump To Disk"))
    bool bDebugDump = false;

    /** UDP IP for LiveLink */
    UPROPERTY(Config, EditAnywhere, Category = "LiveLink", meta = (DisplayName = "LiveLink UDP IP"))
    FString LiveLinkIP;

    /** UDP Port for LiveLink */
    UPROPERTY(Config, EditAnywhere, Category = "LiveLink", meta = (DisplayName = "LiveLink UDP Port"))
    int32 LiveLinkPort;

    /** Audio sample rate */
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (DisplayName = "Sample Rate", ClampMin = "8000", ClampMax = "48000"))
    int32 SampleRate;

    /** Audio channels */
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (DisplayName = "Channels", ClampMin = "1", ClampMax = "2"))
    int32 Channels;

    /** Audio sample width */
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (DisplayName = "Sample Width", ClampMin = "1", ClampMax = "4"))
    int32 SampleWidth;

    /** Minimum buffer duration in seconds */
    UPROPERTY(Config, EditAnywhere, Category = "Audio", meta = (DisplayName = "Min Buffer Duration", ClampMin = "1", ClampMax = "10"))
    int32 MinBufferDuration;

    UPROPERTY(Config, EditAnywhere, Category = "Animation", meta = (DisplayName = "Mouth Scaling Factor", ClampMin = "0.1", ClampMax = "2.0"))
    float MouthScalingFactor;

    UPROPERTY(Config, EditAnywhere, Category = "Animation", meta = (DisplayName = "Eyes Scaling Factor", ClampMin = "0.1", ClampMax = "2.0"))
    float EyesScalingFactor;

    UPROPERTY(Config, EditAnywhere, Category = "Animation", meta = (DisplayName = "Eyebrows Scaling Factor", ClampMin = "0.1", ClampMax = "2.0"))
    float EyebrowsScalingFactor;

    /** Get the voice name as a string */
    FString GetVoiceName() const;
};