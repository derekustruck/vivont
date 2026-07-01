// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontLiveLinkSource.h"
#include "VivontLog.h"
#include "VivontBlendshapeTypes.h" // FBlendShapeFrame
#include "Features/IModularFeatures.h"
#include "HAL/PlatformProcess.h"
#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "LiveLinkRoleTrait.h"
#include "Roles/LiveLinkBasicRole.h" // Include the Role header itself/ For Animation Frame Data (might be redundant now, but safe to keep)
#include "FaceBlendShapeDefinition.h"
#include "HAL/CriticalSection.h" // For FCriticalSection
#include "Async/Async.h" // For potential async tasks if needed
#include "Containers/Ticker.h" // Added for FTSTicker
// Removed audio includes: SoundWave, AudioComponent, GameplayStatics, Engine, EngineUtils

#define LOCTEXT_NAMESPACE "VivontLiveLinkSource"

FVivontLiveLinkSource::FVivontLiveLinkSource(const FText& InSourceName)
    : Client(nullptr)
    , SourceGuid(FGuid::NewGuid())
    , SourceName(InSourceName)
    , SubjectName(FName(*FString::Printf(TEXT("VivontFace_%s"), *InSourceName.ToString()))) // Keep unique subject name
    , SourceStatus(LOCTEXT("SourceStatus_Initializing", "Initializing..."))
    , bIsInitialized(false)
    , CurrentPlaybackFrame(0)
    , CurrentState(EPlaybackState::Idle)
    , BlendDuration(0.0833f) // ~1/12th of a second (was 0.0f, which broke blending via divide-by-zero)
    , CurrentBlendTime(0.0f)
{
    // Blend shape arrays are sized dynamically once the model's curve set is known
    // (see SetCurveNames). They start empty, so idle frames simply carry no curves
    // until the first inference result establishes the schema.

    // Update status
    SourceStatus = LOCTEXT("SourceStatus_Ready", "Ready");
    
    UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink Source created: %s"), *InSourceName.ToString());
}

FVivontLiveLinkSource::~FVivontLiveLinkSource()
{
    // Ensure the main ticker is stopped on destruction
    StopMainTicker();

    UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink Source destroyed: %s"), *SourceName.ToString());
}

void FVivontLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
    // Store references
    Client = InClient;
    SourceGuid = InSourceGuid;
    
    // Initialize the subject
    InitializeSubject(); // This sends the initial zero frame (TargetIdleFrame)

    // Start the main ticker immediately after initialization
    StartMainTicker(); // This will handle sending idle data initially
    
    // Mark as initialized and update status
    bIsInitialized = true;
    SourceStatus = LOCTEXT("SourceStatus_Connected", "Connected");
    UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink Source initialized with client: %s"), *SourceName.ToString());
}

bool FVivontLiveLinkSource::IsSourceStillValid() const
{
    // Source is valid if we have a client
    return Client != nullptr;
}

bool FVivontLiveLinkSource::RequestSourceShutdown()
{
    // Update status
    SourceStatus = LOCTEXT("SourceStatus_Shutdown", "Shutting down...");

    // Stop the main ticker
    StopMainTicker();
    
    // Cleanup
    Client = nullptr;
    bIsInitialized = false;

    UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink Source shutdown requested: %s"), *SourceName.ToString());

    // Return true to indicate we can shut down immediately
    return true;
}

FText FVivontLiveLinkSource::GetSourceType() const
{
    return LOCTEXT("SourceType", "Vivont Face");
}

FText FVivontLiveLinkSource::GetSourceMachineName() const
{
    return LOCTEXT("SourceMachineName", "Local Machine");
}

FText FVivontLiveLinkSource::GetSourceStatus() const
{
    return SourceStatus;
}

