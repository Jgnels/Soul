#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulBattlefieldLayoutActor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulBattlefieldLayoutActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulBattlefieldLayoutActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FName RecipeId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FName Biome;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FName Landform;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FName StrategicFeature;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="50.0"))
    float HexCellSize = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="3", ClampMax="20"))
    int32 BoardRadius = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    bool bSupportsLargeCreatures = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TObjectPtr<USceneComponent> AttackerDeploymentRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TObjectPtr<USceneComponent> DefenderDeploymentRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TObjectPtr<USceneComponent> LandmarkRoot;
};
