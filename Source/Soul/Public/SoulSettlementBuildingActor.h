#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulSettlementBuildingActor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulSettlementBuildingActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulSettlementBuildingActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName SettlementId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName BuildingId;

    // Optional upgrade wing: reads the existing building level; no extra save state.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement", meta=(ClampMin="1"))
    int32 MinimumBuildingLevel = 1;


    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    void ApplyConditionName(FName ConditionName);

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    void ApplyIntegrity(int32 IntegrityPermille, bool bBuilt, bool bConstructing);

    // Opt-in for authored environment/miniature groups. Reads the existing save
    // authority; never seeds, constructs or persists a second building state.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    bool bFollowSettlementState = false;

    virtual void Tick(float DeltaSeconds) override;
    // A campaign representation uses the same state branches as a full city.
    // Both meshes must be owned deterministic derivatives; no geometry is generated here.
    bool ConfigureMiniature(class UStaticMesh* BaseMesh, class UStaticMesh* UpgradeMesh, bool bUpgradeOnly = false);

    // Level-instance / donor geometry already placed in the city map can be assigned here.
    // Soul toggles it from canonical settlement state; geometry does not own gameplay truth.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Settlement|External Presentation")
    TArray<TObjectPtr<AActor>> ConstructionActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Settlement|External Presentation")
    TArray<TObjectPtr<AActor>> IntactActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Settlement|External Presentation")
    TArray<TObjectPtr<AActor>> DamagedActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Settlement|External Presentation")
    TArray<TObjectPtr<AActor>> RuinedActors;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<USceneComponent> ConstructionRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<USceneComponent> IntactRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<USceneComponent> DamagedRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<USceneComponent> RuinedRoot;

private:
    void ShowOnly(USceneComponent* VisibleRoot);
    void SetBranchVisible(USceneComponent* Branch, bool bVisible);
    void SetActorGroupVisible(const TArray<TObjectPtr<AActor>>& Group, bool bVisible);
    void SetAuthoredActorVisible(AActor* Actor, bool bVisible, TSet<AActor*>& Visited);
    TMap<TWeakObjectPtr<AActor>, bool> AuthoredCollision;
};
