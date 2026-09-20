#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RBWeatherControlVolume.generated.h"

class UBoxComponent;

/** Local player-facing override such as an interior, cave, greenhouse, or shelter. */
UCLASS(BlueprintType)
class RBWEATHER_API ARBWeatherControlVolume : public AActor
{
    GENERATED_BODY()

public:
    ARBWeatherControlVolume();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Control Area")
    TObjectPtr<UBoxComponent> Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Area")
    FName ControlAreaId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Area")
    int32 Priority = 0;

    /** Spatial fade inside the control-area boundary. 0 = hard edge. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Control Area", meta=(ClampMin="0.0"))
    double TransitionWidthCm = 100.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
    bool bControlTemperature = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
    double TargetTemperatureC = 20.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature", meta=(ClampMin="0.0"))
    double TemperatureAdjustmentLimitC = 10.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
    double MinimumTemperatureC = 5.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
    double MaximumTemperatureC = 35.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double PrecipitationVisualMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double LensEffectMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects", meta=(ClampMin="0.0", ClampMax="2.0"))
    double WeatherAudioMultiplier = 0.25;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects", meta=(ClampMin="0.0", ClampMax="2.0"))
    double WindMultiplier = 0.1;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    double GetLocationWeight(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    bool ContainsLocation(FVector WorldLocation) const { return GetLocationWeight(WorldLocation) > 0.0; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
