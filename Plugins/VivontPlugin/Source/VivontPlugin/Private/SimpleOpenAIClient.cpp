// Copyright (c) 2025, Derek. All rights reserved.

#include "SimpleOpenAIClient.h"
#include "VivontSettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimpleOpenAI, Log, All);

namespace
{
    const TCHAR* ChatUrl = TEXT("https://api.openai.com/v1/chat/completions");
    const TCHAR* SpeechUrl = TEXT("https://api.openai.com/v1/audio/speech");

    TSharedPtr<FJsonValue> MakeChatMessage(const FString& Role, const FString& Content)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("role"), Role);
        Obj->SetStringField(TEXT("content"), Content);
        return MakeShared<FJsonValueObject>(Obj);
    }
}

void USimpleOpenAIClient::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const UVivontSettings* Settings = GetDefault<UVivontSettings>();
    bDebugLogging = Settings ? Settings->bDebugDump : false;
    VoiceName = Settings ? Settings->GetVoiceName() : TEXT("ash");
    LLMModel = (Settings && !Settings->LLMModel.IsEmpty()) ? Settings->LLMModel : TEXT("gpt-4o-mini");
    TTSModel = (Settings && !Settings->TTSModel.IsEmpty()) ? Settings->TTSModel : TEXT("gpt-4o-mini-tts");
    TTSInstructions = Settings ? Settings->TTSInstructions : FString();
    if (Settings && !Settings->OpenAIApiKey.IsEmpty())
    {
        ApiKey_Internal = Settings->OpenAIApiKey;
    }

    DebugLog(TEXT("Init"), TEXT("SimpleOpenAIClient (REST) initialized"));
}

void USimpleOpenAIClient::Deinitialize()
{
    History.Empty();
    Super::Deinitialize();
}

bool USimpleOpenAIClient::Connect()
{
    if (ApiKey_Internal.IsEmpty())
    {
        // Late-load from settings in case it was set after Initialize.
        if (const UVivontSettings* Settings = GetDefault<UVivontSettings>())
        {
            if (!Settings->OpenAIApiKey.IsEmpty())
            {
                ApiKey_Internal = Settings->OpenAIApiKey;
            }
        }
    }

    if (ApiKey_Internal.IsEmpty())
    {
        HandleError(TEXT("Connect"), TEXT("OpenAI API key is not configured"));
        bIsConnected = false;
        OnConnectionStateChanged.Broadcast(false);
        return false;
    }

    bIsConnected = true;
    OnConnectionStateChanged.Broadcast(true);
    DebugLog(TEXT("Connect"), TEXT("Ready (REST; no persistent connection)"));
    return true;
}

void USimpleOpenAIClient::Disconnect()
{
    History.Empty();
    if (bIsConnected)
    {
        bIsConnected = false;
        OnConnectionStateChanged.Broadcast(false);
    }
}

void USimpleOpenAIClient::SendTextMessage(const FString& Message)
{
    if (Message.IsEmpty())
    {
        return;
    }
    if (!bIsConnected)
    {
        Connect();
    }
    if (ApiKey_Internal.IsEmpty())
    {
        HandleError(TEXT("Send"), TEXT("Cannot send: OpenAI API key not configured"));
        return;
    }

    RequestChatCompletion(Message);
}

void USimpleOpenAIClient::SendMultimodalFromTextArray(const TArray<FString>& TextMessages)
{
    SendTextMessage(FString::Join(TextMessages, TEXT("\n")));
}

void USimpleOpenAIClient::RequestChatCompletion(const FString& UserMessage)
{
    History.Add({ TEXT("user"), UserMessage });
    TrimHistory();
    SendChatHttp(0);
}

void USimpleOpenAIClient::SendChatHttp(int32 Attempt)
{
    // Build messages: system instructions + conversation history.
    TArray<TSharedPtr<FJsonValue>> Messages;
    if (const UVivontSettings* Settings = GetDefault<UVivontSettings>())
    {
        if (!Settings->SystemInstructions.IsEmpty())
        {
            Messages.Add(MakeChatMessage(TEXT("system"), Settings->SystemInstructions));
        }
    }
    for (const FChatTurn& Turn : History)
    {
        Messages.Add(MakeChatMessage(Turn.Role, Turn.Content));
    }

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("model"), LLMModel);
    Body->SetArrayField(TEXT("messages"), Messages);

    FString BodyString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
    FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ChatUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetTimeout(RequestTimeoutSeconds);
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey_Internal));
    Request->SetContentAsString(BodyString);

    TWeakObjectPtr<USimpleOpenAIClient> WeakThis(this);
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis, Attempt](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (USimpleOpenAIClient* Self = WeakThis.Get())
            {
                Self->HandleChatResponse(Response, bWasSuccessful, Attempt);
            }
        });

    DebugLog(TEXT("Chat"), FString::Printf(TEXT("Requesting completion (model %s, %d msgs, attempt %d/%d)"),
        *LLMModel, Messages.Num(), Attempt + 1, MaxRequestAttempts));
    Request->ProcessRequest();
}

