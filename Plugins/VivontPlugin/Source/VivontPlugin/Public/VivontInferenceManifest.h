// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Self-describing runtime contract for the in-house audio->blendshape model.
 *
 * The model team exports this JSON alongside the .onnx so the plugin never
 * hardcodes the output schema (curve count/names) or normalization stats, which
 * are still in flux. Expected JSON shape:
 *
 * {
 *   "model_file":         "vivont_fast_causal_cnn.onnx",   // relative to this manifest
 *   "audio_sample_rate":  16000,
 *   "audio_len":          12800,                            // samples per inference window
 *   "fps":                30,                               // output frames per second
 *   "audio_mean":         0.000168,                         // scalar input normalization
 *   "audio_std":          0.05,
 *   "curve_names":        ["CTRL_expressions_jawOpen", ...],// length == model out_dim
 *   "blend_mean":         [ ... ],                          // length == out_dim
 *   "blend_std":          [ ... ]                           // length == out_dim
 * }
 */
struct VIVONTPLUGIN_API FVivontInferenceManifest
{
    /** Absolute path to the .onnx file (resolved from the manifest location). */
    FString ModelFilePath;

    int32 AudioSampleRate = 16000;
    int32 AudioLen = 12800;
    int32 Fps = 30;

    /**
     * Frames emitted per inference window. FastCausalCNN = 1; the WavLM family emits a short
     * centered block (e.g. 7) and the runner uses the center frame as that window's prediction.
     * Model output is (1, SequenceLen, OutDim).
     */
    int32 SequenceLen = 1;

    /**
     * "scalar" (FastCausalCNN: plugin applies (x - AudioMean) / AudioStd) or "none" (SSL/WavLM:
     * raw waveform in, the model normalizes internally). For "none" the exporter writes
     * AudioMean=0 / AudioStd=1, so the scalar path below is already an identity either way.
     */
    FString AudioNormalization = TEXT("scalar");

    float AudioMean = 0.0f;
    float AudioStd = 1.0f;

    /** Output curve names, in model output order. Drives the LiveLink subject. */
    TArray<FName> CurveNames;

    /** Per-curve denormalization stats (length == CurveNames.Num()). */
    TArray<float> BlendMean;
    TArray<float> BlendStd;

    int32 OutDim() const { return CurveNames.Num(); }

    /** Frames the model emits per window (>= 1). */
    int32 FramesPerWindow() const { return FMath::Max(1, SequenceLen); }

    /** Total floats in one inference output tensor: FramesPerWindow * OutDim. */
    int32 OutputElementCount() const { return FramesPerWindow() * OutDim(); }

    /** Flat offset of the center frame within the output tensor (the prediction for this window). */
    int32 CenterFrameOffset() const { return (FramesPerWindow() / 2) * OutDim(); }

    /** Hop between inference windows, in samples (AudioSampleRate / Fps). One output frame per hop. */
    int32 HopSamples() const { return Fps > 0 ? FMath::Max(1, AudioSampleRate / Fps) : AudioLen; }

    bool IsValid() const
    {
        return OutDim() > 0
            && AudioLen > 0
            && AudioSampleRate > 0
            && !ModelFilePath.IsEmpty()
            && BlendMean.Num() == OutDim()
            && BlendStd.Num() == OutDim();
    }

    /**
     * Load and validate a manifest from a JSON file.
     * @param ManifestPath  Absolute path to the manifest JSON.
     * @param OutManifest   Populated on success.
     * @param OutError      Human-readable error on failure.
     * @return true on success.
     */
    static bool LoadFromFile(const FString& ManifestPath, FVivontInferenceManifest& OutManifest, FString& OutError);
};
