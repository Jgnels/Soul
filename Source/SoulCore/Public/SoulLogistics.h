#pragma once

#include "CoreMinimal.h"

struct FSoulArmyLogisticsState
{
    int32 SupplyPermille = 1000;
    int32 ReadinessPermille = 1000;
    int32 FatiguePermille = 0;
};

class SOULCORE_API FSoulLogisticsRules
{
public:
    static void ApplyTravel(FSoulArmyLogisticsState& State, int32 MovementCost, bool bRoad, bool bHostileTerritory);
    static bool ForceMarch(FSoulArmyLogisticsState& State);
    static void EndDay(FSoulArmyLogisticsState& State, bool bFriendlySettlement, bool bFriendlyTerritory);
};
