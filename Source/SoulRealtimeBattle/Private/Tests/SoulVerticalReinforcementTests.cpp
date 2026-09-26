#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    const FName VerticalPlayerSide(TEXT("Vertical.Player"));
    const FName VerticalEnemySide(TEXT("Vertical.Enemy"));
    const FName VerticalDeployedId(TEXT("Vertical.Enemy.Deployed"));
    const FName VerticalReserveId(TEXT("Vertical.Enemy.Reserve"));

    FSoulRealtimeBattleState MakeVerticalReinforcementState(int32 Cap, int32 Reserves)
    {
        FSoulRealtimeBattleState Battle;
        Battle.MaxActivePerSide = Cap;
        Battle.MaxWaveSize = FMath::Min(4, Cap);
        Battle.ReinforcementTriggerPermille = 700;

        FSoulRealtimeFormation Player;
        Player.FormationId = TEXT("Vertical.Player.Deployed");
        Player.SideId = VerticalPlayerSide;
        Player.UnitId = TEXT("Human.Knight");
        Player.StrategicCount = Player.ActiveCount = Cap;
        Player.MaxActiveRepresentations = Cap;
        Battle.Formations.Add(Player);

        FSoulRealtimeFormation Enemy;
        Enemy.FormationId = VerticalDeployedId;
        Enemy.SideId = VerticalEnemySide;
        Enemy.UnitId = TEXT("Dwarf.Warrior");
        Enemy.StrategicCount = Enemy.ActiveCount = Cap;
        Enemy.MaxActiveRepresentations = Cap;
        Battle.Formations.Add(Enemy);

        Enemy.FormationId = VerticalReserveId;
        Enemy.StrategicCount = Enemy.ReserveCount = Reserves;
        Enemy.ActiveCount = 0;
        Battle.Formations.Add(Enemy);
        return Battle;
    }

    int32 Living(const FSoulRealtimeBattleState& Battle, FName Side)
    {
        return FSoulRealtimeBattleRules::ActiveBodies(Battle, Side) +
            FSoulRealtimeBattleRules::ReserveBodies(Battle, Side);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulVerticalLastActiveDeathTest,
    "Soul.RealtimeBattle.Vertical.LastActiveDeathRetainsHostileReserves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulVerticalLastActiveDeathTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle = MakeVerticalReinforcementState(1, 3);
    TestEqual(TEXT("last physical hostile casualty applied"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, VerticalDeployedId, 1), 1);
    TestEqual(TEXT("no hostile representation remains"),
        FSoulRealtimeBattleRules::ActiveBodies(Battle, VerticalEnemySide), 0);
    TestTrue(TEXT("pending strategic hostiles prevent victory"),
        FSoulRealtimeBattleRules::HasLivingForce(Battle, VerticalEnemySide));
    TestTrue(TEXT("empty frontline requests reinforcement"),
        FSoulRealtimeBattleRules::ShouldReinforce(Battle, VerticalEnemySide));

    const FSoulReinforcementWave Wave = FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, VerticalEnemySide);
    TestEqual(TEXT("one body delivered at the one-body cap"), Wave.TotalBodies(), 1);
    TestEqual(TEXT("delivery debits actual reserve"),
        FSoulRealtimeBattleRules::ReserveBodies(Battle, VerticalEnemySide), 2);
    TestEqual(TEXT("delivery preserves all three survivors"), Living(Battle, VerticalEnemySide), 3);
    TestEqual(TEXT("other side remains untouched"), Living(Battle, VerticalPlayerSide), 1);
    TestEqual(TEXT("full cap prevents a second delivery"),
        FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, VerticalEnemySide).TotalBodies(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulVerticalReserveLedgerTest,
    "Soul.RealtimeBattle.Vertical.SequentialWavesPreserveLedgerAndCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulVerticalReserveLedgerTest::RunTest(const FString&)
{
    for (const int32 Cap : {1, 5, 10, 15})
    {
        const int32 InitialReserves = Cap * 2 + 3;
        const int32 InitialEnemy = Cap + InitialReserves;
        FSoulRealtimeBattleState Battle = MakeVerticalReinforcementState(Cap, InitialReserves);
        int32 Casualties = 0;
        int32 Delivered = 0;
        int32 Steps = 0;
        while (FSoulRealtimeBattleRules::HasLivingForce(Battle, VerticalEnemySide) && Steps++ < 128)
        {
            // Each step is a new confirmed physical death, never a repeated actor event.
            FName VictimFormation;
            for (const FSoulRealtimeFormation& Formation : Battle.Formations)
            {
                if (Formation.SideId == VerticalEnemySide && Formation.ActiveCount > 0)
                {
                    VictimFormation = Formation.FormationId;
                    break;
                }
            }
            if (!VictimFormation.IsNone())
            {
                const int32 Applied = FSoulRealtimeBattleRules::ApplyCasualties(Battle, VictimFormation, 1);
                TestEqual(TEXT("one death debits exactly one body"), Applied, 1);
                Casualties += Applied;
            }
            const int32 BeforeLiving = Living(Battle, VerticalEnemySide);
            const int32 BeforeReserve = FSoulRealtimeBattleRules::ReserveBodies(Battle, VerticalEnemySide);
            const bool bEmptyWithReserve = BeforeReserve > 0 &&
                FSoulRealtimeBattleRules::ActiveBodies(Battle, VerticalEnemySide) == 0;
            const FSoulReinforcementWave Wave =
                FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, VerticalEnemySide);
            Delivered += Wave.TotalBodies();
            TestEqual(TEXT("wave conserves surviving strategic force"), Living(Battle, VerticalEnemySide), BeforeLiving);
            TestEqual(TEXT("physical delivery and reserve debit agree"),
                BeforeReserve - FSoulRealtimeBattleRules::ReserveBodies(Battle, VerticalEnemySide), Wave.TotalBodies());
            TestTrue(TEXT("individual delivery bounded"), Wave.TotalBodies() <= FMath::Min(4, Cap));
            TestTrue(TEXT("simultaneous active cap held"),
                FSoulRealtimeBattleRules::ActiveBodies(Battle, VerticalEnemySide) <= Cap);
            TestEqual(TEXT("casualties plus survivors conserve original army"),
                Casualties + Living(Battle, VerticalEnemySide), InitialEnemy);
            TestEqual(TEXT("enemy losses never debit player army"), Living(Battle, VerticalPlayerSide), Cap);
            if (bEmptyWithReserve && !TestTrue(TEXT("no reserves stranded behind an empty frontline"), Wave.TotalBodies() > 0))
            {
                break;
            }
        }
        TestFalse(FString::Printf(TEXT("cap %d eventually exhausts hostile strategic pool"), Cap),
            FSoulRealtimeBattleRules::HasLivingForce(Battle, VerticalEnemySide));
        TestEqual(TEXT("every reserve was delivered exactly once"), Delivered, InitialReserves);
        TestEqual(TEXT("all dead bodies accounted for"), Casualties, InitialEnemy);
        TestEqual(TEXT("no final reserve can produce a ghost wave"),
            FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, VerticalEnemySide).TotalBodies(), 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulVerticalCasualtyBoundaryTest,
    "Soul.RealtimeBattle.Vertical.CasualtiesCannotConsumeUnspawnedReserve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulVerticalCasualtyBoundaryTest::RunTest(const FString&)
{
    FSoulRealtimeBattleState Battle = MakeVerticalReinforcementState(1, 5);
    TestEqual(TEXT("unknown formation cannot remove anyone"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, TEXT("Unrelated.Formation"), 1), 0);
    TestEqual(TEXT("invalid negative casualty rejected"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, VerticalDeployedId, -1), 0);
    TestEqual(TEXT("overreported batch bounded by physical active bodies"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, VerticalDeployedId, 99), 1);
    TestEqual(TEXT("repeat against an empty formation removes no reserve"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, VerticalDeployedId, 1), 0);
    TestEqual(TEXT("unspawned reserve does not take physical casualties"),
        FSoulRealtimeBattleRules::ApplyCasualties(Battle, VerticalReserveId, 1), 0);
    TestEqual(TEXT("reserve ledger preserved"),
        FSoulRealtimeBattleRules::ReserveBodies(Battle, VerticalEnemySide), 5);
    // ApplyCasualties is intentionally not event-idempotent after another body spawns.
    // The runtime's DefeatedRepresentations set owns actor-event deduplication.
    return true;
}

#endif
