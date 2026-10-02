#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulBattleSpellCue.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

// Bounded cosmetic geometry driven by accepted RB Magic effects.
// It never selects targets, spends resources or applies damage.
UCLASS()
class SOULREALTIMEBATTLE_API ASoulBattleSpellCue : public AActor
{
    GENERATED_BODY()
public:
    ASoulBattleSpellCue();
    void Chain(const TArray<FVector>& Contacts);
    void Blizzard(FVector Center,float Radius,float Duration);
    void Aura(const TArray<AActor*>& Recipients,bool bWard,float Duration);
    virtual void Tick(float DeltaSeconds) override;
private:
    void UpdateGeometry();
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Strands;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Shards;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
    UPROPERTY() TArray<TWeakObjectPtr<AActor>> Anchors;
    TArray<FVector> Points;
    FLinearColor Color;
    float Age=0,Duration=1,Radius=0;
    int32 Kind=0;
};