void FVivontLiveLinkSource::UpdateBlendShapes(const TArray<float>& BlendShapes)
{
    // Check if we have the expected number of blend shapes
    if (BlendShapes.Num() != FACE_BLENDSHAPE_COUNT)
    {
        UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Expected %d blend shapes, but received %d. Ignoring update."),
               *SourceName.ToString(), FACE_BLENDSHAPE_COUNT, BlendShapes.Num());
        return;
    }

    // Lock before updating the shared blend shape data
    FScopeLock Lock(&AnimationDataCriticalSection); // Corrected variable name
    CurrentBlendShapes = BlendShapes;

    // Unlock happens automatically when Lock goes out of scope

    // Send the updated data to LiveLink immediately
    // Note: PushSubjectFrameData_AnyThread allows calling from potentially different threads
    SendBlendShapesToLiveLink();

    // Update status - This function is now primarily for immediate updates if needed,
    // but the main data flow uses ReceiveAnimationData and the timer.
    // Consider if this direct update path is still required or should be removed/refactored.
    // For now, keep it but note its potential redundancy.
    SourceStatus = LOCTEXT("SourceStatus_Receiving", "Receiving Data (Direct Update)"); 
}

void FVivontLiveLinkSource::ReceiveAnimationData(const TArray<FBlendShapeFrame>& AnimationFrames, float InAudioDuration)
{
    if (AnimationFrames.Num() == 0)
    {
        UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Received empty animation data."), *SourceName.ToString());
        return;
    }

    // Lock before accessing shared animation data and state
    FScopeLock Lock(&AnimationDataCriticalSection);

    // Store the new frames and duration
    ReceivedAnimationFrames = AnimationFrames;
    CurrentAudioDuration = InAudioDuration > 0.0f ? InAudioDuration : 0.0f; // Store valid duration, default 0
    CurrentPlaybackFrame = 0; // Always start from the beginning of the new data
    PlaybackElapsedTime = 0.0f; // Reset elapsed time for the new animation

    UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Received %d new animation frames. Audio Duration: %.2f s."), *SourceName.ToString(), ReceivedAnimationFrames.Num(), CurrentAudioDuration);

    // Check the current state to decide how to transition
    if (CurrentState == EPlaybackState::Idle || CurrentState == EPlaybackState::BlendingOut)
    {
        // If we were idle or already blending out, start blending into the new animation
        UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Transitioning from %s to BlendingIn."), *SourceName.ToString(), 
            (CurrentState == EPlaybackState::Idle ? TEXT("Idle") : TEXT("BlendingOut")));

        BlendSourceFrame = CurrentBlendShapes; // Start blend from whatever the current shape is
        if (ReceivedAnimationFrames.IsValidIndex(0))
        {
             BlendTargetFrame = ReceivedAnimationFrames[0].GetRawValues();
             // Ensure target frame matches the active curve count
             if (BlendTargetFrame.Num() != CurveNames.Num()) {
                 BlendTargetFrame.SetNumZeroed(CurveNames.Num());
             }
        }
        else
        {
            // Should not happen if AnimationFrames.Num() > 0, but handle defensively
            UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Received non-empty animation data but index 0 is invalid? Blending to Idle."), *SourceName.ToString());
            BlendTargetFrame = TargetIdleFrame;
        }
        
        CurrentBlendTime = 0.0f; // Reset blend timer
        CurrentState = EPlaybackState::BlendingIn; // Set the new state
        AnimationStartTime = FPlatformTime::Seconds(); // Record start time
        SourceStatus = LOCTEXT("SourceStatus_StartingBlendIn", "Starting Blend In..."); 
    }
    else if (CurrentState == EPlaybackState::Playing || CurrentState == EPlaybackState::BlendingIn)
    {
        // If we are already playing or blending in, the MainTick will pick up the new
        // ReceivedAnimationFrames and CurrentPlaybackFrame reset automatically.
        // We might be interrupting an existing animation or blend.
        // Consider if we need to immediately switch to BlendingIn from the *current*
        // shape to the *new* frame 0, or just let the playback continue/restart.
        // Current logic: Let MainTick handle it by just replacing the data and resetting the index.
        // This means if we were Playing frame 50/100 and receive new data, the next tick
        // will start Playing frame 0/NewTotal. If we were BlendingIn, it will continue
        // blending towards the *old* frame 0, then jump to the *new* frame 0 when Playing starts.
        // This seems acceptable for now.
         UE_LOG(LogVivont, Verbose, TEXT("Vivont (%s): Received new animation data while already %s. Playback will restart/continue with new data."), *SourceName.ToString(),
             (CurrentState == EPlaybackState::Playing ? TEXT("Playing") : TEXT("BlendingIn")));
    }
    // No unlock needed, FScopeLock handles it
}


