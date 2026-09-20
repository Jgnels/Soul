#include "RBWeatherBiomeVolume.h"

#include "Engine/World.h"

#include "Components/BoxComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "RBWeatherDirector.h"
#include "RBWeatherSimulation.h"
#include "RBWeatherSubsystem.h"

ARBWeatherBiomeVolume::ARBWeatherBiomeVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(5000.0, 5000.0, 2500.0));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARBWeatherBiomeVolume::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->RegisterBiome(this);
        }
    }
}

void ARBWeatherBiomeVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->UnregisterBiome(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ARBWeatherBiomeVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARBWeatherBiomeVolume, ReplicatedState);
}

void ARBWeatherBiomeVolume::OnRep_WeatherState()
{
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->SyncCanonicalBiomeFromVolume(this, ReplicatedState);
        }
    }
    OnBiomeStateChanged.Broadcast(ReplicatedState);
}

void ARBWeatherBiomeVolume::PublishState()
{
    ForceNetUpdate();
    OnBiomeStateChanged.Broadcast(ReplicatedState);
}

double ARBWeatherBiomeVolume::GetLocationWeight(FVector WorldLocation) const
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
    return Linear * Linear * (3.0 - 2.0 * Linear); // smoothstep
}

bool ARBWeatherBiomeVolume::AdvanceFromDirector(ARBWeatherDirector* Director, FName Season, int64 DayIndex,
                                                 double HourOfDay, double DeltaGameHours, FString& OutError)
{
    if (!IsValid(Director) || !Director->HasAuthority())
    {
        OutError = TEXT("Biome weather can only be advanced by an authoritative director.");
        return false;
    }
    FRBWeatherClimateRow Climate;
    if (!Director->GetClimateProfile(ClimateProfileOverride, Climate))
    {
        OutError = TEXT("Biome climate profile could not be resolved.");
        return false;
    }
    const int64 Seed = Director->GetBiomeStableSeed(this);
    FRBWeatherSimulationEvents Events;
    bool bOk = false;
    if (!ReplicatedState.bInitialized)
    {
        bOk = FRBWeatherSimulation::Initialize(ReplicatedState, Director->WeatherPresetTable,
            Director->WeatherTransitionTable, Climate, InitialWeather, Season, DayIndex, HourOfDay,
            Seed, Events, OutError);
    }
    else
    {
        bOk = FRBWeatherSimulation::Advance(ReplicatedState, Director->WeatherPresetTable,
            Director->WeatherTransitionTable, Climate, Season, DayIndex, HourOfDay, DeltaGameHours,
            Seed, Events, OutError);
    }
    if (bOk)
    {
        if (UWorld* World = GetWorld())
        {
            if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
            {
                System->SyncCanonicalBiomeFromVolume(this, ReplicatedState);
            }
        }
        PublishState();
    }
    return bOk;
}

bool ARBWeatherBiomeVolume::ForceWeatherFromDirector(ARBWeatherDirector* Director, FName WeatherId,
                                                      double TransitionGameHours, FString& OutError)
{
    if (!IsValid(Director) || !Director->HasAuthority() || !ReplicatedState.bInitialized)
    {
        OutError = TEXT("Biome must be initialized and server-authoritative before forcing weather.");
        return false;
    }
    FRBWeatherClimateRow Climate;
    if (!Director->GetClimateProfile(ClimateProfileOverride, Climate))
    {
        OutError = TEXT("Biome climate profile could not be resolved.");
        return false;
    }
    FRBWeatherSimulationEvents Events;
    const bool bOk = FRBWeatherSimulation::ForceWeather(ReplicatedState, Director->WeatherPresetTable,
        Director->WeatherTransitionTable, Climate, WeatherId, TransitionGameHours,
        Director->GetBiomeStableSeed(this), Events, OutError);
    if (bOk)
    {
        if (UWorld* World = GetWorld())
        {
            if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
            {
                System->SyncCanonicalBiomeFromVolume(this, ReplicatedState);
            }
        }
        PublishState();
    }
    return bOk;
}

