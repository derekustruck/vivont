// Copyright (c) 2025, Derek. All rights reserved.

#include "FaceBlendShapeDefinition.h"
#include "FaceBlendShapeEnum.h"

// Initialize static members for FFaceBlendShapeDefinition
bool FFaceBlendShapeDefinition::bArraysInitialized = false;
TArray<EFaceBlendShape> FFaceBlendShapeDefinition::MouthShapes;
TArray<EFaceBlendShape> FFaceBlendShapeDefinition::EyeShapes;
TArray<EFaceBlendShape> FFaceBlendShapeDefinition::EyebrowShapes;

// Blueprint Function Library Implementation
FString UBlendShapeLibrary::GetBlendShapeName(EFaceBlendShape BlendShape)
{
    // Use StaticEnum instead of FindObject with ANY_PACKAGE 
    const UEnum* EnumPtr = StaticEnum<EFaceBlendShape>();
    if (!EnumPtr)
    {
        return FString::Printf(TEXT("Invalid-%d"), static_cast<int32>(BlendShape));
    }
    
    return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(BlendShape)).ToString();
}

bool UBlendShapeLibrary::IsInMouthGroup(EFaceBlendShape BlendShape)
{
    return FFaceBlendShapeDefinition::IsInGroup(BlendShape, FFaceBlendShapeDefinition::GetMouthBlendShapes());
}

bool UBlendShapeLibrary::IsInEyeGroup(EFaceBlendShape BlendShape)
{
    return FFaceBlendShapeDefinition::IsInGroup(BlendShape, FFaceBlendShapeDefinition::GetEyeBlendShapes());
}

bool UBlendShapeLibrary::IsInEyebrowGroup(EFaceBlendShape BlendShape)
{
    return FFaceBlendShapeDefinition::IsInGroup(BlendShape, FFaceBlendShapeDefinition::GetEyebrowBlendShapes());
}

TArray<float> UBlendShapeLibrary::ScaleBlendShapesBySection(const TArray<float>& BlendShapes, float MouthScale, float EyeScale, float EyebrowScale, float Threshold)
{
    return FFaceBlendShapeDefinition::ScaleBlendShapesBySection(BlendShapes, MouthScale, EyeScale, EyebrowScale, Threshold);
}

TArray<FName> UBlendShapeLibrary::GetAllBlendShapePropertyNames()
{
    TArray<FName> PropertyNames;
    PropertyNames.Reserve(FACE_BLENDSHAPE_COUNT);
    
    const UEnum* EnumPtr = StaticEnum<EFaceBlendShape>();
    if (!EnumPtr)
    {
        return PropertyNames;
    }
    
    // Add all enum values as property names except MAX, NumBlendShapes, and Count
    for (int32 i = 0; i < static_cast<int32>(EFaceBlendShape::MAX); ++i)
    {
        FString DisplayName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();
        if (!DisplayName.IsEmpty())
        {
            PropertyNames.Add(FName(*DisplayName));
        }
    }
    
    return PropertyNames;
}

TArray<float> UBlendShapeLibrary::CreateBlendShapeArray(float DefaultValue)
{
    TArray<float> BlendShapes;
    BlendShapes.Init(DefaultValue, FACE_BLENDSHAPE_COUNT);
    return BlendShapes;
}

bool UBlendShapeLibrary::ValidateBlendShapeArray(const TArray<float>& BlendShapes)
{
    return BlendShapes.Num() == FACE_BLENDSHAPE_COUNT;
}

