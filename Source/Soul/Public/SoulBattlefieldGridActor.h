#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulBattlefieldGridActor.generated.h"

USTRUCT(BlueprintType)
struct SOUL_API FSoulBakedHexCell
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    int32 Q = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    int32 R = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FVector SurfaceNormal = FVector::UpVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    FName SurfaceTag = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    int32 MovementCost = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    bool bValidSurface = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    bool bBlocksLargeUnits = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    bool bDeploymentValid = true;
};

UCLASS(Blueprintable)
class SOUL_API ASoulBattlefieldGridActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulBattlefieldGridActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="50.0"))
    float HexCellSize = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="1", ClampMax="20"))
    int32 BoardRadius = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="100.0"))
    float TraceHeight = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Battlefield", meta=(ClampMin="100.0"))
    float TraceDepth = 20000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TArray<FSoulBakedHexCell> BakedCells;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Soul|Battlefield")
    void BakeGrid();

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Soul|Battlefield")
    void ClearGrid();

    UFUNCTION(BlueprintPure, Category="Soul|Battlefield")
    int32 GetExpectedCellCount() const;

    static TArray<FIntPoint> BuildAxialCoordinates(int32 Radius);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Battlefield")
    TObjectPtr<USceneComponent> SceneRoot;
};
