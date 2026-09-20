#include "RBWeatherSimulation.h"

#include "Core/RBWeatherCore.h"
#include "Engine/DataTable.h"

namespace
{
using namespace RB::Weather;

struct FBridgeData
{
    Dataset Core;
    TMap<FName, uint32> NameToId;
    TMap<uint32, FName> IdToName;
};

bool Fail(FString& OutError, const TCHAR* Message)
{
    OutError = Message;
    return false;
}

Precipitation ToCorePrecip(ERBWeatherPrecipitation P)
{
    switch (P)
    {
    case ERBWeatherPrecipitation::Rain: return Precipitation::Rain;
    case ERBWeatherPrecipitation::Snow: return Precipitation::Snow;
    case ERBWeatherPrecipitation::Sleet: return Precipitation::Sleet;
    case ERBWeatherPrecipitation::Hail: return Precipitation::Hail;
    case ERBWeatherPrecipitation::Dust: return Precipitation::Dust;
    default: return Precipitation::None;
    }
}

ERBWeatherPrecipitation FromCorePrecip(Precipitation P)
{
    switch (P)
    {
    case Precipitation::Rain: return ERBWeatherPrecipitation::Rain;
    case Precipitation::Snow: return ERBWeatherPrecipitation::Snow;
    case Precipitation::Sleet: return ERBWeatherPrecipitation::Sleet;
    case Precipitation::Hail: return ERBWeatherPrecipitation::Hail;
    case Precipitation::Dust: return ERBWeatherPrecipitation::Dust;
    default: return ERBWeatherPrecipitation::None;
    }
}

Color ToCoreColor(const FLinearColor& C)
{
    return {C.R, C.G, C.B, C.A};
}

FLinearColor FromCoreColor(const Color& C)
{
    return FLinearColor(static_cast<float>(C.R), static_cast<float>(C.G),
                        static_cast<float>(C.B), static_cast<float>(C.A));
}

Environment ToCoreEnvironment(const FRBWeatherEnvironment& E)
{
    Environment C{};
    C.CloudCoverage = E.CloudCoverage;
    C.CloudDensity = E.CloudDensity;
    C.SunIntensityScale = E.SunIntensityScale;
    C.SunLightColor = ToCoreColor(E.SunLightColor);
    C.SkyLightIntensityScale = E.SkyLightIntensityScale;
    C.FogDensity = E.FogDensity;
    C.FogInscatteringColor = ToCoreColor(E.FogInscatteringColor);
    C.FogHeightFalloff = E.FogHeightFalloff;
    C.VisibilityKm = E.VisibilityKm;
    C.WindSpeedMps = E.WindSpeedMps;
    C.WindGustMps = E.WindGustMps;
    C.WindDirectionDegrees = E.WindDirectionDegrees;
    C.PrecipitationType = ToCorePrecip(E.PrecipitationType);
    C.PrecipitationIntensity = E.PrecipitationIntensity;
    C.LightningRatePerHour = E.LightningRatePerGameHour;
    C.TemperatureOffsetC = E.TemperatureOffsetC;
    C.Humidity01 = E.Humidity;
    C.WetnessGainPerHour = E.WetnessGainPerGameHour;
    C.DryingPerHour = E.DryingPerGameHour;
    C.SnowGainCmPerHour = E.SnowGainCmPerGameHour;
    C.SnowMeltCmPerHourPerC = E.SnowMeltCmPerGameHourPerC;
    C.SnowMeltStartC = E.SnowMeltStartC;
    C.DustGainPerHour = E.DustGainPerGameHour;
    C.DustWashPerHour = E.DustWashPerGameHour;
    C.LensWetness = E.LensWetness;
    C.LensFrost = E.LensFrost;
    C.LensDust = E.LensDust;
    C.AudioIntensity = E.AudioIntensity;
    return C;
}

FRBWeatherEnvironment FromCoreEnvironment(const Environment& C)
{
    FRBWeatherEnvironment E{};
    E.CloudCoverage = C.CloudCoverage;
    E.CloudDensity = C.CloudDensity;
    E.SunIntensityScale = C.SunIntensityScale;
    E.SunLightColor = FromCoreColor(C.SunLightColor);
    E.SkyLightIntensityScale = C.SkyLightIntensityScale;
    E.FogDensity = C.FogDensity;
    E.FogInscatteringColor = FromCoreColor(C.FogInscatteringColor);
    E.FogHeightFalloff = C.FogHeightFalloff;
    E.VisibilityKm = C.VisibilityKm;
    E.WindSpeedMps = C.WindSpeedMps;
    E.WindGustMps = C.WindGustMps;
    E.WindDirectionDegrees = C.WindDirectionDegrees;
    E.PrecipitationType = FromCorePrecip(C.PrecipitationType);
    E.PrecipitationIntensity = C.PrecipitationIntensity;
    E.LightningRatePerGameHour = C.LightningRatePerHour;
    E.TemperatureOffsetC = C.TemperatureOffsetC;
    E.Humidity = C.Humidity01;
    E.WetnessGainPerGameHour = C.WetnessGainPerHour;
    E.DryingPerGameHour = C.DryingPerHour;
    E.SnowGainCmPerGameHour = C.SnowGainCmPerHour;
    E.SnowMeltCmPerGameHourPerC = C.SnowMeltCmPerHourPerC;
    E.SnowMeltStartC = C.SnowMeltStartC;
    E.DustGainPerGameHour = C.DustGainPerHour;
    E.DustWashPerGameHour = C.DustWashPerHour;
    E.LensWetness = C.LensWetness;
    E.LensFrost = C.LensFrost;
    E.LensDust = C.LensDust;
    E.AudioIntensity = C.AudioIntensity;
    return E;
}

const FRBWeatherSeasonBand& SeasonBand(const FRBWeatherClimateRow& Climate, int32 Index)
{
    switch (Index)
    {
    case 0: return Climate.Season0;
    case 1: return Climate.Season1;
    case 2: return Climate.Season2;
    default: return Climate.Season3;
    }
}

int32 ResolveSeason(const FRBWeatherClimateRow& Climate, FName Season)
{
    if (Season.IsNone()) { return INDEX_NONE; }
    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (SeasonBand(Climate, Index).SeasonName == Season) { return Index; }
    }
    return INDEX_NONE;
}

