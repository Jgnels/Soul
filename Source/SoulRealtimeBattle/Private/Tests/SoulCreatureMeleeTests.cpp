#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulCreatureMeleeContactTest,
    "Soul.RealtimeBattle.Physical.CreatureMeleeContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulCreatureMeleeContactTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto Spawn = [&](FVector Position, float Radius, float HalfHeight)
    {
        auto* Actor = World->SpawnActor<ACharacter>(Position, FRotator::ZeroRotator, Params);
        Actor->GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
        Actor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
        return Actor;
    };
    auto* Human = Spawn(FVector(0,0,96), 42, 96);
    auto* Creature = Spawn(FVector(270,0,170), 150, 170);
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulOldMeleeRegression), false, Human);
    const bool LegacyHit = World->SweepSingleByChannel(Hit, FVector(35,0,141), FVector(355,0,141),
        FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(18), Query);
    TestFalse(TEXT("Movement-overlap creature reproduces old melee miss"), LegacyHit);
    TestTrue(TEXT("Human can physically strike overlapping creature"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human, Creature, 270, Hit));
    TestTrue(TEXT("Creature owns physical impact"), Hit.GetActor() == Creature);
    TestTrue(TEXT("Tall creature can strike shorter human"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Creature, Human, 270, Hit));
    TestTrue(TEXT("Human owns reciprocal physical impact"), Hit.GetActor() == Human);
    auto* Blocker = Spawn(FVector(70,0,96), 22, 96);
    TestTrue(TEXT("An intervening body still produces a contact"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human, Creature, 270, Hit));
    TestTrue(TEXT("Contact cannot skip an intervening allied body"), Hit.GetActor() == Blocker);
    Blocker->GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldStatic);
    TestTrue(TEXT("Scenery also blocks the sweep"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human, Creature, 270, Hit));
    TestTrue(TEXT("Scenery remains first blocker"), Hit.GetActor() == Blocker);
    Blocker->SetActorEnableCollision(false);
    Creature->SetActorLocation(FVector(900,0,170));
    TestFalse(TEXT("Melee cannot hit a distant creature"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human, Creature, 270, Hit));
    // Regression: RVO separates a 115 cm elephant and 150 cm Kraken by
    // ~360 cm. A humanoid center-distance reach made both wait forever.
    Human->SetActorLocation(FVector(0,0,150));
    Human->GetCapsuleComponent()->SetCapsuleSize(115,150);
    Creature->SetActorLocation(FVector(360,0,170));
    const float BodyReach=ASoulRealtimeArenaGameMode::MeleeBodyReach(Human,Creature,175);
    TestTrue(TEXT("Large body engagement begins before avoidance stand-off"),BodyReach>=360);
    TestTrue(TEXT("Body-adjusted attack still requires physical contact"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human,Creature,BodyReach,Hit));
    TestTrue(TEXT("Large target owns actual contact"),Hit.GetActor()==Creature);
    Creature->SetActorLocation(FVector(1200,0,170));
    TestFalse(TEXT("Body reach never becomes remote damage"),
        ASoulRealtimeArenaGameMode::TraceMeleeContact(Human,Creature,BodyReach,Hit));
    World->DestroyWorld(false);
    return true;
}
#endif

