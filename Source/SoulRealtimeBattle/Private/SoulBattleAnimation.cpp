#include "SoulRealtimeBattleArena.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

FRotator ASoulRealtimeArenaGameMode::VisualMeshRotation(ESoulRealtimeMovementArchetype Movement)
{
    // The sampled griffin/dragon clips, like the humanoids and elephant, face
    // mesh +Y. RB orders and Character movement use actor +X. Low-profile donors
    // already face +X. This correction never rotates the authoritative capsule.
    return Movement == ESoulRealtimeMovementArchetype::LowProfile
        ? FRotator::ZeroRotator : FRotator(0, -90, 0);
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveVisualReaction(int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))return NatureInfantryAnimation(TEXT("Get_Hit_1"));
    if(UsesVikingCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Get_Hit_1.Anim_Warrior_Get_Hit_1"));
    if(UsesOrcCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Get_hit.Anim_Orc_Hummer_Get_hit"));
    const TCHAR* Path=nullptr;
    if(FormationRole==ESoulRealtimeFormationRole::Ranged) Path=TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Stunned_Start.Stunned_Start");
    else if(FormationRole==ESoulRealtimeFormationRole::Apex && Side==0) Path=TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_FlyGetHitStationary.ANIM_Griffon_FlyGetHitStationary");
    else if(FormationRole==ESoulRealtimeFormationRole::Apex) Path=TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_FlyStationaryGetHit.ANIM_MOUNTAIN_DRAGON_FlyStationaryGetHit");
    else if(FormationRole==ESoulRealtimeFormationRole::Hero && CampaignVisualSide(Side)==0) Path=TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Stun_Start.Stun_Start");
    else if(FormationRole==ESoulRealtimeFormationRole::Breaker && Side==0) Path=TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_GetHit.ANIM_Elephant_GetHit");
    else if(FormationRole==ESoulRealtimeFormationRole::Breaker && UsesEvilVisualRoster()) Path=TEXT("/Game/Kraken/Animations/KRAKEN_getHit.KRAKEN_getHit");
    else if(Side==0 || !UsesEvilVisualRoster()) Path=TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Get_Hit_1.Anim_Warrior_Get_Hit_1");
    else if(FormationRole==ESoulRealtimeFormationRole::Guard || FormationRole==ESoulRealtimeFormationRole::Hero) Path=TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Get_hit.Anim_Orc_Hummer_Get_hit");
    else Path=TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Get_Hit_1.Anim_Warrior_Get_Hit_1");
    return LoadObject<UAnimationAsset>(nullptr,Path);
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveAerialFall(int32 Side) const
{
    return LoadObject<UAnimationAsset>(nullptr,Side==0 ? TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_Falling.ANIM_Griffon_Falling") : TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_falling.ANIM_MOUNTAIN_DRAGON_falling"));
}

// 0 idle, 1 forward, 2 backward, 3 left, 4 right, 5 braced crouch.
// Clips remain on their donor skeleton; no animation-driven combat outcomes.
UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveStanceAnimation(int32 Side, ESoulRealtimeFormationRole FormationRole, int32 Mode) const
{
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))
    {
        static const TCHAR* Clips[]={TEXT("Idle"),TEXT("Run"),TEXT("Run_Back"),TEXT("Run_Left"),TEXT("Run_Right"),TEXT("Idle_Sit")};
        return NatureInfantryAnimation(Clips[FMath::Clamp(Mode,0,5)]);
    }
    if(UsesVikingCampaignRoster(Side))
    {
        if(Mode==0)return ResolveVisualAnimation(Side,false,FormationRole);
        static const TCHAR* Clips[]={TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Back.Anim_Warrior_Run_Back"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Left.Anim_Warrior_Run_Left"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Right.Anim_Warrior_Run_Right"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Idle_Sit.Anim_Warrior_Idle_Sit")};
        return LoadObject<UAnimationAsset>(nullptr,Clips[FMath::Clamp(Mode-1,0,4)]);
    }
    if(UsesOrcCampaignRoster(Side))return ResolveVisualAnimation(Side,Mode!=0&&Mode!=5,FormationRole);
    if(Mode==0) return ResolveVisualAnimation(Side,false,FormationRole);
    if(FormationRole==ESoulRealtimeFormationRole::Apex || FormationRole==ESoulRealtimeFormationRole::Breaker)
        return ResolveVisualAnimation(Side,Mode!=5,FormationRole);
    if(FormationRole==ESoulRealtimeFormationRole::Ranged)
    {
        static const TCHAR* Clips[]={TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Jog_Fwd.Jog_Fwd"),TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Jog_Bwd.Jog_Bwd"),TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Jog_Left.Jog_Left"),TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Jog_Right.Jog_Right")};
        return Mode==5 ? ResolveVisualAnimation(Side,false,FormationRole) : LoadObject<UAnimationAsset>(nullptr,Clips[FMath::Clamp(Mode-1,0,3)]);
    }
    if(CampaignVisualSide(Side)==0 && FormationRole==ESoulRealtimeFormationRole::Hero)
    {
        static const TCHAR* Clips[]={TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd_Combat.Jog_Fwd_Combat"),TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Bwd_Combat.Jog_Bwd_Combat"),TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Left_Combat.Jog_Left_Combat"),TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Right_Combat.Jog_Right_Combat")};
        return Mode==5 ? ResolveVisualAnimation(Side,false,FormationRole) : LoadObject<UAnimationAsset>(nullptr,Clips[FMath::Clamp(Mode-1,0,3)]);
    }
    if(Side==1 && UsesEvilVisualRoster() && (FormationRole==ESoulRealtimeFormationRole::Guard || FormationRole==ESoulRealtimeFormationRole::Hero))
        return ResolveVisualAnimation(Side,Mode!=5,FormationRole);
    static const TCHAR* Human[]={TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Back.Anim_Warrior_Run_Back"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Left.Anim_Warrior_Run_Left"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Right.Anim_Warrior_Run_Right"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Idle_Sit.Anim_Warrior_Idle_Sit")};
    static const TCHAR* Evil[]={TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Back.Anim_Warrior_Run_Back"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Left.Anim_Warrior_Run_Left"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run_Right.Anim_Warrior_Run_Right"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Idle_Sit.Anim_Warrior_Idle_Sit")};
    return LoadObject<UAnimationAsset>(nullptr,(Side==0 || !UsesEvilVisualRoster() ? Human : Evil)[FMath::Clamp(Mode-1,0,4)]);
}