bool ValidateClimate(const FRBWeatherClimateRow& Climate, FString& OutError)
{
    TSet<FName> Names;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const auto& B = SeasonBand(Climate, Index);
        if (B.SeasonName.IsNone()) return Fail(OutError, TEXT("All four climate season names must be set."));
        if (Names.Contains(B.SeasonName)) return Fail(OutError, TEXT("Climate season names must be unique."));
        Names.Add(B.SeasonName);
        if (!FMath::IsFinite(B.MinimumTemperatureC) || !FMath::IsFinite(B.MaximumTemperatureC) ||
            B.MinimumTemperatureC > B.MaximumTemperatureC)
            return Fail(OutError, TEXT("Climate season temperature bounds are invalid."));
    }
    if (!FMath::IsFinite(Climate.DailyVariationFraction) || Climate.DailyVariationFraction < 0.0 ||
        Climate.DailyVariationFraction > 1.0 || !FMath::IsFinite(Climate.DiurnalFraction) ||
        Climate.DiurnalFraction < 0.0 || Climate.DiurnalFraction > 1.0 ||
        !FMath::IsFinite(Climate.PeakTemperatureHour))
        return Fail(OutError, TEXT("Climate variation settings are invalid."));
    return true;
}

bool BuildBridge(const UDataTable* PresetTable, const UDataTable* TransitionTable,
                 const FRBWeatherClimateRow& Climate, FBridgeData& Out, FString& OutError)
{
    Out = {};
    if (!PresetTable) return Fail(OutError, TEXT("Weather preset DataTable is required."));
    if (PresetTable->GetRowStruct() != FRBWeatherPresetRow::StaticStruct())
        return Fail(OutError, TEXT("Weather preset DataTable must use FRBWeatherPresetRow."));
    if (TransitionTable && TransitionTable->GetRowStruct() != FRBWeatherTransitionRow::StaticStruct())
        return Fail(OutError, TEXT("Weather transition DataTable must use FRBWeatherTransitionRow."));
    if (!ValidateClimate(Climate, OutError)) return false;

    TArray<FName> Names = PresetTable->GetRowNames();
    Names.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
    if (Names.IsEmpty()) return Fail(OutError, TEXT("Weather preset DataTable has no rows."));
    uint32 NumericId = 1;
    for (const FName Name : Names)
    {
        if (Name.IsNone()) return Fail(OutError, TEXT("Weather preset row name cannot be None."));
        const auto* Row = PresetTable->FindRow<FRBWeatherPresetRow>(Name, TEXT("RBWeather BuildBridge"), false);
        if (!Row) return Fail(OutError, TEXT("Weather preset row could not be read."));
        Preset P{};
        P.Id = NumericId;
        P.Env = ToCoreEnvironment(Row->Environment);
        P.ActiveMinHours = Row->ActiveMinGameHours;
        P.ActiveMaxHours = Row->ActiveMaxGameHours;
        P.TransitionMinHours = Row->TransitionMinGameHours;
        P.TransitionMaxHours = Row->TransitionMaxGameHours;
        Out.Core.Presets.push_back(P);
        Out.NameToId.Add(Name, NumericId);
        Out.IdToName.Add(NumericId, Name);
        ++NumericId;
    }

    if (TransitionTable)
    {
        for (const FName RowName : TransitionTable->GetRowNames())
        {
            const auto* Row = TransitionTable->FindRow<FRBWeatherTransitionRow>(RowName, TEXT("RBWeather BuildBridge"), false);
            if (!Row) return Fail(OutError, TEXT("Weather transition row could not be read."));
            const uint32* From = Out.NameToId.Find(Row->FromWeather);
            const uint32* To = Out.NameToId.Find(Row->ToWeather);
            if (!From || !To) return Fail(OutError, TEXT("Weather transition references an unknown preset row name."));
            TransitionRule R{};
            R.FromId = *From;
            R.ToId = *To;
            R.Weight = Row->Weight;
            R.MinimumBaseTemperatureC = Row->MinimumBaseTemperatureC;
            R.MaximumBaseTemperatureC = Row->MaximumBaseTemperatureC;
            if (Row->AllowedSeasons.IsEmpty())
            {
                R.SeasonMask = 0x0F;
            }
            else
            {
                // Season names are scoped to a climate profile. A shared transition table may
                // contain rules for other profiles; unmatched names make this rule inapplicable
                // to the current climate rather than invalidating the entire weather dataset.
                R.SeasonMask = 0;
                for (const FName Allowed : Row->AllowedSeasons)
                {
                    const int32 SeasonIndex = ResolveSeason(Climate, Allowed);
                    if (SeasonIndex != INDEX_NONE)
                    {
                        R.SeasonMask |= static_cast<uint8>(1u << SeasonIndex);
                    }
                }
                if (R.SeasonMask == 0) continue;
            }
            Out.Core.Rules.push_back(R);
        }
    }

    std::string CoreError;
    if (!ValidateDataset(Out.Core, &CoreError))
    {
        OutError = FString::Printf(TEXT("Weather data failed kernel validation: %s"), UTF8_TO_TCHAR(CoreError.c_str()));
        return false;
    }
    return true;
}

