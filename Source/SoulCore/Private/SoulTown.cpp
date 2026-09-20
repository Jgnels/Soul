#include "SoulTown.h"

bool FSoulTownRules::BeginConstruction(
    FSoulCampaignEconomy& Economy,
    FSoulSettlementState& Settlement,
    const FSoulBuildingDefinition& Definition)
{
    if (!FSoulSettlementRules::CanBeginConstruction(Settlement, Definition))
    {
        return false;
    }
    if (!FSoulCampaignRules::CanAfford(Economy, Definition.BuildCost))
    {
        return false;
    }

    if (!FSoulSettlementRules::BeginConstruction(Settlement, Definition))
    {
        return false;
    }

    for (const TPair<FName, int32>& Cost : Definition.BuildCost)
    {
        Economy.Resources.FindOrAdd(Cost.Key) -= FMath::Max(0, Cost.Value);
    }
    return true;
}

bool FSoulTownRules::RecruitFromBuilding(
    FSoulCampaignEconomy& Economy,
    const FSoulSettlementState& Settlement,
    FName BuildingId,
    FName UnitId,
    int32 Quantity,
    int32 MinimumIntegrityPermille)
{
    if (!FSoulSettlementRules::IsOperational(Settlement, BuildingId, MinimumIntegrityPermille))
    {
        return false;
    }
    return FSoulCampaignRules::Recruit(Economy, UnitId, Quantity);
}

int32 FSoulTownRules::EffectiveWeeklyGrowth(
    const FSoulCampaignEconomy& Economy,
    const FSoulSettlementState& Settlement,
    FName BuildingId,
    FName UnitId,
    int32 MinimumIntegrityPermille)
{
    const FSoulRecruitmentPool* Pool = Economy.RecruitmentPools.Find(UnitId);
    if (!Pool)
    {
        return 0;
    }
    if (!FSoulSettlementRules::IsOperational(Settlement, BuildingId, MinimumIntegrityPermille))
    {
        return 0;
    }

    const FSoulBuildingState* Building = Settlement.Buildings.Find(BuildingId);
    const int32 Integrity = Building ? FMath::Clamp(Building->IntegrityPermille, 0, 1000) : 0;
    return FMath::Max(0, (Pool->WeeklyGrowth * Integrity + 999) / 1000);
}
