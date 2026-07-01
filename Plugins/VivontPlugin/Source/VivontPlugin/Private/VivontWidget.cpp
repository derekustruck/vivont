// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h" // Added for GetGameInstance()
#include "VivontLog.h"
#include "VivontAudioManager.h"
#include "SimpleOpenAIClient.h" // Add missing include

void UVivontWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Get the animation manager (moved from GameInstance to Engine subsystem)
    AnimationManager = GEngine->GetEngineSubsystem<UVivontAnimationManager>();
    
    // Get the OpenAI API client
    OpenAIClient = GEngine->GetEngineSubsystem<USimpleOpenAIClient>();
    
    // Get the audio subsystem for recording
    AudioSubsystem = GEngine->GetEngineSubsystem<UVivontAudioSubsystem>();

    // Connect animation manager delegates
    if (AnimationManager)
    {
        AnimationManager->OnAnimationStarted.AddDynamic(this, &UVivontWidget::HandleAnimationStarted);
        AnimationManager->OnAnimationEnded.AddDynamic(this, &UVivontWidget::HandleAnimationEnded);
        AnimationManager->OnAnimationError.AddDynamic(this, &UVivontWidget::HandleAnimationError);
    }
    
    // Connect OpenAI client delegates
    if (OpenAIClient)
    {
        // First remove any existing bindings to prevent duplicates
        OpenAIClient->OnTextResponse.RemoveDynamic(this, &UVivontWidget::HandleTextResponse);
        OpenAIClient->OnConnectionStateChanged.RemoveDynamic(this, &UVivontWidget::HandleConnectionStateChanged);
        OpenAIClient->OnErrorReceived.RemoveDynamic(this, &UVivontWidget::HandleAPIError);
        
        // Now add fresh bindings
        OpenAIClient->OnTextResponse.AddDynamic(this, &UVivontWidget::HandleTextResponse);
        // REMOVED INCORRECT BINDING: OpenAIClient->OnAudioReceived.AddDynamic(this, &UVivontWidget::HandleAudioReceived);
        OpenAIClient->OnConnectionStateChanged.AddDynamic(this, &UVivontWidget::HandleConnectionStateChanged);
        OpenAIClient->OnErrorReceived.AddDynamic(this, &UVivontWidget::HandleAPIError);
        
        // Connect to the API if not already connected
        if (!OpenAIClient->IsConnected())
        {
            OpenAIClient->Connect();
        }
        
        bIsConnected = OpenAIClient->IsConnected();
    }
    
    // Connect button events
    if (SendButton)
    {
        SendButton->OnClicked.AddDynamic(this, &UVivontWidget::OnSendButtonClicked);
    }
    
    if (RecordButton)
    {
        RecordButton->OnClicked.AddDynamic(this, &UVivontWidget::OnRecordButtonClicked);
    }
    
    // Initialize state
    bIsRecording = false;
    AccumulatedResponseText = TEXT("");
    
    // Initialize UI
    UpdateUIState();
    
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(bIsConnected ? TEXT("Connected to API") : TEXT("Connecting to API...")));
    }
    
    if (ResponseText)
    {
        ResponseText->SetText(FText::FromString(TEXT("")));
    }
    
    if (AnimationProgress)
    {
        AnimationProgress->SetPercent(0.0f);
    }
}

void UVivontWidget::NativeDestruct()
{
    // Disconnect delegates
    if (AnimationManager)
    {
        AnimationManager->OnAnimationStarted.RemoveDynamic(this, &UVivontWidget::HandleAnimationStarted);
        AnimationManager->OnAnimationEnded.RemoveDynamic(this, &UVivontWidget::HandleAnimationEnded);
        AnimationManager->OnAnimationError.RemoveDynamic(this, &UVivontWidget::HandleAnimationError);
    }
    
    if (OpenAIClient)
    {
        OpenAIClient->OnTextResponse.RemoveDynamic(this, &UVivontWidget::HandleTextResponse);
        OpenAIClient->OnConnectionStateChanged.RemoveDynamic(this, &UVivontWidget::HandleConnectionStateChanged);
        OpenAIClient->OnErrorReceived.RemoveDynamic(this, &UVivontWidget::HandleAPIError);
    }
    
    // Disconnect button events
    if (SendButton)
    {
        SendButton->OnClicked.RemoveAll(this);
    }
    
    if (RecordButton)
    {
        RecordButton->OnClicked.RemoveAll(this);
    }
    
    Super::NativeDestruct();
}

