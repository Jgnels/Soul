#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    FSoulRealtimeFormation MakeFormation(
        const TCHAR* Id,
        const TCHAR* Side,
        int32 Count,
        ESoulRealtimeFormationRole Role,
        int32 Power = 100,
        int32 Priority = 0)
    {
        FSoulRealtimeFormation F;
        F.FormationId = Id;
        F.SideId = Side;
        F.UnitId = Id;
        F.StrategicCount = Count;
        F.Role = Role;
        F.PowerPerBody = Power;
        F.ReinforcementPriority = Priority;
        return F;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeAllAtOnceTest,
    "Soul.RealtimeBattle.SmallArmyDeploysAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeAllAtOnceTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.MaxActivePerSide = 32;
    Battle.Formations = {
        MakeFormation(TEXT("line"), TEXT("P"), 8, ESoulRealtimeFormationRole::Line),
        MakeFormation(TEXT("archer"), TEXT("P"), 6, ESoulRealtimeFormationRole::Ranged),
        MakeFormation(TEXT("hero"), TEXT("P"), 1, ESoulRealtimeFormationRole::Hero, 600),
        MakeFormation(TEXT("apex"), TEXT("P"), 1, ESoulRealtimeFormationRole::Apex, 900)
    };

    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("all sixteen deploy"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")), 16);
    TestEqual(TEXT("no reserves"), FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("P")), 0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeBoundedDeploymentTest,
    "Soul.RealtimeBattle.LargeArmyUsesReserves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeBoundedDeploymentTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.MaxActivePerSide = 32;
    Battle.Formations = {
        MakeFormation(TEXT("swords"), TEXT("P"), 18, ESoulRealtimeFormationRole::Line),
        MakeFormation(TEXT("spears"), TEXT("P"), 16, ESoulRealtimeFormationRole::Line),
        MakeFormation(TEXT("heavy"), TEXT("P"), 10, ESoulRealtimeFormationRole::Shock),
        MakeFormation(TEXT("archers"), TEXT("P"), 14, ESoulRealtimeFormationRole::Ranged),
        MakeFormation(TEXT("support"), TEXT("P"), 5, ESoulRealtimeFormationRole::Support, 160),
        MakeFormation(TEXT("hero"), TEXT("P"), 1, ESoulRealtimeFormationRole::Hero, 700),
        MakeFormation(TEXT("griffon"), TEXT("P"), 1, ESoulRealtimeFormationRole::Apex, 900)
    };

    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("active cap honored"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")), 32);
    TestEqual(TEXT("remaining force becomes reserve"), FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("P")), 33);
    for (const FSoulRealtimeFormation& F : Battle.Formations)
    {
        TestTrue(
            *FString::Printf(TEXT("%s has a battlefield representation"), *F.FormationId.ToString()),
            F.ActiveCount > 0);
        TestTrue(
            *FString::Printf(TEXT("%s respects RB Combat group cap"), *F.FormationId.ToString()),
            F.ActiveCount <= FRBCombatGroup::MaximumMembers);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeReinforcementWaveTest,
    "Soul.RealtimeBattle.ReinforcementsArriveInBoundedWave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeReinforcementWaveTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.MaxActivePerSide = 32;
    Battle.ReinforcementTriggerPermille = 700;
    Battle.MaxWaveSize = 8;
    Battle.Formations = {
        MakeFormation(TEXT("line"), TEXT("P"), 40, ESoulRealtimeFormationRole::Line),
        MakeFormation(TEXT("archer"), TEXT("P"), 16, ESoulRealtimeFormationRole::Ranged)
    };
    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("initial active"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")), 32);
    const int32 Lost = FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("line"), 12);
    TestEqual(TEXT("twelve casualties applied"), Lost, 12);
    TestTrue(TEXT("wave becomes eligible"), FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("P")));

    const FSoulReinforcementWave Wave =
        FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P"));
    TestEqual(TEXT("wave is bounded"), Wave.TotalBodies(), 8);
    TestEqual(TEXT("active returns to twenty eight"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")), 28);
    TestTrue(TEXT("reserve remains for later waves"), FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("P")) > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeFantasyPowerTest,
    "Soul.RealtimeBattle.FantasyPowerDecouplesFromHeadcount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeFantasyPowerTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.Formations = {
        MakeFormation(TEXT("militia"), TEXT("P"), 8, ESoulRealtimeFormationRole::Line, 100),
        MakeFormation(TEXT("dragon"), TEXT("P"), 1, ESoulRealtimeFormationRole::Apex, 1000)
    };
    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("nine physical bodies"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")), 9);
    TestEqual(TEXT("dragon carries disproportionate power"), FSoulRealtimeBattleRules::ActivePower(Battle, TEXT("P")), 1800);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeVolcanicMagicTest,
    "Soul.RealtimeBattle.VolcanicTerrainChangesMagic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeVolcanicMagicTest::RunTest(const FString&)
{
    const FSoulTerrainMagicProfile Profile =
        FSoulRealtimeBattleRules::MakeVolcanicMagicProfile();

    TestEqual(TEXT("fire empowered"), Profile.MultiplierFor(TEXT("Magic.School.Fire")), 1200);
    TestEqual(TEXT("ice weakened"), Profile.MultiplierFor(TEXT("Magic.School.Ice")), 800);
    TestEqual(TEXT("water weakened"), Profile.MultiplierFor(TEXT("Magic.School.Water")), 850);
    TestEqual(TEXT("electric unchanged"), Profile.MultiplierFor(TEXT("Magic.School.Electric")), 1000);
    return true;
}

#endif