void USimpleOpenAIClient::HandleChatResponse(FHttpResponsePtr Response, bool bWasSuccessful, int32 Attempt)
{
    if (IsTransientFailure(bWasSuccessful, Response))
    {
        if (Attempt + 1 < MaxRequestAttempts)
        {
            TWeakObjectPtr<USimpleOpenAIClient> WeakThis(this);
            ScheduleRetry(Attempt, [WeakThis, Attempt]()
            {
                if (USimpleOpenAIClient* Self = WeakThis.Get()) { Self->SendChatHttp(Attempt + 1); }
            });
            return;
        }
        const int32 FailCode = Response.IsValid() ? Response->GetResponseCode() : 0;
        HandleError(TEXT("Chat"), FString::Printf(TEXT("Chat completion failed after %d attempts (last HTTP %d)"), MaxRequestAttempts, FailCode));
        return;
    }

    const int32 Code = Response->GetResponseCode();
    if (Code != 200)
    {
        HandleError(TEXT("Chat"), FString::Printf(TEXT("Chat completion HTTP %d: %s"), Code, *Response->GetContentAsString().Left(300)));
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        HandleError(TEXT("Chat"), TEXT("Could not parse chat completion JSON"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
    if (!Root->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->Num() == 0)
    {
        HandleError(TEXT("Chat"), TEXT("Chat completion contained no choices"));
        return;
    }

    const TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
    const TSharedPtr<FJsonObject>* MessageObj = nullptr;
    FString Reply;
    if (!Choice.IsValid() ||
        !Choice->TryGetObjectField(TEXT("message"), MessageObj) || !MessageObj ||
        !(*MessageObj)->TryGetStringField(TEXT("content"), Reply) || Reply.IsEmpty())
    {
        HandleError(TEXT("Chat"), TEXT("Chat completion reply was empty"));
        return;
    }

    History.Add({ TEXT("assistant"), Reply });
    TrimHistory();

    OnTextResponse.Broadcast(Reply);
    RequestSpeech(Reply);
}

void USimpleOpenAIClient::RequestSpeech(const FString& Text)
{
    SendSpeechHttp(Text, 0);
}

void USimpleOpenAIClient::SendSpeechHttp(const FString& Text, int32 Attempt)
{
    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("model"), TTSModel);
    Body->SetStringField(TEXT("input"), Text);
    Body->SetStringField(TEXT("voice"), VoiceName);
    Body->SetStringField(TEXT("response_format"), TEXT("pcm")); // 24 kHz, 16-bit, mono
    if (!TTSInstructions.IsEmpty())
    {
        // Steerable delivery for gpt-4o-mini-tts; ignored by classic tts-1.
        Body->SetStringField(TEXT("instructions"), TTSInstructions);
    }

    FString BodyString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
    FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(SpeechUrl);
    Request->SetVerb(TEXT("POST"));
    Request->SetTimeout(RequestTimeoutSeconds);
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey_Internal));
    Request->SetContentAsString(BodyString);

    TWeakObjectPtr<USimpleOpenAIClient> WeakThis(this);
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis, Text, Attempt](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (USimpleOpenAIClient* Self = WeakThis.Get())
            {
                Self->HandleSpeechResponse(Response, bWasSuccessful, Text, Attempt);
            }
        });

    DebugLog(TEXT("TTS"), FString::Printf(TEXT("Requesting speech (model %s, voice %s, %d chars, attempt %d/%d)"),
        *TTSModel, *VoiceName, Text.Len(), Attempt + 1, MaxRequestAttempts));
    Request->ProcessRequest();
}

void USimpleOpenAIClient::HandleSpeechResponse(FHttpResponsePtr Response, bool bWasSuccessful, const FString& Text, int32 Attempt)
{
    if (IsTransientFailure(bWasSuccessful, Response))
    {
        if (Attempt + 1 < MaxRequestAttempts)
        {
            TWeakObjectPtr<USimpleOpenAIClient> WeakThis(this);
            ScheduleRetry(Attempt, [WeakThis, Text, Attempt]()
            {
                if (USimpleOpenAIClient* Self = WeakThis.Get()) { Self->SendSpeechHttp(Text, Attempt + 1); }
            });
            return;
        }
        const int32 FailCode = Response.IsValid() ? Response->GetResponseCode() : 0;
        HandleError(TEXT("TTS"), FString::Printf(TEXT("Speech failed after %d attempts (last HTTP %d)"), MaxRequestAttempts, FailCode));
        return;
    }

    const int32 Code = Response->GetResponseCode();
    if (Code != 200)
    {
        // Error responses are JSON text, not audio.
        HandleError(TEXT("TTS"), FString::Printf(TEXT("Speech HTTP %d: %s"), Code, *Response->GetContentAsString().Left(300)));
        return;
    }

    const TArray<uint8>& Audio = Response->GetContent();
    if (Audio.Num() == 0)
    {
        HandleError(TEXT("TTS"), TEXT("Speech response contained no audio"));
        return;
    }

    if (bDebugLogging)
    {
        const FString DebugPath = FPaths::ProjectSavedDir() / TEXT("Vivont") / TEXT("OpenAI_TTS_Debug.pcm");
        FFileHelper::SaveArrayToFile(Audio, *DebugPath);
        DebugLog(TEXT("TTS"), FString::Printf(TEXT("Saved %d bytes of PCM to %s"), Audio.Num(), *DebugPath));
    }

    UE_LOG(LogSimpleOpenAI, Log, TEXT("Received %d bytes of TTS audio (PCM 16-bit %d Hz mono)"), Audio.Num(), SAMPLE_RATE);
    OnAudioReceived.Broadcast(Audio);
}