void ASoulRealtimeArenaGameMode::PlayAcceptedHitReaction(int32 Victim)
{
    if(!bVisualUnits || !Combatants.IsValidIndex(Victim) || !Actors.IsValidIndex(Victim) || !Actors[Victim]) return;
    auto& Unit=Combatants[Victim];
    // Reactions cannot cancel an attack wind-up or spell, fabricate damage, or stun-lock input.
    if(Unit.Health<=0 || Unit.VisualReactionCooldown>0 || Unit.bVisualAttackPlaying || Unit.PendingMeleeSeconds>0) return;
    if(auto* Clip=ResolveVisualReaction(Unit.Side,Unit.Role))
    {
        Actors[Victim]->GetMesh()->PlayAnimation(Clip,false);
        Actors[Victim]->GetMesh()->SetPlayRate(Clip->GetPlayLength()/FMath::Clamp(Clip->GetPlayLength(),.25f,.65f));
        Unit.bVisualAttackPlaying=true;
        Unit.VisualReactionCooldown=1.2f;
        UE_LOG(LogTemp,Display,TEXT("SOUL_HIT_REACTION: unit=%d clip=%s"),Victim,*Clip->GetName());
    }
}

void ASoulRealtimeArenaGameMode::TickCombatPresentation(float Seconds)
{
    if(Seconds<=0) return;
    GuardPresentationElapsed+=Seconds;
    const bool UpdateGuard=GuardPresentationElapsed>=.2f;
    if(UpdateGuard) GuardPresentationElapsed=0.f;
    // One threat query per eligible group, shared by its soldiers. Converting a
    // group also copies its members, so neither operation belongs per fighter.
    TMap<int32,bool> GroupBrace;
    if(UpdateGuard) GuardPresentationQueries=0;
    for(int32 I=0;I<Combatants.Num();++I)
    {
        auto& Unit=Combatants[I];
        if ((Unit.Health <= 0 || bFinished) && Unit.DragonBreath.IsValid())
            Unit.DragonBreath->DestroyComponent();
        Unit.VisualReactionCooldown=FMath::Max(0.f,Unit.VisualReactionCooldown-Seconds);
        if(!Actors.IsValidIndex(I) || !Actors[I]) continue;
        auto* Actor=Actors[I].Get();
        if(Unit.Health<=0 && Unit.AerialDeathSeconds>=0)
        {
            Unit.AerialDeathSeconds+=Seconds;
            const float Alpha=FMath::Clamp(Unit.AerialDeathSeconds/1.1f,0.f,1.f);
            FVector P=Actor->GetMesh()->GetRelativeLocation();
            P.Z=FMath::Lerp(Unit.AerialDeathStartZ,-Actor->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(),Alpha*Alpha);
            Actor->GetMesh()->SetRelativeLocation(P);
            if(Alpha>=1)
            {
                if(auto* Clip=ResolveVisualDeath(Unit.Side,Unit.Role))
                {
                    Actor->GetMesh()->PlayAnimation(Clip,false);
                    Actor->GetMesh()->SetPlayRate(1.f);
                }
                Unit.AerialDeathSeconds=-1.f;
                UE_LOG(LogTemp,Display,TEXT("SOUL_AERIAL_LANDED: unit=%d meshZ=%.1f"),I,P.Z);
            }
        }
        if(!UpdateGuard || Unit.bPlayerHero || !Bindings.IsValidIndex(I) || !Bindings[I]) continue;
        bool Brace=false;
        if(!bFinished && Unit.Health>0 && !Unit.bVisualAttackPlaying &&
            (Unit.Role==ESoulRealtimeFormationRole::Line || Unit.Role==ESoulRealtimeFormationRole::Guard) &&
            Groups.IsValidIndex(Unit.GroupIndex))
        {
            if(const bool* Cached=GroupBrace.Find(Unit.GroupIndex)) Brace=*Cached;
            else
            {
                FRBCombatGroup Core;
                if(Groups[Unit.GroupIndex].ToCore(Core) &&
                    (Core.Command==ERBGroupCommand::Hold || Core.Command==ERBGroupCommand::Face))
                {
                    float Distance=0;
                    ++GuardPresentationQueries;
                    Brace=FindNearestEnemyToGroup(Unit.GroupIndex,Distance)!=INDEX_NONE && Distance<700.f;
                }
                GroupBrace.Add(Unit.GroupIndex,Brace);
            }
        }
        Bindings[I]->SetGuardIntent(Brace);
    }
}

