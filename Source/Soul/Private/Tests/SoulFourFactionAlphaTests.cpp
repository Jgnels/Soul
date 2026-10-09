#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementVisitGameMode.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
struct FAlphaFixture
{
    UGameInstance* GI;USoulFounderPlaytestStateSubsystem* S;
    FAlphaFixture()
    {
        const FString Old=FCommandLine::Get();FCommandLine::Set(TEXT("-SoulComposition -SoulFourFactionAlpha -SoulAlphaSeed=1701"));
        GI=NewObject<UGameInstance>();GI->AddToRoot();GI->Init();S=GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>();S->InitializeScenario();FCommandLine::Set(*Old);
    }
    ~FAlphaFixture(){GI->Shutdown();GI->RemoveFromRoot();}
};
FRBSaveDomainState Snapshot(USoulFounderPlaytestStateSubsystem* S)
{FRBSaveDomainState D;FString E;S->CaptureRBSaveDomain_Implementation(D,E);return D;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaTurnTest,"Soul.Integration.FourFactionAlpha.DeterministicTurnResume",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaTurnTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;TestTrue(TEXT("isolated playable alpha"),S->bInitialized&&S->IsFourFactionAlpha());
    TestEqual(TEXT("isolated slot"),S->GetCampaignSaveSlotName(),FString(TEXT("Soul.Composition3500.FourFactionAlpha")));
    FString VisitError;
    TestTrue(TEXT("player may visit their owned capital"),ASoulSettlementVisitGameMode::CanVisit(S,VisitError));
    S->AdvanceDay();TestTrue(TEXT("end day schedules AI"),S->IsAlphaTurnActive());
    TestFalse(TEXT("city entry cannot suspend unfinished AI turn"),ASoulSettlementVisitGameMode::CanVisit(S,VisitError));
    const auto Before=Snapshot(S);S->AdvanceDay();TestEqual(TEXT("cannot skip an unfinished turn"),Snapshot(S).Fields[0].StringValue,Before.Fields[0].StringValue);
    TestFalse(TEXT("Human movement waits for AI"),S->MovePlayerTo(TEXT("crossroads")));
    S->RunNextAlphaAction();const auto Mid=Snapshot(S);TestTrue(TEXT("midturn snapshot valid"),Mid.Fields.Num()==1);
    FAlphaFixture Restored;FString Error;TestTrue(TEXT("fresh authority restores cursor"),Restored.S->RestoreRBSaveDomain_Implementation(Mid,Error));
    for(int I=0;I<2;++I){S->RunNextAlphaAction();Restored.S->RunNextAlphaAction();}
    TestFalse(TEXT("exactly three AI slots"),S->IsAlphaTurnActive());
    TestTrue(TEXT("city entry resumes on player turn"),ASoulSettlementVisitGameMode::CanVisit(S,VisitError));
    TestEqual(TEXT("deterministic continuation after restore"),Snapshot(S).Fields[0].StringValue,Snapshot(Restored.S).Fields[0].StringValue);
    const auto Done=Snapshot(S);S->AdvanceEnemyAI();S->RunNextAlphaAction();TestEqual(TEXT("no extra action after third faction"),Snapshot(S).Fields[0].StringValue,Done.Fields[0].StringValue);
    for(FName Id:{FName(TEXT("dwarves")),FName(TEXT("orcs")),FName(TEXT("vikings"))})
    {FSoulFactionCampaignState A;S->InspectFactionArmy(Id,A);TestEqual(TEXT("one paid action"),A.Economy.ActionPoints,2);}
    for(FName Id:{FName(TEXT("nature")),FName(TEXT("dark"))})
    {FSoulFactionCampaignState A;S->InspectFactionArmy(Id,A);TestEqual(TEXT("passive army unchanged"),A.Army.TroopCount,30);TestEqual(TEXT("passive AP unspent"),A.Economy.ActionPoints,3);}
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaRecruitTest,"Soul.Integration.FourFactionAlpha.PaidFiniteRecruitment",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaRecruitTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;FString Error;FSoulControlledCampaignAction A;
    TestTrue(TEXT("prepare real Dwarf pool"),S->PrepareControlledRecruitment(TEXT("dwarves"),4,A,Error));
    TestTrue(TEXT("execute core recruitment"),S->ExecuteControlledAction(A,Error));FSoulFactionCampaignState D;S->InspectFactionArmy(TEXT("dwarves"),D);
    TestEqual(TEXT("real troops"),D.Army.TroopCount,34);TestEqual(TEXT("real gold cost"),D.Economy.Resources[TEXT("gold")],2440);
    TestEqual(TEXT("finite pool spent"),D.Economy.RecruitmentPools[D.Army.UnitId].Available,4);TestEqual(TEXT("action charged"),D.Economy.ActionPoints,2);
    const auto Saved=Snapshot(S);TestFalse(TEXT("replay recruitment rejected"),S->ExecuteControlledAction(A,Error));TestEqual(TEXT("rejection atomic"),Snapshot(S).Fields[0].StringValue,Saved.Fields[0].StringValue);
    FAlphaFixture Fresh;TestTrue(TEXT("pool/resources restore together"),Fresh.S->RestoreRBSaveDomain_Implementation(Saved,Error));
    TestEqual(TEXT("exact pool restore"),Snapshot(Fresh.S).Fields[0].StringValue,Saved.Fields[0].StringValue);
    FSoulControlledCampaignAction Passive;TestFalse(TEXT("Dark cannot recruit"),S->PrepareControlledRecruitment(TEXT("dark"),1,Passive,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaPassiveTest,"Soul.Integration.FourFactionAlpha.PassiveAndMalformedRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaPassiveTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;const auto Save=Snapshot(S);FString Error;FSoulControlledCampaignAction A;
    TestFalse(TEXT("Nature cannot execute military move"),S->PrepareControlledAction(TEXT("nature"),TEXT("nature.primary"),TEXT("nature_treehold"),TEXT("nature_forest_clearing"),A,Error));
    TestFalse(TEXT("direct passive movement rejected too"),S->MoveFactionArmy(TEXT("dark"),TEXT("dark_castle_approach"),Error));
    TestEqual(TEXT("passive rejections atomic"),Snapshot(S).Fields[0].StringValue,Save.Fields[0].StringValue);
    for(const TCHAR* Key:{TEXT("alpha_cursor"),TEXT("alpha_seed")})
    {
        auto Bad=Save;TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Bad.Fields[0].StringValue),O);
        O->SetNumberField(Key,2000000);FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&Bad.Fields[0].StringValue));
        TestFalse(TEXT("invalid turn/seed rejected"),S->RestoreRBSaveDomain_Implementation(Bad,Error));
        TestEqual(TEXT("invalid restore leaves state intact"),Snapshot(S).Fields[0].StringValue,Save.Fields[0].StringValue);
    }
    S->AdvanceDay();const auto Turn=Snapshot(S);auto Stale=Turn;
    TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Stale.Fields[0].StringValue),O);
    O->SetNumberField(TEXT("alpha_turn_day"),0);O->SetNumberField(TEXT("alpha_cursor"),3);
    FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&Stale.Fields[0].StringValue));
    TestFalse(TEXT("stale completed-turn stamp rejected"),S->RestoreRBSaveDomain_Implementation(Stale,Error));
    TestEqual(TEXT("stale turn rejection atomic"),Snapshot(S).Fields[0].StringValue,Turn.Fields[0].StringValue);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaOccupationTest,"Soul.Integration.FourFactionAlpha.OccupiedCapitalDoesNotFreezeCampaign",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaOccupationTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;
    TestTrue(TEXT("authored development binding starts ready"),S->IsSettlementDevelopmentReady());
    // A focused ownership-result fixture reproduces the real day-seven captured-capital stall.
    S->PlayerArmy.FindOrAdd(S->PlayerUnitId)=0;
    FSoulWorldRules::Capture(S->World,TEXT("human_capital"),TEXT("orcs"));
    TestTrue(TEXT("building identity survives occupation"),S->IsSettlementDevelopmentReady());
    TestFalse(TEXT("Human cannot recruit at occupied capital"),S->Recruit(TEXT("human_knight")));
    TestFalse(TEXT("occupied tavern grants no Human service"),S->IsTavernOperational());
    FString Error;TestFalse(TEXT("occupation blocks new construction"),S->BeginSettlementConstruction(S->GetTavernBuildingId(),Error));
    S->AdvanceDay();TestEqual(TEXT("campaign still advances"),S->Economy.Day,2);
    TestTrue(TEXT("AI turn scheduled after occupation"),S->IsAlphaTurnActive());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaWithdrawalTest,"Soul.Integration.FourFactionAlpha.DefeatedWithdrawalCannotCapture",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaWithdrawalTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;auto Fixture=Snapshot(S);FString Error;
    TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture.Fields[0].StringValue),O);
    O->GetObjectField(TEXT("owners"))->SetStringField(TEXT("viking_harbour"),TEXT("orcs"));
    for(const auto& V:O->GetArrayField(TEXT("canonical_regions")))
        if(V->AsObject()->GetStringField(TEXT("id"))==TEXT("viking_harbour"))V->AsObject()->SetStringField(TEXT("owner"),TEXT("orcs"));
    for(const auto& V:O->GetArrayField(TEXT("other_faction_states")))
        if(V->AsObject()->GetStringField(TEXT("faction"))==TEXT("vikings"))V->AsObject()->SetNumberField(TEXT("troops"),0);
    FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&Fixture.Fields[0].StringValue));
    TestTrue(TEXT("legitimate defeated-army state restores"),S->RestoreRBSaveDomain_Implementation(Fixture,Error));
    const auto Before=Snapshot(S);FSoulControlledCampaignAction A;
    TestFalse(TEXT("zero troops cannot capture neutral ridge"),S->PrepareControlledAction(TEXT("vikings"),TEXT("vikings.primary"),TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),A,Error));
    TestEqual(TEXT("rejection preserves state"),Snapshot(S).Fields[0].StringValue,Before.Fields[0].StringValue);
    TestTrue(TEXT("adjacent owned withdrawal admitted"),S->PrepareControlledAction(TEXT("vikings"),TEXT("vikings.primary"),TEXT("viking_harbour"),TEXT("viking_forest_track"),A,Error));
    TestTrue(TEXT("withdrawal uses normal execution"),S->ExecuteControlledAction(A,Error));
    FSoulFactionCampaignState V;S->InspectFactionArmy(TEXT("vikings"),V);
    TestEqual(TEXT("no free replacement troops"),V.Army.TroopCount,0);TestEqual(TEXT("normal AP cost"),V.Economy.ActionPoints,2);
    TestEqual(TEXT("no free gold"),V.Economy.Resources.FindRef(TEXT("gold")),3000);
    TestEqual(TEXT("captured harbour remains Orc"),S->World.Regions[TEXT("viking_harbour")].OwnerFactionId,FName(TEXT("orcs")));
    TestFalse(TEXT("empty army cannot attack back"),S->PrepareControlledAction(TEXT("vikings"),TEXT("vikings.primary"),TEXT("viking_forest_track"),TEXT("viking_harbour"),A,Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaFrontierTest,"Soul.Integration.FourFactionAlpha.BlockedFrontierDoesNotOscillate",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaFrontierTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;auto Recorded=Snapshot(S);FString Error;
    TestTrue(TEXT("read actual second-seed regression state"),FFileHelper::LoadFileToString(Recorded.Fields[0].StringValue,
        *(FPaths::ProjectDir()/TEXT("Source/Soul/Private/Tests/Fixtures/FourFactionBlockedFrontier.json"))));
    TestTrue(TEXT("restore actual day-ten campaign"),S->RestoreRBSaveDomain_Implementation(Recorded,Error));
    S->AdvanceDay();S->RunNextAlphaAction();S->RunNextAlphaAction();
    FSoulFactionCampaignState Orc;S->InspectFactionArmy(TEXT("orcs"),Orc);
    TestEqual(TEXT("Orcs leave the blocked Human frontier by the causeway"),Orc.Army.RegionId,FName(TEXT("dark_ruined_causeway")));
    TestEqual(TEXT("route remains one paid legal action"),Orc.Economy.ActionPoints,2);
    TestFalse(TEXT("passive Dark territory was not attacked"),S->HasPendingBattle());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAlphaFieldFallbackTest,"Soul.Integration.FourFactionAlpha.VisibleEnemyUsesAdmittedFieldFallback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulAlphaFieldFallbackTest::RunTest(const FString&)
{
    FAlphaFixture F;auto* S=F.S;auto Recorded=Snapshot(S);FString Error;
    TestTrue(TEXT("read actual late-campaign blocked encounter"),FFileHelper::LoadFileToString(Recorded.Fields[0].StringValue,
        *(FPaths::ProjectDir()/TEXT("Source/Soul/Private/Tests/Fixtures/FourFactionUnmatchedField.json"))));
    TestTrue(TEXT("restore actual day thirty-six"),S->RestoreRBSaveDomain_Implementation(Recorded,Error));
    S->AdvanceDay();S->RunNextAlphaAction();
    FSoulFactionCampaignState Orc;S->InspectFactionArmy(TEXT("orcs"),Orc);
    FSoulControlledCampaignAction A;
    TestTrue(TEXT("visible exact enemy has existing field fallback"),S->PrepareControlledAction(TEXT("orcs"),Orc.Army.ArmyId,Orc.Army.RegionId,TEXT("dark_ruined_causeway"),A,Error));
    TestFalse(TEXT("admission alone does not start battle"),S->HasPendingBattle());
    S->RunNextAlphaAction();
    TestTrue(TEXT("AI now attacks rather than circling the unbound region"),S->HasPendingBattle());
    TestEqual(TEXT("qualified existing fallback"),S->PendingBattle.BattlefieldId,FName(TEXT("dragon_graveyard")));
    TestEqual(TEXT("exact Orc attacker"),S->PendingBattle.PlayerUnitId,FName(TEXT("orc_hammer_warrior")));
    TestEqual(TEXT("exact Dwarf defender"),S->PendingBattle.EnemyUnitId,FName(TEXT("dwarf_warrior")));
    TestTrue(TEXT("real AI-only combat path"),S->PendingBattle.bAutoResolve);
    S->InspectFactionArmy(TEXT("orcs"),Orc);TestEqual(TEXT("one real AP spent"),Orc.Economy.ActionPoints,2);
    TestEqual(TEXT("no pre-result ownership change"),S->World.Regions.FindChecked(TEXT("dark_ruined_causeway")).OwnerFactionId,FName(TEXT("dwarves")));
    return true;
}
#endif
