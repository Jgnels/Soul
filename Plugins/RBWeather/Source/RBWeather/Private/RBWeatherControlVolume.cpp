#include "RBWeatherControlVolume.h"

#include "Engine/World.h"

#include "Components/BoxComponent.h"
#include "RBWeatherSubsystem.h"

ARBWeatherControlVolume::ARBWeatherControlVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(500.0));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARBWeatherControlVolume::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->RegisterControlArea(this);
        }
    }
}

void ARBWeatherControlVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->UnregisterControlArea(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

double ARBWeatherControlVolume::GetLocationWeight(FVector WorldLocation) const
{
    if (!Bounds) return 0.0;
    const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = Bounds->GetUnscaledBoxExtent();
    const double DX = Extent.X - FMath::Abs(Local.X);
    const double DY = Extent.Y - FMath::Abs(Local.Y);
    const double DZ = Extent.Z - FMath::Abs(Local.Z);
    const double InsideDepth = FMath::Min3(DX, DY, DZ);
    if (InsideDepth < 0.0) return 0.0;
    if (TransitionWidthCm <= 0.0) return 1.0;
    const double Linear = FMath::Clamp(InsideDepth / TransitionWidthCm, 0.0, 1.0);
    return Linear * Linear * (3.0 - 2.0 * Linear);
}
