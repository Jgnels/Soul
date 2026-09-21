#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBAICreatureBehaviorComponent.generated.h"

class URBAIBrainComponent;
class URBAICreatureArchetype;

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAICREATURES_API URBAICreatureBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAICreatureBehaviorComponent();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature")
    TObjectPtr<URBAICreatureArchetype> Archetype;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Creature")
    bool bInstallArchetypeActions = true;

    UFUNCTION(BlueprintCallable, Category="RB AI|Creature")
    void SetVitals(float Health01, float Hunger01, float Fatigue01);

    UFUNCTION(BlueprintCallable, Category="RB AI|Creature")
    void SetPackSupport(float PackSupport01);

    UFUNCTION(BlueprintCallable, Category="RB AI|Creature")
    void SetTerritoryDistance(float TerritoryDistance01);

    UFUNCTION(BlueprintPure, Category="RB AI|Creature")
    URBAIBrainComponent* GetBrain() const { return Brain; }

private:
    void ApplyArchetype();

    UPROPERTY(Transient)
    TObjectPtr<URBAIBrainComponent> Brain;
};
