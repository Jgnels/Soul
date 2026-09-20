#include "RBVariantCombatBindingComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "RBCombatMeleeComponent.h"
#include "RBCombatRangedComponent.h"
#include "UObject/UnrealType.h"

bool URBVariantCombatBindingComponent::IsPerformingAttack() const
{
    if (Router.GetActiveContact().IsValid()) { return true; }
    // The ordinary Epic observer supports native attacks without explicit
    // contact lifecycle calls; honor that Blueprint's attack state as well.
    const auto* NativeAttacking = FindFProperty<FBoolProperty>(GetOwner()->GetClass(), TEXT("Is Attacking"));
    if (NativeAttacking && NativeAttacking->GetPropertyValue_InContainer(GetOwner())) { return true; }
    const auto* Melee = GetOwner()->FindComponentByClass<URBCombatMeleeComponent>();
    const auto* Ranged = GetOwner()->FindComponentByClass<URBCombatRangedComponent>();
    return (Melee && Melee->IsAttacking()) || (Ranged && Ranged->IsDrawing());
}

bool URBVariantCombatBindingComponent::ConfigureGuard(const FRBGuardPolicy& Policy, float EffortPerBlock)
{
    if (!Policy.IsValid() || !FMath::IsFinite(EffortPerBlock) || EffortPerBlock < 0 || EffortPerBlock > 100) { return false; }
    GuardPolicy = Policy; GuardEffort = EffortPerBlock; return true;
}

bool URBVariantCombatBindingComponent::ReceiveProducedImpact(const FRBCombatHit& Evidence,
    const FHitResult& Impact, const FVector& Direction, AActor* Producer, FString& OutError)
{
    if (bApplyingProducedImpact) { OutError = TEXT("Reentrant physical producer rejected."); return false; }
    TGuardValue<bool> ApplyingScope(bApplyingProducedImpact, true);
    IRBCombatConsequenceSink* Sink = GetConsequenceSink();
    if (PendingImpact || !HasBegunPlay() || !Evidence.IsValid() || Evidence.Victim != Combatant
        || !HasValidCombatant() || !CanReceiveCombatDamage() || !Sink || !Sink->IsCombatantValid(Evidence.Attacker)
        || Impact.GetActor() != GetOwner() || !IsValid(Producer) || Producer->GetWorld() != GetWorld()
        || Direction.ContainsNaN() || Direction.IsNearlyZero() || ProducedAttempts.Contains(Evidence.ContactId))
    { OutError = TEXT("Invalid, repeated or reentrant physical impact."); return false; }
    for (const auto& Witness : Evidence.Witnesses)
    { if (!Sink->IsCombatantValid(Witness)) { OutError = TEXT("Unresolved physical witness."); return false; } }
    // Do not replay native health mutation if a host later rejects its consequence.
    // This bounded transient ledger is separate from durable host history.
    if (ProducedAttempts.Num() == FRBCombatRouter::Capacity) { ProducedAttempts.RemoveAt(0, 1, EAllowShrinking::No); }
    ProducedAttempts.Add(Evidence.ContactId);
    FRBCombatHit EffectiveEvidence = Evidence;
    // Incoming travel direction is the opposite of the direction toward its
    // source. This is physical producer evidence, not controller attribution.
    FRBGuardContext Guard;
    Guard.bRequested = bGuardRequested; Guard.bEquipmentEligible = HasEligibleGuardEquipment();
    Guard.bCanParticipate = CanParticipateInCombat(); Guard.bAttacking = IsPerformingAttack();
    Guard.Facing = GetOwner()->GetActorForwardVector(); Guard.ToSource = -Direction;
    LastGuardResult = RBEvaluateGuard(Evidence.AcceptedDamage, GuardPolicy, Guard);
    TGuardValue<const FRBCombatHit*> Scope(PendingImpact, &EffectiveEvidence);
    TGuardValue<AActor*> ProducerScope(PendingProducer, Producer);
    bPendingObserved = false; bPendingAccepted = false; LastError.Reset();
    if (LastGuardResult.IsBlocked() && GuardEffort > 0)
    {
        FRBWeaponProfile Equipped;
        if (!ResolveEquippedWeapon(Equipped) || !TrySpendResources(Equipped.Id, NAME_None, 0, GuardEffort, LastError))
        {
            LastGuardResult.Outcome = ERBGuardOutcome::InsufficientEffort;
            LastGuardResult.EffectiveDamage = Evidence.AcceptedDamage;
        }
    }
    EffectiveEvidence.AcceptedDamage = LastGuardResult.EffectiveDamage;
    // Profile-authored blood loss follows the fraction that actually penetrates
    // this producer's guard. The frozen external native observer stays unchanged.
    EffectiveEvidence.Bleeding *= EffectiveEvidence.AcceptedDamage / Evidence.AcceptedDamage;
    LastError.Reset();
    if (EffectiveEvidence.AcceptedDamage == 0)
    {
        PresentGuardResult(true, Evidence.AcceptedDamage, 0);
        OutError.Reset(); return true;
    }
    UGameplayStatics::ApplyPointDamage(GetOwner(), EffectiveEvidence.AcceptedDamage, Direction.GetSafeNormal(),
        Impact, Producer->GetInstigatorController(), Producer, UDamageType::StaticClass());
    if (!bPendingObserved) { LastError = TEXT("Physical impact produced no native accepted-damage callback."); }
    PresentGuardResult(LastGuardResult.IsBlocked(), Evidence.AcceptedDamage, EffectiveEvidence.AcceptedDamage);
    OutError = LastError;
    return bPendingAccepted;
}

