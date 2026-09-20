#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RBWeatherTypes.h"
#include "RBWeatherBiomeVolume.generated.h"

class UBoxComponent;
class UDataTable;
class ARBWeatherDirector;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBWeatherBiomeStateChangedSignature, const FRBWeatherRuntimeState&, State);

/** Fixed-area climate region. It has no Tick; the director advances every biome from one clock event. */
UCLASS(BlueprintType)
class RBWEATHER_API ARBWeatherBiomeVolume : public AActor
{
    GENERATED_BODY()

public:
    ARBWeatherBiomeVolume();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biome")
    TObjectPtr<UBoxComponent> Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Biome")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Biome")
    int32 Priority = 0;

    /** Fade from the global biome toward this biome across the inside edge. 0 = hard boundary. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Biome", meta=(ClampMin="0.0"))
    double TransitionWidthCm = 1000.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    FName InitialWeather = TEXT("Clear");

    /** None uses the director's climate profile row. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    FName ClimateProfileOverride = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    int64 SeedOffset = 0;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBWeatherBiomeStateChangedSignature OnBiomeStateChanged;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    FRBWeatherRuntimeState GetWeatherState() const { return ReplicatedState; }

    UFUNCTION(BlueprintPure, Category="RB Weather")
    double GetLocationWeight(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    bool ContainsLocation(FVector WorldLocation) const { return GetLocationWeight(WorldLocation) > 0.0; }

    bool AdvanceFromDirector(ARBWeatherDirector* Director, FName Season, int64 DayIndex,
                             double HourOfDay, double DeltaGameHours, FString& OutError);
    bool ForceWeatherFromDirector(ARBWeatherDirector* Director, FName WeatherId,
                                  double TransitionGameHours, FString& OutError);
    bool BuildSnapshot(const ARBWeatherDirector* Director, FRBWeatherSnapshotAtLocation& OutSnapshot,
                       FString& OutError) const;
    bool ValidateSavedState(const ARBWeatherDirector* Director, const FRBWeatherRuntimeState& Candidate,
                            FString& OutError) const;
    void RestoreValidatedState(const FRBWeatherRuntimeState& Candidate);
    void ApplyCanonicalState(const FRBWeatherRuntimeState& Candidate, bool bPublish);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing=OnRep_WeatherState, BlueprintReadOnly, SaveGame, Category="Weather")
    FRBWeatherRuntimeState ReplicatedState;

    UFUNCTION()
    void OnRep_WeatherState();

private:
    void PublishState();
};
