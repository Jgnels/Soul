#include "SoulRealtimeBattleTactics.h"

ESoulBattlePhase FSoulRealtimeTacticalRules::DeterminePhase(
    float ElapsedSeconds,
    float ClosestEnemyDistance,
    int32 AlliedLossPermille,
    int32 EnemyLossPermille,
    int32 AlliedMoralePermille,
    int32 EnemyMoralePermille)
{
    if (AlliedMoralePermille < 260 || EnemyMoralePermille < 260)
        return ESoulBattlePhase::RoutRally;
    if (AlliedLossPermille >= 850 || EnemyLossPermille >= 850)
        return ESoulBattlePhase::Resolution;
    if (ElapsedSeconds < 2.0f)
        return ESoulBattlePhase::Deployment;
    if (ClosestEnemyDistance > 1500.0f)
        return ESoulBattlePhase::Approach;
    if (ClosestEnemyDistance > 850.0f)
        return ESoulBattlePhase::Skirmish;
    if (AlliedLossPermille < 180 && EnemyLossPermille < 180)
        return ESoulBattlePhase::Commit;
    if (FMath::Abs(AlliedLossPermille - EnemyLossPermille) >= 220)
        return ESoulBattlePhase::Exploit;
    return ESoulBattlePhase::Maneuver;
}

int32 FSoulRealtimeTacticalRules::UpdateMorale(
    int32 CurrentMoralePermille,
    const FSoulBattleMoraleInput& Input)
{
    const int32 NewLosses =
        FMath::Max(0, Input.PreviousAlive - Input.CurrentAlive);
    int32 Delta = -NewLosses * 115;
    Delta -= Input.bCaptainLost ? 180 : 0;
    // These are sustained pressures sampled every 0.8 seconds, not new
    // casualty events. Keep their drain gradual enough to react or withdraw.
    Delta -= Input.bFlanked ? 12 : 0;
    Delta -= Input.bLocalDisadvantage ? 10 : 0;
    Delta -= Input.bFriendlyRoutedNearby ? 15 : 0;
    Delta += Input.bHeroSupport ? 24 : 0;
    Delta += Input.bReinforcementsArrived ? 100 : 0;
    if (NewLosses == 0 && !Input.bFlanked &&
        !Input.bLocalDisadvantage && !Input.bFriendlyRoutedNearby)
        Delta += 8;
    return FMath::Clamp(CurrentMoralePermille + Delta, 0, 1000);
}

ESoulBattleMoraleState FSoulRealtimeTacticalRules::MoraleState(
    int32 MoralePermille,
    bool bRouting,
    bool bRallied)
{
    if (bRallied && MoralePermille >= 360)
        return ESoulBattleMoraleState::Rallied;
    if (bRouting)
        return ESoulBattleMoraleState::Routing;
    if (MoralePermille < 260)
        return ESoulBattleMoraleState::Broken;
    if (MoralePermille < 480)
        return ESoulBattleMoraleState::Shaken;
    if (MoralePermille < 720)
        return ESoulBattleMoraleState::Pressured;
    return ESoulBattleMoraleState::Steady;
}

ERBHostGroupOrder FSoulRealtimeTacticalRules::ChooseOrder(
    const FSoulBattleOrderContext& Context)
{
    if (Context.Morale == ESoulBattleMoraleState::Broken ||
        Context.Morale == ESoulBattleMoraleState::Routing)
        return ERBHostGroupOrder::FallBack;

    switch (Context.Kind)
    {
        case ESoulBattleFormationKind::MissileSupport:
            if (!Context.bRangedOperational)
                return Context.EnemyDistance>450 ? ERBHostGroupOrder::Advance : ERBHostGroupOrder::Charge;
            if (Context.bMeleeThreat || Context.EnemyDistance < 520.0f)
                return ERBHostGroupOrder::FallBack;
            return Context.EnemyDistance > 2200.0f
                ? ERBHostGroupOrder::Advance
                : ERBHostGroupOrder::Hold;

        case ESoulBattleFormationKind::Strike:
            if (!Context.bFrontLineEngaged &&
                Context.Phase != ESoulBattlePhase::Exploit)
                return ERBHostGroupOrder::Advance;
            return Context.EnemyDistance > 700.0f
                ? ERBHostGroupOrder::Advance
                : ERBHostGroupOrder::Charge;

        case ESoulBattleFormationKind::CommandReserve:
            if (Context.bFriendlyLineCollapsing ||
                Context.Phase == ESoulBattlePhase::Exploit)
                return Context.EnemyDistance > 650.0f
                    ? ERBHostGroupOrder::Advance
                    : ERBHostGroupOrder::Charge;
            return ERBHostGroupOrder::Hold;

        case ESoulBattleFormationKind::FrontLine:
        default:
            if (Context.EnemyDistance > 700.0f)
                return ERBHostGroupOrder::Advance;
            return Context.Phase == ESoulBattlePhase::Commit ||
                   Context.Phase == ESoulBattlePhase::Maneuver ||
                   Context.Phase == ESoulBattlePhase::Exploit
                ? ERBHostGroupOrder::Charge
                : ERBHostGroupOrder::Hold;
    }
}

