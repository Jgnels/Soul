#include "RBCombatActions.h"

namespace
{
bool Positive(float Value) { return FMath::IsFinite(Value) && Value > 0; }
bool NonNegative(float Value) { return FMath::IsFinite(Value) && Value >= 0; }
}

bool FRBActionDefinition::IsValid() const
{
    return static_cast<uint8>(Style) <= static_cast<uint8>(ERBAttackStyle::Throw)
        && Positive(DamageScale) && NonNegative(Effort) && Positive(WindupSeconds)
        && Positive(MaximumContactSeconds) && Positive(RecoverySeconds)
        && WindupSeconds <= 30 && MaximumContactSeconds <= 5 && RecoverySeconds <= 30;
}

const FRBActionDefinition* FRBWeaponDefinition::FindAction(ERBAttackStyle Attack) const
{
    return Actions.FindByPredicate([Attack](const FRBActionDefinition& Action) { return Action.Style == Attack; });
}

bool FRBWeaponDefinition::IsValid() const
{
    if (!Profile.IsValid() || !Positive(Profile.BaseDamage) || !Positive(Profile.Reach)
        || !Positive(SweepRadius) || Actions.IsEmpty()
        || static_cast<uint8>(Style) > static_cast<uint8>(ERBWeaponStyle::Bow)) { return false; }
    TSet<ERBAttackStyle> Seen;
    for (const auto& Action : Actions)
    {
        if (!Action.IsValid() || Seen.Contains(Action.Style)) { return false; }
        Seen.Add(Action.Style);
    }
    return true;
}

FRBWeaponDefinition FRBWeaponDefinition::MakeDefault(ERBWeaponStyle Kind)
{
    FRBWeaponDefinition Result;
    Result.Style = Kind;
    const TCHAR* Names[] = {TEXT("Unarmed"), TEXT("Sword"), TEXT("Axe"), TEXT("Spear"), TEXT("Bow")};
    const int32 Index = static_cast<int32>(Kind);
    if (Index < 0 || Index >= UE_ARRAY_COUNT(Names)) { return Result; }
    Result.Profile.Category = FName(Names[Index]);
    Result.Profile.Id = FName(*(FString(TEXT("RB.Review.")) + Names[Index]));
    Result.Profile.DamageType = FName(Kind == ERBWeaponStyle::Unarmed ? TEXT("Blunt") : TEXT("Physical"));
    const float Damage[] = {7, 16, 22, 18, 24};
    const float Reach[] = {100, 150, 140, 235, 3200};
    Result.Profile.BaseDamage = Damage[Index];
    Result.Profile.Reach = Reach[Index];
    if (Kind == ERBWeaponStyle::Bow)
    {
        Result.Actions.Add({ERBAttackStyle::Shoot, 1, 4, .18f, .1f, .6f});
    }
    else
    {
        Result.Actions.Add({ERBAttackStyle::Light, 1, 4, .3f, .25f, .5f});
        Result.Actions.Add({ERBAttackStyle::Heavy, 1.5f, 8, .65f, .3f, .85f});
        if (Kind == ERBWeaponStyle::Spear || Kind == ERBWeaponStyle::Sword)
        { Result.Actions.Add({ERBAttackStyle::Thrust, 1.1f, 5, .4f, .2f, .6f}); }
        if (Kind == ERBWeaponStyle::Spear || Kind == ERBWeaponStyle::Axe)
        { Result.Actions.Add({ERBAttackStyle::Throw, 1.2f, 7, .5f, .15f, .7f}); }
    }
    return Result;
}

bool FRBCombatAction::Begin(const FRBActionDefinition& Definition, const FGuid& ActionId)
{
    if (Phase != ERBActionPhase::Ready || !Definition.IsValid() || !ActionId.IsValid()) { return false; }
    Active = Definition; Id = ActionId; PhaseSeconds = 0; Phase = ERBActionPhase::Windup;
    return true;
}

bool FRBCombatAction::OpenContact(const FGuid& ActionId)
{
    if (ActionId != Id || Phase != ERBActionPhase::Windup) { return false; }
    Phase = ERBActionPhase::Contact; PhaseSeconds = 0; return true;
}

bool FRBCombatAction::CloseContact(const FGuid& ActionId)
{
    if (ActionId != Id || Phase != ERBActionPhase::Contact) { return false; }
    Phase = ERBActionPhase::Recovery; PhaseSeconds = 0; return true;
}

void FRBCombatAction::Advance(float Seconds)
{
    if (!NonNegative(Seconds) || Phase == ERBActionPhase::Ready) { return; }
    PhaseSeconds += Seconds;
    // A missing notify fails closed and recovers; it cannot create a live hit.
    if (Phase == ERBActionPhase::Windup && PhaseSeconds >= Active.WindupSeconds + Active.MaximumContactSeconds)
    { PhaseSeconds -= Active.WindupSeconds + Active.MaximumContactSeconds; Phase = ERBActionPhase::Recovery; }
    else if (Phase == ERBActionPhase::Contact && PhaseSeconds >= Active.MaximumContactSeconds)
    { PhaseSeconds -= Active.MaximumContactSeconds; Phase = ERBActionPhase::Recovery; }
    if ((Phase == ERBActionPhase::Recovery || Phase == ERBActionPhase::Interrupted) && PhaseSeconds >= Active.RecoverySeconds)
    { Phase = ERBActionPhase::Ready; PhaseSeconds = 0; Id.Invalidate(); }
}

