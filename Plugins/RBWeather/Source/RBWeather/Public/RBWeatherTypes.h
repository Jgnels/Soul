#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RBWeatherTypes.generated.h"

UENUM(BlueprintType)
enum class ERBWeatherPrecipitation : uint8
{
    None,
    Rain,
    Snow,
    Sleet,
    Hail,
    Dust
};

/** Data-driven environment values. Visual consumers may use only the fields they support. */
USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherEnvironment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky", meta=(ClampMin="0.0", ClampMax="1.0"))
    double CloudCoverage = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky", meta=(ClampMin="0.0", ClampMax="1.0"))
    double CloudDensity = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0.0"))
    double SunIntensityScale = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting")
    FLinearColor SunLightColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lighting", meta=(ClampMin="0.0"))
    double SkyLightIntensityScale = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta=(ClampMin="0.0"))
    double FogDensity = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
    FLinearColor FogInscatteringColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta=(ClampMin="0.0"))
    double FogHeightFalloff = 0.2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog", meta=(ClampMin="0.001"))
    double VisibilityKm = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind", meta=(ClampMin="0.0"))
    double WindSpeedMps = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind", meta=(ClampMin="0.0"))
    double WindGustMps = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wind")
    double WindDirectionDegrees = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Precipitation")
    ERBWeatherPrecipitation PrecipitationType = ERBWeatherPrecipitation::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Precipitation", meta=(ClampMin="0.0", ClampMax="1.0"))
    double PrecipitationIntensity = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storm", meta=(ClampMin="0.0"))
    double LightningRatePerGameHour = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
    double TemperatureOffsetC = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay", meta=(ClampMin="0.0", ClampMax="1.0"))
    double Humidity = 0.35;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double WetnessGainPerGameHour = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double DryingPerGameHour = 0.05;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double SnowGainCmPerGameHour = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double SnowMeltCmPerGameHourPerC = 0.25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface")
    double SnowMeltStartC = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double DustGainPerGameHour = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Surface", meta=(ClampMin="0.0"))
    double DustWashPerGameHour = 0.25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double LensWetness = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double LensFrost = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double LensDust = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
    double AudioIntensity = 0.0;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherPresetRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    FText DisplayName;

    /** How long the completed weather condition remains before another Markov transition is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scheduling", meta=(ClampMin="0.001"))
    double ActiveMinGameHours = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scheduling", meta=(ClampMin="0.001"))
    double ActiveMaxGameHours = 3.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scheduling", meta=(ClampMin="0.0"))
    double TransitionMinGameHours = 0.05;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scheduling", meta=(ClampMin="0.0"))
    double TransitionMaxGameHours = 0.25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment")
    FRBWeatherEnvironment Environment;
};

/** One directed Markov edge. Row name is descriptive only; From/To weather use preset row names. */
USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherTransitionRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    FName FromWeather = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    FName ToWeather = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="0.0"))
    double Weight = 1.0;

    /** Empty means all configured seasons. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    TArray<FName> AllowedSeasons;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    double MinimumBaseTemperatureC = -1000.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
    double MaximumBaseTemperatureC = 1000.0;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherSeasonBand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Season")
    FName SeasonName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Season")
    double MinimumTemperatureC = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Season")
    double MaximumTemperatureC = 20.0;
};

/** Four seasons are supported in the compiled simulation kernel; names remain project-configurable. */
USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherClimateRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate")
    FRBWeatherSeasonBand Season0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate")
    FRBWeatherSeasonBand Season1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate")
    FRBWeatherSeasonBand Season2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate")
    FRBWeatherSeasonBand Season3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate", meta=(ClampMin="0.0", ClampMax="1.0"))
    double DailyVariationFraction = 0.20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate", meta=(ClampMin="0.0", ClampMax="1.0"))
    double DiurnalFraction = 0.75;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Climate")
    double PeakTemperatureHour = 15.0;

    FRBWeatherClimateRow()
    {
        Season0.SeasonName = TEXT("Winter"); Season0.MinimumTemperatureC = -15.0; Season0.MaximumTemperatureC = 7.0;
        Season1.SeasonName = TEXT("Spring"); Season1.MinimumTemperatureC = 10.0; Season1.MaximumTemperatureC = 20.0;
        Season2.SeasonName = TEXT("Summer"); Season2.MinimumTemperatureC = 20.0; Season2.MaximumTemperatureC = 45.0;
        Season3.SeasonName = TEXT("Fall"); Season3.MinimumTemperatureC = 2.0; Season3.MaximumTemperatureC = 17.0;
    }
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    bool bInitialized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    FName CurrentWeather = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    FName TargetWeather = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    double TransitionElapsedGameHours = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    double TransitionDurationGameHours = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    double ActiveRemainingGameHours = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Weather")
    int64 TransitionSerial = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Storm")
    int64 LightningSerial = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Storm")
    double NextLightningGameHours = -1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Surface")
    double Wetness = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Surface")
    double SnowDepthCm = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Surface")
    double Dust = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Time")
    FName CurrentSeason = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Time")
    int64 DayIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Time")
    double HourOfDay = 12.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Temperature")
    double BaseTemperatureC = 15.0;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherSimulationEvents
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    bool bChanged = false;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    bool bTransitionStarted = false;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    bool bTransitionCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    bool bWeatherChanged = false;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    int32 LightningCount = 0;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherSnapshotAtLocation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    FName BiomeId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    double BiomeBlendWeight = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    FName ControlAreaId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    double ControlAreaBlendWeight = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    FName CurrentWeather = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    FName TargetWeather = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    double TransitionAlpha = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Storm")
    int64 LightningSerial = 0;

    /** Smooth/blended presentation environment for the locally rendered view. */
    UPROPERTY(BlueprintReadOnly, Category="Weather|Presentation")
    FRBWeatherEnvironment Environment;

    /** Discrete authoritative environment used by gameplay at this location. */
    UPROPERTY(BlueprintReadOnly, Category="Weather|Gameplay")
    FRBWeatherEnvironment GameplayEnvironment;

    UPROPERTY(BlueprintReadOnly, Category="Weather|Gameplay")
    double Wetness = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather")
    double SnowDepthCm = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather|Gameplay")
    double Dust = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather|Presentation")
    double VisualWetness = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather|Presentation")
    double VisualSnowDepthCm = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Weather|Presentation")
    double VisualDust = 0.0;

    UPROPERTY(BlueprintReadOnly, Category="Temperature|Gameplay")
    double BaseTemperatureC = 15.0;

    UPROPERTY(BlueprintReadOnly, Category="Temperature|Gameplay")
    double OutsideTemperatureC = 15.0;

    UPROPERTY(BlueprintReadOnly, Category="Temperature|Presentation")
    double VisualOutsideTemperatureC = 15.0;

    UPROPERTY(BlueprintReadOnly, Category="Temperature|Gameplay")
    double LocalTemperatureC = 15.0;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    double PrecipitationVisualMultiplier = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    double LensEffectMultiplier = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    double WeatherAudioMultiplier = 1.0;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects")
    double LocalWindMultiplier = 1.0;

    /** Player-local presentation shelter. Never mutates authoritative climate. */
    UPROPERTY(BlueprintReadOnly, Category="Local Effects|Shelter")
    bool bSheltered = false;

    UPROPERTY(BlueprintReadOnly, Category="Local Effects|Shelter")
    double ShelterWeight = 0.0;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherBiomeSaveState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    FName ClimateProfileOverride = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    FName InitialWeather = TEXT("Clear");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    int64 SeedOffset = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    FRBWeatherRuntimeState Weather;
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherSaveSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    int32 SchemaVersion = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    FRBWeatherRuntimeState GlobalWeather;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Save")
    TArray<FRBWeatherBiomeSaveState> Biomes;
};
