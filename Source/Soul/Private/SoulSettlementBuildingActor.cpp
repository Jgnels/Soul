#include "SoulSettlementBuildingActor.h"

#include "Components/SceneComponent.h"

ASoulSettlementBuildingActor::ASoulSettlementBuildingActor()
{
    PrimaryActorTick.bCanEverTick = false;

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

void ASoulSettlementBuildingActor::SetBranchVisible(USceneComponent* Branch, bool bVisible)
{
    if (!Branch) return;
    Branch->SetVisibility(bVisible, true);
    Branch->SetHiddenInGame(!bVisible, true);
}

void ASoulSettlementBuildingActor::ShowOnly(USceneComponent* VisibleRoot)
{
    SetBranchVisible(ConstructionRoot, VisibleRoot == ConstructionRoot);
    SetBranchVisible(IntactRoot, VisibleRoot == IntactRoot);
    SetBranchVisible(DamagedRoot, VisibleRoot == DamagedRoot);
    SetBranchVisible(RuinedRoot, VisibleRoot == RuinedRoot);
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
    if (!bBuilt)
    {
        ShowOnly(bConstructing ? ConstructionRoot : nullptr);
        return;
    }

    const int32 Integrity = FMath::Clamp(IntegrityPermille, 0, 1000);
    if (Integrity <= 0) ShowOnly(RuinedRoot);
    else if (Integrity < 1000) ShowOnly(DamagedRoot);
    else ShowOnly(IntactRoot);
}