void ASoulRealtimeArenaGameMode::PlayDragonBreath(int32 Attacker, int32 Target)
{
    if (!bVisualUnits || bFinished || bBattlePaused || !Combatants.IsValidIndex(Attacker) ||
        !Combatants.IsValidIndex(Target) || !Actors.IsValidIndex(Attacker) || !Actors[Attacker] ||
        !Actors.IsValidIndex(Target) || !Actors[Target]) return;
    auto& Unit = Combatants[Attacker];
    if (Unit.Side != 1 || Unit.Role != ESoulRealtimeFormationRole::Apex || Unit.Health <= 0 ||
        Combatants[Target].Side == Unit.Side || Combatants[Target].Health <= 0) return;
    auto* Mesh = Actors[Attacker]->GetMesh();
    const FName Head(TEXT("MOUNTAIN_DRAGON_-Head"));
    if (!Mesh->DoesSocketExist(Head)) return;
    auto* Effect = LoadObject<UParticleSystem>(nullptr,
        TEXT("/Game/ParagonIggyScorch/FX/Particles/IggyScorch/Abilities/Turret/FX/P_IggyScorch_Turret_Flamethrower"));
    if (!Effect) return;
    // Present the already accepted attack wind-up. RB Combat still decides the
    // single contact: this effect has no collision, damage, targeting or tick search.
    if (Unit.DragonBreath.IsValid()) Unit.DragonBreath->DestroyComponent();
    const FVector Mouth = Mesh->GetSocketTransform(Head).TransformPosition(FVector(0, -85, 0));
    const FVector Aim = Actors[Target]->GetActorLocation() - Mouth;
    auto* Flame = NewObject<UParticleSystemComponent>(Actors[Attacker]);
    Actors[Attacker]->AddInstanceComponent(Flame);
    Flame->SetAutoActivate(false);
    Flame->SetTemplate(Effect);
    Flame->SetupAttachment(Mesh, Head);
    Flame->SetAbsolute(false, false, true);
    Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Flame->SetCanEverAffectNavigation(false);
    Flame->bOverrideLODMethod = true;
    Flame->LODMethod = PARTICLESYSTEMLODMETHOD_DirectSet;
    Flame->SecondsBeforeInactive = 0;
    Flame->RegisterComponent();
    Flame->SetWorldLocationAndRotation(Mouth, Aim.Rotation());
    // The owned emitter is a ~300 cm velocity-aligned sprite, not a 10 m jet.
    // Keep the visible burst near the accepted contact without inheriting rig scale.
    Flame->SetWorldScale3D(FVector(FMath::Clamp(float(Aim.Size() / 300.0), 1.f, 2.5f)));
    Flame->SetLODLevel(0);
    Flame->ActivateSystem(true);
    Unit.DragonBreath = Flame;
    const TWeakObjectPtr<UParticleSystemComponent> Weak(Flame);
    FTimerHandle Stop, Cleanup;
    GetWorld()->GetTimerManager().SetTimer(Stop, [Weak]()
    {
        if (Weak.IsValid()) Weak->DeactivateSystem();
    }, .65f, false);
    GetWorld()->GetTimerManager().SetTimer(Cleanup, [Weak]()
    {
        if (Weak.IsValid()) Weak->DestroyComponent();
    }, 1.2f, false);
    if (bControlDiagnostics)
    {
        UE_LOG(LogTemp, Display, TEXT("SOUL_DRAGON_BREATH: unit=%d target=%d mouth=%s scale=%.2f bounded=1.2"),
            Attacker, Target, *Mouth.ToCompactString(), Flame->GetComponentScale().X);
        FTimerHandle Receipt;
        const int32 Sequence = Unit.VisualAttackSequence;
        const bool Capture = FParse::Param(FCommandLine::Get(), TEXT("SoulDragonBreathProof")) && Sequence <= 3;
        if (Capture && bTacticalCameraActive)
        {
            const FVector PreviousFocus = TacticalFocus;
            const FRotator PreviousRotation = TacticalRotation;
            const float PreviousDistance = TacticalDistance;
            TacticalFocus = (Mouth + Actors[Target]->GetActorLocation()) * .5;
            TacticalRotation = FRotator(-20, Actors[Attacker]->GetActorRotation().Yaw + 140, 0);
            TacticalDistance = 1500;
            const TWeakObjectPtr<ASoulRealtimeArenaGameMode> WeakHost(this);
            FTimerHandle RestoreView;
            GetWorld()->GetTimerManager().SetTimer(RestoreView, [WeakHost, PreviousFocus, PreviousRotation, PreviousDistance]()
            {
                if (!WeakHost.IsValid()) return;
                WeakHost->TacticalFocus = PreviousFocus;
                WeakHost->TacticalRotation = PreviousRotation;
                WeakHost->TacticalDistance = PreviousDistance;
            }, .8f, false);
        }
        GetWorld()->GetTimerManager().SetTimer(Receipt, [Weak, Attacker, Sequence, Capture]()
        {
            if (!Weak.IsValid()) return;
            UE_LOG(LogTemp, Display, TEXT("SOUL_DRAGON_BREATH_LIVE: unit=%d particles=%d active=%d"),
                Attacker, Weak->GetNumActiveParticles(), Weak->IsActive());
            if (Capture && !FScreenshotRequest::IsScreenshotRequested())
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() /
                    FString::Printf(TEXT("Screenshots/Readability/dragon-breath-%d-%d.png"), Attacker, Sequence), false, false);
        }, .45f, false);
    }
}

