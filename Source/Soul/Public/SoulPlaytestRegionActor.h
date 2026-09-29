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
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Soul|Playtest") FName RegionId;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Soul|Playtest") TObjectPtr<class UStaticMeshComponent> Marker;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Soul|Playtest") TObjectPtr<class UTextRenderComponent> Label;
    void Configure(FName InRegionId,const FString& DisplayName,const FVector& Location);
    void SetVisualState(const FLinearColor& Color,bool bExplored,bool bCurrent,bool bVisible=true,bool bSelected=false,int32 Defenders=0);
private:
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Standard;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Selection;
    UPROPERTY() TObjectPtr<class UInstancedStaticMeshComponent> Garrison;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> BannerMaterial;
};