// FFaceBlendShapeDefinition Implementation
void FFaceBlendShapeDefinition::InitializeArrays()
{
    if (bArraysInitialized)
    {
        return;
    }
    
    // Initialize mouth blend shapes
    MouthShapes = {
        EFaceBlendShape::JawForward, EFaceBlendShape::JawLeft, EFaceBlendShape::JawRight, EFaceBlendShape::JawOpen,
        EFaceBlendShape::MouthClose, EFaceBlendShape::MouthFunnel, EFaceBlendShape::MouthPucker,
        EFaceBlendShape::MouthLeft, EFaceBlendShape::MouthRight, EFaceBlendShape::MouthSmileLeft,
        EFaceBlendShape::MouthSmileRight, EFaceBlendShape::MouthFrownLeft, EFaceBlendShape::MouthFrownRight,
        EFaceBlendShape::MouthDimpleLeft, EFaceBlendShape::MouthDimpleRight, EFaceBlendShape::MouthStretchLeft,
        EFaceBlendShape::MouthStretchRight, EFaceBlendShape::MouthRollLower, EFaceBlendShape::MouthRollUpper,
        EFaceBlendShape::MouthShrugLower, EFaceBlendShape::MouthShrugUpper, EFaceBlendShape::MouthPressLeft,
        EFaceBlendShape::MouthPressRight, EFaceBlendShape::MouthLowerDownLeft, EFaceBlendShape::MouthLowerDownRight,
        EFaceBlendShape::MouthUpperUpLeft, EFaceBlendShape::MouthUpperUpRight
    };
    
    // Initialize eye blend shapes
    EyeShapes = {
        EFaceBlendShape::EyeBlinkLeft, EFaceBlendShape::EyeLookDownLeft, EFaceBlendShape::EyeLookInLeft,
        EFaceBlendShape::EyeLookOutLeft, EFaceBlendShape::EyeLookUpLeft, EFaceBlendShape::EyeSquintLeft,
        EFaceBlendShape::EyeWideLeft, EFaceBlendShape::EyeBlinkRight, EFaceBlendShape::EyeLookDownRight,
        EFaceBlendShape::EyeLookInRight, EFaceBlendShape::EyeLookOutRight, EFaceBlendShape::EyeLookUpRight,
        EFaceBlendShape::EyeSquintRight, EFaceBlendShape::EyeWideRight
    };
    
    // Initialize eyebrow blend shapes
    EyebrowShapes = {
        EFaceBlendShape::BrowDownLeft, EFaceBlendShape::BrowDownRight, EFaceBlendShape::BrowInnerUp,
        EFaceBlendShape::BrowOuterUpLeft, EFaceBlendShape::BrowOuterUpRight
    };
    
    bArraysInitialized = true;
}

const TArray<EFaceBlendShape>& FFaceBlendShapeDefinition::GetMouthBlendShapes()
{
    InitializeArrays();
    return MouthShapes;
}

const TArray<EFaceBlendShape>& FFaceBlendShapeDefinition::GetEyeBlendShapes()
{
    InitializeArrays();
    return EyeShapes;
}

const TArray<EFaceBlendShape>& FFaceBlendShapeDefinition::GetEyebrowBlendShapes()
{
    InitializeArrays();
    return EyebrowShapes;
}

bool FFaceBlendShapeDefinition::IsInGroup(EFaceBlendShape BlendShape, const TArray<EFaceBlendShape>& Group)
{
    return Group.Contains(BlendShape);
}

TArray<float> FFaceBlendShapeDefinition::ScaleBlendShapesBySection(const TArray<float>& BlendShapes, float MouthScale, float EyeScale, float EyebrowScale, float Threshold)
{
    // Create empty array and reserve space for efficiency
    TArray<float> ScaledBlendShapes;
    ScaledBlendShapes.Reserve(BlendShapes.Num());
    ScaledBlendShapes.Init(0.0f, BlendShapes.Num());
    
    // Ensure scales are valid to prevent issues
    MouthScale = FMath::Max(0.0f, MouthScale);
    EyeScale = FMath::Max(0.0f, EyeScale);
    EyebrowScale = FMath::Max(0.0f, EyebrowScale);
    
    // Loop through all blend shapes
    for (int32 i = 0; i < BlendShapes.Num(); ++i)
    {
        float Value = BlendShapes[i];
        
        // Skip tiny values to avoid unintended facial movements
        if (Value > Threshold)
        {
            EFaceBlendShape BlendShape = static_cast<EFaceBlendShape>(i);
            float ScaledValue = Value;
            
            // Apply different scaling based on which group the blend shape belongs to
            if (IsInGroup(BlendShape, GetMouthBlendShapes()))
            {
                ScaledValue = Value * MouthScale;
            }
            else if (IsInGroup(BlendShape, GetEyeBlendShapes()))
            {
                ScaledValue = Value * EyeScale;
            }
            else if (IsInGroup(BlendShape, GetEyebrowBlendShapes()))
            {
                ScaledValue = Value * EyebrowScale;
            }
            
            // Special handling for emotional blend shapes
            // Don't scale if it's one of the emotion blend shapes
            if (BlendShape >= EFaceBlendShape::Angry && BlendShape <= EFaceBlendShape::Surprised)
            {
                ScaledValue = Value;  // Keep original emotional expression value
            }
            
            // Ensure scaling stays within valid range (0.0 to 1.0)
            ScaledValue = FMath::Clamp(ScaledValue, 0.0f, 1.0f);
            ScaledBlendShapes[i] = ScaledValue;
        }
        else
        {
            // For values below threshold, still keep them non-negative but don't scale
            ScaledBlendShapes[i] = FMath::Max(Value, 0.0f);
        }
    }
    
    return ScaledBlendShapes;
}
