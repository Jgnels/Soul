#include "RBCombatWeaponData.h"

FRBActionDefinition FRBWeaponAnimation::Definition() const
{
    return {static_cast<ERBAttackStyle>(Attack), DamageScale, Effort, WindupSeconds, MaximumContactSeconds, RecoverySeconds};
}

FRBWeaponProfile URBCombatWeaponData::Profile() const
{
    FRBWeaponProfile Result;
    Result.Id = WeaponId; Result.Category = Category; Result.DamageType = TEXT("Physical");
    Result.BaseDamage = BaseDamage; Result.Reach = Reach;
    Result.BleedingTendency = BleedingTendency; Result.StaggerSeconds = StaggerSeconds;
    return Result;
}

bool URBCombatWeaponData::IsValidWeapon() const
{
    if (!Profile().IsValid() || BaseDamage <= 0 || Reach <= 0 || !FMath::IsFinite(SweepRadius)
        || SweepRadius <= 0 || SweepRadius > 100 || LocalBladeStart.ContainsNaN() || LocalBladeEnd.ContainsNaN()
        || FVector::Dist(LocalBladeStart, LocalBladeEnd) > SweepRadius * 64 || Attacks.IsEmpty()) { return false; }
    if (!FMath::IsFinite(ThrowSpeed) || ThrowSpeed <= 0 || ThrowSpeed > 100000
        || !FMath::IsFinite(ProjectileRadius) || ProjectileRadius <= 0 || ProjectileRadius > 100) { return false; }
    TSet<uint16> Seen;
    for (const auto& Attack : Attacks)
    {
        const uint16 Key = (static_cast<uint16>(Attack.Attack) << 8) | static_cast<uint16>(Attack.ComboStage);
        if (!Attack.Montage || !Attack.Definition().IsValid() || BaseDamage * Attack.DamageScale > 1000
            || Attack.GripRotation.ContainsNaN() || FMath::Abs(Attack.GripRotation.Pitch) > 360
            || FMath::Abs(Attack.GripRotation.Yaw) > 360 || FMath::Abs(Attack.GripRotation.Roll) > 360
            || Attack.ComboStage < 0 || Attack.ComboStage > 7 || Seen.Contains(Key)) { return false; }
        Seen.Add(Key);
    }
    for (const auto& Attack : Attacks)
    { if (!Seen.Contains(static_cast<uint16>(Attack.Attack) << 8)) { return false; } }
    return true;
}
