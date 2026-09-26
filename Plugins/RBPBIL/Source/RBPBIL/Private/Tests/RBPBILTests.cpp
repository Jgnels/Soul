#include "Misc/AutomationTest.h"
#include "Engine/World.h"

#include "RBPBILInfluenceComponent.h"
#include "RBPBILInfluenceVolume.h"
#include "RBPBILLibrary.h"
#include "RBPBILTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBPBILChannelTagsTest,
    "RB.PBIL.ChannelTagsStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBPBILChannelTagsTest::RunTest(const FString&)
{
    TArray<FName> Tags;
    FRBPBILLayers::GetAll(Tags);

    TestEqual(TEXT("eight semantic channels"), Tags.Num(), 8);
    TSet<FName> UniqueTags(Tags);
    TestEqual(TEXT("channel tags are unique"), UniqueTags.Num(), Tags.Num());
    for (const FName Tag : Tags)
    {
        TestFalse(TEXT("channel tag is never None"), Tag.IsNone());
    }

    TestEqual(
        TEXT("threat tag remains stable"),
        FRBPBILLayers::ToTag(ERBPBILChannel::Threat),
        FName(TEXT("PBIL.Threat")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBPBILInfluenceComponentTest,
    "RB.PBIL.InfluenceComponentSemanticConfig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBPBILInfluenceComponentTest::RunTest(const FString&)
{
    URBPBILInfluenceComponent* Component =
        NewObject<URBPBILInfluenceComponent>(GetTransientPackage());
    TestNotNull(TEXT("component created"), Component);
    if (!Component)
    {
        return false;
    }

    Component->SetChannelInfluence(
        ERBPBILChannel::Threat, 900.0f, 2.5f, 150.0f);

    const FName ThreatTag =
        FRBPBILLayers::ToTag(ERBPBILChannel::Threat);
    TestTrue(
        TEXT("semantic source reaches TCAT runtime map"),
        Component->HasInfluenceLayer(ThreatTag));

    const TArray<FTCATInfluenceConfigEntry>& Layers =
        Component->GetInfluenceLayers();
    TestEqual(TEXT("one configured semantic layer"), Layers.Num(), 1);
    if (Layers.Num() == 1)
    {
        TestEqual(TEXT("radius preserved"),
            Layers[0].SourceData.InfluenceRadius, 900.0f);
        TestEqual(TEXT("strength preserved"),
            Layers[0].SourceData.Strength, 2.5f);
    }
    TestTrue(
        TEXT("semantic layer can be removed"),
        Component->RemoveChannelInfluence(ERBPBILChannel::Threat));
    TestFalse(
        TEXT("removed layer is absent"),
        Component->HasInfluenceLayer(ThreatTag));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBPBILPresetAndGuardTest,
    "RB.PBIL.PresetAndQueryGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBPBILPresetAndGuardTest::RunTest(const FString&)
{
    const ARBPBILInfluenceVolume* Volume =
        GetDefault<ARBPBILInfluenceVolume>();
    TestNotNull(TEXT("PBIL volume CDO exists"), Volume);
    if (Volume)
    {
        TestEqual(TEXT("preset exposes all channels"),
            Volume->GetPBILChannelCount(), 8);
    }
    FRBPBILQueryRequest Request;
    Request.Channel = ERBPBILChannel::Threat;
    Request.Origin = FVector::ZeroVector;
    Request.SearchRadius = 500.0f;

    FRBPBILQueryResult Result;
    TestFalse(
        TEXT("query safely rejects null world context"),
        URBPBILLibrary::QueryBestLocationImmediate(
            nullptr, Request, Result));
    TestFalse(TEXT("failed query leaves result unsuccessful"),
        Result.bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBPBILRuntimeDomainTest,
    "RB.PBIL.RuntimeDomainAndRefreshPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBPBILRuntimeDomainTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("isolated world exists"), World))
    {
        return false;
    }
    const FVector Origin(1200.0f, 400.0f, 100.0f);
    ARBPBILInfluenceVolume* Volume = World->SpawnActorDeferred<ARBPBILInfluenceVolume>(
        ARBPBILInfluenceVolume::StaticClass(), FTransform(Origin), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (TestNotNull(TEXT("deferred PBIL field exists"), Volume))
    {
        TestTrue(TEXT("existing adaptive default preserved"), Volume->IsAdaptiveRefreshEnabled());
        TestTrue(TEXT("existing GPU default preserved"), Volume->IsGPURefreshEnabled());
        TestTrue(TEXT("explicit CPU selection succeeds before BeginPlay"),
            Volume->SetRefreshPolicy(ERBPBILRefreshPolicy::CPU));
        TestFalse(TEXT("CPU policy cannot be switched back by adaptive tuning"), Volume->IsAdaptiveRefreshEnabled());
        TestFalse(TEXT("CPU policy excludes GPU readback path"), Volume->IsGPURefreshEnabled());
        TestTrue(TEXT("runtime bounds configured"),
            Volume->ConfigureRuntimeBounds(FVector(600.0f, 450.0f, 150.0f), 150.0f));
        TestEqual(TEXT("columns follow bounded domain"), Volume->GetColumns(), 8);
        TestEqual(TEXT("rows follow bounded domain"), Volume->GetRows(), 6);
        TestTrue(TEXT("translated field encloses origin"), Volume->GetCachedBounds().IsInside(Origin));
        TestFalse(TEXT("degenerate bounds rejected"), Volume->ConfigureRuntimeBounds(FVector::ZeroVector));
        TestFalse(TEXT("invalid cell size rejected"),
            Volume->ConfigureRuntimeBounds(FVector(600.0f), 0.0f));
        TestEqual(TEXT("failed configuration preserves valid grid"), Volume->GetColumns(), 8);
        TestTrue(TEXT("explicit GPU policy supported"), Volume->SetRefreshPolicy(ERBPBILRefreshPolicy::GPU));
        TestFalse(TEXT("fixed GPU is not adaptive"), Volume->IsAdaptiveRefreshEnabled());
        TestTrue(TEXT("fixed GPU enabled"), Volume->IsGPURefreshEnabled());
        // No FinishSpawning: this test checks the pre-registration contract without GPU dispatch.
        Volume->Destroy();
    }
    World->DestroyWorld(false);
    return true;
}
#endif
