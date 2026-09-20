#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RBWeatherTemperatureSourceComponent.generated.h"

/** Attach to fires, heaters, refrigeration, water volumes, etc. No Tick. */
UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBWEATHER_API URBWeatherTemperatureSourceComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    URBWeatherTemperatureSourceComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature", meta=(ClampMin="1.0"))
    double RadiusCm = 500.0;

    /** Positive warms; negative cools. Applied with radial falloff. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature")
    double TemperatureOffsetC = 10.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Temperature", meta=(ClampMin="0.01"))
    double FalloffExponent = 1.0;

    double GetTemperatureOffsetAt(FVector WorldLocation) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
