#pragma once

#include "CoreMinimal.h"
#include "Scene/TCATInfluenceComponent.h"
#include "RBPBILTypes.h"
#include "RBPBILInfluenceComponent.generated.h"

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBPBIL_API URBPBILInfluenceComponent : public UTCATInfluenceComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="RB PBIL")
    void SetChannelInfluence(
        ERBPBILChannel Channel,
        float Radius,
        float Strength,
        float InfluenceZLimitOffset = 0.0f);

    UFUNCTION(BlueprintCallable, Category="RB PBIL")
    bool RemoveChannelInfluence(ERBPBILChannel Channel);
};
