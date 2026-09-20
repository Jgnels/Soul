#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulFortificationSegmentActor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulFortificationSegmentActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulFortificationSegmentActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName SettlementId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName SegmentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    FName BreachScarId;

    UFUNCTION(BlueprintCallable, Category="Soul|Siege")
    void ApplyWallState(int32 IntegrityPermille, bool bRepairing);

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Siege|External Presentation")
    TArray<TObjectPtr<AActor>> IntactActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Siege|External Presentation")
    TArray<TObjectPtr<AActor>> DamagedActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Siege|External Presentation")
    TArray<TObjectPtr<AActor>> BreachedActors;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Soul|Siege|External Presentation")
    TArray<TObjectPtr<AActor>> RepairActors;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> IntactRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> DamagedRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> BreachedRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Siege")
    TObjectPtr<USceneComponent> RepairRoot;

private:
    void SetBranchVisible(USceneComponent* Branch, bool bVisible);
    void SetActorGroupVisible(const TArray<TObjectPtr<AActor>>& Group, bool bVisible);
};
