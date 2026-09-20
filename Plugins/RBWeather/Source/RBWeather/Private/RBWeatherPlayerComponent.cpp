#include "RBWeatherPlayerComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "RBWeatherSubsystem.h"
#include "UObject/ConstructorHelpers.h"

URBWeatherPlayerComponent::URBWeatherPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);

    static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> DefaultWeatherMPC(
        TEXT("/RBWeather/Presentation/MPC_RBWeather.MPC_RBWeather"));
    if (DefaultWeatherMPC.Succeeded())
    {
        WeatherMPC = DefaultWeatherMPC.Object;
    }
}

void URBWeatherPlayerComponent::BeginPlay()
{
    Super::BeginPlay();
    TimeAccumulator = QueryIntervalSeconds; // first locally relevant tick performs a query
    ShelterAccumulator = ShelterQueryIntervalSeconds;
}

bool URBWeatherPlayerComponent::IsRelevantLocalOwner() const
{
    const AActor* OwnerActor = GetOwner();
    if (!IsValid(OwnerActor)) return false;

    if (const APawn* Pawn = Cast<APawn>(OwnerActor))
    {
        return Pawn->IsLocallyControlled();
    }
    if (const APlayerController* Controller = Cast<APlayerController>(OwnerActor))
    {
        return Controller->IsLocalController();
    }
    // Supports attaching to a local presentation actor in single-player/editor test maps.
    return OwnerActor->GetNetMode() != NM_DedicatedServer;
}

void URBWeatherPlayerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!IsRelevantLocalOwner()) return;

    const double SafeDelta = FMath::Max(0.0, static_cast<double>(DeltaTime));
    if (bEnableAutomaticShelter)
    {
        const double ShelterInterval = FMath::Max(0.05, static_cast<double>(ShelterQueryIntervalSeconds));
        ShelterAccumulator += SafeDelta;
        if (ShelterAccumulator + KINDA_SMALL_NUMBER >= ShelterInterval)
        {
            ShelterAccumulator = FMath::Fmod(ShelterAccumulator, ShelterInterval);
            RefreshShelterNow();
        }
    }

    const double SafeInterval = FMath::Max(0.02, static_cast<double>(QueryIntervalSeconds));
    TimeAccumulator += SafeDelta;
    if (TimeAccumulator + KINDA_SMALL_NUMBER < SafeInterval) return;

    // Avoid an unbounded catch-up loop after a hitch. Weather state is queried, not integrated here.
    TimeAccumulator = FMath::Fmod(TimeAccumulator, SafeInterval);
    RefreshWeatherNow();
}

bool URBWeatherPlayerComponent::RefreshShelterNow()
{
    if (!IsRelevantLocalOwner()) return false;
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!IsValid(OwnerActor) || !World) return false;
    if (!bEnableAutomaticShelter)
    {
        bCachedSheltered = false;
        bShelterInitialized = true;
        return true;
    }

    const FVector Start = OwnerActor->GetActorLocation() + FVector(0.0, 0.0, 20.0);
    const FVector End = Start + FVector(0.0, 0.0, FMath::Max(100.0f, ShelterTraceHeightCm));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RBWeatherShelter), false, OwnerActor);
    FHitResult Hit;
    bCachedSheltered = World->LineTraceSingleByChannel(Hit, Start, End, ShelterTraceChannel, Params);
    bShelterInitialized = true;
    return true;
}

void URBWeatherPlayerComponent::ApplyShelterToSnapshot(FRBWeatherSnapshotAtLocation& Snapshot) const
{
    if (!bEnableAutomaticShelter || !bShelterInitialized || !bCachedSheltered) return;
    Snapshot.bSheltered = true;
    Snapshot.ShelterWeight = 1.0;
    Snapshot.PrecipitationVisualMultiplier *= FMath::Clamp(ShelterPrecipitationMultiplier, 0.0, 1.0);
    Snapshot.LensEffectMultiplier *= FMath::Clamp(ShelterLensMultiplier, 0.0, 1.0);
    Snapshot.WeatherAudioMultiplier *= FMath::Clamp(ShelterAudioMultiplier, 0.0, 2.0);
    Snapshot.LocalWindMultiplier *= FMath::Clamp(ShelterWindMultiplier, 0.0, 2.0);
}

