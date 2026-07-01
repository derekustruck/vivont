#include "LiveLinkRemapAsset_EmotionSplit.h"

void ULiveLinkRemapAsset_EmotionSplit::RemapCurveElements_Implementation(TMap<FName, float>& CurveItems) const
{
    TMap<FName, float> OriginalItems = CurveItems;  // Safe copy
    TMap<FName, float> EmotionCurves;

    // Clear and repopulate to guarantee structure
    CurveItems.Reset();

    for (const TPair<FName, float>& Curve : OriginalItems)
    {
        if (IsStandardARKitBlendshape(Curve.Key))
        {
            CurveItems.Add(Curve.Key, Curve.Value);  // ✅ keep value
        }
        else
        {
            CurveItems.Add(Curve.Key, 0.0f);         // ✅ zero out
            EmotionCurves.Add(Curve.Key, Curve.Value);  // (optional) for future use
        }
    }

    // Optional debug
    for (const TPair<FName, float>& Curve : EmotionCurves)
    {
        UE_LOG(LogTemp, Verbose, TEXT("Emotion Curve: %s = %f"), *Curve.Key.ToString(), Curve.Value);
    }
}

bool ULiveLinkRemapAsset_EmotionSplit::IsStandardARKitBlendshape(const FName& CurveName) const
{
    static const TSet<FName> ARKitBlendshapes = {
        "EyeBlinkLeft", "EyeLookDownLeft", "EyeLookInLeft", "EyeLookOutLeft", "EyeLookUpLeft", "EyeSquintLeft", "EyeWideLeft",
        "EyeBlinkRight", "EyeLookDownRight", "EyeLookInRight", "EyeLookOutRight", "EyeLookUpRight", "EyeSquintRight", "EyeWideRight",
        "JawForward", "JawLeft", "JawRight", "JawOpen",
        "MouthClose", "MouthFunnel", "MouthPucker", "MouthLeft", "MouthRight", "MouthSmileLeft", "MouthSmileRight",
        "MouthFrownLeft", "MouthFrownRight", "MouthDimpleLeft", "MouthDimpleRight", "MouthStretchLeft", "MouthStretchRight",
        "MouthRollLower", "MouthRollUpper", "MouthShrugLower", "MouthShrugUpper", "MouthPressLeft", "MouthPressRight",
        "MouthLowerDownLeft", "MouthLowerDownRight", "MouthUpperUpLeft", "MouthUpperUpRight",
        "BrowDownLeft", "BrowDownRight", "BrowInnerUp", "BrowOuterUpLeft", "BrowOuterUpRight",
        "CheekPuff", "CheekSquintLeft", "CheekSquintRight", "NoseSneerLeft", "NoseSneerRight",
        "TongueOut"
    };

    return ARKitBlendshapes.Contains(CurveName);
}
