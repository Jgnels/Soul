#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulCampaignBattleBridge.h"
#include "SoulSettlementStateSubsystem.h"
#include "RBSaveSubsystem.h"
#include "RBSaveCore.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    USoulFounderPlaytestStateSubsystem* Campaign()
    {
        auto* GI = NewObject<UGameInstance>(GetTransientPackage());
        auto* State = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
        State->InitializeScenario();
        return State;
    }
    FSoulCampaignBattleResult Victory(const FSoulCampaignBattleDescriptor& D, int32 Survivors)
    {
        FSoulCampaignBattleResult R;
        R.EncounterId=D.EncounterId;R.TargetRegion=D.TargetRegion;R.bPlayerWon=true;
        R.PlayerSurvivors=Survivors;R.EnemySurvivors=0;return R;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalEncounterTest,
    "Soul.Integration.Vertical.EncounterContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalEncounterTest::RunTest(const FString&)
{
    auto* S=Campaign();
    TestTrue(TEXT("accepted data loaded"),S->bInitialized);
    TestEqual(TEXT("accepted founder graph"),S->World.Regions.Num(),9);
    TestEqual(TEXT("accepted shrine geography"),S->World.Regions[TEXT("ancient_shrine")].Landform,FName(TEXT("shrine_terrace")));
    TestTrue(TEXT("capital to crossroads"),S->MovePlayerTo(TEXT("crossroads")));
    TestTrue(TEXT("crossroads to ford"),S->MovePlayerTo(TEXT("river_ford")));
    const int32 AP=S->Economy.ActionPoints;
    TestFalse(TEXT("hostile movement cannot preoccupy region"),S->MovePlayerTo(TEXT("orc_watch")));
    TestEqual(TEXT("hostile move spends no AP"),S->Economy.ActionPoints,AP);
    TestEqual(TEXT("army remains in origin"),S->PlayerRegion,FName(TEXT("river_ford")));
    TestEqual(TEXT("hostile region still dwarf owned"),S->World.Regions[TEXT("orc_watch")].OwnerFactionId,S->EnemyFaction);
    FSoulCampaignBattleDescriptor D;FString Error;
    TestTrue(TEXT("adjacent descriptor built"),S->BuildBattleDescriptor(TEXT("orc_watch"),D,Error));
    TestEqual(TEXT("real Dragon map"),D.MapPackage,FName(TEXT("/Game/Dragon_graveyard/Level/L_showcase_level")));
    TestEqual(TEXT("real player family"),D.PlayerUnitId,FName(TEXT("human_knight")));
    TestEqual(TEXT("real enemy family"),D.EnemyUnitId,FName(TEXT("dwarf_warrior")));
    TestEqual(TEXT("player strategic transfer"),D.PlayerStrategicCount,S->PlayerArmy[S->PlayerUnitId]);
    TestEqual(TEXT("enemy strategic transfer"),D.EnemyStrategicCount,S->EnemyArmies[TEXT("orc_watch")]);
    TestEqual(TEXT("bounded active force"),D.ActiveCapPerSide,5);
    TestTrue(TEXT("commit encounter"),S->BeginBattle(TEXT("orc_watch")));
    TestEqual(TEXT("exactly one commitment action"),S->Economy.ActionPoints,AP-1);
    TestFalse(TEXT("cannot commit twice"),S->BeginBattle(TEXT("orc_watch")));
    TestFalse(TEXT("cannot move during pending battle"),S->MovePlayerTo(TEXT("crossroads")));
    FRBSaveDomainState Save;
    TestFalse(TEXT("cannot save unresolved battle as settled state"),S->CaptureRBSaveDomain_Implementation(Save,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalVictoryTargetsTest,
    "Soul.Integration.Vertical.VictoryCorrectTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalVictoryTargetsTest::RunTest(const FString&)
{
    for(FName Target:TArray<FName>{TEXT("orc_watch"),TEXT("orc_camp")})
    {
        auto* S=Campaign();
        S->PlayerRegion=Target==TEXT("orc_watch")?TEXT("river_ford"):TEXT("north_pass");
        TestTrue(TEXT("commit selected region"),S->BeginBattle(Target));
        const auto D=S->PendingBattle;const int32 Gold=S->Economy.Resources.FindRef(TEXT("gold"));
        auto R=Victory(D,7);
        auto Wrong=R;Wrong.TargetRegion=TEXT("human_capital");
        TestFalse(TEXT("wrong result target rejected"),S->ApplyBattleResult(Wrong));
        TestTrue(TEXT("valid battle result applied"),S->ApplyBattleResult(R));
        TestEqual(TEXT("actual target captured"),S->World.Regions[Target].OwnerFactionId,S->PlayerFaction);
        TestEqual(TEXT("move only after victory"),S->PlayerRegion,Target);
        TestEqual(TEXT("actual survivors replace strategic army"),S->PlayerArmy[S->PlayerUnitId],7);
        TestEqual(TEXT("defeated enemy pool depleted"),S->EnemyArmies[Target],0);
        const FName Other=Target==TEXT("orc_watch")?TEXT("orc_camp"):TEXT("orc_watch");
        TestEqual(TEXT("unrelated enemy territory unchanged"),S->World.Regions[Other].OwnerFactionId,S->EnemyFaction);
        TestEqual(TEXT("gold reward once"),S->Economy.Resources.FindRef(TEXT("gold")),Gold+600);
        TestFalse(TEXT("duplicate result rejected"),S->ApplyBattleResult(R));
        TestEqual(TEXT("duplicate cannot grant reward"),S->Economy.Resources.FindRef(TEXT("gold")),Gold+600);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalDefeatTest,
    "Soul.Integration.Vertical.DefeatConsequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalDefeatTest::RunTest(const FString&)
{
    auto* S=Campaign();S->PlayerRegion=TEXT("river_ford");
    TestTrue(TEXT("commit encounter"),S->BeginBattle(TEXT("orc_watch")));
    auto R=Victory(S->PendingBattle,1);R.bPlayerWon=false;R.PlayerSurvivors=0;R.EnemySurvivors=4;
    TestTrue(TEXT("defeat applied"),S->ApplyBattleResult(R));
    TestEqual(TEXT("defeat returns to source"),S->PlayerRegion,FName(TEXT("river_ford")));
    TestEqual(TEXT("defeat never captures destination"),S->World.Regions[TEXT("orc_watch")].OwnerFactionId,S->EnemyFaction);
    TestEqual(TEXT("dead player army persists"),S->PlayerArmy[S->PlayerUnitId],0);
    TestEqual(TEXT("surviving hostile strategic state persists"),S->EnemyArmies[TEXT("orc_watch")],4);
    TestEqual(TEXT("no victory XP"),S->Hero.Experience,0);
    TestFalse(TEXT("empty army cannot reenter battle"),S->BeginBattle(TEXT("orc_watch")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalBridgeTest,
    "Soul.Integration.Vertical.BridgeRejectsPrematureVictory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalBridgeTest::RunTest(const FString&)
{
    auto* S=Campaign();S->PlayerRegion=TEXT("river_ford");
    FSoulCampaignBattleDescriptor D;FString Error;S->BuildBattleDescriptor(TEXT("orc_watch"),D,Error);
    auto* GI=NewObject<UGameInstance>(GetTransientPackage());
    auto* Bridge=NewObject<USoulCampaignBattleBridge>(GI);
    int32 Calls=0;Bridge->OnBattleResolved.AddLambda([&Calls](const FSoulCampaignBattleResult&){++Calls;});
    TestTrue(TEXT("descriptor accepted"),Bridge->BeginEncounter(D));
    TestFalse(TEXT("second active descriptor rejected"),Bridge->BeginEncounter(D));
    auto R=Victory(D,7);R.EnemySurvivors=1;
    TestFalse(TEXT("reserve survivor prevents premature victory"),Bridge->ResolveEncounter(R));
    R.EnemySurvivors=0;R.PlayerSurvivors=D.PlayerStrategicCount+1;
    TestFalse(TEXT("fabricated survivors rejected"),Bridge->ResolveEncounter(R));
    R.PlayerSurvivors=7;R.EncounterId=TEXT("wrong");
    TestFalse(TEXT("foreign encounter rejected"),Bridge->ResolveEncounter(R));
    R.EncounterId=D.EncounterId;
    TestTrue(TEXT("real exhausted-enemy result resolves"),Bridge->ResolveEncounter(R));
    TestEqual(TEXT("one campaign notification"),Calls,1);
    TestFalse(TEXT("resolved bridge cannot reapply"),Bridge->ResolveEncounter(R));
    TestNull(TEXT("pending cleared"),Bridge->GetPendingEncounter());
    Bridge->OnBattleResolved.Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalSaveTest,
    "Soul.Integration.Vertical.RBSaveDomainRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalSaveTest::RunTest(const FString&)
{
    for(bool Won:{true,false})
    {
        auto* S=Campaign();S->PlayerRegion=TEXT("river_ford");S->BeginBattle(TEXT("orc_watch"));
        auto R=Victory(S->PendingBattle,7);R.bPlayerWon=Won;R.PlayerSurvivors=Won?7:0;R.EnemySurvivors=Won?0:4;
        R.PlayerReinforcements=2;R.MagicCasts=1;S->ApplyBattleResult(R);
        FRBSaveDomainState Saved;FString Error;
        TestTrue(TEXT("capture RB Save domain"),S->CaptureRBSaveDomain_Implementation(Saved,Error));
        auto* Loaded=Campaign();
        TestTrue(TEXT("restore RB Save domain"),Loaded->RestoreRBSaveDomain_Implementation(Saved,Error));
        TestEqual(TEXT("location restored"),Loaded->PlayerRegion,S->PlayerRegion);
        TestEqual(TEXT("ownership restored"),Loaded->World.Regions[TEXT("orc_watch")].OwnerFactionId,S->World.Regions[TEXT("orc_watch")].OwnerFactionId);
        TestEqual(TEXT("player survivors restored"),Loaded->PlayerArmy[Loaded->PlayerUnitId],R.PlayerSurvivors);
        TestEqual(TEXT("enemy survivors restored"),Loaded->EnemyArmies[TEXT("orc_watch")],R.EnemySurvivors);
        TestEqual(TEXT("XP restored"),Loaded->Hero.Experience,S->Hero.Experience);
        TestTrue(TEXT("idempotence set persisted"),Loaded->ResolvedEncounters.Contains(R.EncounterId));
        TestEqual(TEXT("result identity persisted"),Loaded->LastBattleResult.EncounterId,R.EncounterId);
        TestEqual(TEXT("reinforcement evidence persisted"),Loaded->LastBattleResult.PlayerReinforcements,2);
        const auto Before=Loaded->PlayerRegion;
        auto Broken=Saved;Broken.Fields[0].StringValue=TEXT("{\"player_region\":\"invalid\"}");
        TestFalse(TEXT("malformed snapshot rejected"),Loaded->RestoreRBSaveDomain_Implementation(Broken,Error));
        TestEqual(TEXT("invalid restore leaves current state intact"),Loaded->PlayerRegion,Before);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalSupportedMatchupTest,
    "Soul.Integration.Vertical.SupportedMatchupContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalSupportedMatchupTest::RunTest(const FString&)
{
    for (FName Target : TArray<FName>{TEXT("orc_watch"), TEXT("orc_camp")})
    {
        auto* S = Campaign();
        S->PlayerRegion = Target == TEXT("orc_watch") ? TEXT("river_ford") : TEXT("north_pass");
        FSoulCampaignBattleDescriptor Valid;
        FString Error;
        TestTrue(TEXT("supported matchup builds for either target"),
            S->BuildBattleDescriptor(Target, Valid, Error));

        auto* GI = NewObject<UGameInstance>(GetTransientPackage());
        auto* Bridge = NewObject<USoulCampaignBattleBridge>(GI);
        auto Reject = [this, Bridge](const TCHAR* Label, const FSoulCampaignBattleDescriptor& D)
        {
            TestFalse(Label, D.IsValid());
            TestFalse(TEXT("unsupported matchup cannot enter battle bridge"), Bridge->BeginEncounter(D));
            TestNull(TEXT("rejected descriptor leaves bridge idle"), Bridge->GetPendingEncounter());
        };

        auto Invalid = Valid;
        Invalid.PlayerFaction = TEXT("unknown_faction");
        Reject(TEXT("unknown player faction rejected"), Invalid);
        Invalid = Valid;
        Invalid.EnemyFaction = TEXT("orcs");
        Reject(TEXT("Orcs cannot silently use Dwarf assets"), Invalid);
        Invalid = Valid;
        Invalid.PlayerUnitId = TEXT("unknown_unit");
        Reject(TEXT("unknown player unit rejected"), Invalid);
        Invalid = Valid;
        Invalid.EnemyUnitId = TEXT("orc_warrior");
        Reject(TEXT("unknown enemy unit rejected"), Invalid);
        Invalid = Valid;
        Swap(Invalid.PlayerFaction, Invalid.EnemyFaction);
        Swap(Invalid.PlayerUnitId, Invalid.EnemyUnitId);
        Reject(TEXT("unsupported reversed matchup rejected"), Invalid);

        const int32 AP = S->Economy.ActionPoints;
        S->EnemyUnitId = TEXT("unknown_unit");
        TestFalse(TEXT("campaign cannot commit unsupported descriptor"), S->BeginBattle(Target));
        TestEqual(TEXT("unsupported matchup spends no campaign AP"), S->Economy.ActionPoints, AP);
        TestFalse(TEXT("unsupported matchup leaves campaign idle"), S->HasPendingBattle());
        TestTrue(TEXT("supported descriptor still admitted"), Bridge->BeginEncounter(Valid));
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVerticalScopedSaveTest,
    "Soul.Integration.Vertical.RBSaveScopedDomains",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVerticalScopedSaveTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* Save = NewObject<URBSaveSubsystem>(GI);
    auto* State = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    auto* Settlements = NewObject<USoulSettlementStateSubsystem>(GI);
    State->InitializeScenario();
    auto& Town = Settlements->FindOrAddSettlement(TEXT("human_capital"));
    Town.WallIntegrityPermille = 350;
    FString Error;
    TestTrue(TEXT("register real campaign provider"), Save->RegisterDomainProvider(State, Error));
    TestTrue(TEXT("register real settlement provider"), Save->RegisterDomainProvider(Settlements, Error));

    const TArray<FName> CampaignOnly = {TEXT("Soul.Campaign")};
    const TArray<FName> SettlementOnly = {TEXT("Soul.Settlements")};
    const TArray<FName> Both = {TEXT("Soul.Campaign"), TEXT("Soul.Settlements")};
    rb::save::Snapshot OnlyTown, Captured, Scratch;
    State->PendingBattle.EncounterId = TEXT("unresolved");
    TestTrue(TEXT("unselected failing provider does not block selected checkpoint"),
        Save->CaptureRegisteredDomains(TEXT("Scoped"), OnlyTown, Error, &SettlementOnly));
    TestEqual(TEXT("scope contains exactly selected domain"), static_cast<int32>(OnlyTown.domains.size()), 1);
    TestTrue(TEXT("scope identifies settlement"), OnlyTown.domains[0].id == "Soul.Settlements");
    TestFalse(TEXT("all-domain behavior still fails on unresolved campaign"),
        Save->CaptureRegisteredDomains(TEXT("Scoped"), Scratch, Error));
    TestFalse(TEXT("selected failed provider never silently skipped"),
        Save->CaptureRegisteredDomains(TEXT("Scoped"), Scratch, Error, &CampaignOnly));

    State->PendingBattle = FSoulCampaignBattleDescriptor();
    State->PlayerRegion = TEXT("river_ford");
    TestTrue(TEXT("real campaign encounter committed"), State->BeginBattle(TEXT("orc_watch")));
    TestTrue(TEXT("real campaign consequence applied"), State->ApplyBattleResult(Victory(State->PendingBattle, 7)));
    TestTrue(TEXT("capture required campaign and settlement authorities"),
        Save->CaptureRegisteredDomains(TEXT("Scoped"), Captured, Error, &Both));
    TestEqual(TEXT("two selected domains captured"), static_cast<int32>(Captured.domains.size()), 2);

    State->PlayerRegion = TEXT("human_capital");
    State->PlayerArmy[State->PlayerUnitId] = 99;
    Town.WallIntegrityPermille = 900;
    TestTrue(TEXT("restore explicitly selected authorities"),
        Save->RestoreRegisteredDomains(Captured, Error, &Both));
    TestEqual(TEXT("campaign target restored"), State->PlayerRegion, FName(TEXT("orc_watch")));
    TestEqual(TEXT("campaign casualties restored"), State->PlayerArmy[State->PlayerUnitId], 7);
    TestEqual(TEXT("settlement consequence restored"),
        Settlements->FindSettlement(TEXT("human_capital"))->WallIntegrityPermille, 350);

    rb::save::Snapshot OnlyCampaign;
    TestTrue(TEXT("capture campaign-only payload for later-domain preflight"),
        Save->CaptureRegisteredDomains(TEXT("Scoped"), OnlyCampaign, Error, &CampaignOnly));
    TestEqual(TEXT("campaign-only snapshot excludes later settlement provider"),
        static_cast<int32>(OnlyCampaign.domains.size()), 1);

    State->PlayerRegion = TEXT("human_capital");
    State->PlayerArmy[State->PlayerUnitId] = 23;
    Settlements->FindOrAddSettlement(TEXT("human_capital")).WallIntegrityPermille = 900;
    TestTrue(TEXT("restore settlement from multi-domain snapshot with campaign excluded"),
        Save->RestoreRegisteredDomains(Captured, Error, &SettlementOnly));
    TestEqual(TEXT("scoped restore applies selected settlement state"),
        Settlements->FindSettlement(TEXT("human_capital"))->WallIntegrityPermille, 350);
    TestEqual(TEXT("scoped restore preserves unselected campaign army"),
        State->PlayerArmy[State->PlayerUnitId], 23);
    TestEqual(TEXT("scoped restore preserves unselected campaign region"),
        State->PlayerRegion, FName(TEXT("human_capital")));

    const TArray<FName> Empty;
    const TArray<FName> Duplicate = {TEXT("Soul.Campaign"), TEXT("Soul.Campaign")};
    const TArray<FName> Unknown = {TEXT("Missing.Authority")};
    const TArray<FName> MissingId = {NAME_None};
    for (const TArray<FName>* Invalid : {&Empty, &Duplicate, &Unknown, &MissingId})
    {
        TestFalse(TEXT("invalid capture selection rejected"),
            Save->CaptureRegisteredDomains(TEXT("Scoped"), Scratch, Error, Invalid));
        TestFalse(TEXT("invalid restore selection rejected"),
            Save->RestoreRegisteredDomains(Captured, Error, Invalid));
    }

    State->PlayerArmy[State->PlayerUnitId] = 23;
    TestFalse(TEXT("missing selected snapshot domain rejected"),
        Save->RestoreRegisteredDomains(OnlyTown, Error, &Both));
    TestEqual(TEXT("missing-domain rejection precedes campaign mutation"),
        State->PlayerArmy[State->PlayerUnitId], 23);
    TestFalse(TEXT("missing later registered settlement rejected before restoring campaign"),
        Save->RestoreRegisteredDomains(OnlyCampaign, Error, &Both));
    TestEqual(TEXT("later missing-domain preflight preserves mutated campaign army"),
        State->PlayerArmy[State->PlayerUnitId], 23);
    TestEqual(TEXT("later missing-domain preflight preserves mutated campaign region"),
        State->PlayerRegion, FName(TEXT("human_capital")));
    return true;
}

#endif
