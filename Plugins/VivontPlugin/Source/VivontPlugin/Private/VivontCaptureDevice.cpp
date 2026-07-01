// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontCaptureDevice.h"
#include "VivontLog.h"
#include "AudioCaptureDeviceInterface.h" // Include for FAudioCaptureDeviceParams, IAudioCaptureFactory
#include "Features/IModularFeatures.h" // Required for IModularFeatures

namespace Vivont
{

FAudioCaptureDevice::FAudioCaptureDevice()
    : AudioCaptureStreamPtr(nullptr), SampleRate(16000), NumChannels(1), BitDepth(16), MinBufferDuration(0.1f), bIsCapturing(false), CurrentCaptureTime(0.0)
{
    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice (Stub) constructor called"));
}

FAudioCaptureDevice::~FAudioCaptureDevice()
{
    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice (Stub) destructor called"));
    // No StopCapture call needed here for stub, as StartCapture doesn't acquire resources
}

bool FAudioCaptureDevice::StartCapture(UVivontSettings* InSettings)
{
    FScopeLock Lock(&AudioCriticalSection);
    if (bIsCapturing) // Use direct bool check
    {
        UE_LOG(LogVivontAudio, Warning, TEXT("FAudioCaptureDevice::StartCapture (Stub) called while already capturing."));
        return false;
    }

    // Read settings if needed (even for stub, might be useful later)
    // SampleRate = ...; NumChannels = ...;

    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice::StartCapture (Stub) - Simulating start."));

    // --- STUB IMPLEMENTATION ---
    // In a real implementation, we would create and start the IAudioCaptureStream here.
    // For the stub, we just set the flag.
    bIsCapturing = true; // Use direct assignment
    CapturedAudioData.Empty(); // Clear any old data
    CurrentCaptureTime = 0.0;
    // --- END STUB ---

    return true;
}

void FAudioCaptureDevice::StopCapture()
{
    FScopeLock Lock(&AudioCriticalSection);
    if (!bIsCapturing) // Use direct bool check
    {
        return; // Not capturing
    }

    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice::StopCapture (Stub) - Simulating stop."));

    // --- STUB IMPLEMENTATION ---
    // In a real implementation, we would stop and close the IAudioCaptureStream here
    // and call RemoveCaptureCallback.
    // AudioCaptureStreamPtr->StopStream();
    // AudioCaptureStreamPtr->CloseStream();
    // AudioCaptureStreamPtr->RemoveCaptureCallback(this);
    // AudioCaptureStreamPtr.Reset();
    // --- END STUB ---

    bIsCapturing = false; // Use direct assignment
    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice::StopCapture (Stub) finished. Final buffer size: %d bytes"), CapturedAudioData.Num());
}

TArray<uint8> FAudioCaptureDevice::GetCapturedData()
{
    FScopeLock Lock(&AudioCriticalSection);
    UE_LOG(LogVivontAudio, Log, TEXT("FAudioCaptureDevice::GetCapturedData (Stub) returning empty array."));
    TArray<uint8> ReturnData = MoveTemp(CapturedAudioData); // Return whatever (empty) data we have
    CapturedAudioData.Empty();
    return ReturnData;
}

bool FAudioCaptureDevice::IsCapturing() const
{
    return bIsCapturing; // Use direct bool check
}

// Stub implementation - does nothing but log
void FAudioCaptureDevice::OnAudioCapture(const void* InBuffer, uint32 InBufferFrames, uint32 InNumChannels, uint32 InSampleRate, double InStreamTime, bool bOverFlow)
{
    // UE_LOG(LogVivontAudio, Verbose, TEXT("FAudioCaptureDevice::OnAudioCapture (Stub) called."));
    // In a real implementation, this is where ConvertToPCM would be called.
    if (!bIsCapturing) return; // Use direct bool check
    if (bOverFlow) UE_LOG(LogVivontAudio, Warning, TEXT("Audio capture overflow detected (Stub)!"));

    // FScopeLock Lock(&this->AudioCriticalSection); // Corrected scoping if needed, but commented out for stub
    // const float* FloatBuffer = static_cast<const float*>(InBuffer);
    // this->ConvertToPCM(FloatBuffer, this->CapturedAudioData, InBufferFrames, InNumChannels, this->BitDepth); // Corrected scoping if needed
    // this->CurrentCaptureTime = InStreamTime; // Corrected scoping if needed
}

// Stub implementation - does nothing
void FAudioCaptureDevice::ConvertToPCM(const float* InAudioData, TArray<uint8>& OutPCMData, int32 NumFrames, int32 InNumChannels, int32 InBitDepth)
{
    // UE_LOG(LogVivontAudio, Verbose, TEXT("FAudioCaptureDevice::ConvertToPCM (Stub) called."));
    // Real implementation would convert float samples to PCM bytes.
}

} // namespace Vivont
