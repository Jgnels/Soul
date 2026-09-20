#include "RBWeatherAtmosphereComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "RBWeatherPlayerComponent.h"

URBWeatherAtmosphereComponent::URBWeatherAtmosphereComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void URBWeatherAtmosphereComponent::BeginPlay()
{
    Super::BeginPlay();
    BindWeatherSource();
    CaptureBaselines();
    RefreshAtmosphereNow();
}

void URBWeatherAtmosphereComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WeatherSource)
    {
        WeatherSource->OnWeatherUpdated.RemoveDynamic(this, &URBWeatherAtmosphereComponent::HandleWeatherUpdated);
    }
    Super::EndPlay(EndPlayReason);
}
void URBWeatherAtmosphereComponent::BindWeatherSource()
{
    if (!WeatherSource && GetOwner())
    {
        WeatherSource = GetOwner()->FindComponentByClass<URBWeatherPlayerComponent>();
    }
    if (!WeatherSource) return;
    WeatherSource->OnWeatherUpdated.RemoveDynamic(this, &URBWeatherAtmosphereComponent::HandleWeatherUpdated);
    WeatherSource->OnWeatherUpdated.AddDynamic(this, &URBWeatherAtmosphereComponent::HandleWeatherUpdated);
}

void URBWeatherAtmosphereComponent::CaptureBaselines()
{
    if (SunLight)
    {
        BaseSunIntensity = SunLight->Intensity;
        BaseSunColor = SunLight->GetLightColor();
    }
    if (SkyLight)
    {
        BaseSkyIntensity = SkyLight->Intensity;
    }
    bBaselinesCaptured = true;
}

bool URBWeatherAtmosphereComponent::RefreshAtmosphereNow()
{
    if (!WeatherSource) BindWeatherSource();
    if (!WeatherSource) return false;
    if (!WeatherSource->RefreshWeatherNow()) return false;
    ApplySnapshot(WeatherSource->GetCurrentSnapshot());
    return true;
}
void URBWeatherAtmosphereComponent::HandleWeatherUpdated(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    ApplySnapshot(Snapshot);
}

void URBWeatherAtmosphereComponent::ApplySnapshot(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    if (!bBaselinesCaptured) CaptureBaselines();
    const FRBWeatherEnvironment& Environment = Snapshot.Environment;

    // Rotation/day-night progression is deliberately never touched here.
    if (bControlSunIntensityAndColor && SunLight)
    {
        SunLight->SetIntensity(BaseSunIntensity * static_cast<float>(FMath::Max(0.0, Environment.SunIntensityScale)));
        SunLight->SetLightColor(BaseSunColor * Environment.SunLightColor);
    }
    if (bControlSkyLightIntensity && SkyLight)
    {
        SkyLight->SetIntensity(BaseSkyIntensity * static_cast<float>(FMath::Max(0.0, Environment.SkyLightIntensityScale)));
    }
    if (bControlFog && HeightFog)
    {
        HeightFog->SetFogDensity(static_cast<float>(FMath::Max(0.0, Environment.FogDensity)));
        HeightFog->SetFogHeightFalloff(static_cast<float>(FMath::Max(0.0, Environment.FogHeightFalloff)));
        HeightFog->SetFogInscatteringColor(Environment.FogInscatteringColor);
    }
    if (bControlVolumetricCloudMaterial && VolumetricCloud)
    {
        if (!CloudMaterialInstance)
        {
            if (UMaterialInterface* BaseMaterial = VolumetricCloud->GetMaterial())
            {
                CloudMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
                VolumetricCloud->SetMaterial(CloudMaterialInstance);
            }
        }
        if (CloudMaterialInstance)
        {
            if (!CloudCoverageParameter.IsNone()) CloudMaterialInstance->SetScalarParameterValue(
                CloudCoverageParameter, static_cast<float>(FMath::Clamp(Environment.CloudCoverage, 0.0, 1.0)));
            if (!CloudDensityParameter.IsNone()) CloudMaterialInstance->SetScalarParameterValue(
                CloudDensityParameter, static_cast<float>(FMath::Clamp(Environment.CloudDensity, 0.0, 1.0)));
        }
    }
}
