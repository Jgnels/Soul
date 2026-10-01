#pragma once

#include "CoreMinimal.h"
#include "RBCombatGroupDriver.h"

enum class ESoulBattleFormationKind : uint8
{
    FrontLine,
    MissileSupport,
    Strike,
    CommandReserve
};

enum class ESoulBattleMoraleState : uint8
{
    Steady,
    Pressured,
    Shaken,
    Broken,
    Routing,
    Rallied
};

enum class ESoulBattlePhase : uint8
{
    Deployment,
    Approach,
    Skirmish,
    Commit,
    Maneuver,
    Exploit,
    RoutRally,
    Resolution
};

struct FSoulBattleFormationState
{
    int32 GroupIndex = INDEX_NONE;
    int32 Side = 0;
    ESoulBattleFormationKind Kind = ESoulBattleFormationKind::FrontLine;
    FVector SpawnAnchor = FVector::ZeroVector;
    FVector TacticalAnchor = FVector::ZeroVector;
    int32 InitialBodies = 0;
    int32 PreviousAlive = 0;
    int32 MoralePermille = 1000;
    float ManualOverrideUntil = -1.0f;
    float RoutingSeconds = 0.0f;
    float RallyGraceUntil = -1.0f;
    bool bRouting = false;
    bool bRallied = false;
    bool bShattered = false;
};

struct FSoulBattleMoraleInput
{
    int32 PreviousAlive = 0;
    int32 CurrentAlive = 0;
    bool bCaptainLost = false;
    bool bFlanked = false;
    bool bLocalDisadvantage = false;
    bool bFriendlyRoutedNearby = false;
    bool bHeroSupport = false;
    bool bReinforcementsArrived = false;
};

struct FSoulBattleOrderContext
{
    ESoulBattleFormationKind Kind = ESoulBattleFormationKind::FrontLine;
    ESoulBattleMoraleState Morale = ESoulBattleMoraleState::Steady;
    ESoulBattlePhase Phase = ESoulBattlePhase::Deployment;
    float EnemyDistance = TNumericLimits<float>::Max();
    bool bFrontLineEngaged = false;
    bool bMeleeThreat = false;
    bool bFriendlyLineCollapsing = false;
};

class SOULREALTIMEBATTLE_API FSoulRealtimeTacticalRules
{
public:
    static ESoulBattlePhase DeterminePhase(
        float ElapsedSeconds,
        float ClosestEnemyDistance,
        int32 AlliedLossPermille,
        int32 EnemyLossPermille,
        int32 AlliedMoralePermille,
        int32 EnemyMoralePermille);

    static int32 UpdateMorale(
        int32 CurrentMoralePermille,
        const FSoulBattleMoraleInput& Input);

    static ESoulBattleMoraleState MoraleState(
        int32 MoralePermille,
        bool bRouting,
        bool bRallied);

    static ERBHostGroupOrder ChooseOrder(
        const FSoulBattleOrderContext& Context);

    static int32 ScoreReinforcementAnchor(
        float NearestEnemyDistance,
        float DistanceToFriendlyCenter,
        float ElevationAdvantage,
        bool bTraversable);

    static bool IsMoraleDefeated(
        int32 LivingFormations,
        int32 RoutingFormations,
        int32 ReserveBodies);

    static const TCHAR* FormationKindLabel(ESoulBattleFormationKind Kind);
    static const TCHAR* MoraleLabel(ESoulBattleMoraleState State);
    static const TCHAR* PhaseLabel(ESoulBattlePhase Phase);
};
