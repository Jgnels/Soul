#include "SoulBattleArrow.h"
#include "SoulRealtimeBattleArena.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

ASoulBattleArrow::ASoulBattleArrow()
{
    auto* Arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisibleArrow"));
    Arrow->SetupAttachment(RootComponent);
    Arrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Arrow->SetCastShadow(false);
    auto* Trail=CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ArrowFlightTrail"));
    Trail->SetupAttachment(RootComponent);
    Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UParticleSystem> FlightFX(
        TEXT("/Game/ParagonSparrow/FX/Particles/Sparrow/Abilities/Primary/FX/P_Sparrow_PrimaryAttack.P_Sparrow_PrimaryAttack"));
    if(FlightFX.Succeeded())
    {
        Trail->SetTemplate(FlightFX.Object);
        Trail->bOverrideLODMethod=true;
        Trail->LODMethod=PARTICLESYSTEMLODMETHOD_DirectSet;
        Trail->SetLODLevel(0);
        Trail->SetRelativeScale3D(FVector(0.75f));
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(
        TEXT("/Game/ParagonSparrow/FX/Meshes/Heroes/Sparrow/Abilities/SM_Sparrow_Arrow.SM_Sparrow_Arrow"));
    if (Mesh.Succeeded())
    {
        Arrow->SetStaticMesh(Mesh.Object);
        const FVector Extent = Mesh.Object->GetBounds().BoxExtent;
        const float Length = FMath::Max3(Extent.X, Extent.Y, Extent.Z) * 2.0f;
        const FRotator Axis = Extent.Y > Extent.X && Extent.Y > Extent.Z
            ? FRotator(0, -90, 0)
            : (Extent.Z > Extent.X ? FRotator(90, 0, 0) : FRotator::ZeroRotator);
        const float Scale = 85.0f / FMath::Max(Length, 1.0f);
        Arrow->SetRelativeRotation(Axis);
        Arrow->SetRelativeScale3D(FVector(Scale));
        Arrow->SetRelativeLocation(-Axis.RotateVector(Mesh.Object->GetBounds().Origin) * Scale);
    }
}


#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

void ASoulBattleArrow::Tick(float DeltaSeconds)
{
    if (!bLaunchPresented && GetLaunchEvidence().IsValid())
    {
        bLaunchPresented = true;
        if (auto* Host = GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>())
            Host->PresentBowLaunch(GetLaunchEvidence());
    }
    Super::Tick(DeltaSeconds);
}

ASoulBattleSpellProjectile::ASoulBattleSpellProjectile()
{
    auto* Flame=CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireboltFlight"));
    Flame->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Fire(
        TEXT("/Game/MagicSpells/Fire/FX/NS_Fireball.NS_Fireball"));
    if(Fire.Succeeded()) Flame->SetAsset(Fire.Object);
    Flame->SetRelativeScale3D(FVector(0.22f));
}
void ASoulBattleSpellProjectile::SetMagicOwner(ASoulRealtimeArenaGameMode* InOwner)
{
    MagicOwner=InOwner;
}
void ASoulBattleSpellProjectile::OnPhysicalImpact(const FHitResult& Hit,const FVector&)
{
    if(MagicOwner.IsValid()) MagicOwner->ResolveMagicProjectile(this,Hit);
}
void ASoulBattleSpellProjectile::EndPlay(const EEndPlayReason::Type Reason)
{
    if(MagicOwner.IsValid()) MagicOwner->ForgetMagicProjectile(this);
    Super::EndPlay(Reason);
}
