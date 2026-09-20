#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RBWeatherTypes.h"
#include "RBWeatherSubsystem.generated.h"

class ARBWeatherDirector;
class ARBWeatherBiomeVolume;
class ARBWeatherControlVolume;
class URBWeatherTemperatureSourceComponent;

struct FRBWeatherCanonicalBiomeRecord
{
    FName BiomeId = NAME_None;
    FName ClimateProfileOverride = NAME_None;
    FName InitialWeather = TEXT("Clear");
    int64 SeedOffset = 0;
    FRBWeatherRuntimeState Weather;
};

/** Spatial query/index plus always-loaded canonical biome state. It owns no clock and performs no Tick. */
UCLASS()
class RBWEATHER_API URBWeatherSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void RegisterDirector(ARBWeatherDirector* Director);
    void UnregisterDirector(ARBWeatherDirector* Director);
    void RegisterBiome(ARBWeatherBiomeVolume* Biome);
    void UnregisterBiome(ARBWeatherBiomeVolume* Biome);
    void RegisterControlArea(ARBWeatherControlVolume* Area);
    void UnregisterControlArea(ARBWeatherControlVolume* Area);
    void RegisterTemperatureSource(URBWeatherTemperatureSourceComponent* Source);
    void UnregisterTemperatureSource(URBWeatherTemperatureSourceComponent* Source);

    UFUNCTION(BlueprintPure, Category="RB Weather")
    ARBWeatherDirector* GetDirector() const;

    UFUNCTION(BlueprintPure, Category="RB Weather")
    bool GetWeatherAtLocation(FVector WorldLocation, FRBWeatherSnapshotAtLocation& OutSnapshot) const;

    ARBWeatherBiomeVolume* FindBiomeById(FName BiomeId) const;
    const TArray<TWeakObjectPtr<ARBWeatherBiomeVolume>>& GetBiomes() const { return Biomes; }
    TMap<FName, FRBWeatherCanonicalBiomeRecord>& GetCanonicalBiomes() { return CanonicalBiomes; }
    const TMap<FName, FRBWeatherCanonicalBiomeRecord>& GetCanonicalBiomes() const { return CanonicalBiomes; }
    FRBWeatherCanonicalBiomeRecord* FindCanonicalBiome(FName BiomeId);
    const FRBWeatherCanonicalBiomeRecord* FindCanonicalBiome(FName BiomeId) const;
    void SyncCanonicalBiomeFromVolume(ARBWeatherBiomeVolume* Biome, const FRBWeatherRuntimeState& State);

private:
    TWeakObjectPtr<ARBWeatherDirector> Director;
    TArray<TWeakObjectPtr<ARBWeatherBiomeVolume>> Biomes;
    TMap<FName, FRBWeatherCanonicalBiomeRecord> CanonicalBiomes;
    TArray<TWeakObjectPtr<ARBWeatherControlVolume>> ControlAreas;
    TArray<TWeakObjectPtr<URBWeatherTemperatureSourceComponent>> TemperatureSources;
};
