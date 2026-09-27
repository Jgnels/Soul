#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignManaContinuationTest,
    "Soul.Integration.Vertical.ManaPersistsAcrossBattleReloadAndDay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulCampaignManaContinuationTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    for (bool Won : {false, true})
    {
        auto* S = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
        S->InitializeScenario(); S->PlayerRegion = TEXT("river_ford"); S->Hero.Mana = 24;
        if (!TestTrue(TEXT("start battle with spent campaign mana"), S->BeginBattle(TEXT("orc_watch")))) return false;
        TestEqual(TEXT("encounter carries actual mana"), S->PendingBattle.PlayerMana, 24);
        FSoulCampaignBattleResult R;
        R.EncounterId = S->PendingBattle.EncounterId; R.TargetRegion = S->PendingBattle.TargetRegion;
        R.bPlayerWon = Won; R.PlayerSurvivors = Won ? 10 : 0; R.EnemySurvivors = Won ? 0 : 4;
        R.MagicCasts = 2; R.PlayerManaRemaining = 25;
        TestFalse(TEXT("result cannot invent battle mana"), S->ApplyBattleResult(R));
        R.PlayerManaRemaining = -1;
        TestFalse(TEXT("negative result mana rejected"), S->ApplyBattleResult(R));
        R.PlayerManaRemaining = 8;
        TestTrue(TEXT("actual remaining mana accepted for either outcome"), S->ApplyBattleResult(R));
        TestEqual(TEXT("campaign pays battle mana cost"), S->Hero.Mana, 8);
        FRBSaveDomainState Saved;
        FString Error;
        if (!TestTrue(TEXT("checkpoint spent mana"), S->CaptureRBSaveDomain_Implementation(Saved, Error))) return false;
        auto* Loaded = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
        if (!TestTrue(TEXT("reload spent mana"), Loaded->RestoreRBSaveDomain_Implementation(Saved, Error))) return false;
        TestEqual(TEXT("hero balance persists"), Loaded->Hero.Mana, 8);
        TestEqual(TEXT("result receipt persists"), Loaded->LastBattleResult.PlayerManaRemaining, 8);
        Loaded->AdvanceDay();
        TestEqual(TEXT("day grants only normal six mana recovery"), Loaded->Hero.Mana, 14);
        if (Won)
        {
            if (!TestTrue(TEXT("second encounter starts"), Loaded->BeginBattle(TEXT("orc_camp")))) return false;
            TestEqual(TEXT("second battle does not reset mana to eighty"), Loaded->PendingBattle.PlayerMana, 14);
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCampaignResourceReceiptTest,
    "Soul.Integration.Vertical.ResourceReceiptMigrationAndAtomicRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulCampaignResourceReceiptTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* Source = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    Source->InitializeScenario();
    Source->Hero.Kind = ESoulHeroKind::Paragon;
    Source->Hero.Mana = 17;
    FRBSaveDomainState Saved;
    FString Error;
    if (!TestTrue(TEXT("capture resource and hero identity"), Source->CaptureRBSaveDomain_Implementation(Saved, Error))) return false;
    auto* Loaded = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    if (!TestTrue(TEXT("restore paragon"), Loaded->RestoreRBSaveDomain_Implementation(Saved, Error))) return false;
    TestTrue(TEXT("paragon kind survives reload"), Loaded->Hero.Kind == ESoulHeroKind::Paragon);

    for (const TCHAR* Field : {TEXT("result_mana"), TEXT("hero_kind")})
    {
        for (double Invalid : {-1.0, 0.5, 2147483648.0, 999.0})
        {
            TSharedPtr<FJsonObject> Root;
            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Saved.Fields[0].StringValue), Root)) return false;
            Root->SetNumberField(Field, Invalid);
            auto Broken = Saved;
            Broken.Fields[0].StringValue.Reset();
            FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Broken.Fields[0].StringValue));
            FRBSaveDomainState Before, After;
            Loaded->CaptureRBSaveDomain_Implementation(Before, Error);
            TestFalse(TEXT("invalid optional receipt rejected"), Loaded->RestoreRBSaveDomain_Implementation(Broken, Error));
            Loaded->CaptureRBSaveDomain_Implementation(After, Error);
            TestEqual(TEXT("rejection leaves checkpoint unchanged"), After.Fields[0].StringValue, Before.Fields[0].StringValue);
        }
    }
    TSharedPtr<FJsonObject> Legacy;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Saved.Fields[0].StringValue), Legacy)) return false;
    Legacy->RemoveField(TEXT("result_mana"));
    Legacy->RemoveField(TEXT("hero_kind"));
    Saved.Fields[0].StringValue.Reset();
    FJsonSerializer::Serialize(Legacy.ToSharedRef(), TJsonWriterFactory<>::Create(&Saved.Fields[0].StringValue));
    TestTrue(TEXT("schema-one legacy checkpoint remains loadable"), Loaded->RestoreRBSaveDomain_Implementation(Saved, Error));
    TestEqual(TEXT("legacy resource balance stays authoritative"), Loaded->Hero.Mana, 17);
    TestTrue(TEXT("legacy hero defaults rather than retaining previous paragon"), Loaded->Hero.Kind == ESoulHeroKind::Hero);
    return true;
}
#endif
