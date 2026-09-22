#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleSpatial.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeBattleSpatialPolicyTest,
    "Soul.RealtimeBattle.PBIL.FormationSpatialPolicies",
    EAutomationTestFlags::EditorContext |
    EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeBattleSpatialPolicyTest::RunTest(
    const FString&)
{
    const FSoulFormationSpatialPolicy Line =
        FSoulRealtimeBattleSpatial::MakePolicy(
            ESoulRealtimeFormationRole::Line);
    TestTrue(TEXT("line seeks objectives"),
        Line.WeightFor(ERBPBILChannel::Objective) > 0.0f);
    TestTrue(TEXT("line avoids threat"),
        Line.WeightFor(ERBPBILChannel::Threat) < 0.0f);
    const FSoulFormationSpatialPolicy Shock =
        FSoulRealtimeBattleSpatial::MakePolicy(
            ESoulRealtimeFormationRole::Shock);
    const FSoulSpatialPreference* ShockPrimary =
        Shock.StrongestPreference();
    TestNotNull(TEXT("shock has primary preference"),
        ShockPrimary);
    if (ShockPrimary)
    {
        TestEqual(TEXT("shock seeks flank opportunity"),
            ShockPrimary->Channel,
            ERBPBILChannel::FlankOpportunity);
        TestTrue(TEXT("shock flank weight positive"),
            ShockPrimary->Weight > 0.0f);
    }

    const FSoulFormationSpatialPolicy Ranged =
        FSoulRealtimeBattleSpatial::MakePolicy(
            ESoulRealtimeFormationRole::Ranged);
    const FSoulSpatialPreference* RangedPrimary =
        Ranged.StrongestPreference();
    TestNotNull(TEXT("ranged has primary preference"),
        RangedPrimary);
    if (RangedPrimary)
    {
        TestEqual(TEXT("ranged primarily avoids threat"),
            RangedPrimary->Channel,
            ERBPBILChannel::Threat);
        TestTrue(TEXT("ranged threat weight negative"),
            RangedPrimary->Weight < 0.0f);
    }

    const FSoulFormationSpatialPolicy Support =
        FSoulRealtimeBattleSpatial::MakePolicy(
            ESoulRealtimeFormationRole::Support);
    TestTrue(TEXT("support seeks friendly concentration"),
        Support.WeightFor(
            ERBPBILChannel::FriendlySupport) > 1.0f);
    const ESoulRealtimeFormationRole Roles[] =
    {
        ESoulRealtimeFormationRole::Line,
        ESoulRealtimeFormationRole::Shock,
        ESoulRealtimeFormationRole::Ranged,
        ESoulRealtimeFormationRole::Support,
        ESoulRealtimeFormationRole::Apex,
        ESoulRealtimeFormationRole::Hero
    };

    for (const ESoulRealtimeFormationRole Role : Roles)
    {
        const FSoulFormationSpatialPolicy Policy =
            FSoulRealtimeBattleSpatial::MakePolicy(Role);
        TestTrue(TEXT("all roles avoid magic hazards"),
            Policy.WeightFor(
                ERBPBILChannel::MagicHazard) < 0.0f);
    }
    const FVector Origin(100.0, 200.0, 300.0);
    const FRBPBILQueryRequest ShockQuery =
        FSoulRealtimeBattleSpatial::MakePrimaryQuery(
            ESoulRealtimeFormationRole::Shock, Origin);
    TestEqual(TEXT("shock PBIL channel"),
        ShockQuery.Channel,
        ERBPBILChannel::FlankOpportunity);
    TestTrue(TEXT("shock seeks high influence"),
        ShockQuery.bFindHighest);
    TestEqual(TEXT("origin preserved"),
        ShockQuery.Origin, Origin);
    TestTrue(TEXT("query requires reachable cells"),
        ShockQuery.bReachableOnly);
    return true;
}

#endif