void FVivontLiveLinkSource::SetCurveNames(const TArray<FName>& InCurveNames)
{
    bool bChanged = false;
    {
        FScopeLock Lock(&AnimationDataCriticalSection);
        if (CurveNames != InCurveNames)
        {
            CurveNames = InCurveNames;

            const int32 Count = CurveNames.Num();
            CurrentBlendShapes.Init(0.0f, Count);
            TargetIdleFrame.Init(0.0f, Count);
            BlendSourceFrame.Init(0.0f, Count);
            BlendTargetFrame.Init(0.0f, Count);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        // Re-advertise the subject's static data with the new curve set.
        if (Client && bIsInitialized)
        {
            InitializeSubject();
        }
        UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Curve set updated to %d curves."), *SourceName.ToString(), InCurveNames.Num());
    }
}

void FVivontLiveLinkSource::InitializeSubject()
{
    if (!Client)
    {
        UE_LOG(LogVivont, Warning, TEXT("VivontLiveLinkSource: No client available to initialize subject"));
        return;
    }
    
    // Create subject key
    FLiveLinkSubjectKey SubjectKey(SourceGuid, SubjectName);
    
    // Create static data structure using the Curve type for Basic Role (curve-only data)
    FLiveLinkStaticDataStruct StaticDataStruct(FLiveLinkBaseStaticData::StaticStruct());
    FLiveLinkBaseStaticData* BaseStaticData = StaticDataStruct.Cast<FLiveLinkBaseStaticData>();
    
    if (!BaseStaticData)
    {
        UE_LOG(LogVivont, Error, TEXT("Vivont (%s): Failed to create/cast BaseStaticData"), *SourceName.ToString());
        return;
    }
    
    // Populate curve names from the active model schema (data-driven; control-rig names).
    BaseStaticData->PropertyNames.Reserve(CurveNames.Num());
    for (const FName& CurveName : CurveNames)
    {
        BaseStaticData->PropertyNames.Add(CurveName);
    }
    
    // Push static data with Basic Role for compatibility with faceARKit
    Client->PushSubjectStaticData_AnyThread(SubjectKey, ULiveLinkBasicRole::StaticClass(), MoveTemp(StaticDataStruct));

    UE_LOG(LogVivont, Log, TEXT("Vivont (%s): LiveLink Subject initialized with Basic Role and %d curves (using BaseStaticData)."), *SourceName.ToString(), BaseStaticData->PropertyNames.Num());

    // Send an initial neutral frame immediately after initialization
    // Send an initial neutral frame immediately after initialization
    // This uses CurrentBlendShapes which is initialized to TargetIdleFrame in the constructor.
    SendBlendShapesToLiveLink(); 
}

void FVivontLiveLinkSource::StartMainTicker()
{
    StopMainTicker(); // Ensure any existing ticker is cleared first

    const float FrameRate = 60.0f; 
    const float TickRate = 1.0f / FrameRate;

    MainTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateSP(this, &FVivontLiveLinkSource::MainTick),
        TickRate
    );

    if (!MainTickerHandle.IsValid())
    {
        UE_LOG(LogVivont, Error, TEXT("Vivont (%s): Failed to register main ticker."), *SourceName.ToString());
        SourceStatus = LOCTEXT("SourceStatus_Error", "Error: Ticker Failed");
    } else {
        UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Main ticker started at rate %.4f s (%.1f FPS)."), *SourceName.ToString(), TickRate, FrameRate);
        // Initial status will be set by the tick function based on state
    }
}