bool ToCoreState(const FRBWeatherRuntimeState& U, const FBridgeData& Data, State& C, FString& OutError)
{
    if (U.TransitionSerial < 0 || U.LightningSerial < 0)
        return Fail(OutError, TEXT("Weather serials cannot be negative."));
    const uint32* Current = Data.NameToId.Find(U.CurrentWeather);
    const uint32* Target = Data.NameToId.Find(U.TargetWeather);
    if (!Current || !Target) return Fail(OutError, TEXT("Weather runtime state references an unknown preset."));
    C.Initialized = U.bInitialized;
    C.CurrentId = *Current;
    C.TargetId = *Target;
    C.TransitionElapsedHours = U.TransitionElapsedGameHours;
    C.TransitionDurationHours = U.TransitionDurationGameHours;
    C.ActiveRemainingHours = U.ActiveRemainingGameHours;
    C.TransitionSerial = static_cast<uint64>(U.TransitionSerial);
    C.LightningSerial = static_cast<uint64>(U.LightningSerial);
    C.NextLightningHours = U.NextLightningGameHours;
    C.Wetness01 = U.Wetness;
    C.SnowDepthCm = U.SnowDepthCm;
    C.Dust01 = U.Dust;
    std::string Error;
    if (!ValidateState(C, Data.Core, &Error))
    {
        OutError = FString::Printf(TEXT("Weather runtime state failed kernel validation: %s"), UTF8_TO_TCHAR(Error.c_str()));
        return false;
    }
    return true;
}

