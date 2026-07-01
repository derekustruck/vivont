// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontAudioManager.h"
#include "VivontCaptureDevice.h" // For TUniquePtr destructor
#include "VivontSettings.h"
#include "VivontLog.h"
#include "Kismet/GameplayStatics.h"
#include "AudioMixerDevice.h"
#include "AudioDeviceManager.h"
#include "AudioCaptureCore.h"
#include "AudioCaptureDeviceInterface.h"
#include "Features/IModularFeatures.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "AudioDevice.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundWave.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "DSP/BufferVectorOperations.h"
#include "Misc/ScopeLock.h"
#include "Stats/Stats.h"

// Manual FindBySequence implementation (Keep for WAV parsing)
const uint8* ManualFindBySequence(const uint8* Start, const uint8* End, const uint8* PatternStart, const uint8* PatternEnd)
{
    int64 HaystackSize = End - Start;
    int64 NeedleSize = PatternEnd - PatternStart;
    if (NeedleSize <= 0 || HaystackSize < NeedleSize)
    {
        return End;
    }
    for (int64 i = 0; i <= HaystackSize - NeedleSize; ++i)
    {
        if (FMemory::Memcmp(Start + i, PatternStart, NeedleSize) == 0)
        {
            return Start + i;
        }
    }
    return End;
}

//
// UVivontAudioSubsystem Implementation
//

UVivontAudioSubsystem::UVivontAudioSubsystem()
{
    UE_LOG(LogVivontAudio, Log, TEXT("Vivont Audio Subsystem created"));
}

UVivontAudioSubsystem::~UVivontAudioSubsystem() 
{
    UE_LOG(LogVivontAudio, Log, TEXT("Vivont Audio Subsystem destroyed"));
}

void UVivontAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection) 
{
    Super::Initialize(Collection);
    
    // Initialize the audio capture device for the deprecated methods
    AudioCaptureDevice = MakeUnique<Vivont::FAudioCaptureDevice>();
    
    UE_LOG(LogVivontAudio, Log, TEXT("Vivont Audio Subsystem initialized"));
}

void UVivontAudioSubsystem::Deinitialize() 
{
    UE_LOG(LogVivontAudio, Log, TEXT("Vivont Audio Subsystem deinitializing..."));

    // Clean up audio
    StopAllAudio();
    
    // Clean up audio capture device if still active
    if (AudioCaptureDevice.IsValid() && AudioCaptureDevice->IsCapturing())
    {
        AudioCaptureDevice->StopCapture();
    }
    AudioCaptureDevice.Reset();
    
    // REMOVED: Clean up ProcSoundWave member, as it's no longer used.
    
    UE_LOG(LogVivontAudio, Log, TEXT("Vivont Audio Subsystem deinitialized"));
    Super::Deinitialize();
}

// --- Audio Capture Implementation (Deprecated) ---

bool UVivontAudioSubsystem::StartAudioCapture(UVivontSettings* InSettings)
{
    UE_LOG(LogVivontAudio, Warning, TEXT("StartAudioCapture is deprecated. Use VivontAudioCapture module instead."));
    return false;
}

bool UVivontAudioSubsystem::StopAudioCapture()
{
    UE_LOG(LogVivontAudio, Warning, TEXT("StopAudioCapture is deprecated. Use VivontAudioCapture module instead."));
    return false;
}

bool UVivontAudioSubsystem::IsCapturing() const 
{
    // Check validity before accessing
    return AudioCaptureDevice.IsValid() && AudioCaptureDevice->IsCapturing();
}

// --- Audio Playback Implementation ---