void FRBCombatAction::Interrupt(float RecoverySeconds)
{
    if (!Positive(RecoverySeconds)) { return; }
    Active.RecoverySeconds = RecoverySeconds; Phase = ERBActionPhase::Interrupted;
    PhaseSeconds = 0; Id.Invalidate();
}

bool FRBRangedProfile::IsValid() const
{
    return !Ammunition.IsNone() && Positive(MinimumDrawSeconds) && Positive(FullDrawSeconds)
        && FullDrawSeconds >= MinimumDrawSeconds && Positive(MinimumSpeed)
        && Positive(MaximumSpeed) && MaximumSpeed >= MinimumSpeed
        && NonNegative(GravityScale) && Positive(LifetimeSeconds) && Positive(Radius)
        && FullDrawSeconds <= 30 && MaximumSpeed <= 100000 && GravityScale <= 10
        && LifetimeSeconds <= 30 && Radius <= 100;
}

bool FRBProjectileLaunch::IsValid() const
{
    return ShotId.IsValid() && Source.IsValid() && Weapon.IsValid() && !Position.ContainsNaN()
        && !Velocity.ContainsNaN() && !Acceleration.ContainsNaN() && !Velocity.IsNearlyZero()
        && Positive(LifetimeSeconds) && Positive(Radius) && Positive(Damage)
        && LifetimeSeconds <= 30 && Radius <= 100 && Damage <= 1000
        && Velocity.SizeSquared() <= 1.e10 && Acceleration.SizeSquared() <= 1.e8;
}

bool FRBRangedDraw::Begin(const FRBRangedProfile& Profile)
{
    if (bDrawing || !Profile.IsValid()) { return false; }
    Active = Profile; Elapsed = 0; bDrawing = true; return true;
}

void FRBRangedDraw::Advance(float Seconds)
{
    if (bDrawing && NonNegative(Seconds)) { Elapsed = FMath::Min(Active.FullDrawSeconds, Elapsed + Seconds); }
}

bool FRBRangedDraw::BuildLaunch(const FRBCombatantRef& Source, const FRBWeaponProfile& Weapon,
    const FVector& Origin, const FVector& Direction, FRBProjectileLaunch& Out) const
{
    if (!bDrawing || Elapsed < Active.MinimumDrawSeconds || Direction.ContainsNaN() || Direction.IsNearlyZero()) { return false; }
    FRBProjectileLaunch Candidate;
    Candidate.ShotId = FGuid::NewGuid(); Candidate.Source = Source; Candidate.Weapon = Weapon;
    Candidate.Position = Origin;
    const float Alpha = FMath::Clamp(Elapsed / Active.FullDrawSeconds, .2f, 1.f);
    Candidate.Velocity = Direction.GetSafeNormal() * FMath::Lerp(Active.MinimumSpeed, Active.MaximumSpeed, Alpha);
    Candidate.Acceleration.Z *= Active.GravityScale;
    Candidate.Damage = Weapon.BaseDamage * Alpha;
    Candidate.LifetimeSeconds = Active.LifetimeSeconds; Candidate.Radius = Active.Radius;
    if (!Candidate.IsValid()) { return false; }
    Out = Candidate; return true;
}

bool FRBRangedDraw::CommitRelease()
{
    if (!bDrawing || Elapsed < Active.MinimumDrawSeconds) { return false; }
    Cancel(); return true;
}

void FRBRangedDraw::Cancel() { bDrawing = false; Elapsed = 0; }

bool FRBProjectileFlight::Begin(const FRBProjectileLaunch& Launch)
{
    if (bFlying || !Launch.IsValid()) { return false; }
    Position = Launch.Position; Velocity = Launch.Velocity; Acceleration = Launch.Acceleration;
    Remaining = Launch.LifetimeSeconds; bFlying = true; return true;
}

bool FRBProjectileFlight::Step(float RequestedSeconds, FVector& From, FVector& To, float& ConsumedSeconds)
{
    ConsumedSeconds = 0;
    if (!bFlying || !Positive(RequestedSeconds)) { return false; }
    const float Dt = FMath::Min3(RequestedSeconds, MaximumStep, Remaining);
    From = Position;
    To = Position + Velocity * Dt + .5 * Acceleration * Dt * Dt;
    if (To.ContainsNaN()) { Stop(); return false; }
    Position = To; Velocity += Acceleration * Dt; Remaining -= Dt;
    ConsumedSeconds = Dt;
    if (Remaining <= 0) { Stop(); }
    return true;
}
