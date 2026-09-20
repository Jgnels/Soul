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

void ASoulFortificationSegmentActor::SetActorGroupVisible(
    const TArray<TObjectPtr<AActor>>& Group,
    bool bVisible)
{
    for (AActor* Actor : Group)
    {
        if (!IsValid(Actor) || Actor == this)
        {
            continue;
        }
        Actor->SetActorHiddenInGame(!bVisible);
#if WITH_EDITOR
        Actor->SetIsTemporarilyHiddenInEditor(!bVisible);
#endif
        Actor->SetActorEnableCollision(bVisible);
    }
}

void ASoulFortificationSegmentActor::ApplyWallState(int32 IntegrityPermille, bool bRepairing)
{
    const int32 Integrity = FMath::Clamp(IntegrityPermille, 0, 1000);
    const bool bIntact = Integrity >= 1000 && !bRepairing;
    const bool bDamaged = Integrity > 0 && Integrity < 1000 && !bRepairing;
    const bool bBreached = Integrity <= 0 && !bRepairing;

    SetBranchVisible(IntactRoot, bIntact);
    SetBranchVisible(DamagedRoot, bDamaged);
    SetBranchVisible(BreachedRoot, bBreached);
    SetBranchVisible(RepairRoot, bRepairing);

    SetActorGroupVisible(IntactActors, bIntact);
    SetActorGroupVisible(DamagedActors, bDamaged);
    SetActorGroupVisible(BreachedActors, bBreached);
    SetActorGroupVisible(RepairActors, bRepairing);
}
