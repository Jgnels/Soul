#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulSiegeObjectiveActor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulSiegeObjectiveActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulSiegeObjectiveActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName SettlementId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName ObjectiveId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName LinkedBuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName LinkedBreachScarId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName EffectTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    bool bPrimaryVictoryObjective = false;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> SceneRoot;
};
