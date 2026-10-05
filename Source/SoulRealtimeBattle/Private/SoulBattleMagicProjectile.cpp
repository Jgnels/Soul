#include "SoulRealtimeBattleArena.h"
#include "SoulBattleArrow.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

bool ASoulRealtimeArenaGameMode::LaunchMagicProjectile(
    int32 Caster,int32 Target,const FGuid& CastId,float Damage)
{
    if(!Actors.IsValidIndex(Caster) || !Actors[Caster] ||
        !Actors.IsValidIndex(Target) || !Actors[Target] || !CastId.IsValid() ||
        MagicProjectiles.Contains(CastId) || !FMath::IsFinite(Damage) || Damage<=0 || Damage>1000)
        return false;
    FRBProjectileLaunch Launch;
    Launch.ShotId=CastId;
    Launch.Source=IdentityAt(Caster).Core();
    Launch.Weapon=FRBWeaponDefinition::MakeDefault(ERBWeaponStyle::Bow).Profile;
    Launch.Weapon.Id=TEXT("Soul.Magic.Firebolt");
    Launch.Weapon.DamageType=TEXT("Magic");
    Launch.Weapon.BaseDamage=Damage;
    Launch.Position=Actors[Caster]->GetActorLocation()+FVector(0,0,45);
    const FVector Direction=(Actors[Target]->GetActorLocation()-Launch.Position).GetSafeNormal();
    Launch.Position+=Direction*65;
    Launch.Velocity=Direction*1800;
    Launch.Acceleration=FVector::ZeroVector;
    Launch.Damage=Damage;
    Launch.Radius=12;
    Launch.LifetimeSeconds=4;
    if(!Launch.IsValid()) return false;
    auto* Projectile=GetWorld()->SpawnActor<ASoulBattleSpellProjectile>();
    if(!Projectile) return false;
    Projectile->SetMagicOwner(this);
    if(!Projectile->Launch(Launch,Actors[Caster])) { Projectile->Destroy(); return false; }
    MagicProjectiles.Add(CastId,Projectile);
    UE_LOG(LogTemp,Display,TEXT("SOUL_MAGIC_PROJECTILE_LAUNCH: id=%s source=%d target=%d damage=%.1f"),
        *CastId.ToString(),Caster,Target,Damage);
    return true;
}
void ASoulRealtimeArenaGameMode::ResolveMagicProjectile(
    ASoulBattleSpellProjectile* Projectile,const FHitResult& Hit)
{
    if(!Projectile) return;
    const auto& Launch=Projectile->GetLaunchEvidence();
    const auto* Pending=MagicProjectiles.Find(Launch.ShotId);
    if(!Pending || Pending->Get()!=Projectile) return;
    MagicProjectiles.Remove(Launch.ShotId); // A committed payload can resolve only once.
    const int32 Source=Index(FRBHostIdentity::From(Launch.Source));
    const auto* Binding=Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<USoulRealtimeArenaBinding>() : nullptr;
    const int32 Target=Binding ? Index(FRBHostIdentity::From(Binding->GetCombatant())) : INDEX_NONE;
    const bool Enemy=Source!=INDEX_NONE && Target!=INDEX_NONE &&
        Combatants[Source].Side!=Combatants[Target].Side;
    const bool Accepted=Enemy && ApplyMagicDamage(Target,Launch.Damage);
    if(!Accepted && !bFinished && Source!=INDEX_NONE && Combatants[Source].Side==0)
        PushBattleNotice(Enemy ? TEXT("Firebolt hit a fallen target") :
            Target!=INDEX_NONE ? TEXT("Firebolt blocked by friendly troops") : TEXT("Firebolt blocked by terrain"),0);
    UE_LOG(LogTemp,Display,TEXT("SOUL_MAGIC_PROJECTILE_IMPACT: id=%s source=%d target=%d accepted=%d blocker=%s"),
        *Launch.ShotId.ToString(),Source,Target,Accepted,*GetNameSafe(Hit.GetActor()));
    if (Accepted) PlayBattleSound(ESoulBattleSound::MagicImpact, Hit.ImpactPoint);
    if(!bFinished)
        if(auto* Fire=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/MagicSpells/Fire/FX/NS_Fireball.NS_Fireball")))
            if(auto* FX=UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Fire,Hit.ImpactPoint,
                FRotator::ZeroRotator,FVector(0.22f)))
            {
                const TWeakObjectPtr<UNiagaraComponent> Weak(FX);
                FTimerHandle Cleanup;
                GetWorld()->GetTimerManager().SetTimer(Cleanup,[Weak](){if(Weak.IsValid()) Weak->DestroyComponent();},1.2f,false);
            }
}
void ASoulRealtimeArenaGameMode::ForgetMagicProjectile(ASoulBattleSpellProjectile* Projectile)
{
    if(!Projectile) return;
    const FGuid Id=Projectile->GetLaunchEvidence().ShotId;
    if(MagicProjectiles.FindRef(Id).Get()==Projectile) MagicProjectiles.Remove(Id);
}
