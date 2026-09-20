#pragma once

#include "CoreMinimal.h"
#include "SoulSettlement.h"

struct FSoulSiegeAftermath
{
    int32 WallDamagePermille = 0;
    TMap<FName, int32> BuildingDamagePermille;
    TSet<FName> ScarIds;
};

class SOULCORE_API FSoulSiegeAftermathRules
{
public:
    static void Apply(FSoulSettlementState& Settlement, const FSoulSiegeAftermath& Aftermath);
};
