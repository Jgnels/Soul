#include "RBWeatherTemperatureSourceComponent.h"

#include "Engine/World.h"

#include "RBWeatherSubsystem.h"

URBWeatherTemperatureSourceComponent::URBWeatherTemperatureSourceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBWeatherTemperatureSourceComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->RegisterTemperatureSource(this);
        }
    }
}

void URBWeatherTemperatureSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->UnregisterTemperatureSource(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

double URBWeatherTemperatureSourceComponent::GetTemperatureOffsetAt(FVector WorldLocation) const
{
    if (!FMath::IsFinite(RadiusCm) || RadiusCm <= 0.0 || !FMath::IsFinite(TemperatureOffsetC) ||
        !FMath::IsFinite(FalloffExponent) || FalloffExponent <= 0.0)
    {
        return 0.0;
    }
    const double Distance = FVector::Distance(GetComponentLocation(), WorldLocation);
    if (Distance >= RadiusCm) return 0.0;
    const double Normalized = FMath::Clamp(1.0 - Distance / RadiusCm, 0.0, 1.0);
    return TemperatureOffsetC * FMath::Pow(Normalized, FalloffExponent);
}
