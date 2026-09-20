#include "SoulFortificationSegmentActor.h"

#include "Components/SceneComponent.h"

ASoulFortificationSegmentActor::ASoulFortificationSegmentActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    IntactRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Intact"));
    IntactRoot->SetupAttachment(SceneRoot);

    DamagedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Damaged"));
    DamagedRoot->SetupAttachment(SceneRoot);

    BreachedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Breached"));
    BreachedRoot->SetupAttachment(SceneRoot);

    RepairRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Repair"));
    RepairRoot->SetupAttachment(SceneRoot);

    ApplyWallState(1000, false);
}

void ASoulFortificationSegmentActor::SetBranchVisible(USceneComponent* Branch, bool bVisible)
{
    if (!Branch) return;
    Branch->SetVisibility(bVisible, true);
    Branch->SetHiddenInGame(!bVisible, true);
}

void ASoulFortificationSegmentActor::ApplyWallState(int32 IntegrityPermille, bool bRepairing)
{
    const int32 Integrity = FMath::Clamp(IntegrityPermille, 0, 1000);
    SetBranchVisible(IntactRoot, Integrity >= 1000 && !bRepairing);
    SetBranchVisible(DamagedRoot, Integrity > 0 && Integrity < 1000 && !bRepairing);
    SetBranchVisible(BreachedRoot, Integrity <= 0 && !bRepairing);
    SetBranchVisible(RepairRoot, bRepairing);
}
