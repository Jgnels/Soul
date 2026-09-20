#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBWeatherTypes.h"
#include "RBWeatherAtmosphereComponent.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class URBWeatherPlayerComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
class UMaterialInstanceDynamic;

/**
 * Optional UE-native lighting/fog presentation. It never rotates the sun or owns time-of-day.
 */
UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBWEATHER_API URBWeatherAtmosphereComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBWeatherAtmosphereComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    TObjectPtr<URBWeatherPlayerComponent> WeatherSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    TObjectPtr<UDirectionalLightComponent> SunLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    TObjectPtr<USkyLightComponent> SkyLight;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    TObjectPtr<UExponentialHeightFogComponent> HeightFog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    TObjectPtr<UVolumetricCloudComponent> VolumetricCloud;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    bool bControlSunIntensityAndColor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    bool bControlSkyLightIntensity = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere")
    bool bControlFog = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere|Clouds")
    bool bControlVolumetricCloudMaterial = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere|Clouds")
    FName CloudCoverageParameter = TEXT("RB_CloudCoverage");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atmosphere|Clouds")
    FName CloudDensityParameter = TEXT("RB_CloudDensity");

    UFUNCTION(BlueprintCallable, Category="RB Weather|Atmosphere")
    void CaptureBaselines();

    UFUNCTION(BlueprintCallable, Category="RB Weather|Atmosphere")
    bool RefreshAtmosphereNow();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool bBaselinesCaptured = false;
    float BaseSunIntensity = 1.0f;
    FLinearColor BaseSunColor = FLinearColor::White;
    float BaseSkyIntensity = 1.0f;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CloudMaterialInstance;
    UFUNCTION()
    void HandleWeatherUpdated(const FRBWeatherSnapshotAtLocation& Snapshot);

    void BindWeatherSource();
    void ApplySnapshot(const FRBWeatherSnapshotAtLocation& Snapshot);
};
