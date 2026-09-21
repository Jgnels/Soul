#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBAIThreatComponent.generated.h"

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAICOMBAT_API URBAIThreatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAIThreatComponent();

    UFUNCTION(BlueprintCallable, Category="RB AI|Threat")
    void AddThreat(AActor* Target, float Delta);

    UFUNCTION(BlueprintCallable, Category="RB AI|Threat")
    void SetThreat(AActor* Target, float Value);

    UFUNCTION(BlueprintCallable, Category="RB AI|Threat")
    void RemoveTarget(AActor* Target);

    UFUNCTION(BlueprintCallable, Category="RB AI|Threat")
    void ClearThreat();

    UFUNCTION(BlueprintCallable, Category="RB AI|Threat")
    void ApplyDecay(float Multiplier);

    UFUNCTION(BlueprintPure, Category="RB AI|Threat")
    float GetThreat(AActor* Target) const;

    UFUNCTION(BlueprintPure, Category="RB AI|Threat")
    AActor* GetHighestThreatTarget() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Threat", meta=(ClampMin="0.0"))
    float MaximumThreat = 1000.0f;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<AActor>, float> ThreatByTarget;
};
