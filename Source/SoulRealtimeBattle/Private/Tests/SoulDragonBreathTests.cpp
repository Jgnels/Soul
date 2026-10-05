#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulDragonBreathPresentationTest,
    "Soul.RealtimeBattle.Vertical.ApexFacingAndBoundedBreath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulDragonBreathPresentationTest::RunTest(const FString&)
{
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Host = World->SpawnActor<ASoulRealtimeArenaGameMode>();
    Host->bVisualUnits = true;
    for (int32 Side = 0; Side < 2; ++Side)
    {
        auto* Actor = World->SpawnActor<ACharacter>();
        Actor->SetActorRotation(FRotator(0, Side == 0 ? 0 : 180, 0));
        auto* Mesh = Actor->GetMesh();
        Mesh->SetSkeletalMeshAsset(Host->ResolveVisualMesh(Side, ESoulRealtimeFormationRole::Apex));
        Mesh->SetRelativeRotation(Host->VisualMeshRotation(ESoulRealtimeMovementArchetype::Aerial));
        const FName Head(Side == 0 ? TEXT("GRIFFON_-Head") : TEXT("MOUNTAIN_DRAGON_-Head"));
        const FName Neck(Side == 0 ? TEXT("GRIFFON_-Neck") : TEXT("MOUNTAIN_DRAGON_-Neck"));
        UAnimationAsset* Clips[] = {
            Host->ResolveStanceAnimation(Side, ESoulRealtimeFormationRole::Apex, 0),
            Host->ResolveStanceAnimation(Side, ESoulRealtimeFormationRole::Apex, 1),
            Host->ResolveVisualAttack(Side, ESoulRealtimeFormationRole::Apex, 0)};
        for (auto* Clip : Clips)
        {
            if (!TestNotNull(TEXT("Apex sample"), Clip)) continue;
            Mesh->PlayAnimation(Clip, false);
            Mesh->SetPosition(Clip->GetPlayLength() * .35f, false);
            Mesh->TickAnimation(0, false);
            Mesh->RefreshBoneTransforms();
            Mesh->UpdateComponentToWorld();
            const FVector BodyFacing = (Mesh->GetBoneLocation(Head) - Mesh->GetBoneLocation(Neck)).GetSafeNormal2D();
            TestTrue(Clip->GetName() + TEXT(" faces the commanded direction, not sideways"),
                FVector::DotProduct(BodyFacing, Actor->GetActorForwardVector()) > .85f);
        }
        FSoulRealtimeArenaCombatant Unit;
        Unit.Side = Side; Unit.Role = ESoulRealtimeFormationRole::Apex;
        Host->Combatants.Add(Unit); Host->Actors.Add(Actor);
    }
    Host->Actors[0]->SetActorLocation(FVector(-500, 0, 0));
    const float Health = Host->Combatants[0].Health;
    Host->bBattlePaused = true;
    Host->PlayDragonBreath(1, 0);
    TestFalse(TEXT("Paused attack cannot spawn flame"), Host->Combatants[1].DragonBreath.IsValid());
    Host->bBattlePaused = false;
    Host->PlayDragonBreath(0, 1);
    TestFalse(TEXT("Griffin does not receive dragon flame"), Host->Combatants[0].DragonBreath.IsValid());
    Host->PlayDragonBreath(1, 1);
    TestFalse(TEXT("Friendly target cannot spawn flame"), Host->Combatants[1].DragonBreath.IsValid());
    Host->PlayDragonBreath(1, 0);
    auto* First = Host->Combatants[1].DragonBreath.Get();
    if (TestNotNull(TEXT("Dragon attack has an owned flame component"), First))
    {
        TestEqual(TEXT("Flame follows the audited head bone"), First->GetAttachSocketName(), FName(TEXT("MOUNTAIN_DRAGON_-Head")));
        TestEqual(TEXT("Flame cannot create additional contacts"), First->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        TestFalse(TEXT("Flame cannot change traversal"), First->CanEverAffectNavigation());
        TestEqual(TEXT("Presentation cannot apply damage"), Host->Combatants[0].Health, Health);
        AddInfo(FString::Printf(TEXT("Owned flame: emitters=%d looping=%d warmup=%.2f"),
            First->Template->Emitters.Num(), First->Template->IsLooping(), First->Template->WarmupTime));
        Host->PlayDragonBreath(1, 0);
        TestTrue(TEXT("Repeated presentation retires previous component"), First->IsBeingDestroyed());
        auto* Second = Host->Combatants[1].DragonBreath.Get();
        Host->Combatants[1].Health = 0;
        Host->TickCombatPresentation(.01f);
        TestTrue(TEXT("Dead dragon stops existing flame"), !Second || Second->IsBeingDestroyed());
        Host->PlayDragonBreath(1, 0);
        TestFalse(TEXT("Dead dragon cannot restart flame"), Host->Combatants[1].DragonBreath.IsValid());
    }
    World->DestroyWorld(false);
    return true;
}
#endif
