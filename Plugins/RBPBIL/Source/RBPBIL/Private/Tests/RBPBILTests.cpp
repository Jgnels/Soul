#include "Misc/AutomationTest.h"

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

#endif
