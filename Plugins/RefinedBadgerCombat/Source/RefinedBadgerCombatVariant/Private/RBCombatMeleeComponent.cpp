#include "RBCombatMeleeComponent.h"
#include "RBCombatContactNotify.h"
#include "RBVariantCombatBindingComponent.h"
#include "RBCombatRangedComponent.h"
#include "RBCombatProjectile.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

URBCombatMeleeComponent::URBCombatMeleeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

URBVariantCombatBindingComponent* URBCombatMeleeComponent::Binding() const
{ return GetOwner() ? GetOwner()->FindComponentByClass<URBVariantCombatBindingComponent>() : nullptr; }

bool URBCombatMeleeComponent::Configure(URBCombatWeaponData* Data, USkeletalMeshComponent* AnimationMesh, USceneComponent* WeaponGeometry)
{
    if (IsAttacking() || !Data || !Data->IsValidWeapon() || !AnimationMesh || !WeaponGeometry
        || AnimationMesh->GetOwner() != GetOwner() || WeaponGeometry->GetWorld() != GetWorld()) { return false; }
    if (Animator) { RemoveTickPrerequisiteComponent(Animator); }
    Weapon = Data; Animator = AnimationMesh; Geometry = WeaponGeometry;
    RestGripRotation = Geometry->GetRelativeRotation().Quaternion();
    AddTickPrerequisiteComponent(Animator); return true;
}

bool URBCombatMeleeComponent::RequestAttack(ERBWeaponAttack Attack)
{
    LastError.Reset();
    if (IsAttacking())
    {
        if (Attack == ERBWeaponAttack::Throw) { return false; }
        if (Action.GetPhase() != ERBActionPhase::Contact && Action.GetPhase() != ERBActionPhase::Recovery) { return false; }
        bBuffered = true; BufferedAttack = Attack; BufferRemaining = .3f; return true;
    }
    if (Attack == ERBWeaponAttack::Throw) { return RequestThrow(GetOwner()->GetActorForwardVector()); }
    return StartAttack(Attack, 0);
}

bool URBCombatMeleeComponent::RequestThrow(FVector AimDirection)
{
    if (IsAttacking() || AimDirection.ContainsNaN() || AimDirection.IsNearlyZero()) { return false; }
    ThrowAim = AimDirection.GetSafeNormal();
    return StartAttack(ERBWeaponAttack::Throw, 0);
}

