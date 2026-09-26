#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulPlaytestRegionActor.generated.h"

UCLASS()
class SOUL_API ASoulPlaytestRegionActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulPlaytestRegionActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Playtest")
    FName RegionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Playtest")
    TObjectPtr<class UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Playtest")
    TObjectPtr<class UTextRenderComponent> Label;

    void Configure(FName InRegionId, const FString& DisplayName, const FVector& Location);
    void SetVisualState(const FLinearColor& Color, bool bVisible, bool bCurrent);

    virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

private:
    TObjectPtr<class UMaterialInstanceDynamic> DynamicMaterial;
};
