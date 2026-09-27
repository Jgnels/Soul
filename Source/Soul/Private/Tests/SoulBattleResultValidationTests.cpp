#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulResultTelemetryValidationTest,
    "Soul.Integration.Vertical.ResultTelemetryRejectedBeforeMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulResultTelemetryValidationTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* Campaign = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    auto* Bridge = NewObject<USoulCampaignBattleBridge>(GI);
    Campaign->InitializeScenario();
    Campaign->PlayerRegion = TEXT("river_ford");
    if (!TestTrue(TEXT("campaign encounter starts"), Campaign->BeginBattle(TEXT("orc_watch")))) return false;
    const auto Encounter = Campaign->PendingBattle;
    if (!TestTrue(TEXT("bridge accepts same descriptor"), Bridge->BeginEncounter(Encounter))) return false;
    FSoulCampaignBattleResult Valid;
    Valid.EncounterId = Encounter.EncounterId; Valid.TargetRegion = Encounter.TargetRegion;
    Valid.bPlayerWon = true; Valid.PlayerSurvivors = 7;
    Valid.PlayerManaRemaining = Encounter.PlayerMana;
    const int32 Gold = Campaign->Economy.Resources.FindRef(TEXT("gold"));
    int32 Notifications = 0;
    Bridge->OnBattleResolved.AddLambda([&Notifications](const FSoulCampaignBattleResult&) { ++Notifications; });
    for (int32 Field = 0; Field < 5; ++Field)
    {
        auto Broken = Valid;
        if (Field == 0) Broken.PlayerReinforcements = -1;
        if (Field == 1) Broken.EnemyReinforcements = -1;
        if (Field == 2) Broken.MagicCasts = -1;
        if (Field == 3) Broken.PlayerManaRemaining = -1;
        if (Field == 4) Broken.PlayerManaRemaining = Encounter.PlayerMana + 1;
        TestFalse(TEXT("transport rejects negative telemetry"), Bridge->ResolveEncounter(Broken));
        TestFalse(TEXT("campaign rejects direct malformed result"), Campaign->ApplyBattleResult(Broken));
        TestEqual(TEXT("malformed result cannot pay rewards"), Campaign->Economy.Resources.FindRef(TEXT("gold")), Gold);
        TestEqual(TEXT("malformed result cannot alter army"), Campaign->PlayerArmy.FindRef(Campaign->PlayerUnitId), Encounter.PlayerStrategicCount);
        TestTrue(TEXT("campaign remains pending"), Campaign->HasPendingBattle());
        TestNotNull(TEXT("bridge remains pending"), Bridge->GetPendingEncounter());
    }
    TestEqual(TEXT("malformed results never notify consequence writer"), Notifications, 0);
    TestTrue(TEXT("corrected bridge result still resolves"), Bridge->ResolveEncounter(Valid));
    TestEqual(TEXT("exactly one valid result notification"), Notifications, 1);
    TestTrue(TEXT("corrected campaign result still applies"), Campaign->ApplyBattleResult(Valid));
    Bridge->OnBattleResolved.Clear();
    return true;
}

#endif
