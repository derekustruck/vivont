// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VivontAnimationManager.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "SimpleOpenAIClient.h"
#include "VivontAudioManager.h" // For UVivontAudioSubsystem
#include "VivontLog.h"
#include "VivontWidget.generated.h"

/**
 * Vivont Widget - UI for controlling the Vivont plugin
 */
UCLASS()
class VIVONTPLUGIN_API UVivontWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    
protected:
    /** Input text box for typing messages */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UEditableTextBox* InputTextBox;
    
    /** Send button for text input */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UButton* SendButton;
    
    /** Record button for audio input */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UButton* RecordButton;
    
    /** Status text display */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UTextBlock* StatusText;
    
    /** Response text display */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UTextBlock* ResponseText;
    
    /** Animation progress bar */
    UPROPERTY(BlueprintReadOnly, Category = "Vivont", meta = (BindWidget))
    UProgressBar* AnimationProgress;
    
    /** Send text message */
    UFUNCTION(BlueprintCallable, Category = "Vivont")
    void OnSendButtonClicked();
    
    /** Toggle recording */
    UFUNCTION(BlueprintCallable, Category = "Vivont")
    void OnRecordButtonClicked();
    
    /** Handler for animation start */
    UFUNCTION(Category = "Vivont")
    void HandleAnimationStarted();
    
    /** Handler for animation end */
    UFUNCTION(Category = "Vivont")
    void HandleAnimationEnded();
    
    /** Handler for animation error */
    UFUNCTION(Category = "Vivont")
    void HandleAnimationError(const FString& ErrorMessage);
    
    /** Handler for text response */
    UFUNCTION(Category = "Vivont")
    void HandleTextResponse(const FString& InResponseText);
    
    // REMOVED UNUSED FUNCTION DECLARATION: HandleAudioReceived
    
    /** Handler for connection state changes */
    UFUNCTION(Category = "Vivont")
    void HandleConnectionStateChanged(bool bIsConnected);
    
    /** Handler for API errors */
    UFUNCTION(Category = "Vivont")
    void HandleAPIError(const FString& ErrorMessage);
    
private:
    /** Reference to the animation manager */
    UPROPERTY()
    UVivontAnimationManager* AnimationManager;
    
    /** Reference to the OpenAI client */
    UPROPERTY()
    USimpleOpenAIClient* OpenAIClient;
    
    /** Reference to the audio subsystem */
    UPROPERTY()
    UVivontAudioSubsystem* AudioSubsystem;

    /** Whether recording is active */
    bool bIsRecording;
    
    /** Whether connected to the API */
    bool bIsConnected;
    
    /** Accumulated response text */
    FString AccumulatedResponseText;
    
    /** Update the UI state */
    void UpdateUIState();
};
