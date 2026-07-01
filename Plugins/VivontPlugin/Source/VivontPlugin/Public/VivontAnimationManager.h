// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Containers/Ticker.h"
#include "VivontAnimationManager.generated.h"

// Forward declarations
class USimpleOpenAIClient;
class UVivontBlendshapeRunner;
class UVivontAudioSubsystem;
class UVivontSettings;
class FVivontLiveLinkSource;
struct FBlendShapeFrame; // Defined in VivontBlendshapeTypes.h

// Declare dynamic multicast delegates so they are available in Blueprints if needed.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnimationStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAnimationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimationError, const FString&, ErrorMessage);

/**
 * UVivontAnimationManager
 * Central manager for animation - receives audio, coordinates blendshape generation,
 * and handles timing/sync of audio playback with blendshape frames.
 */
UCLASS()
class VIVONTPLUGIN_API UVivontAnimationManager : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// Overrides for subsystem lifecycle.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Delegate events to be broadcast when animations start, end, or encounter an error.
	UPROPERTY(BlueprintAssignable, Category = "Vivont|Animation")
	FOnAnimationStarted OnAnimationStarted;

	UPROPERTY(BlueprintAssignable, Category = "Vivont|Animation")
	FOnAnimationEnded OnAnimationEnded;

	UPROPERTY(BlueprintAssignable, Category = "Vivont|Animation")
	FOnAnimationError OnAnimationError;

	// Functions for processing audio and text inputs.
	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	void ProcessAudioData(const TArray<uint8>& AudioData); // Removed WorldContextObject parameter

	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	void ProcessTextInput(const FString& TextInput); // Removed WorldContextObject parameter

	// Functions for handling API/delegate callbacks.
	UFUNCTION()
	void OnBlendShapesReceived(bool bSuccess, const TArray<FBlendShapeFrame>& BlendShapes);

	UFUNCTION()
	void OnAPIErrorReceived(const FString& ErrorMessage);

	UFUNCTION()
	void OnAudioGenerated(const TArray<uint8>& AudioData);

	UFUNCTION()
	void OnTextResponseReceived(const FString& ResponseText);

	// Functions for controlling animation/recording state.
	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	void StopAnimation();

	// Helper functions for converting blend shape frames.
	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	FBlendShapeFrame ConvertToBlendShapeFrame(const TArray<float>& Values);

	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	TArray<float> ConvertFromBlendShapeFrame(const FBlendShapeFrame& Frame);

	// A tick function for additional per-frame processing (if needed).
	UFUNCTION(BlueprintCallable, Category = "Vivont|Animation")
	bool AnimationTick(float DeltaTime);

	/** Gets the duration of the last animation sequence processed. Returns 0 if no sequence is active or processed. */
	UFUNCTION(BlueprintPure, Category = "Vivont|Animation")
	float GetLastAnimationDuration() const;

private:
	// Internal member variables.

	// Stores audio data received from OpenAI while waiting for blendshapes from Vivont API.
	UPROPERTY()
	TArray<uint8> PendingAudioData;

	// Current blend shape frames received from the API.
	UPROPERTY()
	TArray<FBlendShapeFrame> CurrentBlendShapes;

	// Playback timing variables
	// float PlaybackStartTime; // REMOVED - Timing handled by LiveLinkSource
	bool bIsPlayingSyncedAudio; // Flag to indicate if audio *should* be playing (managed by AudioSubsystem)
	float LastAudioDuration; // Duration of the last audio clip played

	// Flags which control animation/recording status.
	// bool bIsAnimating; // REMOVED - State handled by LiveLinkSource
	bool bIsRecording; // Note: Recording logic seems deprecated based on comments

	// References to other subsystems and settings.
	UPROPERTY()
	TObjectPtr<class UVivontBlendshapeRunner> Runner;

	UPROPERTY()
	TObjectPtr<class USimpleOpenAIClient> OpenAIAPI;

	UPROPERTY()
	TObjectPtr<class UVivontAudioSubsystem> AudioSubsystem;

	UPROPERTY()
	TObjectPtr<class UVivontSettings> Settings;

	// Handle for animation tick delegate registration.
	FTSTicker::FDelegateHandle AnimationTickHandle;

	// Reference to LiveLink source
	TSharedPtr<FVivontLiveLinkSource> LiveLinkSource;

	// Utility function for saving audio data to file.
	bool SaveAudioToFile(const TArray<uint8>& AudioData, const FString& Filename, bool bAsWav);

	// Handler for when the LiveLink source signals playback completion
	UFUNCTION()
	void HandlePlaybackSequenceCompleted();

	// Private helper methods for animation timing - REMOVED (Handled by LiveLinkSource)
	// void UpdateAnimationTiming(float DeltaTime);
	// int32 GetFrameIndexAtTime(float CurrentTime);
};
