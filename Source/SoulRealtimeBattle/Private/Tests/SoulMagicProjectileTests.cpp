#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "SoulBattleArrow.h"
#include "RBMagicLibrary.h"
#include "RBMagicSpellDefinition.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulMagicProjectileAuthorityTest,
    "Soul.RealtimeBattle.Magic.PhysicalProjectileAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulMagicProjectileAuthorityTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("World"),World)) return false;
    auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* GameInstance=NewObject<UGameInstance>(GEngine);
    Context.OwningGameInstance=GameInstance;
    World->SetGameInstance(GameInstance);
    World->GetWorldSettings()->DefaultGameMode=ASoulRealtimeArenaGameMode::StaticClass();
    World->SetGameMode(FURL());
    auto* GM=World->GetAuthGameMode<ASoulRealtimeArenaGameMode>();
    if(!TestNotNull(TEXT("Actual authority installed"),GM))
    { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false; }
    GM->bBattlePaused=false;
    auto Body=[&](FVector Location,int32 Side)
    {
        FSoulRealtimeArenaCombatant Data;
        Data.Id=FGuid::NewGuid(); Data.Side=Side; Data.Health=100;
        const int32 Index=GM->Combatants.Add(Data);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Actor=World->SpawnActor<ACharacter>(Location,FRotator::ZeroRotator,Params);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        GM->Actors.Add(Actor);
        auto* Binding=NewObject<USoulRealtimeArenaBinding>(Actor);
        Actor->AddInstanceComponent(Binding); Binding->RegisterComponent();
        TestTrue(TEXT("Bind authoritative combatant"),Binding->BindCombatant(GM->IdentityAt(Index).Core()));
        return Actor;
    };
    GM->PlayerHero=Body(FVector(0,0,100),0);
    Body(FVector(1100,0,100),1);
    auto* Spell=LoadObject<URBMagicSpellDefinition>(nullptr,
        TEXT("/Game/Soul/Magic/Spells/DA_Soul_Firebolt.DA_Soul_Firebolt"));
    if(!TestNotNull(TEXT("Firebolt definition"),Spell))
    { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false; }
    FRBMagicCastRequest Request;
    Request.SpellTag=Spell->SpellTag;
    Request.Caster.Domain=TEXT("Soul.Arena"); Request.Caster.Id=GM->Combatants[0].Id;
    Request.Target.Entity.Domain=TEXT("Soul.Arena"); Request.Target.Entity.Id=GM->Combatants[1].Id;
    Request.Target.WorldLocation=GM->Actors[1]->GetActorLocation();
    TArray<FRBMagicEffectIntent> Effects; FString Error;
    auto Cast=[&]() -> ASoulBattleSpellProjectile*
    {
        Request.CastId=FGuid::NewGuid(); GM->SpellCooldowns.Reset();
        if(!URBMagicLibrary::BuildEffectIntents(Spell,Request,Effects,Error) ||
            !GM->TryCommitMagicCast(*Spell,Request,Effects,Error)) { AddError(Error); return nullptr; }
        return GM->MagicProjectiles.FindRef(Request.CastId).Get();
    };
    auto* Bolt=Cast();
    TestNotNull(TEXT("Cast launches physical bolt"),Bolt);
    TestEqual(TEXT("Cast cannot damage before contact"),GM->Combatants[1].Health,100.f);
    if(Bolt)
    {
        Bolt->Tick(.1f);
        TestEqual(TEXT("Flight before contact does no damage"),GM->Combatants[1].Health,100.f);
        Bolt->Tick(1.f);
        TestTrue(TEXT("Swept physical impact applies committed damage"),GM->Combatants[1].Health<100.f);
        TestFalse(TEXT("Impact consumes pending payload"),GM->MagicProjectiles.Contains(Request.CastId));
        const float Health=GM->Combatants[1].Health;
        GM->ResolveMagicProjectile(Bolt,FHitResult());
        TestEqual(TEXT("Duplicate callback cannot damage again"),GM->Combatants[1].Health,Health);
    }
    const float Mana=GM->PlayerMana;
    GM->SpellCooldowns.Reset();
    TestFalse(TEXT("Repeated cast ID cannot spend again"),GM->TryCommitMagicCast(*Spell,Request,Effects,Error));
    TestEqual(TEXT("Replay preserves mana"),GM->PlayerMana,Mana);
    GM->Combatants[1].Health=100;
    auto* Ally=Body(FVector(500,0,100),0);
    Bolt=Cast(); if(Bolt) Bolt->Tick(1.f);
    TestEqual(TEXT("Intervening ally blocks projectile without friendly damage"),GM->Combatants[2].Health,100.f);
    TestEqual(TEXT("Projectile cannot pass through ally to target"),GM->Combatants[1].Health,100.f);
    Ally->GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldStatic);
    Bolt=Cast(); if(Bolt) Bolt->Tick(1.f);
    TestEqual(TEXT("Scenery collision prevents distant damage"),GM->Combatants[1].Health,100.f);
    Ally->SetActorEnableCollision(false);
    Bolt=Cast(); GM->bFinished=true; if(Bolt) Bolt->Tick(1.f);
    TestEqual(TEXT("Resolved battles reject in-flight damage"),GM->Combatants[1].Health,100.f);
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