UAudioComponent* UVivontAudioSubsystem::PlayAudioData(const TArray<uint8>& AudioData, float& OutDuration, float VolumeMultiplier, float PitchMultiplier) 
{
    FScopeLock Lock(&AudioCriticalSection);
    OutDuration = 0.0f; // Initialize output duration
    
    if (AudioData.Num() == 0)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("PlayAudioData: Cannot play empty audio data"));
        return nullptr;
    }
    
    // Use GameViewport->GetWorld() for potentially better context reliability
    UWorld* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr; 
    if (!World)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("PlayAudioData: Failed to obtain valid World context via GEngine->GameViewport->GetWorld()."));
        return nullptr;
    }
    
    // Create sound wave based on data format
    USoundWaveProcedural* SoundWave = IsWAVFormat(AudioData) 
        ? CreateSoundWaveFromWavData(AudioData) 
        : CreateSoundWaveFromRawData(AudioData, 24000, 1, 16);
        
    if (!SoundWave)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("PlayAudioData: Failed to create sound wave"));
        return nullptr;
    }
    
    // REMOVED: Storing ProcSoundWave member reference. SoundWave is local.
    
    // Set the duration before returning
    OutDuration = SoundWave ? SoundWave->Duration : 0.0f;
    
    // Create audio component for playback using the obtained World
    UAudioComponent* NewAudioComponent = UGameplayStatics::CreateSound2D(
        World, SoundWave, VolumeMultiplier, PitchMultiplier, 0.f, nullptr, true);
        
    if (!NewAudioComponent)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("PlayAudioData: Failed to create audio component"));
        // If component creation failed, ensure the locally created SoundWave is cleaned up
        if (SoundWave && SoundWave->IsRooted())
        {
            SoundWave->RemoveFromRoot();
        }
        // ProcSoundWave = nullptr; // No longer needed, SoundWave is local
        return nullptr;
    }
    
    // Configure the audio component
    NewAudioComponent->bAutoDestroy = true;
    NewAudioComponent->OnAudioFinished.AddDynamic(this, &UVivontAudioSubsystem::OnAudioFinishedCallback);
    
    UE_LOG(LogVivontAudio, Log, TEXT("PlayAudioData: Created AudioComponent %s for SoundWave %s (Duration: %.2f)"), 
        *NewAudioComponent->GetName(), *SoundWave->GetName(), SoundWave->Duration);
    
    // Start playback
    NewAudioComponent->Play();
    
    // Verify playback started
    if (!NewAudioComponent->IsPlaying())
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("PlayAudioData: Component %s not playing immediately after Play() call."), 
            *NewAudioComponent->GetName());
    }
    else
    {
        UE_LOG(LogVivontAudio, Log, TEXT("PlayAudioData: Component %s reported playing."), 
            *NewAudioComponent->GetName());
    }
    
    // Track this component
    ActiveAudioComponents.Add(NewAudioComponent);
    ActiveAudioComponent = NewAudioComponent;
    
    return NewAudioComponent;
}

USoundWaveProcedural* UVivontAudioSubsystem::CreateSoundWaveFromWavData(const TArray<uint8>& WavData) 
{
    UE_LOG(LogVivontAudio, Verbose, TEXT("CreateSoundWaveFromWavData: Processing %d bytes"), WavData.Num());
    
    // Validate WAV format
    if (!IsWAVFormat(WavData) || WavData.Num() < 44)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Invalid WAV data"));
        return nullptr;
    }
    
    // Extract audio parameters from WAV header
    int32 NumChannels = GetNumChannelsFromWAV(WavData);
    int32 SampleRate = GetSampleRateFromWAV(WavData);
    int32 BitsPerSample = GetBitsPerSampleFromWAV(WavData);
    
    // Extract PCM data from WAV
    TArray<uint8> PCMData = ExtractPCMFromWAV(WavData);
    if (PCMData.Num() == 0)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Failed to extract PCM data"));
        return nullptr;
    }
    
    // Validate audio parameters
    if (SampleRate <= 0 || NumChannels <= 0 || BitsPerSample <= 0 || (BitsPerSample % 8 != 0))
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Invalid WAV parameters: SR=%d, Ch=%d, BPS=%d"), 
            SampleRate, NumChannels, BitsPerSample);
        return nullptr;
    }
    
    // Create procedural sound wave
    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(GetTransientPackage());
    if (!SoundWave)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Failed to create sound wave object"));
        return nullptr;
    }
    
    // Configure sound wave
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    
    int32 BytesPerSample = BitsPerSample / 8;
    SoundWave->Duration = (float)PCMData.Num() / (SampleRate * NumChannels * BytesPerSample);
    SoundWave->bLooping = false;
    SoundWave->bProcedural = true;
    SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Voice;
    
    // Prevent garbage collection
    SoundWave->AddToRoot();
    
    // Queue audio data
    SoundWave->QueueAudio(PCMData.GetData(), PCMData.Num());
    
    UE_LOG(LogVivontAudio, Log, TEXT("Created sound wave from WAV: %.2fs, %dHz, %dch"), 
        SoundWave->Duration, SampleRate, NumChannels);
        
    return SoundWave;
}

