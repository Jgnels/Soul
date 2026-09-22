#pragma once

#include "CoreMinimal.h"
#include "Scene/TCATInfluenceVolume.h"
#include "RBPBILInfluenceVolume.generated.h"

UCLASS()
class RBPBIL_API ARBPBILInfluenceVolume : public ATCATInfluenceVolume
{
    GENERATED_BODY()

public:
    ARBPBILInfluenceVolume();

    UFUNCTION(CallInEditor, BlueprintCallable, Category="RB PBIL")
    void ApplyPBILPreset();

    UFUNCTION(BlueprintPure, Category="RB PBIL")
    int32 GetPBILChannelCount() const
    {
        return BaseLayerConfigs.Num();
    }
};
