#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "SoulRealtimeBattleTactics.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulGroundOrderCorridorTest,
    "Soul.RealtimeBattle.Physical.GroundOrderCorridor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulGroundOrderCorridorTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("Physical world"),World)) return false;
    auto Box=[World](FVector At,FVector Extent)
    {
        auto* Actor=World->SpawnActor<AActor>();
        auto* Shape=NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Shape);
        Actor->AddInstanceComponent(Shape);
        Shape->SetBoxExtent(Extent);
        Shape->SetCollisionProfileName(TEXT("BlockAll"));
        Shape->SetCollisionObjectType(ECC_WorldStatic);
        Shape->RegisterComponent();
        Actor->SetActorLocation(At);
        return Actor;
    };
    Box(FVector(0,0,-40),FVector(1600,600,40));
    const FVector Start(-900,0,96),End(900,0,96);
    TestTrue(TEXT("Flat route works without navigation data"),
        ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(World,Start,End));
    auto* Wall=Box(FVector(0,0,180),FVector(40,200,180));
    TestFalse(TEXT("Cannot order through a wall"),
        ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(World,Start,End));
    TestTrue(TEXT("Waypoint around wall has a clear corridor"),
        ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(World,FVector(-900,400,96),FVector(900,400,96)));
    Wall->SetActorEnableCollision(false);
    TestFalse(TEXT("Ground ending at a cliff is rejected"),
        ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(World,Start,FVector(1900,0,96)));
    TestFalse(TEXT("Invalid corridor radius rejected"),
        ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(World,Start,End,-1));
    World->DestroyWorld(false);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulStrikeApproachTest,
    "Soul.RealtimeBattle.Tactics.StrikeUsesOutsideApproach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulStrikeApproachTest::RunTest(const FString&)
{
    for(int32 Side=0;Side<2;++Side)
    {
        const float Direction=Side==0 ? 1.0f : -1.0f;
        int32 Stage=0;
        const FVector Target(Direction*1500,200,96);
        FVector Center(-Direction*1350,-Direction*650,96);
        const FVector Staging=FSoulRealtimeTacticalRules::StrikeWaypoint(
            Side,FVector::ZeroVector,Center,Target,-Direction,Stage);
        TestEqual(TEXT("Does not cut into frontage on initial order"),Stage,0);
        TestTrue(TEXT("Outside approach remains away from infantry line"),FMath::Abs(Staging.Y)>1000);
        Center=Staging;
        const FVector Rear=FSoulRealtimeTacticalRules::StrikeWaypoint(
            Side,FVector::ZeroVector,Center,Target,-Direction,Stage);
        TestEqual(TEXT("Reaching staging allows movement down flank"),Stage,1);
        TestTrue(TEXT("Run goes behind selected hostile formation"),(Rear.X-Target.X)*Direction>0);
        TestTrue(TEXT("Flank run holds lateral separation"),FMath::Abs(Rear.Y)>1000);
        Center=Rear;
        const FVector Commit=FSoulRealtimeTacticalRules::StrikeWaypoint(
            Side,FVector::ZeroVector,Center,Target,-Direction,Stage);
        TestEqual(TEXT("Reaching rear permits commitment"),Stage,2);
        TestTrue(TEXT("Commit aims at tactical target"),Commit.Equals(Target));
    }
    return true;
}
#endif