void ASoulRealtimeArenaGameMode::UpdateVisualAnimations()
{
    if(!bVisualUnits) return;
    TMap<int32,FRBCombatGroup> PresentationGroups;
    for(int32 G=0;G<Groups.Num();++G)
    {
        FRBCombatGroup Core;
        if(Groups[G].ToCore(Core)) PresentationGroups.Add(G,MoveTemp(Core));
    }
    for(int32 I=0;I<Actors.Num();++I)
    {
        if(!Actors[I] || !Combatants.IsValidIndex(I) || Combatants[I].Health<=0 || !VisualRunning.IsValidIndex(I)) continue;
        auto& Unit=Combatants[I];
        auto* Actor=Actors[I].Get();
        bool Resume=false;
        if(Unit.bVisualAttackPlaying)
        {
            const auto* Playback=Actor->GetMesh()->GetSingleNodeInstance();
            if(Playback && Playback->IsPlaying()) continue;
            Unit.bVisualAttackPlaying=false;
            Resume=true;
        }
        const FVector Velocity=Actor->GetVelocity().GetSafeNormal2D();
        const bool Moving=Actor->GetVelocity().SizeSquared2D()>FMath::Square(12.f);
        const FRBCombatGroup* Core=PresentationGroups.Find(Unit.GroupIndex);
        const bool Retreat=Core && Core->Command==ERBGroupCommand::FallBack;
        const bool FaceOrder=Core && (Core->Command==ERBGroupCommand::Hold || Core->Command==ERBGroupCommand::Face);
        const bool Directional=Unit.Movement==ESoulRealtimeMovementArchetype::Infantry &&
            Unit.Role!=ESoulRealtimeFormationRole::Breaker &&
            !(Unit.Side==1 && UsesEvilVisualRoster() && (Unit.Role==ESoulRealtimeFormationRole::Guard || Unit.Role==ESoulRealtimeFormationRole::Hero));
        if(!Unit.bPlayerHero)
        {
            if(Core && !Core->Facing.IsNearlyZero() && ((FaceOrder && (!Moving || Directional)) || (Retreat && Directional)))
                Actor->SetActorRotation(Core->Facing.Rotation());
            else if(Moving) Actor->SetActorRotation(Velocity.Rotation());
        }
        int32 Mode=Moving?1:0;
        if(Moving && Directional)
        {
            const float Forward=FVector::DotProduct(Velocity,Actor->GetActorForwardVector());
            const float Right=FVector::DotProduct(Velocity,Actor->GetActorRightVector());
            Mode=FMath::Abs(Right)>.7f ? (Right>0?4:3) : Forward<-.25f ? 2 : 1;
        }
        if(!Moving && Bindings.IsValidIndex(I) && Bindings[I] && Bindings[I]->IsGuardRequested()) Mode=5;
        if(Resume || Unit.VisualLocomotionMode!=Mode)
        {
            if(auto* Clip=ResolveStanceAnimation(Unit.Side,Unit.Role,Mode))
            {
                Actor->GetMesh()->PlayAnimation(Clip,true);
                Actor->GetMesh()->SetPosition(Clip->GetPlayLength()*FMath::Frac((I+1)*.618034f));
                if(Mode==5 || Mode==2)
                    UE_LOG(LogTemp,Display,TEXT("SOUL_STANCE: unit=%d mode=%d clip=%s"),I,Mode,*Clip->GetName());
                Unit.VisualLocomotionMode=Mode;
                VisualRunning[I]=Moving;
            }
        }
        const float Speed=Moving ? FMath::Clamp(Actor->GetVelocity().Size2D()/FMath::Max(1.f,Unit.BaseWalkSpeed),.65f,1.3f) : 1.f;
        Actor->GetMesh()->SetPlayRate(Speed*(.97f+.015f*(I%5)));
    }
}

float ASoulRealtimeArenaGameMode::FieldStrengthEstimate(int32 Side) const
{
    float Strength=0;
    for(const auto& Unit:Combatants)
    {
        if(Unit.Side!=Side || Unit.Health<=0) continue;
        const int32 State=FindFormationState(Unit.GroupIndex);
        const bool Routing=TacticalFormations.IsValidIndex(State) && TacticalFormations[State].bRouting;
        Strength+=Unit.Health*(Routing?.2f:1.f);
    }
    return Strength;
}
