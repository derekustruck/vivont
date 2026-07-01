// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FaceBlendShapeEnum.generated.h"

/**
 * Enumeration for face blend shapes
 * Used by the LiveLinkFace system to control facial animations
 */
UENUM(BlueprintType)
enum class EFaceBlendShape : uint8
{   
    EyeBlinkLeft UMETA(DisplayName="EyeBlinkLeft"),
    EyeLookDownLeft UMETA(DisplayName="EyeLookDownLeft"),
    EyeLookInLeft UMETA(DisplayName="EyeLookInLeft"),
    EyeLookOutLeft UMETA(DisplayName="EyeLookOutLeft"),
    EyeLookUpLeft UMETA(DisplayName="EyeLookUpLeft"),
    EyeSquintLeft UMETA(DisplayName="EyeSquintLeft"),
    EyeWideLeft UMETA(DisplayName="EyeWideLeft"),
    EyeBlinkRight UMETA(DisplayName="EyeBlinkRight"),
    EyeLookDownRight UMETA(DisplayName="EyeLookDownRight"),
    EyeLookInRight UMETA(DisplayName="EyeLookInRight"),
    EyeLookOutRight UMETA(DisplayName="EyeLookOutRight"),
    EyeLookUpRight UMETA(DisplayName="EyeLookUpRight"),
    EyeSquintRight UMETA(DisplayName="EyeSquintRight"),
    EyeWideRight UMETA(DisplayName="EyeWideRight"),
    JawForward UMETA(DisplayName="JawForward"),
    JawLeft UMETA(DisplayName="JawLeft"),
    JawRight UMETA(DisplayName="JawRight"),
    JawOpen UMETA(DisplayName="JawOpen"),
    MouthClose UMETA(DisplayName="MouthClose"),
    MouthFunnel UMETA(DisplayName="MouthFunnel"),
    MouthPucker UMETA(DisplayName="MouthPucker"),
    MouthLeft UMETA(DisplayName="MouthLeft"),
    MouthRight UMETA(DisplayName="MouthRight"),
    MouthSmileLeft UMETA(DisplayName="MouthSmileLeft"),
    MouthSmileRight UMETA(DisplayName="MouthSmileRight"),
    MouthFrownLeft UMETA(DisplayName="MouthFrownLeft"),
    MouthFrownRight UMETA(DisplayName="MouthFrownRight"),
    MouthDimpleLeft UMETA(DisplayName="MouthDimpleLeft"),
    MouthDimpleRight UMETA(DisplayName="MouthDimpleRight"),
    MouthStretchLeft UMETA(DisplayName="MouthStretchLeft"),
    MouthStretchRight UMETA(DisplayName="MouthStretchRight"),
    MouthRollLower UMETA(DisplayName="MouthRollLower"),
    MouthRollUpper UMETA(DisplayName="MouthRollUpper"),
    MouthShrugLower UMETA(DisplayName="MouthShrugLower"),
    MouthShrugUpper UMETA(DisplayName="MouthShrugUpper"),
    MouthPressLeft UMETA(DisplayName="MouthPressLeft"),
    MouthPressRight UMETA(DisplayName="MouthPressRight"),
    MouthLowerDownLeft UMETA(DisplayName="MouthLowerDownLeft"),
    MouthLowerDownRight UMETA(DisplayName="MouthLowerDownRight"),
    MouthUpperUpLeft UMETA(DisplayName="MouthUpperUpLeft"),
    MouthUpperUpRight UMETA(DisplayName="MouthUpperUpRight"),
    BrowDownLeft UMETA(DisplayName="BrowDownLeft"),
    BrowDownRight UMETA(DisplayName="BrowDownRight"),
    BrowInnerUp UMETA(DisplayName="BrowInnerUp"),
    BrowOuterUpLeft UMETA(DisplayName="BrowOuterUpLeft"),
    BrowOuterUpRight UMETA(DisplayName="BrowOuterUpRight"),
    CheekPuff UMETA(DisplayName="CheekPuff"),
    CheekSquintLeft UMETA(DisplayName="CheekSquintLeft"),
    CheekSquintRight UMETA(DisplayName="CheekSquintRight"),
    NoseSneerLeft UMETA(DisplayName="NoseSneerLeft"),
    NoseSneerRight UMETA(DisplayName="NoseSneerRight"),
    TongueOut UMETA(DisplayName="TongueOut"),
    HeadYaw UMETA(DisplayName="HeadYaw"),
    HeadPitch UMETA(DisplayName="HeadPitch"),
    HeadRoll UMETA(DisplayName="HeadRoll"),
    LeftEyeYaw UMETA(DisplayName="LeftEyeYaw"),
    LeftEyePitch UMETA(DisplayName="LeftEyePitch"),
    LeftEyeRoll UMETA(DisplayName="LeftEyeRoll"),
    RightEyeYaw UMETA(DisplayName="RightEyeYaw"),
    RightEyePitch UMETA(DisplayName="RightEyePitch"),
    RightEyeRoll UMETA(DisplayName="RightEyeRoll"),
    Angry UMETA(DisplayName="Angry"),
    Disgusted UMETA(DisplayName="Disgusted"),
    Fearful UMETA(DisplayName="Fearful"),
    Happy UMETA(DisplayName="Happy"),
    Neutral UMETA(DisplayName="Neutral"),
    Sad UMETA(DisplayName="Sad"),
    Surprised UMETA(DisplayName="Surprised"),
    MAX UMETA(Hidden),
    
    // A proper named constant for getting the count, clearer than MAX
    NumBlendShapes = MAX,
    
    // Helpers for array sizing (identical to NumBlendShapes, but more semantically clear)
    Count = NumBlendShapes
};

// Constants for array sizing to use throughout the codebase
static constexpr int32 FACE_BLENDSHAPE_COUNT = static_cast<int32>(EFaceBlendShape::Count);
