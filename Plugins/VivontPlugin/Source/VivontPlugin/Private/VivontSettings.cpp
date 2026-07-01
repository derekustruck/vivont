// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontSettings.h"

UVivontSettings::UVivontSettings()
{
    // Default settings
    OpenAIApiKey = TEXT("");
    VoiceType = EVivontVoiceType::Ash;
    LLMModel = TEXT("gpt-4o-mini");
    TTSModel = TEXT("gpt-4o-mini-tts");
    TTSInstructions = TEXT("");
    SystemInstructions = TEXT("You are a helpful assistant. Keep your answers concise and direct.");
    InferenceManifestPath = TEXT("Content/Models/vivont_manifest.json");
    bUseGPUInference = true;
    bDebugDump = false;
    LiveLinkIP = TEXT("127.0.0.1");
    LiveLinkPort = 11111;
    SampleRate = 22050;
    Channels = 1;
    SampleWidth = 2;
    MinBufferDuration = 6;
    MouthScalingFactor = 1.1f;
    EyesScalingFactor = 1.0f;
    EyebrowsScalingFactor = 0.4f;
}

FString UVivontSettings::GetVoiceName() const
{
    switch (VoiceType)
    {
    case EVivontVoiceType::Ash:
        return TEXT("ash");
    case EVivontVoiceType::Echo:
        return TEXT("echo");
    case EVivontVoiceType::Fable:
        return TEXT("fable");
    case EVivontVoiceType::Onyx:
        return TEXT("onyx");
    case EVivontVoiceType::Nova:
        return TEXT("nova");
    case EVivontVoiceType::Shimmer:
        return TEXT("shimmer");
    default:
        return TEXT("ash");
    }
}