void FromCoreState(const State& C, const FBridgeData& Data, FRBWeatherRuntimeState& U)
{
    U.bInitialized = C.Initialized;
    U.CurrentWeather = Data.IdToName.FindRef(C.CurrentId);
    U.TargetWeather = Data.IdToName.FindRef(C.TargetId);
    U.TransitionElapsedGameHours = C.TransitionElapsedHours;
    U.TransitionDurationGameHours = C.TransitionDurationHours;
    U.ActiveRemainingGameHours = C.ActiveRemainingHours;
    U.TransitionSerial = static_cast<int64>(C.TransitionSerial);
    U.LightningSerial = static_cast<int64>(C.LightningSerial);
    U.NextLightningGameHours = C.NextLightningHours;
    U.Wetness = C.Wetness01;
    U.SnowDepthCm = C.SnowDepthCm;
    U.Dust = C.Dust01;
}

FRBWeatherSimulationEvents FromCoreEvents(const Events& E)
{
    FRBWeatherSimulationEvents U{};
    U.bChanged = E.Changed;
    U.bTransitionStarted = E.TransitionStarted;
    U.bTransitionCompleted = E.TransitionCompleted;
    U.bWeatherChanged = E.WeatherChanged;
    U.LightningCount = static_cast<int32>(FMath::Min<uint32>(E.LightningCount, static_cast<uint32>(MAX_int32)));
    return U;
}

Climate ToCoreClimate(const FRBWeatherClimateRow& U)
{
    Climate C{};
    for (int32 Index = 0; Index < 4; ++Index)
    {
        C.MinimumC[static_cast<std::size_t>(Index)] = SeasonBand(U, Index).MinimumTemperatureC;
        C.MaximumC[static_cast<std::size_t>(Index)] = SeasonBand(U, Index).MaximumTemperatureC;
    }
    C.DailyVariationFraction = U.DailyVariationFraction;
    C.DiurnalFraction = U.DiurnalFraction;
    C.PeakHour = U.PeakTemperatureHour;
    return C;
}

} // namespace

int32 FRBWeatherSimulation::ResolveSeasonIndex(const FRBWeatherClimateRow& Climate, FName Season)
{
    return ResolveSeason(Climate, Season);
}

bool FRBWeatherSimulation::ValidateData(const UDataTable* PresetTable, const UDataTable* TransitionTable,
                                        const FRBWeatherClimateRow& Climate, FString& OutError)
{
    FBridgeData Data;
    return BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError);
}

