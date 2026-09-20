#include "RBWeatherEnvironmentalExposure.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RBWeatherSubsystem.h"

URBWeatherEnvironmentalExposureComponent::URBWeatherEnvironmentalExposureComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);
}

void URBWeatherEnvironmentalExposureComponent::BeginPlay()
{
    Super::BeginPlay();
    TimeAccumulator = QueryIntervalSeconds;
}

bool URBWeatherEnvironmentalExposureComponent::ShouldRunHere() const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return false;
    return !bAuthorityOnly || OwnerActor->HasAuthority();
}

void URBWeatherEnvironmentalExposureComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!ShouldRunHere()) return;
    const double Interval = FMath::Max(0.05, static_cast<double>(QueryIntervalSeconds));
    TimeAccumulator += FMath::Max(0.0, static_cast<double>(DeltaTime));
    if (TimeAccumulator + KINDA_SMALL_NUMBER < Interval) return;
    TimeAccumulator = FMath::Fmod(TimeAccumulator, Interval);
    RefreshExposureNow();
}

bool URBWeatherEnvironmentalExposureComponent::IsSheltered() const
{
    if (!bUseAutomaticShelterTrace) return false;
    const AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!OwnerActor || !World) return false;

    const FVector Start = OwnerActor->GetActorLocation() + FVector(0.0, 0.0, 20.0);
    const FVector End = Start + FVector(0.0, 0.0, FMath::Max(100.0f, ShelterTraceHeightCm));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RBWeatherExposureShelter), false, OwnerActor);
    FHitResult Hit;
    return World->LineTraceSingleByChannel(Hit, Start, End, ShelterTraceChannel, Params);
}

bool URBWeatherEnvironmentalExposureComponent::PrecipitationCanWet(
    ERBWeatherPrecipitation Type, double LocalTemperatureC) const
{
    switch (Type)
    {
    case ERBWeatherPrecipitation::Rain:
    case ERBWeatherPrecipitation::Sleet:
    case ERBWeatherPrecipitation::Hail:
        return true;
    case ERBWeatherPrecipitation::Snow:
        return LocalTemperatureC >= SnowCountsAsWetAboveC;
    case ERBWeatherPrecipitation::None:
    case ERBWeatherPrecipitation::Dust:
    default:
        return false;
    }
}

ERBWeatherTemperatureBand URBWeatherEnvironmentalExposureComponent::ClassifyTemperature(double TemperatureC) const
{
    if (TemperatureC < VeryColdBelowC) return ERBWeatherTemperatureBand::VeryCold;
    if (TemperatureC < ColdBelowC) return ERBWeatherTemperatureBand::Cold;
    if (TemperatureC >= VeryHotAtOrAboveC) return ERBWeatherTemperatureBand::VeryHot;
    if (TemperatureC >= HotAtOrAboveC) return ERBWeatherTemperatureBand::Hot;
    return ERBWeatherTemperatureBand::Normal;
}

bool URBWeatherEnvironmentalExposureComponent::RefreshExposureNow()
{
    if (!ShouldRunHere()) return false;
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!OwnerActor || !World) return false;
    URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>();
    if (!System) return false;

    FRBWeatherSnapshotAtLocation Snapshot;
    if (!System->GetWeatherAtLocation(OwnerActor->GetActorLocation(), Snapshot)) return false;
    FRBWeatherCharacterExposure Exposure;
    Exposure.WeatherId = Snapshot.CurrentWeather;
    Exposure.PrecipitationType = Snapshot.GameplayEnvironment.PrecipitationType;
    Exposure.EnvironmentalTemperatureC = Snapshot.OutsideTemperatureC;
    Exposure.LocalTemperatureC = Snapshot.LocalTemperatureC;
    Exposure.LocalTemperatureDeltaC = Snapshot.LocalTemperatureC - Snapshot.OutsideTemperatureC;
    Exposure.Humidity01 = FMath::Clamp(Snapshot.GameplayEnvironment.Humidity, 0.0, 1.0);
    Exposure.SurfaceWetness01 = FMath::Clamp(Snapshot.Wetness, 0.0, 1.0);
    Exposure.bSheltered = IsSheltered();

    Exposure.PrecipitationExposure01 = FMath::Clamp(
        Snapshot.GameplayEnvironment.PrecipitationIntensity, 0.0, 1.0);
    if (Exposure.bSheltered) Exposure.PrecipitationExposure01 = 0.0;

    Exposure.bWetExposure = Exposure.PrecipitationExposure01 >= WetExposureThreshold &&
        PrecipitationCanWet(Exposure.PrecipitationType, Exposure.LocalTemperatureC);
    Exposure.bWarmingExposure = Exposure.LocalTemperatureDeltaC >= WarmingTemperatureDeltaC;
    Exposure.TemperatureBand = ClassifyTemperature(Exposure.LocalTemperatureC);

    PublishExposure(Exposure);
    return true;
}

void URBWeatherEnvironmentalExposureComponent::PublishExposure(const FRBWeatherCharacterExposure& Exposure)
{
    CurrentExposure = Exposure;
    OnExposureUpdated.Broadcast(CurrentExposure);
    AActor* OwnerActor = GetOwner();
    if (OwnerActor && OwnerActor->GetClass()->ImplementsInterface(URBWeatherExposureSink::StaticClass()))
    {
        IRBWeatherExposureSink::Execute_ApplyRBWeatherExposure(OwnerActor, CurrentExposure);
    }
}
