// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontAudioUtils.h"

bool FVivontAudioUtils::IsWavFormat(const TArray<uint8>& Data)
{
    return Data.Num() >= 12
        && Data[0] == 'R' && Data[1] == 'I' && Data[2] == 'F' && Data[3] == 'F'
        && Data[8] == 'W' && Data[9] == 'A' && Data[10] == 'V' && Data[11] == 'E';
}

TArray<uint8> FVivontAudioUtils::ExtractPcmFromWav(const TArray<uint8>& WavData, int32& OutSampleRate, int32& OutNumChannels, int32& OutBitsPerSample)
{
    TArray<uint8> Pcm;
    OutSampleRate = 0;
    OutNumChannels = 0;
    OutBitsPerSample = 0;

    if (WavData.Num() < 44)
    {
        return Pcm;
    }

    // fmt fields live at fixed offsets for canonical PCM WAV.
    OutNumChannels = WavData[22] | (WavData[23] << 8);
    OutSampleRate = WavData[24] | (WavData[25] << 8) | (WavData[26] << 16) | (WavData[27] << 24);
    OutBitsPerSample = WavData[34] | (WavData[35] << 8);

    // Walk chunks to find 'data' (handles optional chunks before it).
    for (int32 i = 12; i + 8 <= WavData.Num(); )
    {
        const uint32 ChunkSize = WavData[i + 4] | (WavData[i + 5] << 8) | (WavData[i + 6] << 16) | (static_cast<uint32>(WavData[i + 7]) << 24);
        if (WavData[i] == 'd' && WavData[i + 1] == 'a' && WavData[i + 2] == 't' && WavData[i + 3] == 'a')
        {
            const int32 DataOffset = i + 8;
            const int32 DataSize = FMath::Min(static_cast<int32>(ChunkSize), WavData.Num() - DataOffset);
            if (DataSize > 0)
            {
                Pcm.Append(&WavData[DataOffset], DataSize);
            }
            break;
        }
        // Advance past this chunk (chunks are word-aligned).
        i += 8 + ChunkSize + (ChunkSize & 1);
    }

    return Pcm;
}

TArray<float> FVivontAudioUtils::PcmS16ToMonoFloat(const TArray<uint8>& PcmData, int32 NumChannels)
{
    TArray<float> Out;
    if (NumChannels < 1)
    {
        NumChannels = 1;
    }

    const int32 NumSamples = PcmData.Num() / 2; // 16-bit
    const int32 NumFrames = NumSamples / NumChannels;
    Out.Reserve(NumFrames);

    const int16* Samples = reinterpret_cast<const int16*>(PcmData.GetData());
    for (int32 Frame = 0; Frame < NumFrames; ++Frame)
    {
        float Acc = 0.0f;
        for (int32 Ch = 0; Ch < NumChannels; ++Ch)
        {
            Acc += static_cast<float>(Samples[Frame * NumChannels + Ch]) / 32768.0f;
        }
        Out.Add(Acc / static_cast<float>(NumChannels));
    }
    return Out;
}

TArray<float> FVivontAudioUtils::ResampleLinear(const TArray<float>& Input, int32 InRate, int32 OutRate)
{
    if (Input.Num() == 0 || InRate <= 0 || OutRate <= 0 || InRate == OutRate)
    {
        return Input;
    }

    const int64 OutCount = static_cast<int64>(Input.Num()) * OutRate / InRate;
    TArray<float> Out;
    Out.Reserve(static_cast<int32>(OutCount));

    const double Ratio = static_cast<double>(InRate) / static_cast<double>(OutRate);
    for (int64 i = 0; i < OutCount; ++i)
    {
        const double SrcPos = static_cast<double>(i) * Ratio;
        const int32 Idx0 = static_cast<int32>(SrcPos);
        const int32 Idx1 = FMath::Min(Idx0 + 1, Input.Num() - 1);
        const float Frac = static_cast<float>(SrcPos - Idx0);
        Out.Add(FMath::Lerp(Input[Idx0], Input[Idx1], Frac));
    }
    return Out;
}

TArray<float> FVivontAudioUtils::PrepareMonoFloat(const TArray<uint8>& Data, int32 SourceSampleRate, int32 TargetSampleRate)
{
    int32 SampleRate = SourceSampleRate;
    int32 NumChannels = 1;
    TArray<float> Mono;

    if (IsWavFormat(Data))
    {
        int32 Bits = 16;
        const TArray<uint8> Pcm = ExtractPcmFromWav(Data, SampleRate, NumChannels, Bits);
        if (SampleRate <= 0) { SampleRate = SourceSampleRate; }
        if (NumChannels <= 0) { NumChannels = 1; }
        Mono = PcmS16ToMonoFloat(Pcm, NumChannels);
    }
    else
    {
        // Assume raw 16-bit mono PCM at the provided source rate (OpenAI TTS format).
        Mono = PcmS16ToMonoFloat(Data, 1);
    }

    return ResampleLinear(Mono, SampleRate, TargetSampleRate);
}
