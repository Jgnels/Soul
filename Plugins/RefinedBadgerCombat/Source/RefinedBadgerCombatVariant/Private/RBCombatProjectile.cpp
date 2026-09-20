#include "RBCombatProjectile.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "RBVariantCombatBindingComponent.h"

void ARBCombatProjectile::OnPhysicalImpact(const FHitResult& Hit, const FVector& IncomingVelocity)
{
    AActor* Victim = Hit.GetActor();
    auto* Binding = IsValid(Victim) ? Victim->FindComponentByClass<URBVariantCombatBindingComponent>() : nullptr;
    if (!Binding || Binding->GetCombatant() == LaunchEvidence.Source) { return; }
    FRBCombatHit Evidence;
    Evidence.ContactId = LaunchEvidence.ShotId; Evidence.Attacker = LaunchEvidence.Source;
    Evidence.Victim = Binding->GetCombatant(); Evidence.Weapon = LaunchEvidence.Weapon;
    Evidence.AcceptedDamage = LaunchEvidence.Damage; Evidence.ImpactPoint = Hit.ImpactPoint;
    Evidence.Bleeding = LaunchEvidence.Weapon.BleedingTendency;
    Evidence.StaggerSeconds = LaunchEvidence.Weapon.StaggerSeconds;
    Evidence.bHasExactImpact = true;
    FString Error;
    Binding->ReceiveProducedImpact(Evidence, Hit, IncomingVelocity, this, Error);
}

ARBCombatProjectile::ARBCombatProjectile()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileRoot"));
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    // Consumers need a separate server-authority replication qualification.
    bReplicates = false;
}

bool ARBCombatProjectile::Launch(const FRBProjectileLaunch& Evidence, AActor* SourceRepresentation)
{
    if (bLaunched || !GetWorld() || (SourceRepresentation && SourceRepresentation->GetWorld() != GetWorld())
        || !Flight.Begin(Evidence)) { return false; }
    LaunchEvidence = Evidence; IgnoredSource = SourceRepresentation; bLaunched = true;
    SetActorLocationAndRotation(Evidence.Position, Evidence.Velocity.Rotation());
    SetActorTickEnabled(true);
    return true;
}

void ARBCombatProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0 || !Flight.IsFlying()) { return; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RBProjectile), false, this);
    if (AActor* Source = IgnoredSource.Get()) { Query.AddIgnoredActor(Source); }
    float Remaining = DeltaSeconds;
    while (Remaining > 0 && Flight.IsFlying())
    {
        FVector From, To; float Consumed = 0;
        const FVector IncomingVelocity = Flight.GetVelocity();
        if (!Flight.Step(Remaining, From, To, Consumed)) { break; }
        Remaining = FMath::Max(0.f, Remaining - Consumed);
        FHitResult Hit;
        const bool bHit = GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity,
            TraceChannel, FCollisionShape::MakeSphere(LaunchEvidence.Radius), Query);
        if (bHit)
        {
            Flight.Stop(); SetActorTickEnabled(false);
            SetActorLocation(Hit.Location);
            OnPhysicalImpact(Hit, IncomingVelocity);
            if (!IsActorBeingDestroyed()) { PresentImpact(Hit); Destroy(); }
            return;
        }
        SetActorLocationAndRotation(To, Flight.GetVelocity().Rotation());
    }
    if (!Flight.IsFlying())
    {
        SetActorTickEnabled(false); PresentExpiry(); Destroy();
    }
}
