// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "FaceBlendShapeDefinition.h"
#include "Templates/SharedPointer.h" // Includes TSharedFromThis definition
#include "Containers/Ticker.h" // Correct include for FTSTicker
#include "Math/UnrealMathUtility.h" // For FMath::Lerp
struct FBlendShapeFrame;

// Delegate broadcast when a playback sequence naturally completes
DECLARE_DELEGATE(FOnPlaybackSequenceCompleted);

// Enum to manage the playback state
enum class EPlaybackState : uint8
{
	Idle,        // Sending dummy/idle data
	BlendingIn,  // Blending from Idle to first animation frame
	Playing,     // Playing the received animation frames
	BlendingOut  // Blending from last animation frame back to Idle
};

/**
 * Vivont LiveLink Source implementation.
 * This provides an interface to insert Vivont animation data into Unreal's LiveLink system.
 */
class VIVONTPLUGIN_API FVivontLiveLinkSource : public ILiveLinkSource, public TSharedFromThis<FVivontLiveLinkSource>
{
public:
    FVivontLiveLinkSource(const FText& InSourceName); // Renamed parameter for clarity
    virtual ~FVivontLiveLinkSource();

    // Begin ILiveLinkSource Interface
    virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;
    virtual bool IsSourceStillValid() const override;
    virtual bool RequestSourceShutdown() override;
    virtual FText GetSourceType() const override;
    virtual FText GetSourceMachineName() const override;
    virtual FText GetSourceStatus() const override;
    // End ILiveLinkSource Interface

    // Update the blend shapes received from the external source (DEPRECATED - Use ReceiveAnimationData)
    void UpdateBlendShapes(const TArray<float>& BlendShapes);

    /** Receive the full animation data (array of frames) and the corresponding audio duration */
    void ReceiveAnimationData(const TArray<FBlendShapeFrame>& AnimationFrames, float InAudioDuration);

    /**
     * Set the LiveLink curve set from the active model's output schema (data-driven).
     * Re-pushes the subject's static data so the MetaHuman receives the correct
     * control-rig curve names. Safe to call repeatedly; no-ops if unchanged.
     */
    void SetCurveNames(const TArray<FName>& InCurveNames);

    // Diagnose LiveLink connection issues
    // void DiagnoseLiveLinkConnection();

    // Get the source name (used internally or for debugging)
    FText GetSourceName() const { return SourceName; }

    // Delegate broadcast when playback sequence completes based on duration
    FOnPlaybackSequenceCompleted OnPlaybackSequenceCompleted;

private:
    // Handle to the LiveLink client
    ILiveLinkClient* Client;
    
    // Source GUID
    FGuid SourceGuid;
    
    // Name of this source
    FText SourceName;
    
    // Name of the LiveLink subject
    FName SubjectName;

    // Active output curve names (data-driven from the model manifest). Defines curve count.
    TArray<FName> CurveNames;
    
    // Status text to display
    FText SourceStatus;
    
    // Track if source is initialized and connected to a client
    bool bIsInitialized;

    // Blend shape values received from the external source
    TArray<float> CurrentBlendShapes;

    // Critical section to protect access to blend shape data if updated/read from different threads
    FCriticalSection AnimationDataCriticalSection;

    // Store the received animation frames
    TArray<FBlendShapeFrame> ReceivedAnimationFrames;
    
    // Current frame index for playback (used for display/logging, timing uses elapsed time)
    int32 CurrentPlaybackFrame;

    // --- Timing ---
    float CurrentAudioDuration; // Duration of the currently playing audio clip
    float PlaybackElapsedTime;  // Time elapsed since the current animation started playing
    double AnimationStartTime = 0.0; // World time (seconds) when the current animation sequence started

    // --- State Management & Blending ---
    EPlaybackState CurrentState;
    float BlendDuration; // Duration of the blend in/out in seconds
    float CurrentBlendTime; // Current time elapsed during a blend
    TArray<float> BlendSourceFrame; // Frame blending *from*
	TArray<float> BlendTargetFrame; // Frame blending *to*
	TArray<float> TargetIdleFrame; // The target idle/dummy frame (usually zeros)
	double LastTickTime = 0.0; // Added for precise delta calculation

	// Unified ticker handle for continuous updates (replaces PlaybackTickerHandle and DummyDataTickerHandle)
	FTSTicker::FDelegateHandle MainTickerHandle;

    // Initialize the LiveLink subject (called when client is received)
    void InitializeSubject();
    
    // Send the current blend shapes to LiveLink (called by the main ticker)
    void SendBlendShapesToLiveLink();

    // Start the main ticker
    void StartMainTicker();

    // Stop the main ticker
    void StopMainTicker();

    // Main tick function called by the unified ticker
    bool MainTick(float DeltaTime);
};
