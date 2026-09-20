#pragma once
#include "CoreMinimal.h"

enum class ERBGuardOutcome : uint8
{
    Blocked, NotRequested, Ineligible, Attacking, UnknownSource, InvalidDirection, OutsideArc, InvalidPolicy, InsufficientEffort
};

struct REFINEDBADGERCOMBATCORE_API FRBGuardPolicy
{
    // Defaults preserve the inspected Gladiator mathematical policy.
    float BlockPercentage = .8f;
    float MinimumFacingDot = .5f;
    bool IsValid() const;
};

struct REFINEDBADGERCOMBATCORE_API FRBGuardContext
{
    bool bRequested = false;
    bool bEquipmentEligible = false;
    bool bCanParticipate = true;
    bool bAttacking = false;
    bool bKnownDistinctSource = true;
    FVector Facing = FVector::ForwardVector;
    FVector ToSource = FVector::ZeroVector;
};

struct REFINEDBADGERCOMBATCORE_API FRBGuardResult
{
    float IncomingDamage = 0;
    float EffectiveDamage = 0;
    ERBGuardOutcome Outcome = ERBGuardOutcome::NotRequested;
    bool IsBlocked() const { return Outcome == ERBGuardOutcome::Blocked; }
};

// Call before native damage is applied. Never reduce a post-damage observation.
REFINEDBADGERCOMBATCORE_API FRBGuardResult RBEvaluateGuard(float Damage,
    const FRBGuardPolicy& Policy, const FRBGuardContext& Context);
