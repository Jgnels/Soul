#pragma once

#include "RBCombatContracts.h"

enum class ERBWeaponStyle : uint8 { Unarmed, Sword, Axe, Spear, Bow };
enum class ERBAttackStyle : uint8 { Light, Heavy, Thrust, Shoot, Throw };
enum class ERBActionPhase : uint8 { Ready, Windup, Contact, Recovery, Interrupted };

// Transient mechanics only. The host checks ownership, participation and spends
// its existing resources atomically before committing an action.
struct REFINEDBADGERCOMBATCORE_API FRBActionDefinition
{
    ERBAttackStyle Style = ERBAttackStyle::Light;
    float DamageScale = 1;
    float Effort = 4;
    float WindupSeconds = .3f;
    float MaximumContactSeconds = .25f;
    float RecoverySeconds = .5f;
    bool IsValid() const;
};

struct REFINEDBADGERCOMBATCORE_API FRBWeaponDefinition
{
    FRBWeaponProfile Profile;
    ERBWeaponStyle Style = ERBWeaponStyle::Unarmed;
    float SweepRadius = 10;
    TArray<FRBActionDefinition> Actions;
    const FRBActionDefinition* FindAction(ERBAttackStyle Attack) const;
    bool IsValid() const;
    // Review defaults, not balance or animation qualification for any consumer.
    static FRBWeaponDefinition MakeDefault(ERBWeaponStyle Style);
};

class REFINEDBADGERCOMBATCORE_API FRBCombatAction
{
public:
    bool Begin(const FRBActionDefinition& Definition, const FGuid& ActionId);
    // Called only by an authored animation contact window. Timers never open it.
    bool OpenContact(const FGuid& ActionId);
    bool CloseContact(const FGuid& ActionId);
    void Advance(float Seconds);
    void Interrupt(float RecoverySeconds);
    ERBActionPhase GetPhase() const { return Phase; }
    FGuid GetActionId() const { return Id; }
private:
    FRBActionDefinition Active;
    FGuid Id;
    ERBActionPhase Phase = ERBActionPhase::Ready;
    float PhaseSeconds = 0;
};

struct REFINEDBADGERCOMBATCORE_API FRBRangedProfile
{
    FName Ammunition = TEXT("Arrow");
    float MinimumDrawSeconds = .18f;
    float FullDrawSeconds = 1.25f;
    float MinimumSpeed = 1450;
    float MaximumSpeed = 4100;
    float GravityScale = 1;
    float LifetimeSeconds = 4;
    float Radius = 2;
    bool IsValid() const;
};

struct REFINEDBADGERCOMBATCORE_API FRBProjectileLaunch
{
    FGuid ShotId;
    FRBCombatantRef Source;
    FRBWeaponProfile Weapon;
    FVector Position = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    FVector Acceleration = FVector(0, 0, -980);
    float LifetimeSeconds = 4;
    float Radius = 2;
    float Damage = 0;
    bool IsValid() const;
};

// Build is side-effect free; consume ammunition/effort in the host transaction,
// then CommitRelease. A rejected spend leaves the draw intact for retry/cancel.
class REFINEDBADGERCOMBATCORE_API FRBRangedDraw
{
public:
    bool Begin(const FRBRangedProfile& Profile);
    void Advance(float Seconds);
    bool BuildLaunch(const FRBCombatantRef& Source, const FRBWeaponProfile& Weapon,
        const FVector& Origin, const FVector& Direction, FRBProjectileLaunch& Out) const;
    bool CommitRelease();
    void Cancel();
    float GetDrawSeconds() const { return Elapsed; }
    bool IsDrawing() const { return bDrawing; }
private:
    FRBRangedProfile Active;
    float Elapsed = 0;
    bool bDrawing = false;
};

// The caller sweeps each returned segment before advancing further. Every
// segment is <= MaximumStep seconds; excess frame time is never discarded.
class REFINEDBADGERCOMBATCORE_API FRBProjectileFlight
{
public:
    bool Begin(const FRBProjectileLaunch& Launch);
    bool Step(float RequestedSeconds, FVector& From, FVector& To, float& ConsumedSeconds);
    void Stop() { bFlying = false; }
    bool IsFlying() const { return bFlying; }
    FVector GetPosition() const { return Position; }
    FVector GetVelocity() const { return Velocity; }
    static constexpr float MaximumStep = 1.f / 120.f;
private:
    FVector Position = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    FVector Acceleration = FVector::ZeroVector;
    float Remaining = 0;
    bool bFlying = false;
};
