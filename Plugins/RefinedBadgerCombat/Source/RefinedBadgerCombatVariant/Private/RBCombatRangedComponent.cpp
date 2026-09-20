#include "RBCombatRangedComponent.h"
#include "RBVariantCombatBindingComponent.h"
#include "RBCombatProjectile.h"
#include "RBCombatMeleeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

FRBRangedProfile FRBCombatBowSettings::Core() const
{
    FRBRangedProfile Result;
    Result.Ammunition = Ammunition;
    Result.MinimumDrawSeconds = MinimumDrawSeconds; Result.FullDrawSeconds = FullDrawSeconds;
    Result.MinimumSpeed = MinimumSpeed; Result.MaximumSpeed = MaximumSpeed;
    Result.GravityScale = GravityScale; Result.LifetimeSeconds = LifetimeSeconds; Result.Radius = Radius;
    return Result;
}

bool URBCombatRangedComponent::ConfigureBow(const FRBCombatBowSettings& Settings)
{ return Configure(Settings.Core()); }

URBCombatRangedComponent::URBCombatRangedComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

URBVariantCombatBindingComponent* URBCombatRangedComponent::Binding() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<URBVariantCombatBindingComponent>() : nullptr;
}

bool URBCombatRangedComponent::Configure(const FRBRangedProfile& Profile)
{
    if (Draw.IsDrawing()) { LastError = TEXT("Finish or cancel the draw before changing bow settings."); return false; }
    if (!Profile.IsValid()) { LastError = TEXT("Bow settings are outside the supported bounds."); return false; }
    LastError.Reset();
    RangedProfile = Profile; return true;
}

bool URBCombatRangedComponent::BeginDraw()
{
    LastError.Reset();
    auto* Host = Binding();
    FRBWeaponProfile Weapon;
    const auto* Melee = GetOwner()->FindComponentByClass<URBCombatMeleeComponent>();
    if (Melee && Melee->IsAttacking()) { LastError = TEXT("Finish the melee action before drawing."); return false; }
    if (Draw.IsDrawing() || !Host || !Host->ResolveEquippedWeapon(Weapon) || Weapon.Category != FName(TEXT("Bow"))
        || !FMath::IsFinite(DrawEffortPerSecond) || DrawEffortPerSecond < 0
        || !Host->TrySpendResources(Weapon.Id, NAME_None, 0, 0, LastError))
    { if (LastError.IsEmpty()) { LastError = TEXT("Cannot draw without an available equipped bow."); } return false; }
    if (!Draw.Begin(RangedProfile)) { LastError = TEXT("Invalid ranged profile."); return false; }
    DrawWeapon = Weapon; DrawSessionId = FGuid::NewGuid(); SetComponentTickEnabled(true); return true;
}

void URBCombatRangedComponent::CancelDraw()
{
    Draw.Cancel(); DrawSessionId.Invalidate(); SetComponentTickEnabled(false);
}

void URBCombatRangedComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (!Draw.IsDrawing() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) { return; }
    auto* Host = Binding();
    if (!Host || !Host->TrySpendResources(DrawWeapon.Id, NAME_None, 0, DeltaSeconds * DrawEffortPerSecond, LastError))
    { CancelDraw(); return; }
    Draw.Advance(DeltaSeconds);
}

bool URBCombatRangedComponent::ReleaseShot(FVector MuzzleLocation, FVector AimDirection)
{
    LastError.Reset();
    auto* Host = Binding();
    FRBProjectileLaunch Launch;
    if (!Host || !GetWorld() || MuzzleLocation.ContainsNaN()
        || FVector::DistSquared(MuzzleLocation, GetOwner()->GetActorLocation()) > FMath::Square(500.0)
        || !FMath::IsFinite(ReleaseEffort) || ReleaseEffort < 0
        || !Draw.BuildLaunch(Host->GetCombatant(), DrawWeapon, MuzzleLocation, AimDirection, Launch))
    { LastError = TEXT("Shot requires a valid draw and nearby physical muzzle."); return false; }
    FActorSpawnParameters Spawn;
    Spawn.Owner = GetOwner(); Spawn.Instigator = Cast<APawn>(GetOwner());
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Projectile = GetWorld()->SpawnActor<ARBCombatProjectile>(ProjectileClass ? ProjectileClass.Get() : ARBCombatProjectile::StaticClass(), MuzzleLocation,
        AimDirection.Rotation(), Spawn);
    if (!Projectile) { LastError = TEXT("Projectile could not spawn; resources unchanged."); return false; }
    if (!Projectile->Launch(Launch, GetOwner()))
    { Projectile->Destroy(); LastError = TEXT("Invalid launch; resources unchanged."); return false; }
    // No world tick can occur between native spawn/launch and this synchronous
    // host commit. Failed spending removes the unlaunched-in-time producer.
    if (!Host->TrySpendResources(DrawWeapon.Id, RangedProfile.Ammunition, 1, ReleaseEffort, LastError))
    { Projectile->Destroy(); return false; }
    Draw.CommitRelease(); DrawSessionId.Invalidate(); SetComponentTickEnabled(false);
    return true;
}