bool ARBWeatherBiomeVolume::BuildSnapshot(const ARBWeatherDirector* Director,
                                           FRBWeatherSnapshotAtLocation& OutSnapshot, FString& OutError) const
{
    OutSnapshot = {};
    if (!IsValid(Director) || !ReplicatedState.bInitialized)
    {
        OutError = TEXT("Biome snapshot unavailable before initialization.");
        return false;
    }
    FRBWeatherClimateRow Climate;
    if (!Director->GetClimateProfile(ClimateProfileOverride, Climate))
    {
        OutError = TEXT("Biome climate profile could not be resolved.");
        return false;
    }
    double Alpha = 1.0;
    if (!FRBWeatherSimulation::GetVisualEnvironment(ReplicatedState, Director->WeatherPresetTable,
        Director->WeatherTransitionTable, Climate, OutSnapshot.Environment, Alpha, OutError)) return false;
    if (!FRBWeatherSimulation::GetGameplayEnvironment(ReplicatedState, Director->WeatherPresetTable,
        Director->WeatherTransitionTable, Climate, OutSnapshot.GameplayEnvironment, OutError)) return false;
    bool bTempValid = false;
    const double Outside = FRBWeatherSimulation::GetOutsideTemperatureC(ReplicatedState,
        Director->WeatherPresetTable, Director->WeatherTransitionTable, Climate, bTempValid);
    if (!bTempValid)
    {
        OutError = TEXT("Biome outside temperature could not be calculated.");
        return false;
    }
    OutSnapshot.BiomeId = BiomeId;
    OutSnapshot.BiomeBlendWeight = 1.0;
    OutSnapshot.CurrentWeather = ReplicatedState.CurrentWeather;
    OutSnapshot.TargetWeather = ReplicatedState.TargetWeather;
    OutSnapshot.TransitionAlpha = Alpha;
    OutSnapshot.LightningSerial = ReplicatedState.LightningSerial;
    OutSnapshot.Wetness = ReplicatedState.Wetness;
    OutSnapshot.SnowDepthCm = ReplicatedState.SnowDepthCm;
    OutSnapshot.Dust = ReplicatedState.Dust;
    OutSnapshot.VisualWetness = ReplicatedState.Wetness;
    OutSnapshot.VisualSnowDepthCm = ReplicatedState.SnowDepthCm;
    OutSnapshot.VisualDust = ReplicatedState.Dust;
    OutSnapshot.BaseTemperatureC = ReplicatedState.BaseTemperatureC;
    OutSnapshot.OutsideTemperatureC = Outside;
    OutSnapshot.VisualOutsideTemperatureC = Outside;
    OutSnapshot.LocalTemperatureC = Outside;
    OutError.Reset();
    return true;
}

bool ARBWeatherBiomeVolume::ValidateSavedState(const ARBWeatherDirector* Director,
                                                const FRBWeatherRuntimeState& Candidate, FString& OutError) const
{
    if (!IsValid(Director) || !Candidate.bInitialized || !FMath::IsFinite(Candidate.BaseTemperatureC) ||
        !FMath::IsFinite(Candidate.HourOfDay))
    {
        OutError = TEXT("Biome saved weather state has invalid basic fields.");
        return false;
    }
    FRBWeatherClimateRow Climate;
    if (!Director->GetClimateProfile(ClimateProfileOverride, Climate))
    {
        OutError = TEXT("Biome climate profile could not be resolved.");
        return false;
    }
    bool bTemperatureValid = false;
    FRBWeatherSimulation::ComputeBaseTemperatureC(Climate, Candidate.CurrentSeason, Candidate.DayIndex,
        Candidate.HourOfDay, Director->GetBiomeStableSeed(this), bTemperatureValid);
    if (!bTemperatureValid)
    {
        OutError = TEXT("Biome saved weather state uses an unknown season/time context.");
        return false;
    }
    FRBWeatherEnvironment Env;
    double Alpha = 0.0;
    return FRBWeatherSimulation::GetVisualEnvironment(Candidate, Director->WeatherPresetTable,
        Director->WeatherTransitionTable, Climate, Env, Alpha, OutError);
}

void ARBWeatherBiomeVolume::RestoreValidatedState(const FRBWeatherRuntimeState& Candidate)
{
    ApplyCanonicalState(Candidate, true);
}

void ARBWeatherBiomeVolume::ApplyCanonicalState(const FRBWeatherRuntimeState& Candidate, bool bPublish)
{
    ReplicatedState = Candidate;
    if (bPublish) PublishState();
}
