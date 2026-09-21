#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RBAITypes.h"
#include "RBAICreatureArchetype.generated.h"

UCLASS(BlueprintType)
class RBAICREATURES_API URBAICreatureArchetype : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FleeHealthThreshold = 0.30f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature", meta=(ClampMin="0.0", ClampMax="1.0"))
    float AttackHealthFloor = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FeedHungerThreshold = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RestFatigueThreshold = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature", meta=(ClampMin="0.0", ClampMax="1.0"))
    float PackConfidenceWeight = 0.35f;

    UFUNCTION(BlueprintPure, Category="RB AI|Creature")
    TArray<FRBAIActionDefinition> BuildStandardActions() const;
};
