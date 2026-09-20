#include "SoulSettlementBootstrapActor.h"

#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "SoulSettlement.h"
#include "SoulSettlementPresentationController.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"

ASoulSettlementBootstrapActor::ASoulSettlementBootstrapActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASoulSettlementBootstrapActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyScenario();
}

bool ASoulSettlementBootstrapActor::ApplyScenario()
{
    if (!Scenario || Scenario->SettlementId.IsNone() || !GetWorld())
    {
        return false;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    USoulSettlementStateSubsystem* State = GameInstance
        ? GameInstance->GetSubsystem<USoulSettlementStateSubsystem>()
        : nullptr;
    if (!State)
    {
        return false;
    }

    if (bOnlyCreateIfMissing && State->HasSettlement(Scenario->SettlementId))
    {
        return true;
    }

    FSoulSettlementState& Settlement =
        State->FindOrAddSettlement(Scenario->SettlementId);
    Settlement.SettlementId = Scenario->SettlementId;
    Settlement.FactionId = Scenario->FactionId;
    Settlement.RegionId = Scenario->RegionId;
    Settlement.FortificationLevel = FMath::Max(0, Scenario->FortificationLevel);
    Settlement.WallIntegrityPermille =
        FMath::Clamp(Scenario->WallIntegrityPermille, 0, 1000);
    Settlement.PermanentScars = Scenario->PermanentScars;
    Settlement.Buildings.Reset();

    for (const FSoulInitialBuildingSpec& Spec : Scenario->Buildings)
    {
        if (Spec.BuildingId.IsNone())
        {
            continue;
        }

        FSoulBuildingState Building;
        Building.Id = Spec.BuildingId;
        Building.Level = Spec.bBuilt ? FMath::Max(1, Spec.Level) : 0;
        Building.IntegrityPermille = Spec.bBuilt
            ? FMath::Clamp(Spec.IntegrityPermille, 0, 1000)
            : 0;
        Building.Condition = !Spec.bBuilt
            ? ESoulBuildingCondition::Unbuilt
            : Building.IntegrityPermille <= 0
                ? ESoulBuildingCondition::Ruined
                : Building.IntegrityPermille < 1000
                    ? ESoulBuildingCondition::Damaged
                    : ESoulBuildingCondition::Intact;
        Settlement.Buildings.Add(Building.Id, Building);
    }

    // A map can bootstrap before or after presentation actors. Refresh all matching
    // controllers immediately so town/siege views agree with canonical state.
    for (TActorIterator<ASoulSettlementPresentationController> It(GetWorld()); It; ++It)
    {
        ASoulSettlementPresentationController* Controller = *It;
        if (Controller && Controller->SettlementId == Scenario->SettlementId)
        {
            Controller->RefreshSettlementPresentation();
        }
    }

    return true;
}
