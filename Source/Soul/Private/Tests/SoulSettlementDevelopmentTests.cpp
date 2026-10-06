#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "UObject/Package.h"
#include "RBSaveCore.h"
#include "RBSaveSubsystem.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    USoulSettlementScenarioData* DevelopmentScenario()
    {
        auto* Scenario = NewObject<USoulSettlementScenarioData>();
        Scenario->SettlementId = TEXT("human_capital");
        Scenario->RegionId = TEXT("human_capital");
        Scenario->FactionId = TEXT("humans");
        FSoulInitialBuildingSpec Keep;
        Keep.BuildingId = TEXT("human.keep");
        Scenario->Buildings.Add(Keep);
        FSoulInitialBuildingSpec Tavern;
        Tavern.BuildingId = TEXT("human.tavern");
        Tavern.bBuilt = false;
        Scenario->Buildings.Add(Tavern);
        FSoulBuildingDevelopmentSpec Definition;
        Definition.BuildingId = Tavern.BuildingId;
        Definition.BuildDays = 2;
        Definition.BuildCost.Add(TEXT("gold"), 200); // Test fixture, not production balance.
        Definition.Prerequisites.Add(Keep.BuildingId);
        Definition.UnlockIds.Add(TEXT("service.tavern_hero"));
        Scenario->DevelopmentDefinitions.Add(Definition);
        return Scenario;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulDevelopmentDefinitionValidationTest,
    "Soul.Integration.Settlement.DevelopmentDefinitionsRejectInvalidData",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulDevelopmentDefinitionValidationTest::RunTest(const FString&)
{
    FString Error;
    TestTrue(TEXT("valid fixture"), DevelopmentScenario()->ValidateDefinition(Error));
    auto Reject = [this, &Error](const TCHAR* Label, TFunction<void(USoulSettlementScenarioData*)> Mutate)
    {
        auto* Scenario = DevelopmentScenario(); Mutate(Scenario);
        TestFalse(Label, Scenario->ValidateDefinition(Error));
        TestFalse(TEXT("invalid data explains rejection"), Error.IsEmpty());
        auto* GI = NewObject<UGameInstance>(GetTransientPackage());
        auto* Authority = NewObject<USoulSettlementStateSubsystem>(GI);
        TestFalse(TEXT("invalid seed is rejected before state mutation"), Authority->EnsureScenario(Scenario, Error));
        TestFalse(TEXT("rejection did not create a settlement"), Authority->HasSettlement(TEXT("human_capital")));
    };
    Reject(TEXT("empty settlement id"), [](auto* S) { S->SettlementId = NAME_None; });
    Reject(TEXT("duplicate initial id"), [](auto* S) { const auto Duplicate = S->Buildings[0]; S->Buildings.Add(Duplicate); });
    Reject(TEXT("duplicate definition id"), [](auto* S) { const auto Duplicate = S->DevelopmentDefinitions[0]; S->DevelopmentDefinitions.Add(Duplicate); });
    Reject(TEXT("missing definition building"), [](auto* S) { S->DevelopmentDefinitions[0].BuildingId = TEXT("missing"); });
    Reject(TEXT("negative cost"), [](auto* S) { S->DevelopmentDefinitions[0].BuildCost[TEXT("gold")] = -1; });
    Reject(TEXT("zero duration"), [](auto* S) { S->DevelopmentDefinitions[0].BuildDays = 0; });
    Reject(TEXT("negative duration"), [](auto* S) { S->DevelopmentDefinitions[0].BuildDays = -1; });
    Reject(TEXT("missing prerequisite"), [](auto* S) { S->DevelopmentDefinitions[0].Prerequisites.Add(TEXT("missing")); });
    Reject(TEXT("cyclic prerequisites"), [](auto* S)
    {
        FSoulBuildingDevelopmentSpec Keep; Keep.BuildingId = TEXT("human.keep");
        Keep.Prerequisites.Add(TEXT("human.tavern")); S->DevelopmentDefinitions.Add(Keep);
    });
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulDevelopmentCommandTest,
    "Soul.Integration.Settlement.ConstructionGatesCostsDaysAndExistingService",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulDevelopmentCommandTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* State = NewObject<USoulFounderPlaytestStateSubsystem>(GI); State->InitializeScenario();
    auto* Authority = NewObject<USoulSettlementStateSubsystem>(GI);
    auto* Scenario = DevelopmentScenario(); FString Error;
    auto* OtherGI = NewObject<UGameInstance>(GetTransientPackage());
    auto* OtherAuthority = NewObject<USoulSettlementStateSubsystem>(OtherGI);
    TestFalse(TEXT("wrong GameInstance authority rejected"), State->InitializeSettlementDevelopment(Scenario, OtherAuthority, Error));
    TestTrue(TEXT("requested proof remains explicit"), State->IsSettlementDevelopmentEnabled());
    TestFalse(TEXT("failed initialization is not ready"), State->IsSettlementDevelopmentReady());
    TestFalse(TEXT("unready proof cannot hire"), State->HireTavernHero());
    const int32 InitialDay = State->Economy.Day;
    State->AdvanceDay(); TestEqual(TEXT("unready proof cannot advance"), State->Economy.Day, InitialDay);
    if (!TestTrue(TEXT("same-instance scenario ready"), State->InitializeSettlementDevelopment(Scenario, Authority, Error))) return false;
    auto* Town = Authority->FindSettlement(Scenario->SettlementId);
    if (!TestNotNull(TEXT("one settlement authority seeded"), Town)) return false;
    auto& Tavern = Town->Buildings.FindChecked(TEXT("human.tavern"));
    TestFalse(TEXT("missing tavern has no service"), State->IsTavernOperational());
    TestFalse(TEXT("missing tavern blocks existing hire"), State->HireTavernHero());
    TestTrue(TEXT("normal knight recruitment remains available"), State->Recruit(State->PlayerUnitId));
    const int32 InitialGold = State->Economy.Resources[TEXT("gold")];
    auto Denied = [&](const TCHAR* Label)
    {
        TestFalse(Label, State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
        TestEqual(TEXT("denied action preserves resources"), State->Economy.Resources[TEXT("gold")], InitialGold);
        TestTrue(TEXT("denied action preserves unbuilt state"), Tavern.Condition == ESoulBuildingCondition::Unbuilt);
    };
    State->PlayerRegion = TEXT("crossroads"); Denied(TEXT("must be at settlement")); State->PlayerRegion = TEXT("human_capital");
    auto& Region = State->World.Regions.FindChecked(State->PlayerRegion);
    const FName Owner = Region.OwnerFactionId; Region.OwnerFactionId = TEXT("dwarves"); Denied(TEXT("must own region")); Region.OwnerFactionId = Owner;
    Town->FactionId = TEXT("dwarves"); Denied(TEXT("must own settlement")); Town->FactionId = State->PlayerFaction;
    State->bPersistenceBusy = true; Denied(TEXT("save operation locks construction")); State->bPersistenceBusy = false;
    State->PendingBattle.EncounterId = TEXT("pending"); Denied(TEXT("battle locks construction")); State->PendingBattle = FSoulCampaignBattleDescriptor();
    auto& Keep = Town->Buildings.FindChecked(TEXT("human.keep"));
    Keep.Condition = ESoulBuildingCondition::Unbuilt; Denied(TEXT("prerequisite required")); Keep.Condition = ESoulBuildingCondition::Intact;
    State->Economy.Resources[TEXT("gold")] = 199;
    TestFalse(TEXT("insufficient resources"), State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
    TestEqual(TEXT("failed purchase spends nothing"), State->Economy.Resources[TEXT("gold")], 199);
    State->Economy.Resources[TEXT("gold")] = InitialGold;
    TestFalse(TEXT("unknown building rejected"), State->BeginSettlementConstruction(TEXT("missing"), Error));
    TestTrue(TEXT("construction begins"), State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
    TestEqual(TEXT("cost deducted exactly once"), State->Economy.Resources[TEXT("gold")], InitialGold - 200);
    TestEqual(TEXT("construction does not advance campaign clock"), State->Economy.Day, InitialDay);
    TestFalse(TEXT("duplicate purchase denied"), State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
    TestEqual(TEXT("duplicate cost unchanged"), State->Economy.Resources[TEXT("gold")], InitialGold - 200);
    State->AdvanceDay();
    TestEqual(TEXT("campaign advances once"), State->Economy.Day, InitialDay + 1);
    TestEqual(TEXT("construction advances once"), Tavern.ConstructionDaysRemaining, 1);
    TestFalse(TEXT("construction does not unlock service"), State->IsTavernOperational());
    State->AdvanceDay();
    TestEqual(TEXT("completion level"), Tavern.Level, 1);
    TestTrue(TEXT("completed service available"), State->IsTavernOperational());
    TestFalse(TEXT("max-level repeat denied"), State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
    const int32 BeforeHire = State->Economy.Resources[TEXT("gold")];
    TestTrue(TEXT("existing hire action unlocks"), State->HireTavernHero());
    TestEqual(TEXT("existing hire price retained"), State->Economy.Resources[TEXT("gold")], BeforeHire - 1200);
    TestFalse(TEXT("same hero cannot be hired twice"), State->HireTavernHero());
    TestTrue(TEXT("scenario revisit succeeds"), Authority->EnsureScenario(Scenario, Error));
    TestEqual(TEXT("scenario revisit cannot reset built tavern"), Tavern.Level, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulDevelopmentSaveBattleTest,
    "Soul.Integration.Settlement.DevelopmentSurvivesScopedSaveAndBattleBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulDevelopmentSaveBattleTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* State = NewObject<USoulFounderPlaytestStateSubsystem>(GI); State->InitializeScenario();
    auto* Authority = NewObject<USoulSettlementStateSubsystem>(GI);
    auto* Save = NewObject<URBSaveSubsystem>(GI);
    auto* Scenario = DevelopmentScenario(); FString Error;
    if (!TestTrue(TEXT("proof initialized"), State->InitializeSettlementDevelopment(Scenario, Authority, Error))) return false;
    State->BeginSettlementConstruction(TEXT("human.tavern"), Error); State->AdvanceDay();
    TestTrue(TEXT("campaign provider registered"), Save->RegisterDomainProvider(State, Error));
    TestTrue(TEXT("existing settlement provider registered"), Save->RegisterDomainProvider(Authority, Error));
    const TArray<FName> Domains = {TEXT("Soul.Campaign"), TEXT("Soul.Settlements")};
    rb::save::Snapshot During;
    if (!TestTrue(TEXT("mid-construction checkpoint captures both domains"), Save->CaptureRegisteredDomains(TEXT("DevelopmentTest"), During, Error, &Domains))) return false;
    State->AdvanceDay();
    TestTrue(TEXT("completion before rollback"), State->IsTavernOperational());
    TestTrue(TEXT("reload restores both domains"), Save->RestoreRegisteredDomains(During, Error, &Domains));
    TestFalse(TEXT("reload does not unlock early"), State->IsTavernOperational());
    TestEqual(TEXT("remaining construction day restored"), Authority->FindSettlement(Scenario->SettlementId)->Buildings[TEXT("human.tavern")].ConstructionDaysRemaining, 1);
    State->AdvanceDay(); State->HireTavernHero();
    FRBSaveDomainState BeforeBattle, AfterBattle;
    Authority->CaptureRBSaveDomain_Implementation(BeforeBattle, Error);
    // A legal existing hostile edge; the friendly capital is never rewritten as an enemy.
    State->PlayerRegion = TEXT("river_ford");
    if (!TestTrue(TEXT("existing encounter commits"), State->BeginBattle(TEXT("orc_watch")))) return false;
    TestFalse(TEXT("construction remains blocked during battle"), State->BeginSettlementConstruction(TEXT("human.tavern"), Error));
    FSoulCampaignBattleResult Result;
    Result.EncounterId = State->PendingBattle.EncounterId; Result.TargetRegion = State->PendingBattle.TargetRegion;
    Result.bPlayerWon = true; Result.PlayerSurvivors = 7; Result.PlayerManaRemaining = State->PendingBattle.PlayerMana;
    TestTrue(TEXT("valid existing battle result applies"), State->ApplyBattleResult(Result));
    TestFalse(TEXT("duplicate result still rejected"), State->ApplyBattleResult(Result));
    Authority->CaptureRBSaveDomain_Implementation(AfterBattle, Error);
    TestEqual(TEXT("battle does not replace development authority"), AfterBattle.Fields[0].StringValue, BeforeBattle.Fields[0].StringValue);
    rb::save::Snapshot Completed;
    if (!TestTrue(TEXT("completed campaign checkpoint"), Save->CaptureRegisteredDomains(TEXT("DevelopmentTest"), Completed, Error, &Domains))) return false;
    auto* FreshGI = NewObject<UGameInstance>(GetTransientPackage());
    auto* Fresh = NewObject<USoulFounderPlaytestStateSubsystem>(FreshGI); Fresh->InitializeScenario();
    auto* FreshAuthority = NewObject<USoulSettlementStateSubsystem>(FreshGI);
    auto* FreshSave = NewObject<URBSaveSubsystem>(FreshGI);
    Fresh->InitializeSettlementDevelopment(Scenario, FreshAuthority, Error);
    FreshSave->RegisterDomainProvider(Fresh, Error); FreshSave->RegisterDomainProvider(FreshAuthority, Error);
    TestTrue(TEXT("fresh GameInstance restores both authorities"), FreshSave->RestoreRegisteredDomains(Completed, Error, &Domains));
    TestTrue(TEXT("fresh state has completed tavern service"), Fresh->IsTavernOperational());
    TestTrue(TEXT("existing hired-hero result persisted"), Fresh->bSecondHeroHired);
    TestEqual(TEXT("battle survivors persisted"), Fresh->PlayerArmy[Fresh->PlayerUnitId], 7);
    TestTrue(TEXT("reentering environment does not reseed saved town"), FreshAuthority->EnsureScenario(Scenario, Error));
    TestEqual(TEXT("completed level preserved on visit"), FreshAuthority->FindSettlement(Scenario->SettlementId)->Buildings[TEXT("human.tavern")].Level, 1);
    // Exercise the actual UFUNCTION load callback after RB Save restoration, not just initialization.
    const FName SavedRegion = FreshAuthority->FindSettlement(Scenario->SettlementId)->RegionId;
    const FName SavedFaction = FreshAuthority->FindSettlement(Scenario->SettlementId)->FactionId;
    AddExpectedError(TEXT("SOUL_SETTLEMENT_DEVELOPMENT_UNREADY"), EAutomationExpectedErrorFlags::Contains, 2);
    for (bool WrongRegion : {true, false})
    {
        auto* Town = FreshAuthority->FindSettlement(Scenario->SettlementId);
        Town->RegionId = WrongRegion ? FName(TEXT("human_home")) : SavedRegion;
        Town->FactionId = WrongRegion ? SavedFaction : FName(TEXT("dwarves"));
        rb::save::Snapshot Mismatched;
        FreshSave->CaptureRegisteredDomains(TEXT("DevelopmentTest"), Mismatched, Error, &Domains);
        Town->RegionId = SavedRegion; Town->FactionId = SavedFaction;
        TestTrue(TEXT("legacy mismatched binding loads as saved"), FreshSave->RestoreRegisteredDomains(Mismatched, Error, &Domains));
        struct FCallbackParameters { FRBSaveOperationResult Result; } Callback;
        Callback.Result.bSuccess = true;
        Fresh->ProcessEvent(Fresh->FindFunctionChecked(TEXT("OnCampaignLoaded")), &Callback);
        TestFalse(TEXT("F9 callback marks mismatched binding unready"), Fresh->IsSettlementDevelopmentReady());
        TestFalse(TEXT("binding rejection explains problem"), Fresh->GetSettlementDevelopmentError().IsEmpty());
        Town = FreshAuthority->FindSettlement(Scenario->SettlementId);
        TestEqual(TEXT("callback preserves saved development"), Town->Buildings[TEXT("human.tavern")].Level, 1);
        TestEqual(TEXT("callback does not rewrite saved region"), Town->RegionId, WrongRegion ? FName(TEXT("human_home")) : SavedRegion);
        TestEqual(TEXT("callback does not rewrite saved faction"), Town->FactionId, WrongRegion ? SavedFaction : FName(TEXT("dwarves")));
        const int32 DayBefore = Fresh->Economy.Day;
        Fresh->AdvanceDay(); TestEqual(TEXT("unready loaded binding cannot advance"), Fresh->Economy.Day, DayBefore);
        TestTrue(TEXT("valid checkpoint can recover"), FreshSave->RestoreRegisteredDomains(Completed, Error, &Domains));
        Fresh->ProcessEvent(Fresh->FindFunctionChecked(TEXT("OnCampaignLoaded")), &Callback);
        TestTrue(TEXT("valid load clears proof binding error"), Fresh->IsSettlementDevelopmentReady());
    }
    return true;
}
#endif