void FVivontLiveLinkSource::StopMainTicker()
{
     if (MainTickerHandle.IsValid())
     {
        FTSTicker::GetCoreTicker().RemoveTicker(MainTickerHandle);
        MainTickerHandle.Reset();
        UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Main ticker stopped."), *SourceName.ToString());
     }
}

void FVivontLiveLinkSource::SendBlendShapesToLiveLink()
{
    if (!Client || !bIsInitialized)
    {
        UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Cannot send blend shapes. Client not initialized."), *SourceName.ToString());
        return;
    }

    // Create frame data structure for Basic Role
    FLiveLinkFrameDataStruct FrameDataStruct(FLiveLinkBaseFrameData::StaticStruct());
    FLiveLinkBaseFrameData* BaseFrameData = FrameDataStruct.Cast<FLiveLinkBaseFrameData>();
    
    if (!BaseFrameData)
    {
        UE_LOG(LogVivont, Error, TEXT("Vivont (%s): Failed to create/cast BaseFrameData"), *SourceName.ToString());
        return;
    }
    
    // --- Set Time Information ---
    // World Time: Use the recorded start time plus elapsed playback time
    // Note: For Idle/Blending states, PlaybackElapsedTime is 0, so WorldTime reflects the start time or current time.
    double CurrentWorldTime = (CurrentState == EPlaybackState::Playing || CurrentState == EPlaybackState::BlendingIn || CurrentState == EPlaybackState::BlendingOut) ? (AnimationStartTime + PlaybackElapsedTime) : FPlatformTime::Seconds();
    BaseFrameData->WorldTime = FLiveLinkWorldTime(CurrentWorldTime); // Construct WorldTime using the calculated time

    // Scene Time: Use the discrete CurrentPlaybackFrame index
    const int32 FrameRateNum = 60;
    const int32 FrameRateDen = 1;
    FFrameRate Rate(FrameRateNum, FrameRateDen);
    // Calculate frame time based on elapsed time during playback
    // Note: PlaybackElapsedTime is only updated in the Playing state. 
    // For Idle/Blending, frame time might be less meaningful, but we set it anyway.
    // Consider if a different frame time calculation is needed for non-playing states.
    // Use the integer CurrentPlaybackFrame for SceneTime's frame number
    FFrameTime Time(CurrentPlaybackFrame); 
    BaseFrameData->MetaData.SceneTime = FQualifiedFrameTime(Time, Rate);
    
    // Set properties (curves) array
    FScopeLock Lock(&AnimationDataCriticalSection);
    
    // Clear & pre-allocate property values array
    BaseFrameData->PropertyValues.Empty(CurrentBlendShapes.Num());
    
    // Add all current blend shape values to the property values array
    for (float Value : CurrentBlendShapes)
    {
        BaseFrameData->PropertyValues.Add(Value);
    }

    // Create Subject Frame
    FLiveLinkSubjectKey SubjectKey(SourceGuid, SubjectName);
    
    // Push frame to client
    Client->PushSubjectFrameData_AnyThread(SubjectKey, MoveTemp(FrameDataStruct));
}

