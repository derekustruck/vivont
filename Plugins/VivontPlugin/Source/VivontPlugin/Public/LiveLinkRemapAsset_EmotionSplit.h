#pragma once
#include "LiveLinkRemapAsset.h"
#include "LiveLinkRemapAsset_EmotionSplit.generated.h"


UCLASS()
class ULiveLinkRemapAsset_EmotionSplit : public ULiveLinkRemapAsset
{
    GENERATED_BODY()

public:
    virtual void RemapCurveElements_Implementation(TMap<FName, float>& CurveItems) const override;

private:
    bool IsStandardARKitBlendshape(const FName& CurveName) const;
};
