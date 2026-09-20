#include "RBCombatGuard.h"

bool FRBGuardPolicy::IsValid() const
{
    return FMath::IsFinite(BlockPercentage) && BlockPercentage >= 0 && BlockPercentage <= 1
        && FMath::IsFinite(MinimumFacingDot) && MinimumFacingDot >= 0 && MinimumFacingDot <= 1;
}

FRBGuardResult RBEvaluateGuard(float Damage, const FRBGuardPolicy& Policy, const FRBGuardContext& C)
{
    FRBGuardResult R;
    R.IncomingDamage = Damage; R.EffectiveDamage = Damage;
    if (!Policy.IsValid() || !FMath::IsFinite(Damage) || Damage <= 0 || Damage > 1000)
    { R.Outcome = ERBGuardOutcome::InvalidPolicy; return R; }
    if (!C.bRequested) { return R; }
    if (!C.bEquipmentEligible || !C.bCanParticipate) { R.Outcome = ERBGuardOutcome::Ineligible; return R; }
    if (C.bAttacking) { R.Outcome = ERBGuardOutcome::Attacking; return R; }
    if (!C.bKnownDistinctSource) { R.Outcome = ERBGuardOutcome::UnknownSource; return R; }
    const FVector Facing = C.Facing.GetSafeNormal2D();
    const FVector Source = C.ToSource.GetSafeNormal2D();
    if (C.Facing.ContainsNaN() || C.ToSource.ContainsNaN() || Facing.IsNearlyZero() || Source.IsNearlyZero())
    { R.Outcome = ERBGuardOutcome::InvalidDirection; return R; }
    if (FVector::DotProduct(Facing, Source) < Policy.MinimumFacingDot)
    { R.Outcome = ERBGuardOutcome::OutsideArc; return R; }
    R.Outcome = ERBGuardOutcome::Blocked;
    R.EffectiveDamage = Damage * (1 - Policy.BlockPercentage);
    return R;
}