// --- MainTick Implementation (Replaces PlaybackTickInternal and DummyDataTick) ---
bool FVivontLiveLinkSource::MainTick(float DeltaTime) // DeltaTime parameter is now unused but kept for delegate signature
{
	// --- Calculate Actual Delta Time ---
	double CurrentTime = FPlatformTime::Seconds();
	// Use expected delta for the first frame, otherwise calculate actual elapsed time
	float ActualDeltaTime = (LastTickTime > 0.0) ? (float)(CurrentTime - LastTickTime) : (1.0f / 60.0f); 
	LastTickTime = CurrentTime;
	// Optional: Add logging here to check ActualDeltaTime values if needed
	// UE_LOG(LogVivont, Log, TEXT("Vivont (%s): MainTick - ActualDeltaTime: %.6f"), *SourceName.ToString(), ActualDeltaTime);

	FScopeLock Lock(&AnimationDataCriticalSection);

	switch (CurrentState)
    {
        case EPlaybackState::Idle:
        {
            // Ensure we are sending the target idle frame
            if (CurrentBlendShapes != TargetIdleFrame)
            {
                CurrentBlendShapes = TargetIdleFrame;
            }
            SourceStatus = LOCTEXT("SourceStatus_Idle", "Idle (Sending Dummy Data)");
            break; // Keep sending idle frame
        }

		case EPlaybackState::BlendingIn:
		{
			CurrentBlendTime += ActualDeltaTime; // Use calculated delta
			float BlendAlpha = FMath::Clamp(CurrentBlendTime / BlendDuration, 0.0f, 1.0f);

			// Perform Lerp
            for (int32 i = 0; i < CurrentBlendShapes.Num(); ++i)
            {
                CurrentBlendShapes[i] = FMath::Lerp(BlendSourceFrame[i], BlendTargetFrame[i], BlendAlpha);
            }

            SourceStatus = FText::Format(LOCTEXT("SourceStatus_BlendingInFmt", "Blending In ({0}%)"), FText::AsPercent(BlendAlpha));

            if (CurrentBlendTime >= BlendDuration)
            {
                // Blending finished, transition to Playing
                CurrentState = EPlaybackState::Playing;
                CurrentBlendTime = 0.0f; // Reset blend time
                CurrentBlendShapes = BlendTargetFrame; // Ensure exact target frame is set
                UE_LOG(LogVivont, Verbose, TEXT("Vivont (%s): Blending In complete. Transitioning to Playing."), *SourceName.ToString());
            }
            break;
        }

		case EPlaybackState::Playing:
		{
			// --- Time-based Frame Calculation ---
			PlaybackElapsedTime += ActualDeltaTime; // Use calculated delta
			int32 TargetFrameIndex = 0;
			int32 TotalFrames = ReceivedAnimationFrames.Num();

            if (CurrentAudioDuration > 0.0f && TotalFrames > 0)
            {
                // Calculate the target frame based on elapsed time vs total duration
                float TargetFrameFraction = FMath::Clamp(PlaybackElapsedTime / CurrentAudioDuration, 0.0f, 1.0f);
                TargetFrameIndex = FMath::FloorToInt(TargetFrameFraction * TotalFrames);
                // Clamp to ensure we don't exceed bounds, especially with float precision
                TargetFrameIndex = FMath::Clamp(TargetFrameIndex, 0, TotalFrames - 1); 
            }
            else if (TotalFrames > 0)
            {
                 // Fallback if duration is invalid: play frame by frame (original behavior, but shouldn't happen)
                 // Or potentially just jump to the end? Let's stick to frame-by-frame for fallback.
                 TargetFrameIndex = CurrentPlaybackFrame + 1; // Old logic essentially
                 TargetFrameIndex = FMath::Clamp(TargetFrameIndex, 0, TotalFrames - 1);
                 UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Playing state with invalid audio duration (%.2f). Falling back to frame increment."), *SourceName.ToString(), CurrentAudioDuration);
            }
            
            CurrentPlaybackFrame = TargetFrameIndex; // Update the frame index based on time

            // --- Get and Apply Frame Data ---
            if (ReceivedAnimationFrames.IsValidIndex(CurrentPlaybackFrame)) // Check index validity *after* calculation
            {
                // Get the current frame's data
                const FBlendShapeFrame& FrameData = ReceivedAnimationFrames[CurrentPlaybackFrame];
                CurrentBlendShapes = FrameData.GetRawValues();

                // Ensure correct size (safety check)
                if (CurrentBlendShapes.Num() != CurveNames.Num())
                {
                    UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): Playback frame %d has %d shapes, expected %d. Padding/Truncating."),
                           *SourceName.ToString(), CurrentPlaybackFrame, CurrentBlendShapes.Num(), CurveNames.Num());
                    CurrentBlendShapes.SetNumZeroed(CurveNames.Num());
                }
                
                // Update status using elapsed time and duration for better feedback
                SourceStatus = FText::Format(LOCTEXT("SourceStatus_PlayingTimeFmt", "Playing Animation ({0}s / {1}s) Frame {2}/{3}"), 
                    FText::AsNumber(PlaybackElapsedTime), 
                    FText::AsNumber(CurrentAudioDuration), 
                    FText::AsNumber(CurrentPlaybackFrame + 1), 
                    FText::AsNumber(TotalFrames));

                // REMOVED: CurrentPlaybackFrame++; // No longer needed, index is calculated from time

                // Check if playback time has reached or exceeded the audio duration
                if (PlaybackElapsedTime >= CurrentAudioDuration && CurrentAudioDuration > 0.0f)
                {
                    // Broadcast completion delegate FIRST
                    UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Playback sequence completed (Duration: %.2f s). Broadcasting delegate."), *SourceName.ToString(), CurrentAudioDuration);
                    OnPlaybackSequenceCompleted.ExecuteIfBound();

                    // Prepare for Blending Out
                    UE_LOG(LogVivont, Log, TEXT("Vivont (%s): Preparing to blend out."), *SourceName.ToString());
                    BlendSourceFrame = CurrentBlendShapes; // Start blend from the last calculated frame
                    BlendTargetFrame = TargetIdleFrame;    // Blend towards idle
                    CurrentBlendTime = 0.0f;
                    PlaybackElapsedTime = 0.0f; // Reset elapsed time
                    CurrentState = EPlaybackState::BlendingOut;
                    ReceivedAnimationFrames.Empty(); // Clear frames now that we're done
                }
            }
            else
            {
                 // This case might occur if TotalFrames is 0 or TargetFrameIndex calculation leads to invalid index
                 UE_LOG(LogVivont, Warning, TEXT("Vivont (%s): In Playing state but calculated CurrentPlaybackFrame (%d) is invalid for ReceivedAnimationFrames (%d). Transitioning to Idle."), 
                    *SourceName.ToString(), CurrentPlaybackFrame, TotalFrames);
                 CurrentState = EPlaybackState::Idle;
                 CurrentBlendShapes = TargetIdleFrame; // Go straight to idle
                 ReceivedAnimationFrames.Empty(); 
            }
            break;
        }

		case EPlaybackState::BlendingOut:
		{
			CurrentBlendTime += ActualDeltaTime; // Use calculated delta
			float BlendAlpha = FMath::Clamp(CurrentBlendTime / BlendDuration, 0.0f, 1.0f);

			// Perform Lerp
            for (int32 i = 0; i < CurrentBlendShapes.Num(); ++i)
            {
                CurrentBlendShapes[i] = FMath::Lerp(BlendSourceFrame[i], BlendTargetFrame[i], BlendAlpha);
            }

            SourceStatus = FText::Format(LOCTEXT("SourceStatus_BlendingOutFmt", "Blending Out ({0}%)"), FText::AsPercent(BlendAlpha));

            if (CurrentBlendTime >= BlendDuration)
            {
                // Blending finished, transition to Idle
                CurrentState = EPlaybackState::Idle;
                CurrentBlendTime = 0.0f; // Reset blend time
                CurrentBlendShapes = TargetIdleFrame; // Ensure exact idle frame is set
                UE_LOG(LogVivont, Verbose, TEXT("Vivont (%s): Blending Out complete. Transitioning to Idle."), *SourceName.ToString());
            }
            break;
        }
    }

    Lock.Unlock(); // Unlock before sending data

    // Send the calculated blend shapes to LiveLink regardless of state
    SendBlendShapesToLiveLink();

    return true; // Keep the ticker running
}