bool FSoulRealtimeTacticalRules::IsFrontLineCollapsing(
    int32 LivingFormations, int32 RoutingFormations, int32 LowestMoralePermille)
{
    return LivingFormations == 0 || RoutingFormations > 0 || LowestMoralePermille < 480;
}

FVector FSoulRealtimeTacticalRules::AdvanceAnchor(const FSoulBattleOrderContext& Context,
    const FVector& CurrentAnchor, const FVector& EnemyLocation, const FVector& Facing)
{
    if (Context.Kind == ESoulBattleFormationKind::MissileSupport)
        return EnemyLocation - Facing * (Context.bRangedOperational ? 1700.0f : 240.0f);
    if (Context.Kind == ESoulBattleFormationKind::FrontLine ||
        Context.Kind == ESoulBattleFormationKind::CommandReserve ||
        (Context.Kind == ESoulBattleFormationKind::Strike && Context.bFrontLineEngaged))
        return EnemyLocation - Facing * 420.0f;
    return CurrentAnchor;
}

int32 FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(
    float NearestEnemyDistance,
    float DistanceToFriendlyCenter,
    float ElevationAdvantage,
    bool bTraversable)
{
    if (!bTraversable || NearestEnemyDistance < 700.0f)
        return MIN_int32;
    const float Safety = FMath::Min(NearestEnemyDistance, 4000.0f);
    const float Support = FMath::Max(0.0f, 2600.0f - DistanceToFriendlyCenter);
    const float Height = FMath::Clamp(ElevationAdvantage, -300.0f, 300.0f);
    return FMath::RoundToInt(Safety * 2.0f + Support + Height);
}

bool FSoulRealtimeTacticalRules::IsMoraleDefeated(
    int32 LivingFormations,
    int32 RoutingFormations,
    int32 ReserveBodies)
{
    // Once every deployed formation breaks, uncommitted reserves retreat with
    // the army. They remain strategic survivors; feeding them into a collapsed
    // battlefield would create an endless sequence of tiny reinforcement waves.
    (void)ReserveBodies;
    return LivingFormations > 0 &&
        RoutingFormations >= LivingFormations;
}

const TCHAR* FSoulRealtimeTacticalRules::FormationKindLabel(
    ESoulBattleFormationKind Kind)
{
    switch (Kind)
    {
        case ESoulBattleFormationKind::FrontLine: return TEXT("FRONT");
        case ESoulBattleFormationKind::MissileSupport: return TEXT("MISSILE");
        case ESoulBattleFormationKind::Strike: return TEXT("STRIKE");
        case ESoulBattleFormationKind::CommandReserve: return TEXT("RESERVE");
        default: return TEXT("FORMATION");
    }
}

const TCHAR* FSoulRealtimeTacticalRules::MoraleLabel(
    ESoulBattleMoraleState State)
{
    switch (State)
    {
        case ESoulBattleMoraleState::Steady: return TEXT("STEADY");
        case ESoulBattleMoraleState::Pressured: return TEXT("PRESSURED");
        case ESoulBattleMoraleState::Shaken: return TEXT("SHAKEN");
        case ESoulBattleMoraleState::Broken: return TEXT("BROKEN");
        case ESoulBattleMoraleState::Routing: return TEXT("ROUTING");
        case ESoulBattleMoraleState::Rallied: return TEXT("RALLIED");
        default: return TEXT("UNKNOWN");
    }
}

const TCHAR* FSoulRealtimeTacticalRules::PhaseLabel(
    ESoulBattlePhase Phase)
{
    switch (Phase)
    {
        case ESoulBattlePhase::Deployment: return TEXT("DEPLOYMENT");
        case ESoulBattlePhase::Approach: return TEXT("APPROACH");
        case ESoulBattlePhase::Skirmish: return TEXT("SKIRMISH");
        case ESoulBattlePhase::Commit: return TEXT("COMMIT");
        case ESoulBattlePhase::Maneuver: return TEXT("MANEUVER");
        case ESoulBattlePhase::Exploit: return TEXT("EXPLOIT");
        case ESoulBattlePhase::RoutRally: return TEXT("ROUT / RALLY");
        case ESoulBattlePhase::Resolution: return TEXT("RESOLUTION");
        default: return TEXT("BATTLE");
    }
}


FVector FSoulRealtimeTacticalRules::StrikeWaypoint(int32 Side,const FVector& Origin,
    const FVector& Center,const FVector& Target,float FlankSign,int32& Stage)
{
    const float Direction=Side==0 ? 1.0f : -1.0f;
    const float Lane=Origin.Y+(FlankSign<0 ? -1150.0f : 1150.0f);
    const FVector Staging(Origin.X-Direction*1000.0f,Lane,Center.Z);
    const FVector Rear(FMath::Clamp(Target.X+Direction*250.0f,Origin.X-1900.0,Origin.X+1900.0),Lane,Center.Z);
    if(Stage==0 && FVector::DistSquared2D(Center,Staging)<FMath::Square(240.0f)) Stage=1;
    if(Stage==1 && FVector::DistSquared2D(Center,Rear)<FMath::Square(280.0f)) Stage=2;
    return Stage==0 ? Staging : Stage==1 ? Rear : Target;
}
