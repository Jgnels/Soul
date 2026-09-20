#pragma once

#include "CoreMinimal.h"
#include "RBWeatherTypes.h"

class UDataTable;

/** Unreal adapter around the same pure C++ kernel used by standalone verification. */
class RBWEATHER_API FRBWeatherSimulation
{
public:
    static bool ValidateData(const UDataTable* PresetTable, const UDataTable* TransitionTable,
                             const FRBWeatherClimateRow& Climate, FString& OutError);

    static bool Initialize(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                           const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                           FName InitialWeather, FName Season, int64 DayIndex, double HourOfDay,
                           int64 StableSeed, FRBWeatherSimulationEvents& OutEvents, FString& OutError);

    static bool Advance(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                        const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                        FName Season, int64 DayIndex, double HourOfDay, double DeltaGameHours,
                        int64 StableSeed, FRBWeatherSimulationEvents& OutEvents, FString& OutError);

    static bool ForceWeather(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                             const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                             FName TargetWeather, double TransitionGameHours, int64 StableSeed,
                             FRBWeatherSimulationEvents& OutEvents, FString& OutError);

    static bool GetVisualEnvironment(const FRBWeatherRuntimeState& State, const UDataTable* PresetTable,
                                     const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                     FRBWeatherEnvironment& OutEnvironment, double& OutTransitionAlpha,
                                     FString& OutError);

    static bool GetGameplayEnvironment(const FRBWeatherRuntimeState& State, const UDataTable* PresetTable,
                                       const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                       FRBWeatherEnvironment& OutEnvironment, FString& OutError);

    static double ComputeBaseTemperatureC(const FRBWeatherClimateRow& Climate, FName Season,
                                          int64 DayIndex, double HourOfDay, int64 StableSeed,
                                          bool& bOutValid);

    static FRBWeatherEnvironment BlendEnvironments(const FRBWeatherEnvironment& A,
                                                     const FRBWeatherEnvironment& B, double Alpha);

    static double ApplyLocalTemperatureControl(double CurrentC, double TargetC, double OffsetMagnitudeC,
                                               double MinimumC, double MaximumC, bool& bOutValid);

    static double GetOutsideTemperatureC(const FRBWeatherRuntimeState& State,
                                         const UDataTable* PresetTable,
                                         const UDataTable* TransitionTable,
                                         const FRBWeatherClimateRow& Climate,
                                         bool& bOutValid);

private:
    static int32 ResolveSeasonIndex(const FRBWeatherClimateRow& Climate, FName Season);
};
