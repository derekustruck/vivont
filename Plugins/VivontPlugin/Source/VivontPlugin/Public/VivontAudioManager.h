// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AudioCaptureCore.h" // Provides FAudioCapture, FOnAudioCaptureFunction
#include "AudioCaptureDeviceInterface.h" // Provides FAudioCaptureDeviceParams
#include "AudioMixerTypes.h"
#include "DSP/BufferVectorOperations.h"
#include "Containers/Queue.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/CriticalSection.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h" // Include for TUniquePtr
#include "UObject/WeakObjectPtr.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "Subsystems/EngineSubsystem.h"
#include "VivontCaptureDevice.h"
#include "VivontSettings.h" // Included instead of forward declaring
#include "VivontAudioManager.generated.h"

// Forward declarations
class UVivontAnimationManager;

// Forward declare engine types
namespace Audio { class FAudioCapture; } // Forward declare FAudioCapture

/** Delegate triggered when audio capture stops and provides the data */
DECLARE_DELEGATE_OneParam(FVivontCaptureStoppedDelegate, const TArray<uint8>&);

/**
 * Audio Subsystem - Dedicated solely to handling audio playback.
 * Follows UE5 subsystem pattern for better integration with engine lifecycle.
 */
UCLASS()
class VIVONTPLUGIN_API UVivontAudioSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    UVivontAudioSubsystem();
    virtual ~UVivontAudioSubsystem();

    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    // --- Audio Capture ---
    /** [DEPRECATED] Starts capturing audio from the default device using provided settings. */
    bool StartAudioCapture(UVivontSettings* InSettings);
    /** [DEPRECATED] Stops audio capture and triggers the OnCaptureStopped delegate with the captured data. */
    bool StopAudioCapture();
    /** Delegate called when audio capture is stopped. */
    FVivontCaptureStoppedDelegate OnCaptureStopped;
    /** Returns true if audio is currently being captured. */
    bool IsCapturing() const;

    // --- Audio Playback ---
    /** Plays the provided audio data (WAV or raw PCM) using an AudioComponent. Outputs the duration of the audio. */
    UAudioComponent* PlayAudioData(const TArray<uint8>& AudioData, float& OutDuration, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f);
    /** Creates a USoundWaveProcedural from raw WAV byte data. */
    USoundWaveProcedural* CreateSoundWaveFromWavData(const TArray<uint8>& WavData);
    /** Creates a USoundWaveProcedural from raw PCM byte data. */
    USoundWaveProcedural* CreateSoundWaveFromRawData(const TArray<uint8>& RawData, int32 SampleRate = 24000, int32 NumChannels = 1, int32 BitDepth = 16);
    /** Calculates the duration of audio data without creating sound objects. Returns true on success. */
    bool GetAudioDuration(const TArray<uint8>& AudioData, float& OutDuration, int32 DefaultSampleRate = 24000, int32 DefaultNumChannels = 1, int32 DefaultBitDepth = 16);

    UFUNCTION()
    void OnAudioFinishedCallback(); // Callback when audio component finishes playing

private:
    // --- Audio Capture (Deprecated) ---
    TUniquePtr<Vivont::FAudioCaptureDevice> AudioCaptureDevice;

    // --- Audio Components ---
    FCriticalSection AudioCriticalSection; // Protects shared audio resources
    // REMOVED: ProcSoundWave member - a new one will be created per playback.
    TWeakObjectPtr<UAudioComponent> ActiveAudioComponent; // The audio component currently playing (or last played)
    TArray<TWeakObjectPtr<UAudioComponent>> ActiveAudioComponents; // Keep track of all components created by this subsystem (for cleanup)
    TMap<FString, TWeakObjectPtr<USoundWaveProcedural>> CachedSoundWaves; // Cache for potentially reusable sound waves (NOTE: Currently unused, consider removal if not needed)

    // --- Private Methods ---
    /** Stops all audio components created by this subsystem. */
    void StopAllAudio();
    /** Removes inactive/invalid audio components from the tracking array. */
    void CleanupInactiveAudioComponents();
    /** Checks if the provided data appears to be in WAV format. */
    bool IsWAVFormat(const TArray<uint8>& AudioData);
    /** Extracts raw PCM data from a WAV byte array. */
    TArray<uint8> ExtractPCMFromWAV(const TArray<uint8>& WavData);
    /** Gets the sample rate from WAV header data. */
    int32 GetSampleRateFromWAV(const TArray<uint8>& WavData);
    /** Gets the number of channels from WAV header data. */
    int32 GetNumChannelsFromWAV(const TArray<uint8>& WavData);
    /** Gets the bits per sample from WAV header data. */
    int32 GetBitsPerSampleFromWAV(const TArray<uint8>& WavData);
};
