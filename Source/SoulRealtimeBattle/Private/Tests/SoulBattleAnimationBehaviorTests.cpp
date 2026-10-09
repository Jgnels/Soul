#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattleAnimationBehaviorTest,
    "Soul.RealtimeBattle.Vertical.AnimationStancesAndFieldStrength",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattleAnimationBehaviorTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();
    Host->bVisualUnits=true;
    Host->EnemyVisualFaction=TEXT("evil");
    for(int32 Side=0;Side<2;++Side)
    for(int32 R=0;R<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++R)
    {
        const auto Role=static_cast<ESoulRealtimeFormationRole>(R);
        auto* Mesh=Host->ResolveVisualMesh(Side,Role);
        if(!TestNotNull(TEXT("Roster mesh"),Mesh)) continue;
        TSet<UAnimationAsset*> Variants;
        auto Check=[&](UAnimationAsset* Asset)
        {
            if(!TestNotNull(TEXT("Presentation asset"),Asset)) return;
            auto* Clip=Cast<UAnimSequence>(Asset);
            if(!TestNotNull(TEXT("Sequence"),Clip)) return;
            TestTrue(Clip->GetName()+TEXT(" matches ")+Mesh->GetName(),Clip->GetSkeleton()->IsCompatibleMesh(Mesh,true));
            TestFalse(Clip->GetName()+TEXT(" is a full pose"),Clip->IsValidAdditive());
            TestTrue(TEXT("Positive animation duration"),Clip->GetPlayLength()>0);
        };
        for(int32 V=0;V<4;++V)
        {
            auto* Clip=Host->ResolveVisualAttack(Side,Role,V);
            if(Role==ESoulRealtimeFormationRole::Ranged)
                TestNull(TEXT("Bow skeleton never receives a warrior melee clip"),Clip);
            else { Check(Clip);Variants.Add(Clip); }
        }
        if(Role==ESoulRealtimeFormationRole::Line || Role==ESoulRealtimeFormationRole::Shock)
            TestTrue(TEXT("Infantry has distinct attack choices"),Variants.Num()>=3);
        Check(Host->ResolveVisualReaction(Side,Role));
        for(int32 M=0;M<6;++M) Check(Host->ResolveStanceAnimation(Side,Role,M));
        if(Role==ESoulRealtimeFormationRole::Apex) Check(Host->ResolveAerialFall(Side));
        if(Side==1 && (Role==ESoulRealtimeFormationRole::Guard || Role==ESoulRealtimeFormationRole::Hero))
        {
            auto* Equipped=World->SpawnActor<ACharacter>();
            Equipped->GetMesh()->SetSkeletalMeshAsset(Mesh);
            TestTrue(TEXT("Hammer rig has its authored palm attachment"),Equipped->GetMesh()->DoesSocketExist(TEXT("CATRigRArmPalm")));
            Host->EquipVisualWeapons(Equipped,Side,Role);
            TInlineComponentArray<UStaticMeshComponent*> Pieces(Equipped);
            int32 Weapons=0;
            for(auto* Piece:Pieces) if(Piece->ComponentHasTag(TEXT("SoulVisualWeapon")))
            {
                ++Weapons;
                TestEqual(TEXT("Orc carries its separate hammer"),GetNameSafe(Piece->GetStaticMesh()),FString(TEXT("SM_Hummer")));
                TestEqual(TEXT("Hammer follows the CAT palm"),Piece->GetAttachSocketName(),FName(TEXT("CATRigRArmPalm")));
                TestTrue(TEXT("Hammer does not inherit the CAT authoring-unit scale"),
                    Piece->GetComponentScale().Equals(Equipped->GetMesh()->GetComponentScale(),.001f));
                TestEqual(TEXT("Cosmetic hammer cannot introduce extra contact"),Piece->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
                TestFalse(TEXT("Cosmetic hammer does not alter navigation"),Piece->CanEverAffectNavigation());
            }
            TestEqual(TEXT("Heavy orc has exactly one cosmetic weapon"),Weapons,1);
            Equipped->Destroy();
        }
    }
    FSoulRealtimeArenaCombatant Unit;
    Unit.Id=FGuid::NewGuid();Unit.Side=0;Unit.Health=100;Unit.MaxHealth=100;Unit.GroupIndex=0;
    Host->Combatants.Add(Unit);
    FSoulBattleFormationState Formation;Formation.GroupIndex=0;Formation.Side=0;
    Host->TacticalFormations.Add(Formation);
    auto* Actor=World->SpawnActor<ACharacter>();Host->Actors.Add(Actor);
    Actor->GetMesh()->SetSkeletalMesh(Host->ResolveVisualMesh(0,ESoulRealtimeFormationRole::Line));
    TestEqual(TEXT("Field force starts at actual health"),Host->FieldStrengthEstimate(0),100.f);
    Host->Combatants[0].Health=40;
    TestEqual(TEXT("Wounds reduce field estimate"),Host->FieldStrengthEstimate(0),40.f);
    Host->TacticalFormations[0].bRouting=true;
    TestEqual(TEXT("Routing discounts remaining force"),Host->FieldStrengthEstimate(0),8.f);
    TestEqual(TEXT("No absent enemy/reserve force invented"),Host->FieldStrengthEstimate(1),0.f);
    Host->Combatants[0].PendingMeleeSeconds=.3f;
    Host->PlayAcceptedHitReaction(0);
    TestFalse(TEXT("Reaction cannot interrupt pending melee"),Host->Combatants[0].bVisualAttackPlaying);
    Host->Combatants[0].PendingMeleeSeconds=0;
    Host->PlayAcceptedHitReaction(0);
    TestTrue(TEXT("Idle wounded troop reacts"),Host->Combatants[0].bVisualAttackPlaying);
    TestEqual(TEXT("Reaction never applies extra damage"),Host->Combatants[0].Health,40.f);
    const float Cooldown=Host->Combatants[0].VisualReactionCooldown;
    Host->PlayAcceptedHitReaction(0);
    TestEqual(TEXT("Repeated hit cannot extend reaction lock"),Host->Combatants[0].VisualReactionCooldown,Cooldown);
    Host->bBattlePaused=false;
    Host->Combatants[0].bVisualAttackPlaying=false;
    Host->Combatants[0].VisualReactionCooldown=0;
    Host->Combatants[0].WardPoints=10;Host->Combatants[0].WardSeconds=2;
    TestTrue(TEXT("Ward accepts spell impact"),Host->ApplyMagicDamage(0,5));
    TestEqual(TEXT("Absorbed spell preserves health"),Host->Combatants[0].Health,40.f);
    TestFalse(TEXT("Absorbed spell does not fake a wound reaction"),Host->Combatants[0].bVisualAttackPlaying);
    TestTrue(TEXT("Overflow spell damage accepted"),Host->ApplyMagicDamage(0,10));
    TestEqual(TEXT("Ward overflow applies damage only once"),Host->Combatants[0].Health,35.f);
    TestTrue(TEXT("Accepted spell wound triggers bounded reaction"),Host->Combatants[0].bVisualAttackPlaying);
    Host->Combatants[0].Health=0;Host->Combatants[0].Role=ESoulRealtimeFormationRole::Apex;
    Host->Combatants[0].AerialDeathStartZ=180;Host->Combatants[0].AerialDeathSeconds=0;
    Actor->GetMesh()->SetSkeletalMesh(Host->ResolveVisualMesh(0,ESoulRealtimeFormationRole::Apex));
    Actor->GetMesh()->SetRelativeLocation(FVector(0,0,180));
    Host->TickCombatPresentation(0);
    TestEqual(TEXT("Paused defeat does not fall"),Actor->GetMesh()->GetRelativeLocation().Z,180.0);
    Host->TickCombatPresentation(.55f);
    TestTrue(TEXT("Defeated flyer physically descends"),Actor->GetMesh()->GetRelativeLocation().Z<180);
    Host->TickCombatPresentation(.6f);
    TestEqual(TEXT("Flyer lands at capsule foot plane"),Actor->GetMesh()->GetRelativeLocation().Z,
        -double(Actor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    TestEqual(TEXT("Landing transitions once"),Host->Combatants[0].AerialDeathSeconds,-1.f);
    TestEqual(TEXT("Dead units never contribute field force"),Host->FieldStrengthEstimate(0),0.f);

    // A stationary Face order used to leave the mesh facing its last movement.
    Host->Combatants[0].Health=100;
    Host->Combatants[0].Role=ESoulRealtimeFormationRole::Line;
    Host->Combatants[0].bVisualAttackPlaying=false;
    Actor->GetMesh()->SetSkeletalMesh(Host->ResolveVisualMesh(0,ESoulRealtimeFormationRole::Line));
    FRBHostGroup Group;
    Group.Id=FGuid::NewGuid();Group.Leader=Host->IdentityAt(0);
    Group.Members.Add(Group.Leader);Group.Order=ERBHostGroupOrder::Face;
    Group.Facing=FVector::RightVector;
    Host->Groups.Add(Group);Host->VisualRunning.Add(false);
    Host->UpdateVisualAnimations();
    TestTrue(TEXT("Stationary face order reaches physical soldier"),Actor->GetActorForwardVector().Equals(FVector::RightVector,.001f));
    Host->Groups[0].Order=ERBHostGroupOrder::FallBack;
    Actor->GetCharacterMovement()->Velocity=-FVector::RightVector*150;
    Host->UpdateVisualAnimations();
    TestEqual(TEXT("Fallback plays backward locomotion while facing threat"),Host->Combatants[0].VisualLocomotionMode,2);
    Host->Combatants[0].Role=ESoulRealtimeFormationRole::Breaker;
    Actor->GetMesh()->SetSkeletalMesh(Host->ResolveVisualMesh(0,ESoulRealtimeFormationRole::Breaker));
    Host->UpdateVisualAnimations();
    TestTrue(TEXT("Elephant faces travel during fallback without a backward clip"),Actor->GetActorForwardVector().Equals(-FVector::RightVector,.001f));
    TestEqual(TEXT("Elephant uses forward creature locomotion"),Host->Combatants[0].VisualLocomotionMode,1);
    Host->Groups[0].Order=ERBHostGroupOrder::Face;
    Host->UpdateVisualAnimations();
    TestTrue(TEXT("Creature repositions facing travel under a Face order"),Actor->GetActorForwardVector().Equals(-FVector::RightVector,.001f));
    Actor->GetCharacterMovement()->Velocity=FVector::ZeroVector;
    Host->UpdateVisualAnimations();
    TestTrue(TEXT("Settled creature adopts requested group facing"),Actor->GetActorForwardVector().Equals(FVector::RightVector,.001f));
    Host->Groups[0].Order=ERBHostGroupOrder::FallBack;
    Actor->GetCharacterMovement()->Velocity=-FVector::RightVector*150;
    Host->Combatants[0].Role=ESoulRealtimeFormationRole::Line;
    Actor->GetMesh()->SetSkeletalMesh(Host->ResolveVisualMesh(0,ESoulRealtimeFormationRole::Line));
    Host->Combatants[0].VisualReactionCooldown=0;
    Host->Combatants[0].VisualReactionCooldown=0;
    Host->PlayAcceptedHitReaction(0);
    TestTrue(TEXT("Reaction owns playback before completion"),Host->Combatants[0].bVisualAttackPlaying);
    Actor->GetMesh()->GetSingleNodeInstance()->SetPlaying(false);
    Host->UpdateVisualAnimations();
    TestFalse(TEXT("Completed reaction releases animation lock"),Host->Combatants[0].bVisualAttackPlaying);
    TestTrue(TEXT("Locomotion resumes after one-shot reaction"),Actor->GetMesh()->GetSingleNodeInstance()->IsPlaying());
    Host->Combatants[0].bPlayerHero=true;
    Actor->SetActorRotation(FRotator::ZeroRotator);
    Host->Groups[0].Order=ERBHostGroupOrder::Face;
    Host->UpdateVisualAnimations();
    TestTrue(TEXT("Group facing never overrides player look"),Actor->GetActorForwardVector().Equals(FVector::ForwardVector,.001f));
    Host->Combatants[0].bPlayerHero=false;

    // Threat work scales with groups, not the number of soldiers sharing one.
    Host->Bindings.Add(NewObject<USoulRealtimeArenaBinding>(Actor));
    for(int32 I=1;I<16;++I)
    {
        auto Soldier=Unit;Soldier.Id=FGuid::NewGuid();
        Host->Combatants.Add(Soldier);
        auto* Body=World->SpawnActor<ACharacter>();
        Host->Actors.Add(Body);
        Host->Bindings.Add(NewObject<USoulRealtimeArenaBinding>(Body));
        Host->Groups[0].Members.Add(Host->IdentityAt(I));
    }
    auto Enemy=Unit;Enemy.Id=FGuid::NewGuid();Enemy.Side=1;
    Host->Combatants.Add(Enemy);
    auto* EnemyActor=World->SpawnActor<ACharacter>();EnemyActor->SetActorLocation(FVector(300,0,0));
    Host->Actors.Add(EnemyActor);
    Host->TickCombatPresentation(.21f);
    TestEqual(TEXT("Sixteen soldiers share one guard threat query"),Host->GuardPresentationQueries,1);
    TestTrue(TEXT("Near enemy braces formation"),Host->Bindings[0]->IsGuardRequested());
    Host->Groups[0].Order=ERBHostGroupOrder::Charge;
    Host->TickCombatPresentation(.21f);
    TestEqual(TEXT("Charge skips defensive threat query"),Host->GuardPresentationQueries,0);
    TestFalse(TEXT("Charge releases old guard intent"),Host->Bindings[0]->IsGuardRequested());
    Host->Groups[0].Order=ERBHostGroupOrder::Hold;
    Host->bFinished=true;
    Host->TickCombatPresentation(.21f);
    TestEqual(TEXT("Resolved battle stops guard threat queries"),Host->GuardPresentationQueries,0);
    TestFalse(TEXT("Resolved survivors leave defensive guard"),Host->Bindings[0]->IsGuardRequested());
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulOrcCampaignRosterTest,
    "Soul.RealtimeBattle.Vertical.ExactOrcCampaignRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulOrcCampaignRosterTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();
    Host->bVisualUnits=true;
    Host->EnemyVisualFaction=TEXT("orcs");Host->EnemyVisualUnitId=TEXT("orc_hammer_warrior");
    TestFalse(TEXT("standalone roster cannot masquerade as exact campaign proof"),Host->UsesOrcCampaignRoster(1));
    Host->bCampaignBattle=true;
    TestTrue(TEXT("exact campaign Orc pair recognized"),Host->UsesOrcCampaignRoster(1));
    TestFalse(TEXT("human side cannot inherit enemy mesh"),Host->UsesOrcCampaignRoster(0));
    auto* Mesh=Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line);
    if(!TestNotNull(TEXT("owned Orc mesh loads"),Mesh)){World->DestroyWorld(false);return false;}
    TestEqual(TEXT("exact owned mesh"),Mesh->GetPathName(),FString(TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer.SK_Orc_Hummer")));
    auto Check=[&](UAnimationAsset* Asset)
    {
        auto* Clip=Cast<UAnimSequence>(Asset);
        if(!TestNotNull(TEXT("native full-pose sequence"),Clip))return;
        TestTrue(TEXT("strict own-skeleton compatibility"),Clip->GetSkeleton()->IsCompatibleMesh(Mesh,true));
        TestFalse(TEXT("no additive substitution"),Clip->IsValidAdditive());
        TestTrue(TEXT("positive duration"),Clip->GetPlayLength()>0);
        TestTrue(TEXT("clip comes from owned Orc family"),Clip->GetPathName().StartsWith(TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/")));
    };
    for(int32 R=0;R<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++R)
    {
        const auto Role=static_cast<ESoulRealtimeFormationRole>(R);
        TestTrue(TEXT("deployment/reserves admit hammer infantry only"),Host->CampaignFormationRole(1,Role)==ESoulRealtimeFormationRole::Line);
        TestTrue(TEXT("player formation roles unchanged"),Host->CampaignFormationRole(0,Role)==Role);
        TestTrue(TEXT("no requested tactical role substitutes another enemy species"),Host->ResolveVisualMesh(1,Role)==Mesh);
        Check(Host->ResolveVisualAnimation(1,false,Role));Check(Host->ResolveVisualAnimation(1,true,Role));
        Check(Host->ResolveVisualDeath(1,Role));Check(Host->ResolveVisualReaction(1,Role));
        TSet<UAnimationAsset*> Attacks;
        for(int32 V=0;V<3;++V){auto* Clip=Host->ResolveVisualAttack(1,Role,V);Check(Clip);Attacks.Add(Clip);}
        TestEqual(TEXT("three authored hammer attacks"),Attacks.Num(),3);
        for(int32 Mode=0;Mode<6;++Mode)Check(Host->ResolveStanceAnimation(1,Role,Mode));
    }
    auto* Body=World->SpawnActor<ACharacter>();Body->GetMesh()->SetSkeletalMeshAsset(Mesh);
    Host->EquipVisualWeapons(Body,1,ESoulRealtimeFormationRole::Line);
    TInlineComponentArray<UStaticMeshComponent*> Pieces(Body);int32 Weapons=0;
    for(auto* Piece:Pieces)if(Piece->ComponentHasTag(TEXT("SoulVisualWeapon")))
    {
        ++Weapons;TestEqual(TEXT("owned hammer"),GetNameSafe(Piece->GetStaticMesh()),FString(TEXT("SM_Hummer")));
        TestEqual(TEXT("native CAT palm attachment"),Piece->GetAttachSocketName(),FName(TEXT("CATRigRArmPalm")));
        TestTrue(TEXT("CAT authoring scale canceled"),Piece->GetComponentScale().Equals(Body->GetMesh()->GetComponentScale(),.001f));
        TestEqual(TEXT("cosmetic weapon has no independent damage collision"),Piece->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    }
    TestEqual(TEXT("one visible hammer"),Weapons,1);
    FSoulRealtimeArenaCombatant Unit;Unit.Id=FGuid::NewGuid();Unit.Side=1;Unit.Role=ESoulRealtimeFormationRole::Line;Unit.Health=160;Unit.MaxHealth=160;
    Host->Combatants.Add(Unit);
    const auto Weapon=Host->Profile(0);
    TestEqual(TEXT("existing RB weapon profile carries Orc identity"),Weapon.Id,FName(TEXT("Soul.Orc.Hammer")));
    TestEqual(TEXT("hammer category"),Weapon.Category,FName(TEXT("Hammer")));
    TestTrue(TEXT("existing RB physical damage/reach retained"),Weapon.BaseDamage>0&&Weapon.Reach>0);
    Host->EnemyVisualFaction=TEXT("dwarves");Host->EnemyVisualUnitId=TEXT("dwarf_warrior");
    TestFalse(TEXT("Dwarf pair never selects Orc roster"),Host->UsesOrcCampaignRoster(1));
    TestTrue(TEXT("Dwarf mesh differs from Orc"),Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line)!=Mesh);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulVikingCampaignRosterTest,
    "Soul.RealtimeBattle.Vertical.ExactVikingCampaignRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulVikingCampaignRosterTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();
    Host->bVisualUnits=true;
    Host->EnemyVisualFaction=TEXT("vikings");Host->EnemyVisualUnitId=TEXT("viking_axe_warrior");
    TestFalse(TEXT("standalone roster cannot masquerade as exact campaign proof"),Host->UsesVikingCampaignRoster(1));
    Host->bCampaignBattle=true;
    TestTrue(TEXT("exact campaign Viking pair recognized"),Host->UsesVikingCampaignRoster(1));
    TestFalse(TEXT("human side cannot inherit enemy mesh"),Host->UsesVikingCampaignRoster(0));
    auto* Mesh=Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line);
    if(!TestNotNull(TEXT("owned Viking mesh loads"),Mesh)){World->DestroyWorld(false);return false;}
    TestEqual(TEXT("exact owned mesh"),Mesh->GetPathName(),FString(TEXT("/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full.SK_Ulf_Full")));
    auto Check=[&](UAnimationAsset* Asset)
    {
        auto* Clip=Cast<UAnimSequence>(Asset);
        if(!TestNotNull(TEXT("native full-pose sequence"),Clip))return;
        TestTrue(TEXT("strict own-skeleton compatibility"),Clip->GetSkeleton()->IsCompatibleMesh(Mesh,true));
        TestFalse(TEXT("no additive substitution"),Clip->IsValidAdditive());
        TestTrue(TEXT("positive duration"),Clip->GetPlayLength()>0);
        TestTrue(TEXT("clip comes from owned Viking family"),Clip->GetPathName().StartsWith(TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/")));
    };
    for(int32 R=0;R<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++R)
    {
        const auto Role=static_cast<ESoulRealtimeFormationRole>(R);
        TestTrue(TEXT("deployment/reserves admit exact infantry only"),Host->CampaignFormationRole(1,Role)==ESoulRealtimeFormationRole::Line);
        TestTrue(TEXT("player formation roles unchanged"),Host->CampaignFormationRole(0,Role)==Role);
        TestTrue(TEXT("no requested tactical role substitutes another enemy species"),Host->ResolveVisualMesh(1,Role)==Mesh);
        Check(Host->ResolveVisualAnimation(1,false,Role));Check(Host->ResolveVisualAnimation(1,true,Role));
        Check(Host->ResolveVisualDeath(1,Role));Check(Host->ResolveVisualReaction(1,Role));
        TSet<UAnimationAsset*> Attacks;
        for(int32 V=0;V<4;++V){auto* Clip=Host->ResolveVisualAttack(1,Role,V);Check(Clip);Attacks.Add(Clip);}
        TestEqual(TEXT("four authored warrior attacks"),Attacks.Num(),4);
        for(int32 Mode=0;Mode<6;++Mode)Check(Host->ResolveStanceAnimation(1,Role,Mode));
    }
    auto* Body=World->SpawnActor<ACharacter>();Body->GetMesh()->SetSkeletalMeshAsset(Mesh);
    Host->EquipVisualWeapons(Body,1,ESoulRealtimeFormationRole::Line);
    TInlineComponentArray<UStaticMeshComponent*> Pieces(Body);int32 Weapons=0;
    for(auto* Piece:Pieces)if(Piece->ComponentHasTag(TEXT("SoulVisualWeapon")))
    {
        ++Weapons;TestEqual(TEXT("owned Viking axe"),GetNameSafe(Piece->GetStaticMesh()),FString(TEXT("SM_Viking_Axe")));
        TestEqual(TEXT("native right hand attachment"),Piece->GetAttachSocketName(),FName(TEXT("hand_r")));
        TestTrue(TEXT("weapon authoring scale matches body"),Piece->GetComponentScale().Equals(Body->GetMesh()->GetComponentScale(),.001f));
        TestEqual(TEXT("cosmetic weapon has no independent damage collision"),Piece->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    }
    TestEqual(TEXT("one visible axe"),Weapons,1);
    FSoulRealtimeArenaCombatant Unit;Unit.Id=FGuid::NewGuid();Unit.Side=1;Unit.Role=ESoulRealtimeFormationRole::Line;Unit.Health=160;Unit.MaxHealth=160;
    Host->Combatants.Add(Unit);
    const auto Weapon=Host->Profile(0);
    TestEqual(TEXT("existing RB weapon profile carries Viking identity"),Weapon.Id,FName(TEXT("Soul.Viking.Axe")));
    TestEqual(TEXT("native Viking axe category"),Weapon.Category,FName(TEXT("Axe")));
    TestTrue(TEXT("existing RB physical damage/reach retained"),Weapon.BaseDamage>0&&Weapon.Reach>0);
    Host->StrategicBodies[1]=30;Host->ActiveCap=15;Host->SetupReinforcementState();
    bool FoundReserve=false;
    for(const auto& Formation:Host->ReinforcementBattle.Formations)if(Formation.SideId==FName(TEXT("Enemy")))
    {FoundReserve=true;TestEqual(TEXT("reserve keeps exact Viking unit identity"),Formation.UnitId,FName(TEXT("viking_axe_warrior")));}
    TestTrue(TEXT("Viking reserve exists"),FoundReserve);
    Host->EnemyVisualFaction=TEXT("dwarves");Host->EnemyVisualUnitId=TEXT("dwarf_warrior");
    TestFalse(TEXT("Dwarf pair never selects Viking roster"),Host->UsesVikingCampaignRoster(1));
    TestTrue(TEXT("Dwarf mesh differs from Viking"),Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line)!=Mesh);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulNatureCampaignRosterTest,
    "Soul.RealtimeBattle.Vertical.ExactNatureCampaignRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulNatureCampaignRosterTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();
    Host->bVisualUnits=true;
    Host->EnemyVisualFaction=TEXT("nature");Host->EnemyVisualUnitId=TEXT("nature_bear_warrior");
    TestFalse(TEXT("standalone roster cannot masquerade as exact campaign proof"),Host->UsesNatureCampaignRoster(1));
    Host->bCampaignBattle=true;
    TestTrue(TEXT("exact campaign Nature pair recognized"),Host->UsesNatureCampaignRoster(1));
    TestFalse(TEXT("human side cannot inherit enemy mesh"),Host->UsesNatureCampaignRoster(0));
    auto* Mesh=Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line);
    if(!TestNotNull(TEXT("owned Nature mesh loads"),Mesh)){World->DestroyWorld(false);return false;}
    TestEqual(TEXT("exact owned mesh"),Mesh->GetPathName(),FString(TEXT("/Game/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full.SK_Bear_Full")));
    auto Check=[&](UAnimationAsset* Asset)
    {
        auto* Clip=Cast<UAnimSequence>(Asset);
        if(!TestNotNull(TEXT("native full-pose sequence"),Clip))return;
        TestTrue(TEXT("strict own-skeleton compatibility"),Clip->GetSkeleton()->IsCompatibleMesh(Mesh,true));
        TestFalse(TEXT("no additive substitution"),Clip->IsValidAdditive());
        TestTrue(TEXT("positive duration"),Clip->GetPlayLength()>0);
        TestTrue(TEXT("clip comes from owned Nature family"),Clip->GetPathName().StartsWith(TEXT("/Game/Animals_Warrior_Pack/Animations/1With_Weapon/")));
    };
    for(int32 R=0;R<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++R)
    {
        const auto Role=static_cast<ESoulRealtimeFormationRole>(R);
        TestTrue(TEXT("deployment/reserves admit exact infantry only"),Host->CampaignFormationRole(1,Role)==ESoulRealtimeFormationRole::Line);
        TestTrue(TEXT("Nature proof fields exact Human infantry"),Host->CampaignFormationRole(0,Role)==ESoulRealtimeFormationRole::Line);
        TestTrue(TEXT("no requested tactical role substitutes another enemy species"),Host->ResolveVisualMesh(1,Role)==Mesh);
        Check(Host->ResolveVisualAnimation(1,false,Role));Check(Host->ResolveVisualAnimation(1,true,Role));
        Check(Host->ResolveVisualDeath(1,Role));Check(Host->ResolveVisualReaction(1,Role));
        TSet<UAnimationAsset*> Attacks;
        for(int32 V=0;V<4;++V){auto* Clip=Host->ResolveVisualAttack(1,Role,V);Check(Clip);Attacks.Add(Clip);}
        TestEqual(TEXT("four authored warrior attacks"),Attacks.Num(),4);
        for(int32 Mode=0;Mode<6;++Mode)Check(Host->ResolveStanceAnimation(1,Role,Mode));
    }
    auto* Body=World->SpawnActor<ACharacter>();Body->GetMesh()->SetSkeletalMeshAsset(Mesh);
    Host->EquipVisualWeapons(Body,1,ESoulRealtimeFormationRole::Line);
    TInlineComponentArray<UStaticMeshComponent*> Pieces(Body);int32 Weapons=0;
    for(auto* Piece:Pieces)if(Piece->ComponentHasTag(TEXT("SoulVisualWeapon")))
    {
        ++Weapons;TestEqual(TEXT("owned blade (donor package is named Axe)"),GetNameSafe(Piece->GetStaticMesh()),FString(TEXT("SM_Warrior_02_Axe")));
        TestEqual(TEXT("native right hand attachment"),Piece->GetAttachSocketName(),FName(TEXT("hand_r")));
        TestTrue(TEXT("weapon authoring scale matches body"),Piece->GetComponentScale().Equals(Body->GetMesh()->GetComponentScale(),.001f));
        TestEqual(TEXT("cosmetic weapon has no independent damage collision"),Piece->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    }
    TestEqual(TEXT("one visible blade"),Weapons,1);
    FSoulRealtimeArenaCombatant Unit;Unit.Id=FGuid::NewGuid();Unit.Side=1;Unit.Role=ESoulRealtimeFormationRole::Line;Unit.Health=160;Unit.MaxHealth=160;
    Host->Combatants.Add(Unit);
    const auto Weapon=Host->Profile(0);
    TestEqual(TEXT("existing RB weapon profile carries Nature identity"),Weapon.Id,FName(TEXT("Soul.Nature.BearBlade")));
    TestEqual(TEXT("rendered blade category"),Weapon.Category,FName(TEXT("Sword")));
    TestTrue(TEXT("existing RB physical damage/reach retained"),Weapon.BaseDamage>0&&Weapon.Reach>0);
    Host->StrategicBodies[1]=30;Host->ActiveCap=15;Host->SetupReinforcementState();
    bool FoundReserve=false;
    for(const auto& Formation:Host->ReinforcementBattle.Formations)if(Formation.SideId==FName(TEXT("Enemy")))
    {FoundReserve=true;TestEqual(TEXT("reserve keeps exact Nature unit identity"),Formation.UnitId,FName(TEXT("nature_bear_warrior")));}
    TestTrue(TEXT("Nature reserve exists"),FoundReserve);
    Host->EnemyVisualFaction=TEXT("dwarves");Host->EnemyVisualUnitId=TEXT("dwarf_warrior");
    TestFalse(TEXT("Dwarf pair never selects Nature roster"),Host->UsesNatureCampaignRoster(1));
    TestTrue(TEXT("Dwarf mesh differs from Nature"),Host->ResolveVisualMesh(1,ESoulRealtimeFormationRole::Line)!=Mesh);
    World->DestroyWorld(false);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulReverseCampaignRosterTest,"Soul.RealtimeBattle.Vertical.ExactReverseCampaignRosters",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulReverseCampaignRosterTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();Host->bCampaignBattle=true;Host->bVisualUnits=true;
    Host->EnemyVisualFaction=TEXT("humans");Host->EnemyVisualUnitId=TEXT("human_knight");Host->EnemyVisualRegion=TEXT("north_pass");
    const TMap<FName,FName> Units={{TEXT("dwarves"),TEXT("dwarf_warrior")},{TEXT("orcs"),TEXT("orc_hammer_warrior")},{TEXT("vikings"),TEXT("viking_axe_warrior")},{TEXT("nature"),TEXT("nature_bear_warrior")}};
    const TMap<FName,FString> Bodies={{TEXT("dwarves"),TEXT("SK_Dwarf_Bedvar_Full")},{TEXT("orcs"),TEXT("SK_Orc_Hummer")},{TEXT("vikings"),TEXT("SK_Ulf_Full")},{TEXT("nature"),TEXT("SK_Bear_Full")}};
    const TMap<FName,FString> Weapons={{TEXT("dwarves"),TEXT("SM_Dwarf_Bedvar_Hammer_Mesh")},{TEXT("orcs"),TEXT("SM_Hummer")},{TEXT("vikings"),TEXT("SM_Viking_Axe")},{TEXT("nature"),TEXT("SM_Warrior_02_Axe")}};
    for(const auto& Pair:Units)
    {
        Host->PlayerVisualFaction=Pair.Key;Host->PlayerVisualUnitId=Pair.Value;
        TArray<FName> Defenders={TEXT("humans")};
        if(Pair.Key!=TEXT("nature"))for(FName Other:TArray<FName>{TEXT("dwarves"),TEXT("orcs"),TEXT("vikings")})if(Other!=Pair.Key)Defenders.Add(Other);
        for(FName Defender:Defenders)
        {
        Host->EnemyVisualFaction=Defender;Host->EnemyVisualUnitId=Defender==TEXT("humans")?FName(TEXT("human_knight")):Units[Defender];
        const FString DefenderBody=Defender==TEXT("humans")?FString(TEXT("SK_Knight_02_Full_01")):Bodies[Defender];
        const FString DefenderWeapon=Defender==TEXT("humans")?FString(TEXT("SM_Knight_02_Sword")):Weapons[Defender];
        for(int32 Side=0;Side<2;++Side)
        {
            auto* Mesh=Host->ResolveVisualMesh(Side,ESoulRealtimeFormationRole::Line);
            if(!TestNotNull(TEXT("exact reverse body loads"),Mesh))continue;
            TestEqual(TEXT("side-correct body"),Mesh->GetName(),Side==0?Bodies[Pair.Key]:DefenderBody);
            for(int32 R=0;R<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++R)
            {
                const auto Role=static_cast<ESoulRealtimeFormationRole>(R);
                TestTrue(TEXT("reverse proof fields exact infantry"),Host->CampaignFormationRole(Side,Role)==ESoulRealtimeFormationRole::Line);
                TestTrue(TEXT("no side/role substitution"),Host->ResolveVisualMesh(Side,Role)==Mesh);
                TArray<UAnimationAsset*> Clips={Host->ResolveVisualAnimation(Side,false,Role),Host->ResolveVisualAnimation(Side,true,Role),Host->ResolveVisualReaction(Side,Role),Host->ResolveVisualDeath(Side,Role)};
                for(int32 V=0;V<4;++V)Clips.Add(Host->ResolveVisualAttack(Side,Role,V));
                for(int32 M=0;M<6;++M)Clips.Add(Host->ResolveStanceAnimation(Side,Role,M));
                for(auto* Asset:Clips)
                {
                    auto* Clip=Cast<UAnimSequence>(Asset);if(!TestNotNull(TEXT("owned full pose"),Clip))continue;
                    TestTrue(TEXT("compatible exact skeleton"),Clip->GetSkeleton()->IsCompatibleMesh(Mesh,true));
                    TestTrue(TEXT("usable nonadditive animation"),Clip->GetPlayLength()>0&&!Clip->IsValidAdditive());
                }
            }
            auto* Body=World->SpawnActor<ACharacter>();Body->GetMesh()->SetSkeletalMeshAsset(Mesh);Host->EquipVisualWeapons(Body,Side,ESoulRealtimeFormationRole::Line);
            int32 Count=0;TInlineComponentArray<UStaticMeshComponent*> Pieces(Body);
            for(auto* P:Pieces)if(P->ComponentTags.Contains(TEXT("SoulVisualWeapon")))
            {++Count;TestEqual(TEXT("side-correct weapon"),GetNameSafe(P->GetStaticMesh()),Side==0?Weapons[Pair.Key]:DefenderWeapon);TestTrue(TEXT("actual bone attachment"),Mesh->GetRefSkeleton().FindBoneIndex(P->GetAttachSocketName())!=INDEX_NONE);}
            TestEqual(TEXT("one exact weapon"),Count,1);
        }
        Host->StrategicBodies[0]=30;Host->StrategicBodies[1]=45;Host->ActiveCap=15;Host->SetupReinforcementState();
        for(const auto& Formation:Host->ReinforcementBattle.Formations)
            TestEqual(TEXT("reserves retain actual faction unit"),Formation.UnitId,Formation.SideId==TEXT("Human")?Pair.Value:Host->EnemyVisualUnitId);
    }
        }
    World->DestroyWorld(false);return true;
}
#endif
