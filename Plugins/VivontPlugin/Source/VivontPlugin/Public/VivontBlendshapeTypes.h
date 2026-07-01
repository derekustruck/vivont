// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VivontBlendshapeTypes.generated.h"

/**
 * A single frame of blend shape / control-rig curve values produced by the
 * in-house inference model.
 *
 * The frame is variable length: its size is defined by the active model's
 * output dimension (see the inference manifest's curve list), not a fixed enum.
 * Values are stored verbatim (denormalized) and are NOT clamped to 0..1, because
 * MetaHuman control-rig curves are legitimately bipolar (e.g. jawLeft/Right).
 */
USTRUCT(BlueprintType)
struct VIVONTPLUGIN_API FBlendShapeFrame
{
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<float> Values;

public:
    FBlendShapeFrame() = default;

    explicit FBlendShapeFrame(const TArray<float>& InValues)
        : Values(InValues)
    {
    }

    explicit FBlendShapeFrame(TArray<float>&& InValues)
        : Values(MoveTemp(InValues))
    {
    }

    /** Get a value by index with bounds checking. */
    float GetValue(int32 Index) const
    {
        return Values.IsValidIndex(Index) ? Values[Index] : 0.0f;
    }

    /** Set a value by index with bounds checking (no clamping). */
    void SetValue(int32 Index, float Value)
    {
        if (Values.IsValidIndex(Index))
        {
            Values[Index] = Value;
        }
    }

    /** Read-only access to the raw value array. */
    const TArray<float>& GetRawValues() const { return Values; }

    /** Replace all values. */
    void SetAllValues(const TArray<float>& NewValues) { Values = NewValues; }
    void SetAllValues(TArray<float>&& NewValues) { Values = MoveTemp(NewValues); }

    /** Zero every value (preserving size). */
    void Reset()
    {
        for (float& Value : Values)
        {
            Value = 0.0f;
        }
    }

    int32 GetNumValues() const { return Values.Num(); }

    bool IsValid() const { return Values.Num() > 0; }
};

/** Broadcast when a full blend shape sequence has been inferred for an utterance. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBlendShapesReceived, bool, bSuccess, const TArray<FBlendShapeFrame>&, BlendShapes);

/** Broadcast when inference fails. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInferenceError, const FString&, ErrorMessage);
