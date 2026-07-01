// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Stateless audio helpers for the inference pipeline.
 * Consolidates WAV/PCM handling that was previously duplicated across the
 * OpenAI client, animation manager, API client and audio subsystem.
 */
class VIVONTPLUGIN_API FVivontAudioUtils
{
public:
    /** True if the buffer begins with a RIFF/WAVE header. */
    static bool IsWavFormat(const TArray<uint8>& Data);

    /**
     * Extract the raw PCM payload from a WAV buffer and report its format.
     * Returns an empty array if no valid 'data' chunk is found.
     */
    static TArray<uint8> ExtractPcmFromWav(const TArray<uint8>& WavData, int32& OutSampleRate, int32& OutNumChannels, int32& OutBitsPerSample);

    /**
     * Convert interleaved 16-bit PCM to mono float32 in [-1, 1].
     * Multi-channel input is averaged down to mono.
     */
    static TArray<float> PcmS16ToMonoFloat(const TArray<uint8>& PcmData, int32 NumChannels);

    /**
     * Convert an arbitrary audio buffer (WAV or raw 16-bit PCM) into mono float32
     * resampled to the target sample rate. Used to feed the inference model.
     * @param Data            WAV or raw PCM bytes.
     * @param SourceSampleRate Sample rate to assume when Data is raw PCM.
     * @param TargetSampleRate Desired output rate (e.g. 16000 for the model).
     */
    static TArray<float> PrepareMonoFloat(const TArray<uint8>& Data, int32 SourceSampleRate, int32 TargetSampleRate);

    /** Linear-interpolation resample of a mono float signal. */
    static TArray<float> ResampleLinear(const TArray<float>& Input, int32 InRate, int32 OutRate);
};
