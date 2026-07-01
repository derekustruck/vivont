// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AudioCaptureCore.h" // Provides IAudioCaptureCallback
#include "AudioCaptureDeviceInterface.h" // Provides IAudioCaptureStream, IAudioCaptureFactory, FAudioCaptureDeviceParams
#include "HAL/ThreadSafeBool.h"
#include "HAL/CriticalSection.h"
#include "Templates/SharedPointer.h"

// Forward declare engine types
namespace Audio { class IAudioCaptureStream; }
class UVivontSettings;

namespace Vivont
{
    /**
     * Audio Capture Device - Handles capturing audio from input devices (STUB IMPLEMENTATION)
     * NOTE: UE 5.5 callback mechanism needs investigation. Removed IAudioCaptureCallback inheritance for now.
     */
    class VIVONTPLUGIN_API FAudioCaptureDevice // Removed inheritance: : public Audio::IAudioCaptureCallback
    {
    public:
        FAudioCaptureDevice();
        virtual ~FAudioCaptureDevice(); // Virtual destructor

        // --- Public API ---
        bool StartCapture(UVivontSettings* InSettings);
        void StopCapture();
        TArray<uint8> GetCapturedData();
        bool IsCapturing() const;

        //~ Begin Audio::IAudioCaptureCallback Interface (Removed - Needs replacement for UE 5.5)
        /* virtual */ void OnAudioCapture(const void* InBuffer, uint32 InBufferFrames, uint32 InNumChannels, uint32 InSampleRate, double InStreamTime, bool bOverFlow); // Removed 'virtual' and 'override'
        //~ End Audio::IAudioCaptureCallback Interface

    private:
        void ConvertToPCM(const float* InAudioData, TArray<uint8>& OutPCMData, int32 NumFrames, int32 InNumChannels, int32 InBitDepth);

        // Stub members - Real implementation would use IAudioCaptureStream
        TSharedPtr<Audio::IAudioCaptureStream> AudioCaptureStreamPtr; // Keep for structure, but won't be fully used in stub
        FCriticalSection AudioCriticalSection;
        TArray<uint8> CapturedAudioData;
        int32 SampleRate;
        int32 NumChannels;
        int32 BitDepth;
        float MinBufferDuration;
        FThreadSafeBool bIsCapturing;
        double CurrentCaptureTime;
    };

} // namespace Vivont