bool FRBWeatherSimulation::Initialize(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                                      const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                      FName InitialWeather, FName Season, int64 DayIndex, double HourOfDay,
                                      int64 StableSeed, FRBWeatherSimulationEvents& OutEvents, FString& OutError)
{
    OutEvents = {};
    FBridgeData Data;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError)) return false;
    const uint32* InitialId = Data.NameToId.Find(InitialWeather);
    if (!InitialId) return Fail(OutError, TEXT("Initial weather is not a preset row name."));
    const int32 SeasonIndex = ResolveSeason(Climate, Season);
    if (SeasonIndex == INDEX_NONE) return Fail(OutError, TEXT("Initial season is not configured by the climate row."));
    bool bTempValid = false;
    const double BaseTemp = ComputeBaseTemperatureC(Climate, Season, DayIndex, HourOfDay, StableSeed, bTempValid);
    if (!bTempValid) return Fail(OutError, TEXT("Could not compute initial base temperature."));

    State CoreState{};
    Events CoreEvents{};
    if (!RB::Weather::Initialize(CoreState, Data.Core, *InitialId, static_cast<uint64>(StableSeed), &CoreEvents))
        return Fail(OutError, TEXT("Weather kernel initialization failed."));
    FRBWeatherRuntimeState Candidate{};
    FromCoreState(CoreState, Data, Candidate);
    Candidate.CurrentSeason = Season;
    Candidate.DayIndex = DayIndex;
    Candidate.HourOfDay = HourOfDay;
    Candidate.BaseTemperatureC = BaseTemp;
    InOutState = Candidate;
    OutEvents = FromCoreEvents(CoreEvents);
    OutError.Reset();
    return true;
}

bool FRBWeatherSimulation::Advance(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                                   const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                   FName Season, int64 DayIndex, double HourOfDay, double DeltaGameHours,
                                   int64 StableSeed, FRBWeatherSimulationEvents& OutEvents, FString& OutError)
{
    OutEvents = {};
    FBridgeData Data;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError)) return false;
    const int32 SeasonIndex = ResolveSeason(Climate, Season);
    if (SeasonIndex == INDEX_NONE) return Fail(OutError, TEXT("Season is not configured by the climate row."));
    bool bTempValid = false;
    const double BaseTemp = ComputeBaseTemperatureC(Climate, Season, DayIndex, HourOfDay, StableSeed, bTempValid);
    if (!bTempValid) return Fail(OutError, TEXT("Could not compute base temperature."));
    State CoreState{};
    if (!ToCoreState(InOutState, Data, CoreState, OutError)) return false;
    Context C{};
    C.Season = static_cast<uint8>(SeasonIndex);
    C.BaseTemperatureC = BaseTemp;
    Events CoreEvents{};
    if (!RB::Weather::Advance(CoreState, Data.Core, C, DeltaGameHours, static_cast<uint64>(StableSeed), CoreEvents))
        return Fail(OutError, TEXT("Weather kernel advance failed; caller state was preserved."));
    FRBWeatherRuntimeState Candidate = InOutState;
    FromCoreState(CoreState, Data, Candidate);
    Candidate.CurrentSeason = Season;
    Candidate.DayIndex = DayIndex;
    Candidate.HourOfDay = HourOfDay;
    Candidate.BaseTemperatureC = BaseTemp;
    InOutState = Candidate;
    OutEvents = FromCoreEvents(CoreEvents);
    OutError.Reset();
    return true;
}

bool FRBWeatherSimulation::ForceWeather(FRBWeatherRuntimeState& InOutState, const UDataTable* PresetTable,
                                        const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                        FName TargetWeather, double TransitionGameHours, int64 StableSeed,
                                        FRBWeatherSimulationEvents& OutEvents, FString& OutError)
{
    OutEvents = {};
    FBridgeData Data;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError)) return false;
    const uint32* TargetId = Data.NameToId.Find(TargetWeather);
    if (!TargetId) return Fail(OutError, TEXT("Forced weather is not a preset row name."));
    State CoreState{};
    if (!ToCoreState(InOutState, Data, CoreState, OutError)) return false;
    Events CoreEvents{};
    if (!RB::Weather::ForceWeather(CoreState, Data.Core, *TargetId, TransitionGameHours,
                                   static_cast<uint64>(StableSeed), CoreEvents))
        return Fail(OutError, TEXT("Weather kernel force transition failed."));
    FRBWeatherRuntimeState Candidate = InOutState;
    FromCoreState(CoreState, Data, Candidate);
    InOutState = Candidate;
    OutEvents = FromCoreEvents(CoreEvents);
    OutError.Reset();
    return true;
}

