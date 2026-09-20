#include "RBCombatBlueprintBinding.h"

FRBWeaponProfile FRBHostWeapon::Core() const
{
    FRBWeaponProfile Result;
    Result.Id = Id; Result.Category = Category; Result.DamageType = DamageType;
    Result.BaseDamage = BaseDamage; Result.Reach = Reach;
    Result.BleedingTendency = BleedingTendency; Result.StaggerSeconds = StaggerSeconds;
    return Result;
}

FRBHostWeapon FRBHostWeapon::From(const FRBWeaponProfile& Value)
{
    FRBHostWeapon Result;
    Result.Id = Value.Id; Result.Category = Value.Category; Result.DamageType = Value.DamageType;
    Result.BaseDamage = Value.BaseDamage; Result.Reach = Value.Reach;
    Result.BleedingTendency = Value.BleedingTendency; Result.StaggerSeconds = Value.StaggerSeconds;
    return Result;
}

FRBHostHit FRBHostHit::From(const FRBCombatHit& Value)
{
    FRBHostHit Result;
    Result.ContactId = Value.ContactId; Result.Attacker = FRBHostIdentity::From(Value.Attacker);
    Result.Victim = FRBHostIdentity::From(Value.Victim); Result.Weapon = FRBHostWeapon::From(Value.Weapon);
    Result.AcceptedDamage = Value.AcceptedDamage; Result.Bleeding = Value.Bleeding; Result.StaggerSeconds = Value.StaggerSeconds;
    Result.ImpactPoint = Value.ImpactPoint; Result.bHasExactImpact = Value.bHasExactImpact;
    for (const auto& Witness : Value.Witnesses) { Result.Witnesses.Add(FRBHostIdentity::From(Witness)); }
    return Result;
}

bool URBCombatBlueprintBinding::SetGuardConfiguration(float BlockedFraction, float MinimumFacingDot, float EffortCost)
{
    FRBGuardPolicy Policy; Policy.BlockPercentage = BlockedFraction; Policy.MinimumFacingDot = MinimumFacingDot;
    return ConfigureGuard(Policy, EffortCost);
}

bool URBCombatBlueprintBinding::IsCombatantValid(const FRBCombatantRef& Identity) const
{ return Identity.IsValid() && HostIdentityExists(FRBHostIdentity::From(Identity)); }
bool URBCombatBlueprintBinding::ResolveWeapon(const FRBCombatantRef& Identity, FRBWeaponProfile& Out) const
{
    FRBHostWeapon Weapon;
    if (Identity != GetCombatant() || !IsCombatantValid(Identity) || !HostReadEquippedWeapon(FRBHostIdentity::From(Identity), Weapon)) { return false; }
    const auto Candidate = Weapon.Core();
    if (!Candidate.IsValid()) { return false; }
    Out = Candidate; return true;
}
bool URBCombatBlueprintBinding::TryApplyCombatHit(const FRBCombatHit& Hit, FString& Error)
{
    if (!Hit.IsValid() || !IsCombatantValid(Hit.Attacker) || !IsCombatantValid(Hit.Victim))
    { Error = TEXT("Host hit requires valid evidence and existing identities."); return false; }
    for (const auto& Witness : Hit.Witnesses)
    { if (!IsCombatantValid(Witness)) { Error = TEXT("Host hit includes an unknown witness."); return false; } }
    return HostCommitAcceptedHit(FRBHostHit::From(Hit), Error);
}
bool URBCombatBlueprintBinding::TrySpendCombatResources(const FRBCombatantRef& Identity, FName WeaponId, FName Item,
    int32 Quantity, float Effort, FString& Error)
{
    if (Identity != GetCombatant() || !IsCombatantValid(Identity) || !CanParticipateInCombat() || WeaponId.IsNone()
        || Quantity < 0 || Quantity > 1 || (Quantity == 0) != Item.IsNone() || !FMath::IsFinite(Effort) || Effort < 0 || Effort > 100)
    { Error = TEXT("Invalid host resource request."); return false; }
    return HostSpendResources(FRBHostIdentity::From(Identity), WeaponId, Item, Quantity, Effort, Error);
}
bool URBCombatBlueprintBinding::CanParticipateInCombat() const
{ return HasValidCombatant() && HostCanAct(GetHostIdentity()); }
bool URBCombatBlueprintBinding::CanReceiveCombatDamage() const
{ return HasValidCombatant() && HostCanReceiveDamage(GetHostIdentity()); }
bool URBCombatBlueprintBinding::HasEligibleGuardEquipment() const
{ return HasValidCombatant() && HostHasEligibleGuardEquipment(GetHostIdentity()); }

bool URBCombatBlueprintBinding::HostIdentityExists_Implementation(FRBHostIdentity Identity) const { return false; }
bool URBCombatBlueprintBinding::HostReadEquippedWeapon_Implementation(FRBHostIdentity Identity, FRBHostWeapon& Weapon) const { return false; }
bool URBCombatBlueprintBinding::HostCommitAcceptedHit_Implementation(const FRBHostHit& Hit, FString& Error)
{ Error = TEXT("Implement HostCommitAcceptedHit against the game's authority."); return false; }
bool URBCombatBlueprintBinding::HostSpendResources_Implementation(FRBHostIdentity Identity, FName WeaponId, FName ConsumedItem, int32 Quantity, float Effort, FString& Error)
{ Error = TEXT("Implement HostSpendResources against the game's authority."); return false; }
bool URBCombatBlueprintBinding::HostCanAct_Implementation(FRBHostIdentity Identity) const { return false; }
bool URBCombatBlueprintBinding::HostCanReceiveDamage_Implementation(FRBHostIdentity Identity) const { return false; }
bool URBCombatBlueprintBinding::HostHasEligibleGuardEquipment_Implementation(FRBHostIdentity Identity) const { return false; }
