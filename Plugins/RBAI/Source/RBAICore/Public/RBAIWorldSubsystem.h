#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RBAIWorldSubsystem.generated.h"

class URBAIBrainComponent;

UCLASS()
class RBAICORE_API URBAIWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    void RegisterBrain(URBAIBrainComponent* Brain);
    void UnregisterBrain(URBAIBrainComponent* Brain);
    void RequestImmediateEvaluation(URBAIBrainComponent* Brain);

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

    UPROPERTY(EditAnywhere, Category="RB AI|Scheduler", meta=(ClampMin="0.01"))
    double DecisionBudgetMs = 0.5;

    UPROPERTY(EditAnywhere, Category="RB AI|Scheduler", meta=(ClampMin="1"))
    int32 MaxEvaluationsPerFrame = 64;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Scheduler|Diagnostics")
    int32 LastVisitedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Scheduler|Diagnostics")
    int32 LastEvaluatedCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Scheduler|Diagnostics")
    double LastDecisionTimeMs = 0.0;

    UFUNCTION(BlueprintPure, Category="RB AI|Scheduler|Diagnostics")
    int32 GetRegisteredBrainCount() const { return Brains.Num(); }

private:
    float DistanceSquaredToNearestPlayer(const FVector& Location) const;
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<URBAIBrainComponent>> Brains;

    int32 Cursor = 0;
};
