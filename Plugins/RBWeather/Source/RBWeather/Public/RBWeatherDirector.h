#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RBWeatherTypes.h"
#include "RBWeatherDirector.generated.h"

class UDataTable;
class ARBWeatherBiomeVolume;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBWeatherStateChangedSignature, const FRBWeatherRuntimeState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBWeatherLightningSignature, int64, LightningSerial);

/** Global fallback biome, canonical weather tables, replication and external-time integration point. */
UCLASS(BlueprintType)
class RBWEATHER_API ARBWeatherDirector : public AActor
{
    GENERATED_BODY()

public:
    ARBWeatherDirector();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Data")
    TObjectPtr<UDataTable> WeatherPresetTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Data")
    TObjectPtr<UDataTable> WeatherTransitionTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Data")
    TObjectPtr<UDataTable> ClimateTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Data")
    FName ClimateProfileRow = TEXT("Temperate");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    FName InitialWeather = TEXT("Clear");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weather")
    int64 StableSeed = 1337;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBWeatherStateChangedSignature OnGlobalWeatherStateChanged;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBWeatherLightningSignature OnGlobalLightning;

    /** Call from Hyper Time Manager or any canonical time source on the SERVER only. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Weather|Time")
    bool AdvanceExternalTime(double DeltaGameHours, FName Season, int64 DayIndex, double HourOfDay);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Weather|Control")
    bool ForceGlobalWeather(FName WeatherId, double TransitionGameHours = 0.25);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Weather|Control")
    bool ForceBiomeWeather(FName BiomeId, FName WeatherId, double TransitionGameHours = 0.25);

    UFUNCTION(BlueprintPure, Category="RB Weather")
    FRBWeatherRuntimeState GetGlobalState() const { return GlobalState; }

    UFUNCTION(BlueprintPure, Category="RB Weather")
    bool GetGlobalSnapshot(FRBWeatherSnapshotAtLocation& OutSnapshot) const;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    bool GetClimateProfile(FName OverrideRow, FRBWeatherClimateRow& OutClimate) const;

    UFUNCTION(BlueprintCallable, Category="RB Weather|Save")
    FRBWeatherSaveSnapshot MakeSnapshot() const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="RB Weather|Save")
    bool RestoreSnapshot(const FRBWeatherSaveSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category="RB Weather|Validation")
    bool ValidateConfiguredData(FString& OutError) const;

    int64 GetBiomeStableSeed(const ARBWeatherBiomeVolume* Biome) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing=OnRep_GlobalState, BlueprintReadOnly, SaveGame, Category="Weather")
    FRBWeatherRuntimeState GlobalState;

    UFUNCTION()
    void OnRep_GlobalState();

private:
    int64 LastObservedLightningSerial = -1;
    void PublishGlobal(const FRBWeatherSimulationEvents& Events);
    bool InitializeGlobalIfNeeded(FName Season, int64 DayIndex, double HourOfDay, FString& OutError);
};
