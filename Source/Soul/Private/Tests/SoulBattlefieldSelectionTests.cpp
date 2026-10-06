#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "UObject/Package.h"
#include "SoulFounderPlaytestStateSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulPlayableBattlefieldSelectionTest,
    "Soul.Integration.Vertical.BattlefieldSelectionUsesDestinationGeography",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulPlayableBattlefieldSelectionTest::RunTest(const FString&)
{
    auto* GI = NewObject<UGameInstance>(GetTransientPackage());
    auto* S = NewObject<USoulFounderPlaytestStateSubsystem>(GI);
    S->InitializeScenario();
    S->PlayerRegion = TEXT("river_ford");
    FSoulCampaignBattleDescriptor D;
    FString Error;
    if (!TestTrue(TEXT("default descriptor builds"), S->BuildBattleDescriptor(TEXT("orc_watch"), D, Error))) return false;
    TestEqual(TEXT("proven default retained"), D.MapPackage, S->BattleMap);
    TestEqual(TEXT("destination recipe identity travels with descriptor"), D.BattlefieldId, FName(TEXT("dragon_watch")));
    TestEqual(TEXT("target landform travels with descriptor"), D.BattleContext.Landform, S->World.Regions[TEXT("orc_watch")].Landform);

    TestEqual(TEXT("actual approach transported"), D.BattleContext.AttackerApproach,
        S->World.Regions[TEXT("orc_watch")].ApproachFromNeighbor.FindRef(TEXT("river_ford")));
    TestEqual(TEXT("selection does not move campaign army"), S->PlayerRegion, FName(TEXT("river_ford")));
    TestEqual(TEXT("selection does not spend actions"), S->Economy.ActionPoints, 3);

    // Transport fixtures only: no maps are loaded or admitted to shipped content.
    FSoulBattlefieldTemplate Candidate;
    Candidate.Id = TEXT("test_watch"); Candidate.MapPackage = TEXT("/Game/Test/Watch");
    Candidate.ArenaOrigin = FVector(10, 20, 30);
    Candidate.bPlayable = true;
    S->BattlefieldTemplates.Add(Candidate);
    S->BuildBattleDescriptor(TEXT("orc_watch"), D, Error);
    TestEqual(TEXT("unmatched candidate cannot displace default"), D.MapPackage, S->BattleMap);
    S->BattlefieldTemplates.RemoveAt(S->BattlefieldTemplates.Num() - 1);
    Candidate.BaseScore = 10000;
    Candidate.bPlayable = false;
    Candidate.Landforms.Add(D.BattleContext.Landform);
    S->BattlefieldTemplates.Add(Candidate);
    S->BuildBattleDescriptor(TEXT("orc_watch"), D, Error);
    TestEqual(TEXT("unadmitted high-scoring candidate cannot change map"), D.MapPackage, S->BattleMap);
    S->BattlefieldTemplates.Last().bPlayable = true;
    S->BuildBattleDescriptor(TEXT("orc_watch"), D, Error);
    TestEqual(TEXT("admitted matching landform chooses binding"), D.MapPackage, Candidate.MapPackage);
    TestEqual(TEXT("binding supplies its own spawn origin"), D.ArenaOrigin, Candidate.ArenaOrigin);
    Candidate.Id = TEXT("aaa_test_watch"); Candidate.bPlayable = true;
    Candidate.MapPackage = TEXT("/Game/Test/TieBreak");
    S->BattlefieldTemplates.Add(Candidate);
    S->BuildBattleDescriptor(TEXT("orc_watch"), D, Error);
    TestEqual(TEXT("equal score uses stable id"), D.BattlefieldId, Candidate.Id);
    const auto SelectedOrigin = D.ArenaOrigin;
    if (!TestTrue(TEXT("selected battle commits"), S->BeginBattle(TEXT("orc_watch")))) return false;
    S->BattlefieldTemplates.Reset();
    TestEqual(TEXT("committed map survives registry changes"), S->PendingBattle.MapPackage, Candidate.MapPackage);
    TestEqual(TEXT("committed target survives registry changes"), S->PendingBattle.TargetRegion, FName(TEXT("orc_watch")));
    TestEqual(TEXT("committed origin survives registry changes"), S->PendingBattle.ArenaOrigin, SelectedOrigin);
    return true;
}
#endif