void UVivontWidget::OnSendButtonClicked()
{
    if (!OpenAIClient || !InputTextBox)
    {
        return;
    }
    
    FString InputText = InputTextBox->GetText().ToString();
    if (InputText.IsEmpty())
    {
        return;
    }
    
    // Clear input text box
    InputTextBox->SetText(FText::FromString(TEXT("")));
    
    // Clear previous response
    AccumulatedResponseText = TEXT("");
    if (ResponseText)
    {
        ResponseText->SetText(FText::FromString(TEXT("")));
    }
    
    // Send message to API
    OpenAIClient->SendTextMessage(InputText);
    
    // Update UI
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(TEXT("Processing request...")));
    }
    
    UpdateUIState();
}

void UVivontWidget::OnRecordButtonClicked()
{
    if (!AudioSubsystem || !AnimationManager)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("Audio subsystem or Animation Manager not available")));
        }
        return;
    }
    
    // Get settings - needed for audio capture
    UVivontSettings* Settings = GetMutableDefault<UVivontSettings>();
    if (!Settings)
    {
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("Settings not available")));
        }
        return;
    }
    
    if (bIsRecording)
    {
        // Stop recording
        bIsRecording = false;
        AudioSubsystem->StopAudioCapture();
        
        if (StatusText)
        {
            StatusText->SetText(FText::FromString(TEXT("Processing audio...")));
        }
    }
    else
    {
        // Start recording
        if (AudioSubsystem->StartAudioCapture(Settings))
        {
            bIsRecording = true;
            
            if (StatusText)
            {
                StatusText->SetText(FText::FromString(TEXT("Recording audio...")));
            }
            
            // Clear previous response
            AccumulatedResponseText = TEXT("");
            if (ResponseText)
            {
                ResponseText->SetText(FText::FromString(TEXT("")));
            }
        }
        else
        {
            // Failed to start recording
            if (StatusText)
            {
                StatusText->SetText(FText::FromString(TEXT("Failed to start audio recording")));
            }
        }
    }
    
    UpdateUIState();
}

void UVivontWidget::HandleAnimationStarted()
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(TEXT("Animation playing...")));
    }
    
    if (AnimationProgress)
    {
        AnimationProgress->SetPercent(0.0f);
    }
    
    UpdateUIState();
}

void UVivontWidget::HandleAnimationEnded()
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(TEXT("Ready")));
    }
    
    if (AnimationProgress)
    {
        AnimationProgress->SetPercent(0.0f);
    }
    
    UpdateUIState();
}

void UVivontWidget::HandleAnimationError(const FString& ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("Animation Error: %s"), *ErrorMessage)));
    }
    
    // Stop recording if active
    if (bIsRecording && AudioSubsystem)
    {
        AudioSubsystem->StopAudioCapture();
        bIsRecording = false;
    }
    
    UpdateUIState();
}

void UVivontWidget::HandleTextResponse(const FString& InResponseText)
{
    // Accumulate the text (streaming responses come in chunks)
    AccumulatedResponseText += InResponseText;
    
    if (ResponseText)
    {
        ResponseText->SetText(FText::FromString(AccumulatedResponseText));
    }
}

// REMOVED UNUSED FUNCTION: HandleAudioReceived

void UVivontWidget::HandleConnectionStateChanged(bool bConnected)
{
    bIsConnected = bConnected;
    
    if (StatusText)
    {
        FString StateText = bConnected ? TEXT("Connected to API") : TEXT("Disconnected from API");
        StatusText->SetText(FText::FromString(StateText));
    }
    
    UpdateUIState();
}

void UVivontWidget::HandleAPIError(const FString& ErrorMessage)
{
    if (StatusText)
    {
        StatusText->SetText(FText::FromString(FString::Printf(TEXT("API Error: %s"), *ErrorMessage)));
    }
    
    UpdateUIState();
}

void UVivontWidget::UpdateUIState()
{
    if (SendButton)
    {
        SendButton->SetIsEnabled(bIsConnected && !bIsRecording);
    }
    
    if (RecordButton)
    {
        // Only enable recording when not in a text request and connected
        RecordButton->SetIsEnabled(bIsConnected);
        
        FText ButtonText = bIsRecording ? FText::FromString(TEXT("Stop Recording")) : FText::FromString(TEXT("Start Recording"));
        RecordButton->SetToolTipText(ButtonText);
    }
    
    if (InputTextBox)
    {
        InputTextBox->SetIsEnabled(bIsConnected && !bIsRecording);
    }
}