bool URBCombatMeleeComponent::StartAttack(ERBWeaponAttack Attack, int32 ComboStage)
{
    auto* Host = Binding();
    auto* Anim = Animator ? Animator->GetAnimInstance() : nullptr;
    const auto* Ranged = GetOwner()->FindComponentByClass<URBCombatRangedComponent>();
    if (Ranged && Ranged->IsDrawing()) { LastError = TEXT("Cancel bow draw before a melee attack."); return false; }
    if (!Weapon || !Weapon->IsValidWeapon() || !Geometry || !Host || !Host->IsAbleToAct() || !Anim || !Host->ResolveEquippedWeapon(AttackWeapon)
        || AttackWeapon.Id != Weapon->WeaponId || Attack == ERBWeaponAttack::Shoot)
    { LastError = TEXT("Melee requires a configured equipped weapon, animator and geometry."); return false; }
    if (Attack == ERBWeaponAttack::Throw && Weapon->Category != FName(TEXT("Spear")) && Weapon->Category != FName(TEXT("Axe")))
    { LastError = TEXT("This weapon is not configured for throwing."); return false; }
    const auto* Entry = Weapon->Attacks.FindByPredicate([Attack, ComboStage](const FRBWeaponAnimation& A) { return A.Attack == Attack && A.ComboStage == ComboStage; });
    if (!Entry && ComboStage > 0)
    { ComboStage = 0; Entry = Weapon->Attacks.FindByPredicate([Attack](const FRBWeaponAnimation& A) { return A.Attack == Attack && A.ComboStage == 0; }); }
    if (!Entry) { LastError = TEXT("Weapon has no authored attack of this type."); return false; }
    const bool bHasContact = Entry->Montage->Notifies.ContainsByPredicate([](const FAnimNotifyEvent& N)
    { return N.NotifyStateClass && N.NotifyStateClass->IsA<URBCombatContactNotify>(); });
    if (!bHasContact) { LastError = TEXT("Attack montage needs an RB Combat Contact notify state."); return false; }
    ActiveBladeStart = Weapon->LocalBladeStart; ActiveBladeEnd = Weapon->LocalBladeEnd;
    ActiveSweepRadius = Weapon->SweepRadius; ActiveThrowSpeed = Weapon->ThrowSpeed;
    ActiveProjectileRadius = Weapon->ProjectileRadius; ActiveProjectileClass = ThrownProjectileClass;
    const FGuid Id = Host->BeginNativeAttackContact();
    ActiveDefinition = Entry->Definition();
    if (!Action.Begin(ActiveDefinition, Id)) { Host->EndNativeAttackContact(); return false; }
    PlayingMontage = Entry->Montage;
    Geometry->SetRelativeRotation(RestGripRotation * Entry->GripRotation.Quaternion());
    if (Anim->Montage_Play(PlayingMontage) <= 0)
    { InterruptAttack(); LastError = TEXT("Attack montage failed to play."); return false; }
    const auto* Instance = Anim->GetActiveInstanceForMontage(PlayingMontage);
    ActiveMontageId = Instance ? Instance->GetInstanceID() : INDEX_NONE;
    if (ActiveMontageId == INDEX_NONE || !Host->TrySpendResources(AttackWeapon.Id, NAME_None, 0, ActiveDefinition.Effort, LastError))
    { InterruptAttack(); return false; }
    // Snapshot data for this action; later loadout edits cannot rewrite a swing.
    ActiveAttack = Attack; ActiveComboStage = ComboStage;
    ContactedActors.Reset(); SetComponentTickEnabled(true); return true;
}

bool URBCombatMeleeComponent::ReleaseThrownWeapon()
{
    auto* Host = Binding();
    if (!Host || !Geometry || !GetWorld()) { return false; }
    FRBProjectileLaunch Launch;
    Launch.ShotId = Action.GetActionId(); Launch.Source = Host->GetCombatant(); Launch.Weapon = AttackWeapon;
    Launch.Position = Geometry->GetComponentLocation(); Launch.Velocity = ThrowAim * ActiveThrowSpeed;
    Launch.Damage = AttackWeapon.BaseDamage * ActiveDefinition.DamageScale; Launch.Radius = ActiveProjectileRadius;
    FActorSpawnParameters Spawn;
    Spawn.Owner = GetOwner(); Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Projectile = GetWorld()->SpawnActor<ARBCombatProjectile>(ActiveProjectileClass ? ActiveProjectileClass.Get() : ARBCombatProjectile::StaticClass(), Launch.Position, ThrowAim.Rotation(), Spawn);
    if (!Projectile) { LastError = TEXT("Thrown projectile could not spawn."); return false; }
    if (!Projectile->Launch(Launch, GetOwner()) || !Host->TrySpendResources(AttackWeapon.Id, AttackWeapon.Id, 1, 0, LastError))
    { Projectile->Destroy(); return false; }
    PresentThrownWeapon(Projectile);
    return true;
}

void URBCombatMeleeComponent::InterruptAttack()
{
    if (Geometry) { Geometry->SetRelativeRotation(RestGripRotation); }
    Action.Interrupt(.25f); ActiveMontageId = INDEX_NONE; bBuffered = false;
    if (auto* Host = Binding()) { Host->EndNativeAttackContact(); }
    if (Animator && Animator->GetAnimInstance() && PlayingMontage)
    { Animator->GetAnimInstance()->Montage_Stop(.1f, PlayingMontage); }
    SetComponentTickEnabled(true);
}

