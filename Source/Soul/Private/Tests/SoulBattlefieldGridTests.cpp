#include "Misc/AutomationTest.h"
#include "SoulBattlefieldGridActor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulBattlefieldGridCoordinatesTest,
    "Soul.Integration.BattlefieldGrid.AxialCoordinateCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulBattlefieldGridCoordinatesTest::RunTest(const FString&)
{
    TestEqual(TEXT("radius zero has one cell"),
        ASoulBattlefieldGridActor::BuildAxialCoordinates(0).Num(), 1);
    TestEqual(TEXT("radius one has seven cells"),
        ASoulBattlefieldGridActor::BuildAxialCoordinates(1).Num(), 7);
    TestEqual(TEXT("radius eight has 217 cells"),
        ASoulBattlefieldGridActor::BuildAxialCoordinates(8).Num(), 217);

    const TArray<FIntPoint> Cells =
        ASoulBattlefieldGridActor::BuildAxialCoordinates(4);
    TSet<FIntPoint> Unique;
    for (const FIntPoint& Cell : Cells)
    {
        Unique.Add(Cell);
    }
    TestEqual(TEXT("baked axial cells are unique"), Unique.Num(), Cells.Num());
    return true;
}

#endif
