#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleTactics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulTacticalOrdersDifferTest,
    "Soul.RealtimeBattle.Tactics.OrdersAreDistinct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulTacticalOrdersDifferTest::RunTest(const FString&)
{
    FSoulBattleOrderContext Context;
    Context.Phase = ESoulBattlePhase::Commit;
    Context.EnemyDistance = 480.0f;
    Context.bFrontLineEngaged = true;

    Context.Kind = ESoulBattleFormationKind::FrontLine;
    TestEqual(TEXT("front line commits"), FSoulRealtimeTacticalRules::ChooseOrder(Context), ERBHostGroupOrder::Charge);

    Context.Kind = ESoulBattleFormationKind::MissileSupport;
    Context.bMeleeThreat = true;
    TestEqual(TEXT("threatened missiles withdraw"), FSoulRealtimeTacticalRules::ChooseOrder(Context), ERBHostGroupOrder::FallBack);

    Context.bRangedOperational=false;
    Context.EnemyDistance=1900.0f;
    TestEqual(TEXT("Empty bows advance instead of holding a deadlock"),
        FSoulRealtimeTacticalRules::ChooseOrder(Context),ERBHostGroupOrder::Advance);
    Context.EnemyDistance=300.0f;
    TestEqual(TEXT("Trapped empty bows commit to close combat"),
        FSoulRealtimeTacticalRules::ChooseOrder(Context),ERBHostGroupOrder::Charge);
    Context.bRangedOperational=true;
    Context.Kind = ESoulBattleFormationKind::CommandReserve;
    Context.bMeleeThreat = false;
    Context.bFriendlyLineCollapsing = false;
    TestEqual(TEXT("reserve remains held"), FSoulRealtimeTacticalRules::ChooseOrder(Context), ERBHostGroupOrder::Hold);

    Context.Kind = ESoulBattleFormationKind::Strike;
    Context.EnemyDistance = 1000.0f;
    TestEqual(TEXT("strike maneuvers before contact"), FSoulRealtimeTacticalRules::ChooseOrder(Context), ERBHostGroupOrder::Advance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulTacticalMoraleTest,
    "Soul.RealtimeBattle.Tactics.MoraleBreakAndRally",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulTacticalMoraleTest::RunTest(const FString&)
{
    FSoulBattleMoraleInput HeavyLoss;
    HeavyLoss.PreviousAlive = 8;
    HeavyLoss.CurrentAlive = 3;
    HeavyLoss.bCaptainLost = true;
    HeavyLoss.bFlanked = true;
    const int32 Broken = FSoulRealtimeTacticalRules::UpdateMorale(1000, HeavyLoss);
    TestTrue(TEXT("heavy losses break formation"), Broken < 260);
    TestEqual(TEXT("broken state is readable"),
        FSoulRealtimeTacticalRules::MoraleState(Broken, false, false),
        ESoulBattleMoraleState::Broken);

    FSoulBattleMoraleInput Rally;
    Rally.PreviousAlive = 3;
    Rally.CurrentAlive = 3;
    Rally.bHeroSupport = true;
    Rally.bReinforcementsArrived = true;
    const int32 Recovered = FSoulRealtimeTacticalRules::UpdateMorale(330, Rally);
    TestTrue(TEXT("hero and reinforcements can rally"), Recovered >= 360);
    TestEqual(TEXT("rallied state is explicit"),
        FSoulRealtimeTacticalRules::MoraleState(Recovered, false, true),
        ESoulBattleMoraleState::Rallied);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulTacticalSafeArrivalTest,
    "Soul.RealtimeBattle.Tactics.SafeReinforcementAnchor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulTacticalSafeArrivalTest::RunTest(const FString&)
{
    TestEqual(TEXT("enemy-dominated edge rejected"),
        FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(400, 900, 0, true),
        MIN_int32);
    const int32 Safe = FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(1800, 1000, 80, true);
    const int32 Remote = FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(1800, 2400, 0, true);
    TestTrue(TEXT("safe supporting edge preferred"), Safe > Remote);
    TestEqual(TEXT("blocked anchor rejected"),
        FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(3000, 400, 0, false),
        MIN_int32);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulTacticalPhaseAndResolutionTest,
    "Soul.RealtimeBattle.Tactics.PhasesAndMoraleResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulTacticalPhaseAndResolutionTest::RunTest(const FString&)
{
    TestEqual(TEXT("separated armies approach"),
        FSoulRealtimeTacticalRules::DeterminePhase(5, 2200, 0, 0, 1000, 1000),
        ESoulBattlePhase::Approach);
    TestEqual(TEXT("loss advantage produces exploit"),
        FSoulRealtimeTacticalRules::DeterminePhase(40, 500, 100, 420, 800, 500),
        ESoulBattlePhase::Exploit);
    TestTrue(TEXT("all living formations routing with no reserve resolves"),
        FSoulRealtimeTacticalRules::IsMoraleDefeated(3, 3, 0));
    TestTrue(TEXT("uncommitted reserve retreats after total active rout"),
        FSoulRealtimeTacticalRules::IsMoraleDefeated(3, 3, 4));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSustainedMoralePressureTest,
    "Soul.RealtimeBattle.Tactics.PressureAllowsReaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSustainedMoralePressureTest::RunTest(const FString&)
{
    FSoulBattleMoraleInput Pressure;
    Pressure.PreviousAlive = Pressure.CurrentAlive = 5;
    Pressure.bLocalDisadvantage = true;
    Pressure.bFlanked = true;
    int32 Morale = 1000;
    for (int32 Step = 0; Step < 25; ++Step) // Twenty seconds without casualties.
        Morale = FSoulRealtimeTacticalRules::UpdateMorale(Morale, Pressure);
    TestTrue(TEXT("Persistent pressure matters"), Morale < 720);
    TestTrue(TEXT("Healthy formation has time to respond before routing"), Morale >= 260);
    Pressure.bFlanked = Pressure.bLocalDisadvantage = false;
    const int32 Recovered = FSoulRealtimeTacticalRules::UpdateMorale(Morale, Pressure);
    TestTrue(TEXT("Escaping pressure allows recovery"), Recovered > Morale);
    return true;
}
