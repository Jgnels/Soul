#pragma once

#include "CoreMinimal.h"

enum class ESoulSiegeLayer : uint8
{
    OuterField,
    Walls,
    InnerSettlement,
    Keep,
    Resolved
};

enum class ESoulSiegeObjective : uint8
{
    Gatehouse,
    Armory,
    MageTower,
    Keep
};

struct FSoulSiegePreparation
{
    bool bLadders = false;
    bool bBatteringRam = false;
    bool bSiegeTower = false;
    bool bWallBreach = false;
    bool bDefenderBarricades = false;
    bool bReinforcedGate = false;
    bool bAmmoStores = false;
    bool bMagicalWard = false;
};

struct FSoulSiegeState
{
    ESoulSiegeLayer Layer = ESoulSiegeLayer::OuterField;
    int32 EncirclementDays = 0;
    int32 DefenderSupplyPermille = 1000;
    int32 GateIntegrityPermille = 1000;
    int32 GateMaximumIntegrity = 1000;
    int32 CourtyardControlMillis = 0;
    int32 WallBreaches = 0;
    bool bMagicWardActive = false;
    bool bArmoryActive = true;
    bool bGatehouseActive = true;
    bool bVictory = false;
    TSet<ESoulSiegeObjective> CapturedObjectives;
};

class SOULCORE_API FSoulSiegeRules
{
public:
    static FSoulSiegeState Begin(const FSoulSiegePreparation& Preparation);
    static bool CanAssaultWalls(const FSoulSiegePreparation& Preparation, bool bHasFlyingUnit);
    static void AdvanceEncirclementDay(FSoulSiegeState& State);
    static void CaptureObjective(FSoulSiegeState& State, ESoulSiegeObjective Objective);
    static void OpenBreach(FSoulSiegeState& State);
    static void AdvanceLayer(FSoulSiegeState& State);
    // Real-time adapter supplies only RB-accepted damage and physical occupancy.
    static bool ApplyGateDamage(FSoulSiegeState& State, int32 AcceptedDamage);
    static bool AdvanceCourtyard(FSoulSiegeState& State, int32 Millis, int32 Attackers, int32 Defenders);
};