USoundWaveProcedural* UVivontAudioSubsystem::CreateSoundWaveFromRawData(const TArray<uint8>& RawData, int32 SampleRate, int32 NumChannels, int32 BitDepth) 
{
    // Validate input
    if (RawData.Num() == 0)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Empty raw audio data"));
        return nullptr;
    }
    
    if (SampleRate <= 0 || NumChannels <= 0 || BitDepth <= 0 || (BitDepth % 8 != 0))
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Invalid raw audio parameters: SR=%d, Ch=%d, BD=%d"), 
            SampleRate, NumChannels, BitDepth);
        return nullptr;
    }
    
    // Create procedural sound wave
    USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(GetTransientPackage());
    if (!SoundWave)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("Failed to create sound wave object"));
        return nullptr;
    }
    
    // Configure sound wave
    SoundWave->SetSampleRate(SampleRate);
    SoundWave->NumChannels = NumChannels;
    
    int32 BytesPerSample = BitDepth / 8;
    SoundWave->Duration = (float)RawData.Num() / (SampleRate * NumChannels * BytesPerSample);
    SoundWave->bLooping = false;
    SoundWave->bProcedural = true;
    SoundWave->SoundGroup = ESoundGroup::SOUNDGROUP_Voice;
    
    // Prevent garbage collection
    SoundWave->AddToRoot();
    
    // Queue audio data
    SoundWave->QueueAudio(RawData.GetData(), RawData.Num());
    
    UE_LOG(LogVivontAudio, Log, TEXT("Created sound wave from PCM: %dHz, %dch, %.2fs, %d bytes"), 
        SampleRate, NumChannels, SoundWave->Duration, RawData.Num());
        
    return SoundWave;
}

void UVivontAudioSubsystem::OnAudioFinishedCallback() 
{
    FScopeLock Lock(&AudioCriticalSection);
    UE_LOG(LogVivontAudio, Log, TEXT("OnAudioFinishedCallback invoked"));
    
    // Find the finished component
    UAudioComponent* FinishedComponent = nullptr;
    for (int32 i = ActiveAudioComponents.Num() - 1; i >= 0; --i) 
    {
        UAudioComponent* Comp = ActiveAudioComponents[i].Get();
        
        if (IsValid(Comp) && !Comp->IsPlaying()) 
        {
            FinishedComponent = Comp;
            UE_LOG(LogVivontAudio, Log, TEXT("Found finished component: %s"), *FinishedComponent->GetName());
            ActiveAudioComponents.RemoveAt(i);
            break;
        } 
        else if (!IsValid(Comp)) 
        {
            UE_LOG(LogVivontAudio, Warning, TEXT("Removed invalid audio component pointer during OnAudioFinishedCallback check."));
            ActiveAudioComponents.RemoveAt(i);
        }
    }
    
    // Handle the case where we found a finished component
    if (FinishedComponent) 
    {
        if (FinishedComponent == ActiveAudioComponent.Get()) 
        {
            UE_LOG(LogVivontAudio, Log, TEXT("Active audio component finished."));
            ActiveAudioComponent = nullptr;
        }
    } 
    else 
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("OnAudioFinishedCallback invoked, but no finished component found in active list. List size: %d"), 
            ActiveAudioComponents.Num());
            
        if (!ActiveAudioComponent.IsValid()) 
        {
            UE_LOG(LogVivontAudio, Warning, TEXT("ActiveAudioComponent became invalid."));
            ActiveAudioComponent = nullptr;
        }
    }
    
    // Cleanup any inactive components
    CleanupInactiveAudioComponents();
}

// --- Audio Playback Utilities ---

void UVivontAudioSubsystem::StopAllAudio() 
{
    FScopeLock Lock(&AudioCriticalSection);
    UE_LOG(LogVivontAudio, Log, TEXT("StopAllAudio: Stopping %d active components"), ActiveAudioComponents.Num());
    
    // Stop all active components
    for (int32 i = ActiveAudioComponents.Num() - 1; i >= 0; i--) 
    {
        UAudioComponent* Component = ActiveAudioComponents[i].Get();
        if (IsValid(Component)) 
        {
            if (Component->IsPlaying()) 
            {
                UE_LOG(LogVivontAudio, Verbose, TEXT("Stopping audio component %d (%s)"), i, *Component->GetName());
                Component->OnAudioFinished.RemoveAll(this);
                Component->Stop();
            }
        }
        else 
        {
            UE_LOG(LogVivontAudio, Warning, TEXT("StopAllAudio: Found invalid component pointer at index %d"), i);
        }
    }
    
    // Clear all active components
    ActiveAudioComponents.Empty();
    ActiveAudioComponent = nullptr;
    
    // REMOVED: Resetting ProcSoundWave member, as it's no longer used.
    // Individual sound waves are tied to components which are stopped/destroyed.
    
    UE_LOG(LogVivontAudio, Log, TEXT("StopAllAudio: All audio playback stopped"));
}

