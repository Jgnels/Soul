#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "HAL/IConsoleManager.h"
#include "SoulSettlementBuildingActor.h"
#include "SoulSettlementPresentationController.h"

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAuthoredBuildingGroupTest,
    "Soul.Integration.Settlement.AuthoredGroupHidesChildrenAndPreservesCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulAuthoredBuildingGroupTest::RunTest(const FString&)
{
    const auto Init = UWorld::InitializationValues().InitializeScenes(false).AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("group test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Building = World->SpawnActor<ASoulSettlementBuildingActor>();
    auto MakeActor = [&]()
    {
        auto* Actor = World->SpawnActor<AActor>();
        Actor->SetRootComponent(NewObject<USceneComponent>(Actor));
        return Actor;
    };
    auto* Hall = MakeActor();
    auto* Door = MakeActor();
    auto* Decoration = MakeActor();
    auto* Unrelated = MakeActor();
    Door->AttachToActor(Hall, FAttachmentTransformRules::KeepWorldTransform);
    Decoration->AttachToActor(Door, FAttachmentTransformRules::KeepWorldTransform);
    Decoration->SetActorEnableCollision(false);
    Building->IntactActors = {Hall, Door, Building}; // Duplicate traversal and self-reference are harmless.
    Building->ApplyConditionName(TEXT("Unbuilt"));
    TestTrue(TEXT("whole authored hierarchy hidden"), Hall->IsHidden() && Door->IsHidden() && Decoration->IsHidden());
    TestFalse(TEXT("hidden door cannot block travel"), Door->GetActorEnableCollision());
    TestFalse(TEXT("unrelated city structure untouched"), Unrelated->IsHidden());
    Building->ApplyConditionName(TEXT("Intact"));
    TestFalse(TEXT("built hierarchy visible"), Hall->IsHidden() || Door->IsHidden() || Decoration->IsHidden());
    TestTrue(TEXT("authored door collision restored"), Door->GetActorEnableCollision());
    TestFalse(TEXT("noninteractive authored clutter remains noncolliding"), Decoration->GetActorEnableCollision());
    Building->ApplyConditionName(TEXT("Building"));
    Building->ApplyConditionName(TEXT("Intact"));
    TestTrue(TEXT("repeated load/state projection does not forget original collision"), Door->GetActorEnableCollision());
    TestFalse(TEXT("repeat does not enable decorative collision"), Decoration->GetActorEnableCollision());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulAuthoredExposureNormalizationTest,
    "Soul.Integration.Settlement.AuthoredExposureIsOptInIdempotentAndHandlesLateVolumes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulAuthoredExposureNormalizationTest::RunTest(const FString&)
{
    auto* Extended = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"));
    if (!TestNotNull(TEXT("renderer exposure convention"), Extended)) return false;
    const int32 Previous = Extended->GetInt();
    ON_SCOPE_EXIT { Extended->SetWithCurrentPriority(Previous); };
    const auto Init = UWorld::InitializationValues().InitializeScenes(false).AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("exposure test world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Adapter = World->SpawnActor<ASoulSettlementPresentationController>();
    auto MakeVolume = [&](float EV)
    {
        auto* Volume = World->SpawnActor<APostProcessVolume>();
        Volume->Settings.bOverride_AutoExposureMinBrightness = true;
        Volume->Settings.bOverride_AutoExposureMaxBrightness = true;
        Volume->Settings.AutoExposureMinBrightness = EV;
        Volume->Settings.AutoExposureMaxBrightness = EV;
        Volume->Settings.AutoExposureBias = 5.25f;
        return Volume;
    };
    auto* Hall = MakeVolume(-1.f);
    auto* Exterior = MakeVolume(1.f);
    Extended->SetWithCurrentPriority(0);
    Adapter->Tick(0);
    TestEqual(TEXT("ordinary worlds are unchanged"), Hall->Settings.AutoExposureMinBrightness, -1.f);
    Adapter->bAuthoredEV100Exposure = true;
    Extended->SetWithCurrentPriority(1);
    Adapter->Tick(0);
    TestEqual(TEXT("native EV100 renderer needs no conversion"), Hall->Settings.AutoExposureMinBrightness, -1.f);
    Extended->SetWithCurrentPriority(0);
    const auto* Attenuation = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.LensAttenuation"));
    const float LuminanceMax = .78f / FMath::Max(Attenuation ? Attenuation->GetFloat() : .78f, .01f);
    Adapter->Tick(0);
    TestEqual(TEXT("interior negative EV normalized"), Hall->Settings.AutoExposureMinBrightness, LuminanceMax * .5f);
    TestEqual(TEXT("positive exterior EV also normalized"), Exterior->Settings.AutoExposureMaxBrightness, LuminanceMax * 2.f);
    TestEqual(TEXT("authored exposure compensation retained"), Hall->Settings.AutoExposureBias, 5.25f);
    auto* LateVolume = MakeVolume(2.f);
    Adapter->Tick(0);
    TestEqual(TEXT("repeated projection does not reconvert"), Hall->Settings.AutoExposureMinBrightness, LuminanceMax * .5f);
    TestEqual(TEXT("late sublevel volume is normalized"), LateVolume->Settings.AutoExposureMinBrightness, LuminanceMax * 4.f);
    TestEqual(TEXT("global renderer convention unchanged"), Extended->GetInt(), 0);
    return true;
}
#endif
