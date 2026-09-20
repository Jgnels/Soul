#include "SoulSettlement.h"

bool FSoulSettlementRules::PrerequisitesMet(const FSoulSettlementState& Settlement, const FSoulBuildingDefinition& Definition)
{
    for (FName RequiredId : Definition.Prerequisites)
    {
        const FSoulBuildingState* Existing = Settlement.Buildings.Find(RequiredId);
        if (!Existing || Existing->Condition == ESoulBuildingCondition::Unbuilt || Existing->Condition == ESoulBuildingCondition::Building || Existing->Condition == ESoulBuildingCondition::Ruined)
        {
            return false;
        }
    }
    return true;
}

bool FSoulSettlementRules::BeginConstruction(FSoulSettlementState& Settlement, const FSoulBuildingDefinition& Definition)
{
    if (!PrerequisitesMet(Settlement, Definition))
    {
        return false;
    }

    FSoulBuildingState& State = Settlement.Buildings.FindOrAdd(Definition.Id);
    if (State.Condition == ESoulBuildingCondition::Building)
    {
        return false;
    }
    if (State.Level >= Definition.MaxLevel && State.Condition != ESoulBuildingCondition::Ruined)
    {
        return false;
    }

    State.Id = Definition.Id;
    State.ConstructionDaysRemaining = FMath::Max(1, Definition.BuildDays);
    State.Condition = ESoulBuildingCondition::Building;
    if (State.Level <= 0)
    {
        State.IntegrityPermille = 0;
    }
    return true;
}

void FSoulSettlementRules::AdvanceDay(FSoulSettlementState& Settlement)
{
    for (TPair<FName, FSoulBuildingState>& Pair : Settlement.Buildings)
    {
        FSoulBuildingState& State = Pair.Value;
        if (State.Condition != ESoulBuildingCondition::Building)
        {
            continue;
        }

        State.ConstructionDaysRemaining = FMath::Max(0, State.ConstructionDaysRemaining - 1);
        if (State.ConstructionDaysRemaining == 0)
        {
            State.Level = FMath::Max(1, State.Level + 1);
            State.IntegrityPermille = 1000;
            State.Condition = ESoulBuildingCondition::Intact;
        }
    }
}

bool FSoulSettlementRules::DamageBuilding(FSoulSettlementState& Settlement, FName BuildingId, int32 DamagePermille, FName ScarId)
{
    FSoulBuildingState* State = Settlement.Buildings.Find(BuildingId);
    if (!State || State->Condition == ESoulBuildingCondition::Unbuilt || State->Condition == ESoulBuildingCondition::Building)
    {
        return false;
    }

    State->IntegrityPermille = FMath::Clamp(State->IntegrityPermille - FMath::Max(0, DamagePermille), 0, 1000);
    State->Condition = State->IntegrityPermille == 0 ? ESoulBuildingCondition::Ruined
        : State->IntegrityPermille < 1000 ? ESoulBuildingCondition::Damaged
        : ESoulBuildingCondition::Intact;

    if (!ScarId.IsNone() && DamagePermille > 0)
    {
        Settlement.PermanentScars.Add(ScarId);
    }
    return true;
}

bool FSoulSettlementRules::RepairBuilding(FSoulSettlementState& Settlement, FName BuildingId, int32 RepairPermille)
{
    FSoulBuildingState* State = Settlement.Buildings.Find(BuildingId);
    if (!State || State->Condition == ESoulBuildingCondition::Unbuilt || State->Condition == ESoulBuildingCondition::Building)
    {
        return false;
    }

    State->IntegrityPermille = FMath::Clamp(State->IntegrityPermille + FMath::Max(0, RepairPermille), 0, 1000);
    State->Condition = State->IntegrityPermille >= 1000 ? ESoulBuildingCondition::Intact : ESoulBuildingCondition::Damaged;
    return true;
}

void FSoulSettlementRules::DamageWalls(FSoulSettlementState& Settlement, int32 DamagePermille, FName ScarId)
{
    Settlement.WallIntegrityPermille = FMath::Clamp(Settlement.WallIntegrityPermille - FMath::Max(0, DamagePermille), 0, 1000);
    if (!ScarId.IsNone() && DamagePermille > 0)
    {
        Settlement.PermanentScars.Add(ScarId);
    }
}

void FSoulSettlementRules::RepairWalls(FSoulSettlementState& Settlement, int32 RepairPermille)
{
    Settlement.WallIntegrityPermille = FMath::Clamp(Settlement.WallIntegrityPermille + FMath::Max(0, RepairPermille), 0, 1000);
}

FSoulSettlementProjection FSoulSettlementRules::Project(const FSoulSettlementState& Settlement)
{
    FSoulSettlementProjection Projection;
    Projection.WallIntegrityPermille = Settlement.WallIntegrityPermille;
    Projection.Scars = Settlement.PermanentScars;

    for (const TPair<FName, FSoulBuildingState>& Pair : Settlement.Buildings)
    {
        const FSoulBuildingState& State = Pair.Value;
        if (State.Condition == ESoulBuildingCondition::Unbuilt)
        {
            continue;
        }

        Projection.VisibleBuildings.Add(Pair.Key);
        if (State.Condition == ESoulBuildingCondition::Damaged)
        {
            Projection.DamagedBuildings.Add(Pair.Key);
        }
        else if (State.Condition == ESoulBuildingCondition::Ruined)
        {
            Projection.RuinedBuildings.Add(Pair.Key);
        }
    }
    return Projection;
}
