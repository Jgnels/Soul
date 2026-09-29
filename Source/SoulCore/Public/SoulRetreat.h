#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SoulTypes.h"
#include "SoulLogistics.h"

struct FSoulRetreatResult
{
    bool bAllowed = false;
    int32 UnitsLost = 0;
    int32 SupplyLost = 0;
    int32 ReadinessLost = 0;
};

class SOULCORE_API FSoulRetreatRules
{
public:
    static FSoulRetreatResult Resolve(TArray<FSoulRegimentState>& Regiments, FName RetreatingSide, FSoulArmyLogisticsState& Logistics, bool bEnemyControlsExit, bool bCommanderAbilityAllowsSafeRetreat, FRandomStream& Rng);
};
