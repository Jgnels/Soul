#include "Misc/AutomationTest.h"
#include "SoulSettlementStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulSettlementDomainRoundTripTest,
    "Soul.Integration.Save.SettlementDomainRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulSettlementDomainRoundTripTest::RunTest(const FString&)
{
    UGameInstance* SourceGameInstance = NewObject<UGameInstance>(GetTransientPackage());
    USoulSettlementStateSubsystem* Source =
        NewObject<USoulSettlementStateSubsystem>(SourceGameInstance);

    FSoulSettlementState& Town = Source->FindOrAddSettlement(TEXT("human_capital"));
    Town.FactionId = TEXT("humans");
    Town.RegionId = TEXT("greenvale");
    Town.FortificationLevel = 2;
    Town.WallIntegrityPermille = 350;
    Town.PermanentScars.Add(TEXT("west_wall_breach"));

    FSoulBuildingState Range;
    Range.Id = TEXT("archery_range");
    Range.Level = 1;
    Range.IntegrityPermille = 0;
    Range.Condition = ESoulBuildingCondition::Ruined;

    Town.Buildings.Add(Range.Id, Range);

    FRBSaveDomainState Domain;
    FString Error;
    TestTrue(TEXT("capture succeeds"),
        Source->CaptureRBSaveDomain_Implementation(Domain, Error));
    TestTrue(TEXT("capture error empty"), Error.IsEmpty());

    UGameInstance* RestoredGameInstance = NewObject<UGameInstance>(GetTransientPackage());
    USoulSettlementStateSubsystem* Restored =
        NewObject<USoulSettlementStateSubsystem>(RestoredGameInstance);
    TestTrue(TEXT("restore succeeds"),
        Restored->RestoreRBSaveDomain_Implementation(Domain, Error));

    const FSoulSettlementState* After =
        Restored->FindSettlement(TEXT("human_capital"));
    TestNotNull(TEXT("settlement restored"), After);
    if (!After)
    {
        return false;
    }

    TestEqual(TEXT("wall integrity restored"), After->WallIntegrityPermille, 350);
    TestTrue(TEXT("breach scar restored"),
        After->PermanentScars.Contains(TEXT("west_wall_breach")));

    const FSoulBuildingState* AfterRange =
        After->Buildings.Find(TEXT("archery_range"));
    TestNotNull(TEXT("building restored"), AfterRange);
    if (AfterRange)
    {
        TestTrue(TEXT("ruined condition restored"),
            AfterRange->Condition == ESoulBuildingCondition::Ruined);
        TestEqual(TEXT("building integrity restored"),
            AfterRange->IntegrityPermille, 0);
    }

    return true;
}

#endif
