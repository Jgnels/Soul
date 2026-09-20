#include "RBCombatRouter.h"

namespace
{
bool Bounded(float Value, float Max) { return FMath::IsFinite(Value) && Value >= 0 && Value <= Max; }
}

bool FRBWeaponProfile::IsValid() const
{
    return !Id.IsNone() && !Category.IsNone() && Bounded(BaseDamage, 1000)
        && Bounded(Reach, 10000) && Bounded(BleedingTendency, 1000) && Bounded(StaggerSeconds, 30);
}

bool FRBCombatHit::IsValid() const
{
    if (!ContactId.IsValid() || !Attacker.IsValid() || !Victim.IsValid() || Attacker == Victim
        || !Weapon.IsValid() || !Bounded(AcceptedDamage, 1000) || AcceptedDamage <= 0
        || !Bounded(Bleeding, 1000) || !Bounded(StaggerSeconds, 30)
        || !FMath::IsFinite(ImpactPoint.X) || !FMath::IsFinite(ImpactPoint.Y)
        || !FMath::IsFinite(ImpactPoint.Z) || Witnesses.Num() > 32)
    { return false; }
    TSet<FRBCombatantRef> Unique;
    for (const FRBCombatantRef& Witness : Witnesses)
    {
        if (!Witness.IsValid() || Witness == Attacker || Witness == Victim || Unique.Contains(Witness))
        { return false; }
        Unique.Add(Witness);
    }
    return true;
}

FGuid FRBCombatRouter::BeginContact()
{
    if (bPublishing) { return FGuid(); }
    ActiveVictims.Reset();
    ActiveContact = FGuid::NewGuid();
    return ActiveContact;
}

void FRBCombatRouter::EndContact()
{
    if (bPublishing) { return; }
    ActiveContact.Invalidate();
    ActiveVictims.Reset();
}

bool FRBCombatRouter::Publish(const FRBCombatHit& Hit, IRBCombatConsequenceSink& Sink, FString& OutError)
{
    OutError.Reset();
    if (bPublishing) { OutError = TEXT("Reentrant combat publication rejected."); return false; }
    // Protect validation too: host callbacks may not reenter or alter the lifecycle.
    TGuardValue<bool> PublishingGuard(bPublishing, true);
    if (!Hit.IsValid() || !Sink.IsCombatantValid(Hit.Attacker) || !Sink.IsCombatantValid(Hit.Victim))
    { OutError = TEXT("Invalid combat evidence or unresolved host identity."); return false; }
    for (const FRBCombatantRef& Witness : Hit.Witnesses)
    {
        if (!Sink.IsCombatantValid(Witness))
        { OutError = TEXT("Unresolved witness identity."); return false; }
    }
    const FKey Key{Hit.ContactId, Hit.Attacker, Hit.Victim};
    const bool bActive = Hit.ContactId == ActiveContact;
    if (Published.Contains(Key) || (bActive && ActiveVictims.Contains(Hit.Victim)))
    { OutError = TEXT("Duplicate combat contact/victim rejected."); return false; }
    if (bActive && ActiveVictims.Num() >= Capacity)
    { OutError = TEXT("Active contact victim capacity reached; end the contact before another attack."); return false; }
    if (!Sink.TryApplyCombatHit(Hit, OutError))
    {
        if (OutError.IsEmpty()) { OutError = TEXT("Host consequence sink rejected the hit."); }
        return false;
    }
    if (Published.Num() == Capacity) { Published.RemoveAt(0, 1, EAllowShrinking::No); }
    Published.Add(Key);
    if (bActive) { ActiveVictims.Add(Hit.Victim); }
    OutError.Reset();
    return true;
}
