#include "SoulSiege.h"

FSoulSiegeState FSoulSiegeRules::Begin(const FSoulSiegePreparation& Preparation)
{
    FSoulSiegeState State;
    State.GateIntegrityPermille = Preparation.bReinforcedGate ? 1300 : 1000;
    State.GateMaximumIntegrity = State.GateIntegrityPermille;
    State.bMagicWardActive = Preparation.bMagicalWard;
    State.bArmoryActive = Preparation.bAmmoStores;
    State.bGatehouseActive = true;
    return State;
}

bool FSoulSiegeRules::CanAssaultWalls(const FSoulSiegePreparation& Preparation, bool bHasFlyingUnit)
{
    return bHasFlyingUnit
        || Preparation.bLadders
        || Preparation.bSiegeTower
        || Preparation.bWallBreach
        || Preparation.bBatteringRam;
}

void FSoulSiegeRules::AdvanceEncirclementDay(FSoulSiegeState& State)
{
    ++State.EncirclementDays;
    State.DefenderSupplyPermille = FMath::Max(200, State.DefenderSupplyPermille - 70);
}

void FSoulSiegeRules::CaptureObjective(FSoulSiegeState& State, ESoulSiegeObjective Objective)
{
    State.CapturedObjectives.Add(Objective);
    switch (Objective)
    {
        case ESoulSiegeObjective::Gatehouse:
            State.bGatehouseActive = false;
            State.GateIntegrityPermille = FMath::Min(State.GateIntegrityPermille, 300);
            break;
        case ESoulSiegeObjective::Armory:
            State.bArmoryActive = false;
            break;
        case ESoulSiegeObjective::MageTower:
            State.bMagicWardActive = false;
            break;
        case ESoulSiegeObjective::Keep:
            State.bVictory = true;
            State.Layer = ESoulSiegeLayer::Resolved;
            break;
    }
}

void FSoulSiegeRules::OpenBreach(FSoulSiegeState& State)
{
    ++State.WallBreaches;
}

void FSoulSiegeRules::AdvanceLayer(FSoulSiegeState& State)
{
    if (State.bVictory) return;
    switch (State.Layer)
    {
        case ESoulSiegeLayer::OuterField: State.Layer = ESoulSiegeLayer::Walls; break;
        case ESoulSiegeLayer::Walls: State.Layer = ESoulSiegeLayer::InnerSettlement; break;
        case ESoulSiegeLayer::InnerSettlement: State.Layer = ESoulSiegeLayer::Keep; break;
        case ESoulSiegeLayer::Keep:
        case ESoulSiegeLayer::Resolved:
            break;
    }
}

bool FSoulSiegeRules::ApplyGateDamage(FSoulSiegeState& State, int32 Damage)
{
    if (Damage<=0 || Damage>100000 || State.GateIntegrityPermille<=0 || State.bVictory) return false;
    State.GateIntegrityPermille=FMath::Max(0,State.GateIntegrityPermille-Damage);
    if(State.GateIntegrityPermille==0){State.bGatehouseActive=false;OpenBreach(State);State.Layer=ESoulSiegeLayer::InnerSettlement;}
    return true;
}
bool FSoulSiegeRules::AdvanceCourtyard(FSoulSiegeState& State,int32 Millis,int32 Attackers,int32 Defenders)
{
    if(Millis<=0||Millis>1000||Attackers<0||Defenders<0||State.bVictory||State.GateIntegrityPermille>0||State.WallBreaches<=0)return false;
    if(Attackers>0&&Defenders==0)State.CourtyardControlMillis=FMath::Min(15000,State.CourtyardControlMillis+Millis);
    else if(Attackers==0)State.CourtyardControlMillis=FMath::Max(0,State.CourtyardControlMillis-Millis);
    // A living defender contests; the clock never advances through opposition.
    if(State.CourtyardControlMillis==15000)CaptureObjective(State,ESoulSiegeObjective::Keep);
    return State.bVictory;
}