bool FRBWeatherSimulation::GetVisualEnvironment(const FRBWeatherRuntimeState& State, const UDataTable* PresetTable,
                                                const UDataTable* TransitionTable, const FRBWeatherClimateRow& Climate,
                                                FRBWeatherEnvironment& OutEnvironment, double& OutTransitionAlpha,
                                                FString& OutError)
{
    FBridgeData Data;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError)) return false;
    RB::Weather::State CoreState{};
    if (!ToCoreState(State, Data, CoreState, OutError)) return false;
    Environment CoreEnv{};
    if (!RB::Weather::GetVisualEnvironment(CoreState, Data.Core, CoreEnv))
        return Fail(OutError, TEXT("Weather kernel could not build the visual environment."));
    OutEnvironment = FromCoreEnvironment(CoreEnv);
    OutTransitionAlpha = RB::Weather::GetTransitionAlpha(CoreState);
    OutError.Reset();
    return true;
}

bool FRBWeatherSimulation::GetGameplayEnvironment(const FRBWeatherRuntimeState& State,
                                                    const UDataTable* PresetTable,
                                                    const UDataTable* TransitionTable,
                                                    const FRBWeatherClimateRow& Climate,
                                                    FRBWeatherEnvironment& OutEnvironment, FString& OutError)
{
    FBridgeData Data;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, OutError)) return false;
    RB::Weather::State CoreState{};
    if (!ToCoreState(State, Data, CoreState, OutError)) return false;
    Environment CoreEnv{};
    if (!RB::Weather::GetGameplayEnvironment(CoreState, Data.Core, CoreEnv))
        return Fail(OutError, TEXT("Weather kernel could not build the gameplay environment."));
    OutEnvironment = FromCoreEnvironment(CoreEnv);
    OutError.Reset();
    return true;
}

FRBWeatherEnvironment FRBWeatherSimulation::BlendEnvironments(const FRBWeatherEnvironment& A,
                                                                    const FRBWeatherEnvironment& B,
                                                                    double Alpha)
{
    return FromCoreEnvironment(RB::Weather::BlendEnvironment(ToCoreEnvironment(A), ToCoreEnvironment(B), Alpha));
}

double FRBWeatherSimulation::ApplyLocalTemperatureControl(double CurrentC, double TargetC,
                                                              double OffsetMagnitudeC, double MinimumC,
                                                              double MaximumC, bool& bOutValid)
{
    const double Result = RB::Weather::ApplyLocalTemperatureControl(CurrentC, TargetC, OffsetMagnitudeC, MinimumC, MaximumC);
    bOutValid = FMath::IsFinite(Result);
    return bOutValid ? Result : CurrentC;
}

double FRBWeatherSimulation::ComputeBaseTemperatureC(const FRBWeatherClimateRow& Climate, FName Season,
                                                     int64 DayIndex, double HourOfDay, int64 StableSeed,
                                                     bool& bOutValid)
{
    bOutValid = false;
    FString Error;
    if (!ValidateClimate(Climate, Error)) return 0.0;
    const int32 SeasonIndex = ResolveSeason(Climate, Season);
    if (SeasonIndex == INDEX_NONE) return 0.0;
    const double Result = RB::Weather::ComputeGlobalTemperatureC(ToCoreClimate(Climate),
        static_cast<uint8>(SeasonIndex), DayIndex, HourOfDay, static_cast<uint64>(StableSeed));
    bOutValid = FMath::IsFinite(Result);
    return bOutValid ? Result : 0.0;
}

double FRBWeatherSimulation::GetOutsideTemperatureC(const FRBWeatherRuntimeState& State,
                                                    const UDataTable* PresetTable,
                                                    const UDataTable* TransitionTable,
                                                    const FRBWeatherClimateRow& Climate,
                                                    bool& bOutValid)
{
    bOutValid = false;
    FBridgeData Data;
    FString Error;
    if (!BuildBridge(PresetTable, TransitionTable, Climate, Data, Error)) return 0.0;
    RB::Weather::State CoreState{};
    if (!ToCoreState(State, Data, CoreState, Error)) return 0.0;
    const double Result = RB::Weather::GetOutsideTemperatureC(CoreState, Data.Core, State.BaseTemperatureC);
    bOutValid = FMath::IsFinite(Result);
    return bOutValid ? Result : 0.0;
}
