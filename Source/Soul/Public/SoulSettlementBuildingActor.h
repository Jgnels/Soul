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

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    void ApplyConditionName(FName ConditionName);

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    void ApplyIntegrity(int32 IntegrityPermille, bool bBuilt, bool bConstructing);

protected:
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
};
