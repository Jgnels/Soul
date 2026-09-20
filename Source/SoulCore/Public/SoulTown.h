#pragma once

#include "CoreMinimal.h"
#include "SoulCampaign.h"
#include "SoulSettlement.h"

class SOULCORE_API FSoulTownRules
{
public:
    static bool BeginConstruction(
        FSoulCampaignEconomy& Economy,
        FSoulSettlementState& Settlement,
        const FSoulBuildingDefinition& Definition);

    static void AdvanceDay(
        FSoulCampaignEconomy& Economy,
        const FSoulSettlementState& Settlement,
        int32 MinimumIntegrityPermille = 500);

    static bool RecruitFromBuilding(
        FSoulCampaignEconomy& Economy,
        const FSoulSettlementState& Settlement,
        FName BuildingId,
        FName UnitId,
        int32 Quantity,
        int32 MinimumIntegrityPermille = 500);

    static int32 EffectiveWeeklyGrowth(
        const FSoulCampaignEconomy& Economy,
        const FSoulSettlementState& Settlement,
        FName BuildingId,
        FName UnitId,
        int32 MinimumIntegrityPermille = 500);
};
