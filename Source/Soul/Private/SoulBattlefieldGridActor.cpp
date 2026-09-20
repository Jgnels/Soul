#include "SoulBattlefieldGridActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

ASoulBattlefieldGridActor::ASoulBattlefieldGridActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
}

TArray<FIntPoint> ASoulBattlefieldGridActor::BuildAxialCoordinates(int32 Radius)
{
    const int32 R = FMath::Max(0, Radius);
    TArray<FIntPoint> Result;
    Result.Reserve(1 + 3 * R * (R + 1));

    for (int32 Q = -R; Q <= R; ++Q)
    {
        const int32 MinR = FMath::Max(-R, -Q - R);
        const int32 MaxR = FMath::Min(R, -Q + R);
        for (int32 AxialR = MinR; AxialR <= MaxR; ++AxialR)
        {
            Result.Emplace(Q, AxialR);
        }
    }
    return Result;
}

int32 ASoulBattlefieldGridActor::GetExpectedCellCount() const
{
    const int32 R = FMath::Max(0, BoardRadius);
    return 1 + 3 * R * (R + 1);
}

void ASoulBattlefieldGridActor::ClearGrid()
{
    Modify();
    BakedCells.Reset();
#if WITH_EDITOR
    MarkPackageDirty();
#endif
}

void ASoulBattlefieldGridActor::BakeGrid()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    Modify();
    BakedCells.Reset();

    const FVector Origin = GetActorLocation();
    const double Root3 = FMath::Sqrt(3.0);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SoulBattlefieldGridBake), true, this);
    Params.bReturnPhysicalMaterial = true;

    for (const FIntPoint& Axial : BuildAxialCoordinates(BoardRadius))
    {
        // Pointy-top axial coordinates.
        const double LocalX = HexCellSize * Root3 * (Axial.X + Axial.Y * 0.5);
        const double LocalY = HexCellSize * 1.5 * Axial.Y;
        const FVector XY = Origin + FVector(LocalX, LocalY, 0.0);
        const FVector Start(XY.X, XY.Y, Origin.Z + TraceHeight);
        const FVector End(XY.X, XY.Y, Origin.Z - TraceDepth);

        FHitResult Hit;
        FSoulBakedHexCell Cell;
        Cell.Q = Axial.X;
        Cell.R = Axial.Y;

        if (World->LineTraceSingleByChannel(
                Hit, Start, End, ECC_Visibility, Params))
        {
            Cell.bValidSurface = true;
            Cell.WorldLocation = Hit.ImpactPoint;
            Cell.SurfaceNormal = Hit.ImpactNormal;

            if (const UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get())
            {
                Cell.SurfaceTag = PhysMat->GetFName();
            }

            const float Z = Hit.ImpactNormal.Z;
            if (Z >= 0.92f)
            {
                Cell.MovementCost = 100;
            }
            else if (Z >= 0.75f)
            {
                Cell.MovementCost = 125;
            }
            else if (Z >= 0.55f)
            {
                Cell.MovementCost = 175;
                Cell.bBlocksLargeUnits = true;
            }
            else
            {
                Cell.MovementCost = 1000;
                Cell.bBlocksLargeUnits = true;
                Cell.bDeploymentValid = false;
            }
        }
        else
        {
            Cell.WorldLocation = XY;
            Cell.MovementCost = 1000;
            Cell.bValidSurface = false;
            Cell.bBlocksLargeUnits = true;
            Cell.bDeploymentValid = false;
        }

        BakedCells.Add(Cell);
    }

#if WITH_EDITOR
    MarkPackageDirty();
#endif
}
