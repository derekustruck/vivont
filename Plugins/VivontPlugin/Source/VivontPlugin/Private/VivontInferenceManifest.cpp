// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontInferenceManifest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    bool ReadFloatArray(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field, TArray<float>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray = nullptr;
        if (!Obj->TryGetArrayField(Field, JsonArray) || !JsonArray)
        {
            return false;
        }
        Out.Reset(JsonArray->Num());
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            Out.Add(static_cast<float>(Value->AsNumber()));
        }
        return true;
    }
}

bool FVivontInferenceManifest::LoadFromFile(const FString& ManifestPath, FVivontInferenceManifest& OutManifest, FString& OutError)
{
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *ManifestPath))
    {
        OutError = FString::Printf(TEXT("Could not read manifest file: %s"), *ManifestPath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = FString::Printf(TEXT("Manifest is not valid JSON: %s"), *ManifestPath);
        return false;
    }

    FVivontInferenceManifest M;

    // Resolve the model file relative to the manifest directory unless absolute.
    FString ModelFile;
    if (!Root->TryGetStringField(TEXT("model_file"), ModelFile) || ModelFile.IsEmpty())
    {
        OutError = TEXT("Manifest missing 'model_file'");
        return false;
    }
    M.ModelFilePath = FPaths::IsRelative(ModelFile)
        ? FPaths::Combine(FPaths::GetPath(ManifestPath), ModelFile)
        : ModelFile;
    FPaths::NormalizeFilename(M.ModelFilePath);

    // Scalar fields (fall back to defaults already set in the struct).
    int32 IntValue = 0;
    if (Root->TryGetNumberField(TEXT("audio_sample_rate"), IntValue)) { M.AudioSampleRate = IntValue; }
    if (Root->TryGetNumberField(TEXT("audio_len"), IntValue)) { M.AudioLen = IntValue; }
    if (Root->TryGetNumberField(TEXT("fps"), IntValue)) { M.Fps = IntValue; }
    if (Root->TryGetNumberField(TEXT("sequence_len"), IntValue)) { M.SequenceLen = FMath::Max(1, IntValue); }

    FString NormValue;
    if (Root->TryGetStringField(TEXT("audio_normalization"), NormValue) && !NormValue.IsEmpty())
    {
        M.AudioNormalization = NormValue;
    }

    double DoubleValue = 0.0;
    if (Root->TryGetNumberField(TEXT("audio_mean"), DoubleValue)) { M.AudioMean = static_cast<float>(DoubleValue); }
    if (Root->TryGetNumberField(TEXT("audio_std"), DoubleValue)) { M.AudioStd = static_cast<float>(DoubleValue); }
    if (FMath::IsNearlyZero(M.AudioStd))
    {
        OutError = TEXT("Manifest 'audio_std' is zero (would divide by zero)");
        return false;
    }

    // Curve names.
    const TArray<TSharedPtr<FJsonValue>>* NamesArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("curve_names"), NamesArray) || !NamesArray || NamesArray->Num() == 0)
    {
        OutError = TEXT("Manifest missing non-empty 'curve_names'");
        return false;
    }
    M.CurveNames.Reset(NamesArray->Num());
    for (const TSharedPtr<FJsonValue>& Value : *NamesArray)
    {
        M.CurveNames.Add(FName(*Value->AsString()));
    }

    if (!ReadFloatArray(Root, TEXT("blend_mean"), M.BlendMean))
    {
        OutError = TEXT("Manifest missing 'blend_mean'");
        return false;
    }
    if (!ReadFloatArray(Root, TEXT("blend_std"), M.BlendStd))
    {
        OutError = TEXT("Manifest missing 'blend_std'");
        return false;
    }

    if (M.BlendMean.Num() != M.OutDim() || M.BlendStd.Num() != M.OutDim())
    {
        OutError = FString::Printf(TEXT("Manifest stat length mismatch: curve_names=%d blend_mean=%d blend_std=%d"),
            M.OutDim(), M.BlendMean.Num(), M.BlendStd.Num());
        return false;
    }

    OutManifest = MoveTemp(M);
    return true;
}
