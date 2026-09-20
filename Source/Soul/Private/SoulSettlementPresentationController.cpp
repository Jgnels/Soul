#include "SoulSettlementPresentationController.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "SoulFortificationSegmentActor.h"
#include "SoulSettlementBuildingActor.h"
#include "SoulSettlementStateSubsystem.h"

ASoulSettlementPresentationController::ASoulSettlementPresentationController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASoulSettlementPresentationController::BeginPlay()
{
    Super::BeginPlay();
    RefreshSettlementPresentation();
}

USoulSettlementStateSubsystem*
ASoulSettlementPresentationController::ResolveState() const
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance
        ? GameInstance->GetSubsystem<USoulSettlementStateSubsystem>()
        : nullptr;
}

bool ASoulSettlementPresentationController::RefreshSettlementPresentation()
{
    if (SettlementId.IsNone() || !GetWorld())
    {
        return false;
    }

    USoulSettlementStateSubsystem* State = ResolveState();
    if (!State || !State->HasSettlement(SettlementId))
    {
        return false;
    }

    for (TActorIterator<ASoulSettlementBuildingActor> It(GetWorld()); It; ++It)
    {
        ASoulSettlementBuildingActor* BuildingActor = *It;
        if (!BuildingActor || BuildingActor->SettlementId != SettlementId)
        {
            continue;
        }

        const FName Condition = State->GetBuildingConditionName(
            SettlementId, BuildingActor->BuildingId);
        const int32 Integrity = State->GetBuildingIntegrity(
            SettlementId, BuildingActor->BuildingId);

        if (Condition.IsNone())
        {
            BuildingActor->ApplyConditionName(TEXT("Unbuilt"));
        }
        else if (Integrity >= 0)
        {
            BuildingActor->ApplyIntegrity(
                Integrity,
                Condition != TEXT("Unbuilt"),
                Condition == TEXT("Building") || Condition == TEXT("Repairing"));
        }
        else
        {
            BuildingActor->ApplyConditionName(Condition);
        }
    }

    const int32 WallIntegrity = State->GetWallIntegrity(SettlementId);
    for (TActorIterator<ASoulFortificationSegmentActor> It(GetWorld()); It; ++It)
    {
        ASoulFortificationSegmentActor* Segment = *It;
        if (!Segment || Segment->SettlementId != SettlementId)
        {
            continue;
        }

        const bool bPersistentlyBreached =
            !Segment->BreachScarId.IsNone()
            && State->HasSettlementScar(SettlementId, Segment->BreachScarId);

        Segment->ApplyWallState(
            bPersistentlyBreached ? 0 : FMath::Max(0, WallIntegrity),
            false);
    }

    return true;
}
