#include "SoulRetreat.h"

FSoulRetreatResult FSoulRetreatRules::Resolve(TArray<FSoulRegimentState>& Regiments, FName RetreatingSide, FSoulArmyLogisticsState& Logistics, bool bEnemyControlsExit, bool bCommanderAbilityAllowsSafeRetreat, FRandomStream& Rng)
{
    FSoulRetreatResult Result;
    if (RetreatingSide.IsNone())
    {
        return Result;
    }

    Result.bAllowed = true;
    const int32 LossPermille = bCommanderAbilityAllowsSafeRetreat ? 50 : (bEnemyControlsExit ? 250 : 120);

    for (FSoulRegimentState& Regiment : Regiments)
    {
        if (!Regiment.bAlive || Regiment.SideId != RetreatingSide || Regiment.Count <= 0)
        {
            continue;
        }

        int32 Losses = 0;
        for (int32 Index = 0; Index < Regiment.Count; ++Index)
        {
            if (Rng.RandRange(0, 999) < LossPermille)
            {
                ++Losses;
            }
        }

        Losses = FMath::Min(Losses, Regiment.Count);
        Regiment.Count -= Losses;
        Result.UnitsLost += Losses;
        if (Regiment.Count <= 0)
        {
            Regiment.Count = 0;
            Regiment.CurrentHitPoints = 0;
            Regiment.bAlive = false;
        }
    }

    Result.SupplyLost = bCommanderAbilityAllowsSafeRetreat ? 60 : 150;
    Result.ReadinessLost = bCommanderAbilityAllowsSafeRetreat ? 100 : 250;
    Logistics.SupplyPermille = FMath::Max(0, Logistics.SupplyPermille - Result.SupplyLost);
    Logistics.ReadinessPermille = FMath::Max(0, Logistics.ReadinessPermille - Result.ReadinessLost);
    Logistics.FatiguePermille = FMath::Min(1000, Logistics.FatiguePermille + 150);
    return Result;
}
