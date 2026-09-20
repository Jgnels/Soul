#include "SoulLogistics.h"

void FSoulLogisticsRules::ApplyTravel(FSoulArmyLogisticsState& State, int32 MovementCost, bool bRoad, bool bHostileTerritory)
{
    const int32 Cost = FMath::Max(0, MovementCost);
    int32 SupplyLoss = Cost * 12;
    int32 ReadinessLoss = Cost * 8;
    int32 FatigueGain = Cost * 10;

    if (bRoad)
    {
        SupplyLoss = (SupplyLoss * 70) / 100;
        ReadinessLoss = (ReadinessLoss * 75) / 100;
        FatigueGain = (FatigueGain * 75) / 100;
    }
    if (bHostileTerritory)
    {
        SupplyLoss = (SupplyLoss * 130) / 100;
    }

    State.SupplyPermille = FMath::Clamp(State.SupplyPermille - SupplyLoss, 0, 1000);
    State.ReadinessPermille = FMath::Clamp(State.ReadinessPermille - ReadinessLoss, 0, 1000);
    State.FatiguePermille = FMath::Clamp(State.FatiguePermille + FatigueGain, 0, 1000);
}

bool FSoulLogisticsRules::ForceMarch(FSoulArmyLogisticsState& State)
{
    if (State.ReadinessPermille < 350 || State.SupplyPermille < 250)
    {
        return false;
    }
    State.ReadinessPermille = FMath::Max(0, State.ReadinessPermille - 200);
    State.SupplyPermille = FMath::Max(0, State.SupplyPermille - 120);
    State.FatiguePermille = FMath::Min(1000, State.FatiguePermille + 250);
    return true;
}

void FSoulLogisticsRules::EndDay(FSoulArmyLogisticsState& State, bool bFriendlySettlement, bool bFriendlyTerritory)
{
    if (bFriendlySettlement)
    {
        State.SupplyPermille = FMath::Min(1000, State.SupplyPermille + 350);
        State.ReadinessPermille = FMath::Min(1000, State.ReadinessPermille + 300);
        State.FatiguePermille = FMath::Max(0, State.FatiguePermille - 400);
    }
    else if (bFriendlyTerritory)
    {
        State.SupplyPermille = FMath::Min(1000, State.SupplyPermille + 120);
        State.ReadinessPermille = FMath::Min(1000, State.ReadinessPermille + 160);
        State.FatiguePermille = FMath::Max(0, State.FatiguePermille - 220);
    }
    else
    {
        State.SupplyPermille = FMath::Max(0, State.SupplyPermille - 50);
        State.ReadinessPermille = FMath::Min(1000, State.ReadinessPermille + 60);
        State.FatiguePermille = FMath::Max(0, State.FatiguePermille - 100);
    }
}
