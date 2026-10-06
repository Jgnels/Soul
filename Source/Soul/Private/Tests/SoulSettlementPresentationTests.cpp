#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "SoulSettlementBuildingActor.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSettlementConstructionPresentationTest,
    "Soul.Integration.Settlement.ConstructionPresentationPrecedesIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSettlementConstructionPresentationTest::RunTest(const FString&)
{
    const auto Init = UWorld::InitializationValues().InitializeScenes(false).AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("transient presentation world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Building = World->SpawnActor<ASoulSettlementBuildingActor>();
    if (!TestNotNull(TEXT("building presentation"), Building)) return false;
    AActor* Groups[4] = {};
    for (auto& Actor : Groups)
    {
        Actor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("external authored actor group"), Actor)) return false;
    }
    Building->ConstructionActors.Add(Groups[0]);
    Building->IntactActors.Add(Groups[1]);
    Building->DamagedActors.Add(Groups[2]);
    Building->RuinedActors.Add(Groups[3]);
    const FName BranchNames[] = {TEXT("Construction"), TEXT("Intact"), TEXT("Damaged"), TEXT("Ruined")};
    TInlineComponentArray<USceneComponent*> Components;
    Building->GetComponents(Components);
    USceneComponent* Branches[4] = {};
    for (int32 I = 0; I < 4; ++I)
    {
        for (auto* Component : Components) if (Component->GetFName() == BranchNames[I]) Branches[I] = Component;
        if (!TestNotNull(TEXT("presentation branch"), Branches[I])) return false;
    }
    struct FCase { const TCHAR* Name; int32 Integrity; bool Built, Constructing; int32 Visible; };
    const FCase Cases[] = {
        {TEXT("unbuilt"), 0, false, false, -1},
        // Controller reports Building as built; first construction has zero integrity.
        {TEXT("first construction"), 0, true, true, 0},
        {TEXT("completed"), 1000, true, false, 1},
        // An upgrade preserves old integrity while construction is active.
        {TEXT("upgrade"), 1000, true, true, 0},
        {TEXT("upgrade complete"), 1000, true, false, 1},
        {TEXT("damaged"), 450, true, false, 2},
        {TEXT("ruined"), 0, true, false, 3},
        {TEXT("unbuilt construction caller"), 0, false, true, 0},
        {TEXT("unbuilt after reload"), 0, false, false, -1}};
    for (const auto& Case : Cases)
    {
        Building->ApplyIntegrity(Case.Integrity, Case.Built, Case.Constructing);
        for (int32 I = 0; I < 4; ++I)
        {
            const bool Visible = I == Case.Visible;
            TestEqual(FString::Printf(TEXT("%s branch %d hidden"), Case.Name, I), !!Branches[I]->bHiddenInGame, !Visible);
            TestEqual(FString::Printf(TEXT("%s external %d hidden"), Case.Name, I), Groups[I]->IsHidden(), !Visible);
            TestEqual(FString::Printf(TEXT("%s external %d collision"), Case.Name, I), Groups[I]->GetActorEnableCollision(), Visible);
        }
    }
    return true;
}
#endif