void UVivontAudioSubsystem::CleanupInactiveAudioComponents() 
{
    FScopeLock Lock(&AudioCriticalSection);
    bool bRemoved = false;
    
    // Remove invalid or inactive components
    for (int32 i = ActiveAudioComponents.Num() - 1; i >= 0; i--) 
    {
        UAudioComponent* Component = ActiveAudioComponents[i].Get();
        if (!Component || !Component->IsPlaying()) 
        {
            UE_LOG(LogVivontAudio, Verbose, TEXT("Cleaning up inactive/invalid component %s"), 
                Component ? *Component->GetName() : TEXT("INVALID"));
            ActiveAudioComponents.RemoveAt(i);
            bRemoved = true;
        }
    }
    
    if (bRemoved)
    {
        UE_LOG(LogVivontAudio, Log, TEXT("CleanupInactiveAudioComponents finished. Remaining: %d"), 
            ActiveAudioComponents.Num());
    }
}

// --- Audio Duration Calculation ---

bool UVivontAudioSubsystem::GetAudioDuration(const TArray<uint8>& AudioData, float& OutDuration, int32 DefaultSampleRate, int32 DefaultNumChannels, int32 DefaultBitDepth)
{
    OutDuration = 0.0f;
    if (AudioData.Num() == 0)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("GetAudioDuration: Cannot calculate duration for empty audio data"));
        return false;
    }

    int32 SampleRate = 0;
    int32 NumChannels = 0;
    int32 BitsPerSample = 0;
    int32 BytesPerSample = 0;
    uint32 PCMDataSize = 0;

    if (IsWAVFormat(AudioData))
    {
        if (AudioData.Num() < 44) // Need at least header size
        {
             UE_LOG(LogVivontAudio, Warning, TEXT("GetAudioDuration: WAV data too small (%d bytes)"), AudioData.Num());
             return false;
        }
        SampleRate = GetSampleRateFromWAV(AudioData);
        NumChannels = GetNumChannelsFromWAV(AudioData);
        BitsPerSample = GetBitsPerSampleFromWAV(AudioData);

        // Find the 'data' chunk size
        const uint8* WavPtr = AudioData.GetData();
        const int32 WavSize = AudioData.Num();
        const uint8 DataChunkID[] = { 'd', 'a', 't', 'a' };
        const int32 ChunkIDSize = UE_ARRAY_COUNT(DataChunkID);
        const uint8* DataChunkPtr = ManualFindBySequence(WavPtr + 12, WavPtr + WavSize, DataChunkID, DataChunkID + ChunkIDSize);

        if (DataChunkPtr == WavPtr + WavSize)
        {
            UE_LOG(LogVivontAudio, Warning, TEXT("GetAudioDuration: Failed to find 'data' chunk in WAV"));
            return false;
        }

        int32 DataSizeOffset = (DataChunkPtr - WavPtr) + ChunkIDSize;
        if (DataSizeOffset + 4 > WavSize)
        {
             UE_LOG(LogVivontAudio, Warning, TEXT("GetAudioDuration: Not enough data after 'data' chunk ID in WAV"));
             return false;
        }
        FMemory::Memcpy(&PCMDataSize, WavPtr + DataSizeOffset, sizeof(PCMDataSize));

        if (PCMDataSize == 0)
        {
             UE_LOG(LogVivontAudio, Warning, TEXT("GetAudioDuration: PCM data size in WAV header is zero"));
             return false;
        }
    }
    else
    {
        // Assume raw PCM data
        SampleRate = DefaultSampleRate;
        NumChannels = DefaultNumChannels;
        BitsPerSample = DefaultBitDepth;
        PCMDataSize = AudioData.Num();
    }

    // Validate parameters
    if (SampleRate <= 0 || NumChannels <= 0 || BitsPerSample <= 0 || (BitsPerSample % 8 != 0))
    {
        UE_LOG(LogVivontAudio, Error, TEXT("GetAudioDuration: Invalid audio parameters (SR=%d, Ch=%d, BPS=%d)"),
            SampleRate, NumChannels, BitsPerSample);
        return false;
    }

    BytesPerSample = BitsPerSample / 8;
    if (SampleRate * NumChannels * BytesPerSample == 0)
    {
        UE_LOG(LogVivontAudio, Error, TEXT("GetAudioDuration: Calculated zero divisor for duration calculation"));
        return false;
    }

    OutDuration = (float)PCMDataSize / (SampleRate * NumChannels * BytesPerSample);
    UE_LOG(LogVivontAudio, Verbose, TEXT("GetAudioDuration: Calculated duration %.4f seconds (PCM Size: %u, SR: %d, Ch: %d, BPS: %d)"),
        OutDuration, PCMDataSize, SampleRate, NumChannels, BitsPerSample);

    return true;
}