URBVariantCombatBindingComponent::URBVariantCombatBindingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBVariantCombatBindingComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    { Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &URBVariantCombatBindingComponent::HandleOwnerAnyDamage); }
}

void URBVariantCombatBindingComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (AActor* Owner = GetOwner())
    { Owner->OnTakeAnyDamage.RemoveDynamic(this, &URBVariantCombatBindingComponent::HandleOwnerAnyDamage); }
    Super::EndPlay(Reason);
}

bool URBVariantCombatBindingComponent::HasValidCombatant() const
{
    const IRBCombatConsequenceSink* Sink = GetConsequenceSink();
    return Combatant.IsValid() && Sink && Sink->IsCombatantValid(Combatant);
}

bool URBVariantCombatBindingComponent::ResolveEquippedWeapon(FRBWeaponProfile& OutProfile) const
{
    const auto* Provider = GetWeaponProvider();
    return HasValidCombatant() && Provider && Provider->ResolveWeapon(Combatant, OutProfile) && OutProfile.IsValid();
}

bool URBVariantCombatBindingComponent::TrySpendResources(FName WeaponProfileId, FName ConsumedItem,
    int32 Quantity, float Effort, FString& OutError)
{
    auto* Authority = GetResourceAuthority();
    FRBWeaponProfile Equipped;
    const auto* Provider = GetWeaponProvider();
    if (!HasValidCombatant() || !Authority || !Provider || !Provider->ResolveWeapon(Combatant, Equipped)
        || Equipped.Id != WeaponProfileId)
    { OutError = TEXT("Action requires a host resource authority and matching equipped weapon."); return false; }
    return Authority->TrySpendCombatResources(Combatant, WeaponProfileId, ConsumedItem, Quantity, Effort, OutError);
}

bool URBVariantCombatBindingComponent::BindCombatant(const FRBCombatantRef& Identity)
{
    LastError.Reset();
    if (const AActor* Owner = GetOwner())
    {
        TArray<URBVariantCombatBindingComponent*> Bindings;
        Owner->GetComponents(Bindings);
        for (const auto* Binding : Bindings)
        {
            if (Binding != this && Binding->Combatant.IsValid())
            { LastError = TEXT("Only one combat binding may be bound on an actor."); return false; }
        }
    }
    const IRBCombatConsequenceSink* Sink = GetConsequenceSink();
    if (!Identity.IsValid() || !Sink || !Sink->IsCombatantValid(Identity))
    { LastError = TEXT("Binding requires an existing host combatant."); return false; }
    // Repeated binding cannot erase replay protection. A representation cannot
    // silently switch identity; create a new component for a different combatant.
    if (Combatant.IsValid() && Combatant != Identity)
    { LastError = TEXT("Combat binding identity is immutable for its lifetime."); return false; }
    Combatant = Identity;
    return true;
}

FGuid URBVariantCombatBindingComponent::BeginNativeAttackContact()
{
    if (!HasValidCombatant()) { LastError = TEXT("Unresolved attacker identity."); return FGuid(); }
    return Router.BeginContact();
}