void USimpleOpenAIClient::TrimHistory()
{
    // Keep the most recent MaxHistoryTurns entries (user/assistant turns).
    const int32 Excess = History.Num() - MaxHistoryTurns;
    if (Excess > 0)
    {
        History.RemoveAt(0, Excess, EAllowShrinking::No);
    }
}

bool USimpleOpenAIClient::IsTransientFailure(bool bWasSuccessful, const FHttpResponsePtr& Response)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        return true; // connection failure / timeout — worth one more try
    }
    const int32 Code = Response->GetResponseCode();
    return Code == 429 || (Code >= 500 && Code <= 599);
}

void USimpleOpenAIClient::ScheduleRetry(int32 Attempt, TFunction<void()> Retry)
{
    const float Delay = 0.5f * static_cast<float>(1 << Attempt); // exponential backoff: 0.5s, 1s, ...
    DebugLog(TEXT("Retry"), FString::Printf(TEXT("Transient failure; retrying in %.1fs"), Delay));
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([RetryFn = MoveTemp(Retry)](float) -> bool
        {
            RetryFn();
            return false; // one-shot
        }), Delay);
}

void USimpleOpenAIClient::HandleError(const FString& Source, const FString& Message)
{
    const FString Full = FString::Printf(TEXT("[%s] %s"), *Source, *Message);
    UE_LOG(LogSimpleOpenAI, Error, TEXT("%s"), *Full);
    OnErrorReceived.Broadcast(Full);
}

void USimpleOpenAIClient::DebugLog(const FString& Category, const FString& Message)
{
    if (bDebugLogging)
    {
        UE_LOG(LogSimpleOpenAI, Log, TEXT("[%s] %s"), *Category, *Message);
    }
}

bool USimpleOpenAIClient::SaveAudioToWAV(const FString& Filepath, const TArray<uint8>& AudioData)
{
    // Already a WAV? Save as-is.
    if (AudioData.Num() >= 12 &&
        AudioData[0] == 'R' && AudioData[1] == 'I' && AudioData[2] == 'F' && AudioData[3] == 'F' &&
        AudioData[8] == 'W' && AudioData[9] == 'A' && AudioData[10] == 'V' && AudioData[11] == 'E')
    {
        return FFileHelper::SaveArrayToFile(AudioData, *Filepath);
    }
    return FFileHelper::SaveArrayToFile(CreateWAVFromPCM(AudioData), *Filepath);
}

TArray<uint8> USimpleOpenAIClient::CreateWAVFromPCM(const TArray<uint8>& PCMData)
{
    TArray<uint8> Wav;
    Wav.SetNumUninitialized(44);

    const int32 DataSize = PCMData.Num();
    const int32 ChunkSize = DataSize + 36;
    const int32 ByteRate = SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    const int16 BlockAlign = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);

    auto W32 = [&Wav](int32 Offset, uint32 Value)
    {
        Wav[Offset] = Value & 0xFF;
        Wav[Offset + 1] = (Value >> 8) & 0xFF;
        Wav[Offset + 2] = (Value >> 16) & 0xFF;
        Wav[Offset + 3] = (Value >> 24) & 0xFF;
    };
    auto W16 = [&Wav](int32 Offset, uint16 Value)
    {
        Wav[Offset] = Value & 0xFF;
        Wav[Offset + 1] = (Value >> 8) & 0xFF;
    };

    Wav[0] = 'R'; Wav[1] = 'I'; Wav[2] = 'F'; Wav[3] = 'F';
    W32(4, ChunkSize);
    Wav[8] = 'W'; Wav[9] = 'A'; Wav[10] = 'V'; Wav[11] = 'E';
    Wav[12] = 'f'; Wav[13] = 'm'; Wav[14] = 't'; Wav[15] = ' ';
    W32(16, 16);                 // fmt chunk size
    W16(20, 1);                  // PCM
    W16(22, NUM_CHANNELS);
    W32(24, SAMPLE_RATE);
    W32(28, ByteRate);
    W16(32, BlockAlign);
    W16(34, BITS_PER_SAMPLE);
    Wav[36] = 'd'; Wav[37] = 'a'; Wav[38] = 't'; Wav[39] = 'a';
    W32(40, DataSize);

    Wav.Append(PCMData);
    return Wav;
}
