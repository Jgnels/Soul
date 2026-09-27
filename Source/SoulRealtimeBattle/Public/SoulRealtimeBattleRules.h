#pragma once

#include "CoreMinimal.h"
#include "RBCombatCommands.h"

enum class ESoulRealtimeFormationRole : uint8
{
    Line,
    Guard,
    Breaker,
    Shock,
    Ranged,
    Support,
    Apex,
    Hero
};

struct FSoulRealtimeFormation
{
    FName FormationId;
    FName SideId;
    FName UnitId;
    ESoulRealtimeFormationRole Role = ESoulRealtimeFormationRole::Line;
    int32 StrategicCount = 0;
    int32 ActiveCount = 0;
    int32 ReserveCount = 0;
    int32 PowerPerBody = 100;
    int32 ReinforcementPriority = 0;
    int32 MaxActiveRepresentations = FRBCombatGroup::MaximumMembers;
    bool bPersistentRegiment = true;
};

struct FSoulReinforcementWave
{
    FName SideId;
    TMap<FName, int32> FormationCounts;

    int32 TotalBodies() const;
};

struct FSoulTerrainMagicProfile
{
    FName ProfileId;
    TMap<FName, int32> SchoolPowerPermille;

    int32 MultiplierFor(FName SchoolId) const;
};

struct FSoulRealtimeBattleState
{
    TArray<FSoulRealtimeFormation> Formations;
    int32 MaxActivePerSide = 32;
    int32 ReinforcementTriggerPermille = 700;
    int32 MaxWaveSize = 8;
    bool bReinforcementsEnabled = true;
};

class SOULREALTIMEBATTLE_API FSoulRealtimeBattleRules
{
public:
    static void InitializeDeployment(FSoulRealtimeBattleState& Battle);
    static int32 ActiveBodies(const FSoulRealtimeBattleState& Battle, FName SideId);
    static int32 ReserveBodies(const FSoulRealtimeBattleState& Battle, FName SideId);
    static int32 ActivePower(const FSoulRealtimeBattleState& Battle, FName SideId);
    static bool HasLivingForce(const FSoulRealtimeBattleState& Battle, FName SideId);

    static int32 ApplyCasualties(
        FSoulRealtimeBattleState& Battle,
        FName FormationId,
        int32 Casualties);

    static bool ShouldReinforce(
        const FSoulRealtimeBattleState& Battle,
        FName SideId);

    static FSoulReinforcementWave BuildAndApplyWave(
        FSoulRealtimeBattleState& Battle,
        FName SideId);
    // Read-only presentation of the same allocation used for actual arrivals.
    static FSoulReinforcementWave PreviewWave(
        const FSoulRealtimeBattleState& Battle,
        FName SideId);
    static FSoulTerrainMagicProfile MakeVolcanicMagicProfile();
};
