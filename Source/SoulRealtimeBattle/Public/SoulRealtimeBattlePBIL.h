#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SoulRealtimeBattlePBIL.generated.h"

class AActor;
class UWorld;
class ARBPBILInfluenceVolume;
class URBPBILInfluenceComponent;

/** Adapts real battle occupancy to PBIL; RB Combat retains movement and attack authority. */
UCLASS()
class SOULREALTIMEBATTLE_API USoulRealtimeBattlePBIL : public UObject
{
    GENERATED_BODY()

public:
    bool Configure(UWorld* World, const FVector& Origin,
        const FVector& HalfExtents = FVector(5000.0f, 3500.0f, 1500.0f));
    bool RegisterUnit(AActor* Unit, bool bEnemy);
    void UnregisterUnit(AActor* Unit);
    bool QueryApproach(AActor* Unit, bool bEnemy, const FVector& Target, FVector& OutLocation);
    void Shutdown();

    int32 GetQueryCount() const { return QueryCount; }
    int32 GetSuccessfulQueryCount() const { return SuccessfulQueryCount; }
    int32 GetRegisteredUnitCount() const;

private:
    struct FUnitSource
    {
        TWeakObjectPtr<AActor> Unit;
        TWeakObjectPtr<URBPBILInfluenceComponent> Influence;
        bool bEnemy = false;
    };

    TArray<FUnitSource> Sources;
    TWeakObjectPtr<UWorld> BattleWorld;

    UPROPERTY(Transient)
    TObjectPtr<ARBPBILInfluenceVolume> Field;

    int32 QueryCount = 0;
    int32 SuccessfulQueryCount = 0;
};