// --- WAV Format Utilities ---

bool UVivontAudioSubsystem::IsWAVFormat(const TArray<uint8>& AudioData)
{
    return AudioData.Num() >= 12 &&
           AudioData[0] == 'R' &&
           AudioData[1] == 'I' &&
           AudioData[2] == 'F' &&
           AudioData[3] == 'F' &&
           AudioData[8] == 'W' &&
           AudioData[9] == 'A' &&
           AudioData[10] == 'V' &&
           AudioData[11] == 'E';
}

TArray<uint8> UVivontAudioSubsystem::ExtractPCMFromWAV(const TArray<uint8>& WavData) 
{
    TArray<uint8> PCMData;
    
    // Validate WAV format
    if (!IsWAVFormat(WavData) || WavData.Num() < 44)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("ExtractPCMFromWAV: Invalid WAV"));
        return PCMData;
    }
    
    // Find the 'data' chunk
    const uint8* WavPtr = WavData.GetData();
    const int32 WavSize = WavData.Num();
    const uint8 DataChunkID[] = { 'd', 'a', 't', 'a' };
    const int32 ChunkIDSize = UE_ARRAY_COUNT(DataChunkID);
    const uint8* DataChunkPtr = ManualFindBySequence(WavPtr + 12, WavPtr + WavSize, DataChunkID, DataChunkID + ChunkIDSize);
    
    if (DataChunkPtr == WavPtr + WavSize)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("ExtractPCMFromWAV: Failed to find 'data' chunk"));
        return PCMData;
    }
    
    // Get the data size and offset
    int32 DataSizeOffset = (DataChunkPtr - WavPtr) + ChunkIDSize;
    int32 PCMDataOffset = DataSizeOffset + 4;
    
    if (PCMDataOffset >= WavSize)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("ExtractPCMFromWAV: Not enough data after 'data' chunk ID"));
        return PCMData;
    }
    
    // Get the data size
    uint32 DataSize = 0;
    FMemory::Memcpy(&DataSize, WavPtr + DataSizeOffset, sizeof(DataSize));
    
    if (PCMDataOffset + DataSize > (uint32)WavSize)
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("ExtractPCMFromWAV: Chunk size (%u) exceeds available bytes (%d)"), 
            DataSize, WavSize - PCMDataOffset);
        return PCMData;
    }
    
    // Extract the PCM data
    PCMData.Append(WavPtr + PCMDataOffset, DataSize);
    UE_LOG(LogVivontAudio, Verbose, TEXT("Extracted %d bytes PCM from WAV"), PCMData.Num());
    
    return PCMData;
}

int32 UVivontAudioSubsystem::GetSampleRateFromWAV(const TArray<uint8>& WavData) 
{
    if (!IsWAVFormat(WavData) || WavData.Num() < 28)
    {
        return 0;
    }
    
    uint32 SampleRate = 0;
    FMemory::Memcpy(&SampleRate, WavData.GetData() + 24, sizeof(SampleRate));
    return SampleRate;
}

int32 UVivontAudioSubsystem::GetNumChannelsFromWAV(const TArray<uint8>& WavData) 
{
    if (!IsWAVFormat(WavData) || WavData.Num() < 24)
    {
        return 0;
    }
    
    uint16 NumChannels = 0;
    FMemory::Memcpy(&NumChannels, WavData.GetData() + 22, sizeof(NumChannels));
    return NumChannels;
}

int32 UVivontAudioSubsystem::GetBitsPerSampleFromWAV(const TArray<uint8>& WavData) 
{
    if (!IsWAVFormat(WavData) || WavData.Num() < 36)
    {
        return 0;
    }
    
    uint16 BitsPerSample = 0;
    FMemory::Memcpy(&BitsPerSample, WavData.GetData() + 34, sizeof(BitsPerSample));
    return BitsPerSample;
}
