// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "SimpleOpenAIClient.generated.h"

// Forward declarations
class UVivontSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioReceived, const TArray<uint8>&, AudioData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenAITextResponse, const FString&, ResponseText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenAIConnectionStateChanged, bool, bIsConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnErrorReceived, const FString&, ErrorMessage);

/**
 * Simple OpenAI client (REST).
 *
 * Text-driven avatar LLM + TTS. A user message goes to Chat Completions for the
 * reply text, then to the /audio/speech TTS endpoint for 24 kHz mono 16-bit PCM
 * audio. Replaces the previous Realtime WebSocket implementation; the delegate
 * surface (OnTextResponse / OnAudioReceived / ...) is unchanged so the animation
 * manager and widget bindings keep working.
 */
UCLASS(BlueprintType)
class VIVONTPLUGIN_API USimpleOpenAIClient : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Validate configuration (API key) and mark ready. No persistent connection with REST. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    bool Connect();

    /** Mark not-ready and clear conversation context. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void Disconnect();

    /** Send a user message: Chat Completions reply -> TTS audio. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SendTextMessage(const FString& Message);

    /** Convenience: join several lines into one user message and send. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SendMultimodalFromTextArray(const TArray<FString>& TextMessages);

    /** True once an API key is configured (ready to send). */
    UFUNCTION(BlueprintPure, Category = "OpenAI")
    bool IsConnected() const { return bIsConnected; }

    /** Set the API key directly. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SetAPIKey(const FString& ApiKey) { ApiKey_Internal = ApiKey; }

    /** Set the TTS voice (e.g. "alloy", "ash", "echo", "fable", "onyx", "nova", "shimmer"). */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SetVoice(const FString& Voice) { VoiceName = Voice; }

    /** Save received audio (raw PCM or WAV) to a WAV file. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    bool SaveAudioToWAV(const FString& Filepath, const TArray<uint8>& AudioData);

    /** Enable verbose debug logging + audio dumps. */
    UFUNCTION(BlueprintCallable, Category = "OpenAI")
    void SetDebugLogging(bool bEnable) { bDebugLogging = bEnable; }

    /** Fired when reply audio (24 kHz mono 16-bit PCM) is received. */
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
    FOnAudioReceived OnAudioReceived;

    /** Fired with the assistant's reply text. */
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
    FOnOpenAITextResponse OnTextResponse;

    /** Fired when readiness changes. */
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
    FOnOpenAIConnectionStateChanged OnConnectionStateChanged;

    /** Fired on any request/config error. */
    UPROPERTY(BlueprintAssignable, Category = "OpenAI")
    FOnErrorReceived OnErrorReceived;

private:
    /** One conversation turn. */
    struct FChatTurn
    {
        FString Role;     // "user" | "assistant"
        FString Content;
    };

    // Request builders / handlers. The Send*Http variants carry a retry Attempt index; the
    // Request* entry points do the one-time state work (history append) then dispatch attempt 0.
    void RequestChatCompletion(const FString& UserMessage);
    void SendChatHttp(int32 Attempt);
    void HandleChatResponse(FHttpResponsePtr Response, bool bWasSuccessful, int32 Attempt);
    void RequestSpeech(const FString& Text);
    void SendSpeechHttp(const FString& Text, int32 Attempt);
    void HandleSpeechResponse(FHttpResponsePtr Response, bool bWasSuccessful, const FString& Text, int32 Attempt);

    /** Transient = worth retrying: no response, HTTP 429, or 5xx. */
    static bool IsTransientFailure(bool bWasSuccessful, const FHttpResponsePtr& Response);
    /** Run Retry after an exponential backoff for the given (already-completed) Attempt. */
    void ScheduleRetry(int32 Attempt, TFunction<void()> Retry);

    void HandleError(const FString& Source, const FString& Message);
    void DebugLog(const FString& Category, const FString& Message);
    void TrimHistory();
    TArray<uint8> CreateWAVFromPCM(const TArray<uint8>& PCMData);

    // Config (resolved from settings on Initialize)
    FString ApiKey_Internal;
    FString VoiceName;
    FString LLMModel;
    FString TTSModel;
    FString TTSInstructions;
    bool bIsConnected = false;
    bool bDebugLogging = false;

    // Conversation context (trimmed to a recent window)
    TArray<FChatTurn> History;
    static constexpr int32 MaxHistoryTurns = 12;

    // Request resilience
    static constexpr int32 MaxRequestAttempts = 3;   // initial + 2 retries
    static constexpr float RequestTimeoutSeconds = 30.0f;

    // OpenAI PCM audio format constants
    static constexpr int32 SAMPLE_RATE = 24000;
    static constexpr int32 BITS_PER_SAMPLE = 16;
    static constexpr int32 NUM_CHANNELS = 1;
};
