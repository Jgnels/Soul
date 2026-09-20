#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RBWeatherTypes.h"
#include "RBWeatherPresentationProfile.generated.h"

class UNiagaraSystem;
class USoundBase;

/** Optional project-side art assigned to one weather presentation slot. */
USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherPresentationSlot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    TSoftObjectPtr<USoundBase> LoopingSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.0"))
    float VisualScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation", meta=(ClampMin="0.0"))
    float AudioVolumeScale = 1.0f;
};
USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherPresentationOverride
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FName WeatherId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
    FRBWeatherPresentationSlot Slot;
};

/**
 * Pure presentation configuration. It never owns weather state or timing.
 * RB Weather ships a native default profile; project-specific and third-party assets remain optional overrides.
 */
UCLASS(BlueprintType)
class RBWEATHER_API URBWeatherPresentationProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Precipitation")
    FRBWeatherPresentationSlot Rain;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Precipitation")
    FRBWeatherPresentationSlot Snow;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Precipitation")
    FRBWeatherPresentationSlot Sleet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Precipitation")
    FRBWeatherPresentationSlot Hail;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Precipitation")
    FRBWeatherPresentationSlot Dust;

    /** Optional special art for named states such as Blizzard or Sandstorm. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather Overrides")
    TArray<FRBWeatherPresentationOverride> WeatherOverrides;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wind")
    TSoftObjectPtr<UNiagaraSystem> WindSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm")
    TSoftObjectPtr<UNiagaraSystem> LightningSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Storm")
    TSoftObjectPtr<USoundBase> LightningSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName IntensityParameter = TEXT("User.RB_Intensity");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName WindVelocityParameter = TEXT("User.RB_WindVelocity");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName WindSpeedParameter = TEXT("User.RB_WindSpeedMps");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName WindGustParameter = TEXT("User.RB_WindGustMps");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName ShelterParameter = TEXT("User.RB_Sheltered");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName TemperatureParameter = TEXT("User.RB_TemperatureC");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Niagara Parameters")
    FName HumidityParameter = TEXT("User.RB_Humidity");

    const FRBWeatherPresentationSlot* ResolveSlot(const FRBWeatherSnapshotAtLocation& Snapshot) const;
};
