#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "RBWeatherTypes.h"
#include "RBWeatherEnvironmentalExposure.generated.h"

UENUM(BlueprintType)
enum class ERBWeatherTemperatureBand : uint8
{
    VeryCold,
    Cold,
    Normal,
    Hot,
    VeryHot
};

USTRUCT(BlueprintType)
struct RBWEATHER_API FRBWeatherCharacterExposure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Weather") FName WeatherId = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="Weather") ERBWeatherPrecipitation PrecipitationType = ERBWeatherPrecipitation::None;
    UPROPERTY(BlueprintReadOnly, Category="Temperature") double EnvironmentalTemperatureC = 15.0;
    UPROPERTY(BlueprintReadOnly, Category="Temperature") double LocalTemperatureC = 15.0;
    UPROPERTY(BlueprintReadOnly, Category="Temperature") double LocalTemperatureDeltaC = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Weather") double PrecipitationExposure01 = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Weather") double Humidity01 = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Weather") double SurfaceWetness01 = 0.0;
    UPROPERTY(BlueprintReadOnly, Category="Weather") bool bSheltered = false;
    UPROPERTY(BlueprintReadOnly, Category="Weather") bool bWetExposure = false;
    UPROPERTY(BlueprintReadOnly, Category="Weather") bool bWarmingExposure = false;
    UPROPERTY(BlueprintReadOnly, Category="Temperature") ERBWeatherTemperatureBand TemperatureBand = ERBWeatherTemperatureBand::Normal;
};

UINTERFACE(BlueprintType)
class RBWEATHER_API URBWeatherExposureSink : public UInterface
{
    GENERATED_BODY()
};

/** Optional integration point implemented by Attribute Manager adapters or other gameplay systems. */
class RBWEATHER_API IRBWeatherExposureSink
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Weather|Exposure")
    void ApplyRBWeatherExposure(const FRBWeatherCharacterExposure& Exposure);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBWeatherExposureUpdatedSignature,
                                             const FRBWeatherCharacterExposure&, Exposure);
/** Server-side environmental bridge. It owns no character attributes or state effects. */
UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBWEATHER_API URBWeatherEnvironmentalExposureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBWeatherEnvironmentalExposureComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exposure", meta=(ClampMin="0.05"))
    float QueryIntervalSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exposure")
    bool bAuthorityOnly = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter")
    bool bUseAutomaticShelterTrace = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter", meta=(ClampMin="100.0"))
    float ShelterTraceHeightCm = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter")
    TEnumAsByte<ECollisionChannel> ShelterTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wet", meta=(ClampMin="0.0", ClampMax="1.0"))
    double WetExposureThreshold = 0.05;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wet")
    double SnowCountsAsWetAboveC = -2.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Warming", meta=(ClampMin="0.0"))
    double WarmingTemperatureDeltaC = 3.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature")
    double VeryColdBelowC = -15.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature")
    double ColdBelowC = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature")
    double HotAtOrAboveC = 25.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature")
    double VeryHotAtOrAboveC = 35.0;

    UPROPERTY(BlueprintAssignable, Category="RB Weather|Exposure")
    FRBWeatherExposureUpdatedSignature OnExposureUpdated;

    UFUNCTION(BlueprintCallable, Category="RB Weather|Exposure")
    bool RefreshExposureNow();

    UFUNCTION(BlueprintPure, Category="RB Weather|Exposure")
    FRBWeatherCharacterExposure GetCurrentExposure() const { return CurrentExposure; }
    UFUNCTION(BlueprintPure, Category="RB Weather|Exposure")
    ERBWeatherTemperatureBand ClassifyTemperature(double TemperatureC) const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient)
    FRBWeatherCharacterExposure CurrentExposure;

    double TimeAccumulator = 0.0;

    bool IsSheltered() const;
    bool ShouldRunHere() const;
    bool PrecipitationCanWet(ERBWeatherPrecipitation Type, double LocalTemperatureC) const;
    void PublishExposure(const FRBWeatherCharacterExposure& Exposure);
};
