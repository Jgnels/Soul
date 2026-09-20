#pragma once

#include "CoreMinimal.h"

// Host-issued identity, never an Actor address or a core-owned person database.
struct REFINEDBADGERCOMBATCORE_API FRBCombatantRef
{
    FName Domain;
    FGuid Id;
    bool IsValid() const { return !Domain.IsNone() && Id.IsValid(); }
    bool operator==(const FRBCombatantRef& Other) const { return Domain == Other.Domain && Id == Other.Id; }
    bool operator!=(const FRBCombatantRef& Other) const { return !(*this == Other); }
    friend uint32 GetTypeHash(const FRBCombatantRef& Ref) { return HashCombine(GetTypeHash(Ref.Domain), GetTypeHash(Ref.Id)); }
};

// Descriptive equipment evidence supplied by the host. Never applies native damage.
// Zero metadata means unspecified; it is not inferred equipment or injury truth.
struct REFINEDBADGERCOMBATCORE_API FRBWeaponProfile
{
    FName Id;
    FName Category;
    FName DamageType;
    float BaseDamage = 0;
    float Reach = 0;
    float BleedingTendency = 0;
    float StaggerSeconds = 0;
    bool IsValid() const;
};

struct REFINEDBADGERCOMBATCORE_API FRBCombatHit
{
    FGuid ContactId;
    FRBCombatantRef Attacker;
    FRBCombatantRef Victim;
    FRBWeaponProfile Weapon;
    float AcceptedDamage = 0;
    float Bleeding = 0;
    float StaggerSeconds = 0;
    FVector ImpactPoint = FVector::ZeroVector;
    // AnyDamage has no point result: false means ImpactPoint is an actor-location estimate.
    bool bHasExactImpact = false;
    TArray<FRBCombatantRef> Witnesses;
    bool IsValid() const;
};

class REFINEDBADGERCOMBATCORE_API IRBCombatConsequenceSink
{
public:
    virtual ~IRBCombatConsequenceSink() = default;
    virtual bool IsCombatantValid(const FRBCombatantRef& Combatant) const = 0;
    // Must validate and commit atomically. On false, host truth MUST be unchanged.
    // Core cannot roll back an arbitrary host. Implement copy/validate/commit there.
    virtual bool TryApplyCombatHit(const FRBCombatHit& Hit, FString& OutError) = 0;
};

class REFINEDBADGERCOMBATCORE_API IRBCombatWeaponProvider
{
public:
    virtual ~IRBCombatWeaponProvider() = default;
    virtual bool ResolveWeapon(const FRBCombatantRef& Attacker, FRBWeaponProfile& OutProfile) const = 0;
};

class REFINEDBADGERCOMBATCORE_API IRBCombatResourceAuthority
{
public:
    virtual ~IRBCombatResourceAuthority() = default;
    // Atomic host transaction. Reject without spending anything. A zero quantity
    // requires no consumed item; one consumes an arrow or the thrown weapon.
    virtual bool TrySpendCombatResources(const FRBCombatantRef& Person, FName WeaponProfileId,
        FName ConsumedItem, int32 Quantity, float Effort, FString& OutError) = 0;
};
