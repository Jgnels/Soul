#include "SoulSettlementBuildingActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "SoulSettlementStateSubsystem.h"

ASoulSettlementBuildingActor::ASoulSettlementBuildingActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = .2f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    ConstructionRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Construction"));
    ConstructionRoot->SetupAttachment(SceneRoot);

    IntactRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Intact"));
    IntactRoot->SetupAttachment(SceneRoot);

    DamagedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Damaged"));
    DamagedRoot->SetupAttachment(SceneRoot);

    RuinedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Ruined"));
    RuinedRoot->SetupAttachment(SceneRoot);

    ShowOnly(nullptr);
}

bool ASoulSettlementBuildingActor::ConfigureMiniature(UStaticMesh* BaseMesh, UStaticMesh* UpgradeMesh)
{
    if (!BaseMesh || !UpgradeMesh || GetInstanceComponents().Num() != 0) return false;
    auto AddMesh = [&](UStaticMesh* Mesh, USceneComponent* Branch)
    {
        auto* Component = NewObject<UStaticMeshComponent>(this);
        Component->SetupAttachment(Branch);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        AddInstanceComponent(Component);
        Component->RegisterComponent();
    };
    AddMesh(BaseMesh, SceneRoot);
    AddMesh(UpgradeMesh, IntactRoot);
    bFollowSettlementState = true;
    SetActorTickEnabled(true);
    Tick(0);
    return true;
}

void ASoulSettlementBuildingActor::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(bFollowSettlementState);
    if (bFollowSettlementState) Tick(0);
}

void ASoulSettlementBuildingActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bFollowSettlementState || SettlementId.IsNone() || BuildingId.IsNone()) return;
    auto* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    auto* State = GI ? GI->GetSubsystem<USoulSettlementStateSubsystem>() : nullptr;
    // Reapply to catch nested level instances that finish loading after this
    // wrapper. Only this explicitly authored group is traversed, not the city.
    ApplyConditionName(State ? State->GetBuildingConditionName(SettlementId, BuildingId) : NAME_None);
}

void ASoulSettlementBuildingActor::SetBranchVisible(USceneComponent* Branch, bool bVisible)
{
    if (!Branch) return;
    Branch->SetVisibility(bVisible, true);
    Branch->SetHiddenInGame(!bVisible, true);
}

void ASoulSettlementBuildingActor::SetActorGroupVisible(
    const TArray<TObjectPtr<AActor>>& Group,
    bool bVisible)
{
    TSet<AActor*> Visited;
    for (AActor* Actor : Group)
    {
        SetAuthoredActorVisible(Actor, bVisible, Visited);
    }
}

void ASoulSettlementBuildingActor::SetAuthoredActorVisible(AActor* Actor, bool bVisible, TSet<AActor*>& Visited)
{
    if (!IsValid(Actor) || Actor == this || Visited.Contains(Actor)) return;
    Visited.Add(Actor);
    const TWeakObjectPtr<AActor> Key(Actor);
    if (!AuthoredCollision.Contains(Key)) AuthoredCollision.Add(Key, Actor->GetActorEnableCollision());
    Actor->SetActorHiddenInGame(!bVisible);
#if WITH_EDITOR
    Actor->SetIsTemporarilyHiddenInEditor(!bVisible);
#endif
    Actor->SetActorEnableCollision(bVisible && AuthoredCollision.FindChecked(Key));
    TArray<AActor*> AttachedActors;
    Actor->GetAttachedActors(AttachedActors);
    for (AActor* Child : AttachedActors) SetAuthoredActorVisible(Child, bVisible, Visited);
    if (auto* Instance = Cast<ILevelInstanceInterface>(Actor))
        if (ULevel* Level = Instance->GetLoadedLevel())
            for (AActor* Child : Level->Actors) SetAuthoredActorVisible(Child, bVisible, Visited);
}

void ASoulSettlementBuildingActor::ShowOnly(USceneComponent* VisibleRoot)
{
    const bool bConstruction = VisibleRoot == ConstructionRoot;
    const bool bIntact = VisibleRoot == IntactRoot;
    const bool bDamaged = VisibleRoot == DamagedRoot;
    const bool bRuined = VisibleRoot == RuinedRoot;

    SetBranchVisible(ConstructionRoot, bConstruction);
    SetBranchVisible(IntactRoot, bIntact);
    SetBranchVisible(DamagedRoot, bDamaged);
    SetBranchVisible(RuinedRoot, bRuined);

    SetActorGroupVisible(ConstructionActors, bConstruction);
    SetActorGroupVisible(IntactActors, bIntact);
    SetActorGroupVisible(DamagedActors, bDamaged);
    SetActorGroupVisible(RuinedActors, bRuined);
}

void ASoulSettlementBuildingActor::ApplyConditionName(FName ConditionName)
{
    if (ConditionName == TEXT("Building") || ConditionName == TEXT("Repairing"))
    {
        ShowOnly(ConstructionRoot);
    }
    else if (ConditionName == TEXT("Intact"))
    {
        ShowOnly(IntactRoot);
    }
    else if (ConditionName == TEXT("Damaged"))
    {
        ShowOnly(DamagedRoot);
    }
    else if (ConditionName == TEXT("Ruined"))
    {
        ShowOnly(RuinedRoot);
    }
    else
    {
        ShowOnly(nullptr);
    }
}

void ASoulSettlementBuildingActor::ApplyIntegrity(int32 IntegrityPermille, bool bBuilt, bool bConstructing)
{
    // Construction is authoritative even when an upgrade retains its old integrity.
    if (bConstructing)
    {
        ShowOnly(ConstructionRoot);
        return;
    }
    if (!bBuilt)
    {
        ShowOnly(nullptr);
        return;
    }

    const int32 Integrity = FMath::Clamp(IntegrityPermille, 0, 1000);
    if (Integrity <= 0) ShowOnly(RuinedRoot);
    else if (Integrity < 1000) ShowOnly(DamagedRoot);
    else ShowOnly(IntactRoot);
}