bool URBVariantCombatBindingComponent::PublishEvidence(URBVariantCombatBindingComponent* Victim,
    const FRBCombatHit& Hit, FString& OutError)
{
    if (!Victim || Victim == this || !GetWorld() || GetWorld() != Victim->GetWorld()
        || Hit.Attacker != Combatant || Hit.Victim != Victim->Combatant || !Victim->HasValidCombatant())
    { OutError = TEXT("Combat evidence requires distinct bound actors in the same world."); return false; }
    IRBCombatConsequenceSink* Sink = GetConsequenceSink();
    if (!Sink) { OutError = TEXT("Host consequence sink unavailable."); return false; }
    const bool bAccepted = Router.Publish(Hit, *Sink, OutError);
    if (bAccepted) { Victim->OnAcceptedContact.Broadcast(Hit.AcceptedDamage, Hit.ImpactPoint, Hit.Weapon.Category, Hit.bHasExactImpact, Hit.ContactId); }
    return bAccepted;
}

URBVariantCombatBindingComponent* URBVariantCombatBindingComponent::ResolveAttacker(
    AActor* DamageCauser, AController* InstigatedBy) const
{
    const auto Find = [this](AActor* Actor) -> URBVariantCombatBindingComponent*
    {
        if (!IsValid(Actor) || Actor == GetOwner()) { return nullptr; }
        auto* Binding = Actor->FindComponentByClass<URBVariantCombatBindingComponent>();
        return Binding && Binding->Combatant != Combatant && Binding->HasValidCombatant() ? Binding : nullptr;
    };
    if (auto* Direct = Find(DamageCauser)) { return Direct; }
    if (IsValid(DamageCauser))
    {
        if (auto* Instigator = Find(DamageCauser->GetInstigator())) { return Instigator; }
        if (auto* Owner = Find(DamageCauser->GetOwner())) { return Owner; }
    }
    return IsValid(InstigatedBy) ? Find(InstigatedBy->GetPawn()) : nullptr;
}

void URBVariantCombatBindingComponent::HandleOwnerAnyDamage(AActor* DamagedActor, float Damage,
    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    (void)DamageType;
    LastError.Reset();
    if (DamagedActor != GetOwner() || !HasValidCombatant() || !FMath::IsFinite(Damage) || Damage <= 0)
    { LastError = TEXT("Ignored invalid native damage or unbound victim."); return; }
    if (PendingImpact && DamageCauser == PendingProducer && !bPendingObserved)
    {
        bPendingObserved = true;
        FRBCombatHit Hit = *PendingImpact;
        Hit.AcceptedDamage = Damage;
        LastObservedContact = Hit.ContactId;
        IRBCombatConsequenceSink* Sink = GetConsequenceSink();
        bPendingAccepted = Sink && Router.Publish(Hit, *Sink, LastError);
        UE_LOG(LogTemp, Display, TEXT("RB_COMBAT_PRODUCED: accepted=%d contact=%s damage=%.2f error=%s"),
            bPendingAccepted, *Hit.ContactId.ToString(), Damage, *LastError);
        if (bPendingAccepted) { OnAcceptedContact.Broadcast(Hit.AcceptedDamage, Hit.ImpactPoint, Hit.Weapon.Category, Hit.bHasExactImpact, Hit.ContactId); }
        return;
    }
    auto* Attacker = ResolveAttacker(DamageCauser, InstigatedBy);
    if (!Attacker)
    { LastError = TEXT("Ignored native damage without a distinct host attacker."); return; }
    FRBCombatHit Hit;
    const IRBCombatWeaponProvider* Provider = Attacker->GetWeaponProvider();
    if (!Provider || !Provider->ResolveWeapon(Attacker->Combatant, Hit.Weapon) || !Hit.Weapon.IsValid())
    { LastError = TEXT("Accepted native damage has no valid host weapon profile."); return; }
    Hit.ContactId = Attacker->GetActiveContactId();
    if (!Hit.ContactId.IsValid()) { Hit.ContactId = FGuid::NewGuid(); }
    LastObservedContact = Hit.ContactId;
    Hit.Attacker = Attacker->Combatant;
    Hit.Victim = Combatant;
    Hit.AcceptedDamage = Damage;
    Hit.ImpactPoint = DamagedActor->GetActorLocation();
    // No inferred bleeding, witnesses, or post-damage guard reduction.
    const bool bAccepted = Attacker->PublishEvidence(this, Hit, LastError);
    UE_LOG(LogTemp, Display, TEXT("RB_COMBAT_NATIVE: accepted=%d contact=%s attacker=%s victim=%s damage=%.2f error=%s"),
        bAccepted, *Hit.ContactId.ToString(), *Hit.Attacker.Id.ToString(), *Hit.Victim.Id.ToString(), Damage, *LastError);
}