bool URBWeatherPlayerComponent::RefreshWeatherNow()
{
    if (!IsRelevantLocalOwner()) return false;
    AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();
    if (!IsValid(OwnerActor) || !World) return false;

    URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>();
    if (!System) return false;

    if (bEnableAutomaticShelter && !bShelterInitialized) RefreshShelterNow();
    FRBWeatherSnapshotAtLocation NewSnapshot;
    if (!System->GetWeatherAtLocation(OwnerActor->GetActorLocation(), NewSnapshot)) return false;
    ApplyShelterToSnapshot(NewSnapshot);

    const bool bHadSnapshot = bHasSnapshot;
    const FName PreviousWeather = bHadSnapshot ? CurrentSnapshot.CurrentWeather : NAME_None;
    const FName NewLightningSource = NewSnapshot.BiomeId;
    const bool bSameLightningSource = bHadSnapshot && LastLightningSourceId == NewLightningSource;
    const int64 PreviousLightning = bSameLightningSource ? LastLightningSerial : NewSnapshot.LightningSerial;

    CurrentSnapshot = NewSnapshot;
    bHasSnapshot = true;
    ApplyMaterialParameters(CurrentSnapshot);
    OnWeatherUpdated.Broadcast(CurrentSnapshot);

    if (bHadSnapshot && PreviousWeather != CurrentSnapshot.CurrentWeather)
    {
        OnWeatherChanged.Broadcast(PreviousWeather, CurrentSnapshot.CurrentWeather);
    }

    if (!bHadSnapshot || !bSameLightningSource)
    {
        // Joining or crossing into a different independent weather history must not replay strikes.
        LastLightningSourceId = NewLightningSource;
        LastLightningSerial = CurrentSnapshot.LightningSerial;
    }
    else if (CurrentSnapshot.LightningSerial > PreviousLightning)
    {
        // Emit the newest strike only. Packet loss/late updates should not replay a backlog of flashes/audio.
        LastLightningSerial = CurrentSnapshot.LightningSerial;
        OnLightning.Broadcast(CurrentSnapshot.LightningSerial);
    }
    else
    {
        LastLightningSerial = CurrentSnapshot.LightningSerial;
    }
    LastLightningSourceId = NewLightningSource;
    return true;
}

void URBWeatherPlayerComponent::ApplyMaterialParameters(const FRBWeatherSnapshotAtLocation& S) const
{
    if (!WeatherMPC) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(WeatherMPC);
    if (!Instance) return;

    const double Precip = FMath::Clamp(S.Environment.PrecipitationIntensity * S.PrecipitationVisualMultiplier, 0.0, 1.0);
    const double LensMultiplier = FMath::Max(0.0, S.LensEffectMultiplier);
    const double AudioMultiplier = FMath::Max(0.0, S.WeatherAudioMultiplier);
    const double WindMultiplier = FMath::Max(0.0, S.LocalWindMultiplier);

    auto Set = [Instance](const TCHAR* Name, double Value)
    {
        if (FMath::IsFinite(Value))
        {
            Instance->SetScalarParameterValue(FName(Name), static_cast<float>(Value));
        }
    };

    auto SetVector = [Instance](const TCHAR* Name, const FLinearColor& Value)
    {
        if (FMath::IsFinite(static_cast<double>(Value.R)) && FMath::IsFinite(static_cast<double>(Value.G)) &&
            FMath::IsFinite(static_cast<double>(Value.B)) && FMath::IsFinite(static_cast<double>(Value.A)))
        {
            Instance->SetVectorParameterValue(FName(Name), Value);
        }
    };

    Set(TEXT("RB_TransitionAlpha"), S.TransitionAlpha);
    Set(TEXT("RB_PrecipitationType"), static_cast<double>(static_cast<uint8>(S.Environment.PrecipitationType)));
    Set(TEXT("RB_CloudCoverage"), S.Environment.CloudCoverage);
    Set(TEXT("RB_CloudDensity"), S.Environment.CloudDensity);
    Set(TEXT("RB_SunIntensityScale"), S.Environment.SunIntensityScale);
    Set(TEXT("RB_SkyLightIntensityScale"), S.Environment.SkyLightIntensityScale);
    Set(TEXT("RB_FogDensity"), S.Environment.FogDensity);
    Set(TEXT("RB_FogHeightFalloff"), S.Environment.FogHeightFalloff);
    Set(TEXT("RB_VisibilityKm"), S.Environment.VisibilityKm);
    Set(TEXT("RB_WindSpeedMps"), S.Environment.WindSpeedMps * WindMultiplier);
    Set(TEXT("RB_WindGustMps"), S.Environment.WindGustMps * WindMultiplier);
    Set(TEXT("RB_WindDirectionDegrees"), S.Environment.WindDirectionDegrees);
    Set(TEXT("RB_PrecipitationIntensity"), Precip);
    Set(TEXT("RB_Wetness"), S.VisualWetness);
    Set(TEXT("RB_SnowDepthCm"), S.VisualSnowDepthCm);
    Set(TEXT("RB_Dust"), S.VisualDust);
    Set(TEXT("RB_TemperatureC"), S.LocalTemperatureC);
    Set(TEXT("RB_OutsideTemperatureC"), S.OutsideTemperatureC);
    Set(TEXT("RB_VisualOutsideTemperatureC"), S.VisualOutsideTemperatureC);
    Set(TEXT("RB_BaseTemperatureC"), S.BaseTemperatureC);
    Set(TEXT("RB_Sheltered"), S.bSheltered ? 1.0 : 0.0);
    Set(TEXT("RB_Humidity"), S.Environment.Humidity);
    Set(TEXT("RB_LensWetness"), FMath::Clamp(S.Environment.LensWetness * LensMultiplier, 0.0, 1.0));
    Set(TEXT("RB_LensFrost"), FMath::Clamp(S.Environment.LensFrost * LensMultiplier, 0.0, 1.0));
    Set(TEXT("RB_LensDust"), FMath::Clamp(S.Environment.LensDust * LensMultiplier, 0.0, 1.0));
    Set(TEXT("RB_WeatherAudioIntensity"), FMath::Clamp(S.Environment.AudioIntensity * AudioMultiplier, 0.0, 2.0));
    SetVector(TEXT("RB_SunLightColor"), S.Environment.SunLightColor);
    SetVector(TEXT("RB_FogInscatteringColor"), S.Environment.FogInscatteringColor);
}
