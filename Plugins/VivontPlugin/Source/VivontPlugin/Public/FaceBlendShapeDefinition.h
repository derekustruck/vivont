// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FaceBlendShapeEnum.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaceBlendShapeDefinition.generated.h"

/**
 * Blueprint function library for handling face blend shapes
 * Provides utilities for working with face blend shapes in the LiveLinkFace system
 */
UCLASS()
class VIVONTPLUGIN_API UBlendShapeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    /** Get the display name of a blend shape */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static FString GetBlendShapeName(EFaceBlendShape BlendShape);
    
    /** Check if a blend shape is in a specific group */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static bool IsInMouthGroup(EFaceBlendShape BlendShape);
    
    /** Check if a blend shape is in the eye group */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static bool IsInEyeGroup(EFaceBlendShape BlendShape);
    
    /** Check if a blend shape is in the eyebrow group */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static bool IsInEyebrowGroup(EFaceBlendShape BlendShape);
    
    /** Scale blend shapes by section */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static TArray<float> ScaleBlendShapesBySection(const TArray<float>& BlendShapes, float MouthScale, float EyeScale, float EyebrowScale, float Threshold = 0.0f);
    
    /** Get all property names for LiveLink integration */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static TArray<FName> GetAllBlendShapePropertyNames();
    
    /** Get the number of blend shapes */
    UFUNCTION(BlueprintPure, Category = "Face|BlendShapes")
    static int32 GetBlendShapeCount() { return FACE_BLENDSHAPE_COUNT; }
    
    /** Initialize a blend shape array with the correct size */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static TArray<float> CreateBlendShapeArray(float DefaultValue = 0.0f);
    
    /** Validate a blend shape array has the correct size */
    UFUNCTION(BlueprintCallable, Category = "Face|BlendShapes")
    static bool ValidateBlendShapeArray(const TArray<float>& BlendShapes);
};

/**
 * Utility class for internal C++ use
 * Provides direct access to blend shape group definitions
 */
class VIVONTPLUGIN_API FFaceBlendShapeDefinition
{
public:
    static const TArray<EFaceBlendShape>& GetMouthBlendShapes();
    static const TArray<EFaceBlendShape>& GetEyeBlendShapes();
    static const TArray<EFaceBlendShape>& GetEyebrowBlendShapes();
    
    static bool IsInGroup(EFaceBlendShape BlendShape, const TArray<EFaceBlendShape>& Group);
    static TArray<float> ScaleBlendShapesBySection(const TArray<float>& BlendShapes, float MouthScale, float EyeScale, float EyebrowScale, float Threshold = 0.0f);
    
private:
    // Initialize the arrays once and cache them
    static void InitializeArrays();
    static bool bArraysInitialized;
    static TArray<EFaceBlendShape> MouthShapes;
    static TArray<EFaceBlendShape> EyeShapes;
    static TArray<EFaceBlendShape> EyebrowShapes;
};