void URBCombatMeleeComponent::OpenContact(int32 MontageInstanceId)
{
    if (MontageInstanceId != ActiveMontageId || !Geometry || !Action.OpenContact(Action.GetActionId())) { return; }
    if (ActiveAttack == ERBWeaponAttack::Throw)
    { ReleaseThrownWeapon(); Action.CloseContact(Action.GetActionId()); return; }
    PreviousStart = Geometry->GetComponentTransform().TransformPosition(ActiveBladeStart);
    PreviousEnd = Geometry->GetComponentTransform().TransformPosition(ActiveBladeEnd);
    SweepContact(MontageInstanceId);
}

void URBCombatMeleeComponent::CloseContact(int32 MontageInstanceId)
{
    if (MontageInstanceId == ActiveMontageId) { Action.CloseContact(Action.GetActionId()); }
}

void URBCombatMeleeComponent::SweepContact(int32 MontageInstanceId)
{
    auto* Host = Binding();
    if (MontageInstanceId != ActiveMontageId || Action.GetPhase() != ERBActionPhase::Contact || !Geometry || !Host || !GetWorld()) { return; }
    if (!Host->IsAbleToAct()) { InterruptAttack(); return; }
    const FVector Start = Geometry->GetComponentTransform().TransformPosition(ActiveBladeStart);
    const FVector End = Geometry->GetComponentTransform().TransformPosition(ActiveBladeEnd);
    if (Start.ContainsNaN() || End.ContainsNaN()) { InterruptAttack(); return; }
    const int32 Samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist(Start, End) / (ActiveSweepRadius * 2)), 1, 32);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RBMelee), false, GetOwner());
    if (Geometry->GetOwner() != GetOwner()) { Query.AddIgnoredActor(Geometry->GetOwner()); }
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / Samples;
        const FVector From = FMath::Lerp(PreviousStart, PreviousEnd, Alpha);
        const FVector To = FMath::Lerp(Start, End, Alpha);
        FHitResult Hit;
        if (!GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, TraceChannel,
            FCollisionShape::MakeSphere(ActiveSweepRadius), Query)) { continue; }
        AActor* Victim = Hit.GetActor();
        if (!IsValid(Victim) || ContactedActors.Contains(Victim)) { continue; }
        ContactedActors.Add(Victim);
        auto* Receiver = Victim->FindComponentByClass<URBVariantCombatBindingComponent>();
        if (!Receiver) { continue; }
        FRBCombatHit Evidence;
        Evidence.ContactId = Action.GetActionId(); Evidence.Attacker = Host->GetCombatant(); Evidence.Victim = Receiver->GetCombatant();
        Evidence.Weapon = AttackWeapon; Evidence.AcceptedDamage = AttackWeapon.BaseDamage * ActiveDefinition.DamageScale;
        Evidence.Bleeding = AttackWeapon.BleedingTendency;
        Evidence.StaggerSeconds = AttackWeapon.StaggerSeconds;
        Evidence.ImpactPoint = Hit.ImpactPoint; Evidence.bHasExactImpact = true;
        FVector Travel = To - From;
        if (Travel.IsNearlyZero()) { Travel = Victim->GetActorLocation() - GetOwner()->GetActorLocation(); }
        Receiver->ReceiveProducedImpact(Evidence, Hit, Travel, GetOwner(), LastError);
    }
    PreviousStart = Start; PreviousEnd = End;
}

void URBCombatMeleeComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    Action.Advance(DeltaSeconds);
    if (bBuffered) { BufferRemaining -= DeltaSeconds; if (BufferRemaining < 0) { bBuffered = false; } }
    if (Action.GetPhase() == ERBActionPhase::Ready)
    {
        if (Geometry) { Geometry->SetRelativeRotation(RestGripRotation); }
        if (auto* Host = Binding()) { Host->EndNativeAttackContact(); }
        ActiveMontageId = INDEX_NONE; SetComponentTickEnabled(false);
        if (bBuffered)
        {
            const auto Next = BufferedAttack;
            const int32 NextStage = Next == ActiveAttack ? ActiveComboStage + 1 : 0;
            bBuffered = false; StartAttack(Next, NextStage);
        }
    }
}

void URBCombatMeleeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(Geometry)) { Geometry->SetRelativeRotation(RestGripRotation); }
    Super::EndPlay(EndPlayReason);
}
