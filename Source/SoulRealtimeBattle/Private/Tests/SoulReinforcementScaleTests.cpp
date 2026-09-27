#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSmallFormationCapTest,
    "Soul.RealtimeBattle.SmallArmyHonorsFormationCap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSmallFormationCapTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.MaxActivePerSide = 35;
    FSoulRealtimeFormation F;
    F.FormationId = TEXT("line"); F.SideId = TEXT("P");
    F.StrategicCount = 20; F.MaxActiveRepresentations = 4;
    Battle.Formations.Add(F);
    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("small army honors formation limit"), Battle.Formations[0].ActiveCount, 4);
    TestEqual(TEXT("undeployed survivors remain in reserve"), Battle.Formations[0].ReserveCount, 16);
    TestFalse(TEXT("full formation is not advertised as ready"), FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("P")));
    TestEqual(TEXT("full formation cannot spawn twice"), FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P")).TotalBodies(), 0);
    FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("line"), 1);
    TestTrue(TEXT("casualty opens a receiving formation"), FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("P")));
    TestEqual(TEXT("one free slot admits exactly one body"), FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P")).TotalBodies(), 1);
    TestEqual(TEXT("wave preserves formation cap"), Battle.Formations[0].ActiveCount, 4);
    TestEqual(TEXT("casualty preserves strategic conservation"), Battle.Formations[0].StrategicCount, 19);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulLargePoolConservationTest,
    "Soul.RealtimeBattle.LargePoolsConserveAt30To70Active",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulLargePoolConservationTest::RunTest(const FString&)
{
    for (int32 Cap : {15, 25, 35})
    {
        FSoulRealtimeBattleState Battle;
        Battle.MaxActivePerSide = Cap;
        Battle.MaxWaveSize = 4;
        for (FName Side : TArray<FName>{TEXT("P"), TEXT("E")})
        {
            FSoulRealtimeFormation F;
            F.FormationId = Side; F.SideId = Side;
            F.StrategicCount = 1000; F.MaxActiveRepresentations = Cap;
            Battle.Formations.Add(F);
        }
        FSoulRealtimeBattleRules::InitializeDeployment(Battle);
        TestEqual(TEXT("two sides fill requested active cap"),
            FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")) + FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("E")), 2 * Cap);
        int32 Lost = 0;
        for (int32 Step = 0; Step < 1000; ++Step)
        {
            Lost += FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("E"), 1);
            const auto Wave = FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("E"));
            TestTrue(TEXT("wave remains bounded"), Wave.TotalBodies() <= 4);
            const int32 Active = FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("E"));
            const int32 Reserve = FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("E"));
            TestTrue(TEXT("side active limit holds through all waves"), Active <= Cap);
            TestEqual(TEXT("no body is lost or duplicated"), Lost + Active + Reserve, 1000);
        }
        TestEqual(TEXT("every hostile body accounted for"), Lost, 1000);
        TestFalse(TEXT("no stranded hostile reserve"), FSoulRealtimeBattleRules::HasLivingForce(Battle, TEXT("E")));
        TestEqual(TEXT("other side untouched"), FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P")) + FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("P")), 1000);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulMixedRoleReinforcementTest,
    "Soul.RealtimeBattle.HeroApexPriorityAndNormalizedCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulMixedRoleReinforcementTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle;
    Battle.MaxActivePerSide = 1;
    for (ESoulRealtimeFormationRole Role : {ESoulRealtimeFormationRole::Line,
        ESoulRealtimeFormationRole::Apex, ESoulRealtimeFormationRole::Hero})
    {
        FSoulRealtimeFormation F;
        F.FormationId = FName(*FString::Printf(TEXT("role_%d"), static_cast<int32>(Role)));
        F.SideId = TEXT("P"); F.Role = Role;
        F.StrategicCount = Role == ESoulRealtimeFormationRole::Hero ? 1 : 5;
        F.MaxActiveRepresentations = 1;
        Battle.Formations.Add(F);
    }
    FSoulRealtimeBattleRules::InitializeDeployment(Battle);
    TestEqual(TEXT("separate hero has initial priority"), Battle.Formations[2].ActiveCount, 1);
    TestEqual(TEXT("apex does not bypass the side cap"), Battle.Formations[1].ActiveCount, 0);
    TestFalse(TEXT("full side waits"), FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("P")));
    FSoulRealtimeBattleRules::ApplyCasualties(Battle, Battle.Formations[2].FormationId, 1);
    TestEqual(TEXT("apex fills the lost hero's slot"), FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P")).TotalBodies(), 1);
    TestEqual(TEXT("apex receives priority over line"), Battle.Formations[1].ActiveCount, 1);
    TestEqual(TEXT("dead hero cannot respawn without reserve"), Battle.Formations[2].StrategicCount, 0);

    // Deployment and reinforcement must normalize the same invalid cap to one.
    Battle.MaxActivePerSide = 0;
    Battle.MaxWaveSize = 0;
    FSoulRealtimeBattleRules::ApplyCasualties(Battle, Battle.Formations[1].FormationId, 1);
    TestEqual(TEXT("normalized cap does not strand surviving reserve"), FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P")).TotalBodies(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulWavePreviewConservationTest,
    "Soul.RealtimeBattle.WavePreviewConservesAt30To70Active",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulWavePreviewConservationTest::RunTest(const FString&)
{
    for (int32 Cap : {15, 25, 35})
    {
        FSoulRealtimeBattleState Battle;
        Battle.MaxActivePerSide = Cap;
        Battle.MaxWaveSize = 4;
        for (FName Side : TArray<FName>{TEXT("P"), TEXT("E")})
        {
            FSoulRealtimeFormation F;
            F.FormationId = Side; F.SideId = Side;
            F.StrategicCount = 1000; F.MaxActiveRepresentations = Cap;
            Battle.Formations.Add(F);
        }
        FSoulRealtimeBattleRules::InitializeDeployment(Battle);
        TestEqual(TEXT("full frontline previews no arrival"), FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("P")).TotalBodies(), 0);
        TestEqual(TEXT("unknown side previews no arrival"), FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("unknown")).TotalBodies(), 0);
        int32 Lost = 0;
        for (int32 Step = 0; Step < 1000; ++Step)
        {
            Lost += FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("E"), 1);
            const int32 Active = Battle.Formations[1].ActiveCount, Reserve = Battle.Formations[1].ReserveCount;
            const auto Preview = FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("E"));
            TestEqual(TEXT("repeated HUD reads agree"), FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("E")).TotalBodies(), Preview.TotalBodies());
            TestEqual(TEXT("preview cannot consume reserve"), Battle.Formations[1].ReserveCount, Reserve);
            TestEqual(TEXT("preview cannot spawn active bodies"), Battle.Formations[1].ActiveCount, Active);
            TestEqual(TEXT("preview cannot alter strategic survivors"), Battle.Formations[1].StrategicCount, 1000 - Lost);
            {
                // Production arena allocates on a candidate, then commits only
                // after physical spawn success. Discarding it must preserve retry.
                auto FailedCandidate = Battle;
                TestEqual(TEXT("discarded allocation agrees with preview"), FSoulRealtimeBattleRules::BuildAndApplyWave(FailedCandidate, TEXT("E")).TotalBodies(), Preview.TotalBodies());
            }
            const auto Applied = FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("E"));
            TestEqual(TEXT("actual arrival agrees with displayed count"), Applied.TotalBodies(), Preview.TotalBodies());
            TestEqual(TEXT("actual formation allocation agrees"), Applied.FormationCounts.FindRef(TEXT("E")), Preview.FormationCounts.FindRef(TEXT("E")));
            TestEqual(TEXT("only actual arrival debits reserve"), Battle.Formations[1].ReserveCount, Reserve - Applied.TotalBodies());
            TestEqual(TEXT("losses plus active plus reserve conserved"), Lost + Battle.Formations[1].ActiveCount + Battle.Formations[1].ReserveCount, 1000);
            TestTrue(TEXT("active side cap maintained"), Battle.Formations[1].ActiveCount <= Cap);
            TestEqual(TEXT("allied strategic pool untouched"), Battle.Formations[0].StrategicCount, 1000);
        }
        TestEqual(TEXT("all enemy bodies accounted for"), Lost, 1000);
        TestEqual(TEXT("exhausted reserves preview no ghost arrival"), FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("E")).TotalBodies(), 0);
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("P"), Cap);
        Battle.bReinforcementsEnabled = false;
        TestEqual(TEXT("disabled reinforcement previews no arrival"), FSoulRealtimeBattleRules::PreviewWave(Battle, TEXT("P")).TotalBodies(), 0);
    }
    return true;
}

#endif
