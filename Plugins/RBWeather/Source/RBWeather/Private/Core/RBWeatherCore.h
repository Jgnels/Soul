#pragma once

// Engine-independent production weather kernel. This file is compiled by both
// UnrealBuildTool (through RBWeatherSimulation.cpp) and the standalone tests.
// Keep Unreal types and mock engine headers out of this layer.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace RB::Weather
{
constexpr double MaxDeltaHours = 1000000.0;
constexpr double MaxSurfaceSnowCm = 10000.0;
constexpr std::size_t MaxTransitionsPerAdvance = 100000;

enum class Precipitation : std::uint8_t
{
    None,
    Rain,
    Snow,
    Sleet,
    Hail,
    Dust
};

struct Color
{
    double R = 1.0;
    double G = 1.0;
    double B = 1.0;
    double A = 1.0;
};

struct Environment
{
    // 0..1 unless otherwise noted.
    double CloudCoverage = 0.0;
    double CloudDensity = 0.0;
    double SunIntensityScale = 1.0;
    Color SunLightColor{};
    double SkyLightIntensityScale = 1.0;
    double FogDensity = 0.0;
    Color FogInscatteringColor{};
    double FogHeightFalloff = 0.2;
    double VisibilityKm = 100.0;
    double WindSpeedMps = 0.0;
    double WindGustMps = 0.0;
    double WindDirectionDegrees = 0.0;
    Precipitation PrecipitationType = Precipitation::None;
    double PrecipitationIntensity = 0.0;
    double LightningRatePerHour = 0.0;
    double TemperatureOffsetC = 0.0;
    double Humidity01 = 0.35;
    double WetnessGainPerHour = 0.0;
    double DryingPerHour = 0.05;
    double SnowGainCmPerHour = 0.0;
    double SnowMeltCmPerHourPerC = 0.25;
    double SnowMeltStartC = 0.0;
    double DustGainPerHour = 0.0;
    double DustWashPerHour = 0.25;
    double LensWetness = 0.0;
    double LensFrost = 0.0;
    double LensDust = 0.0;
    double AudioIntensity = 0.0;
};

struct Preset
{
    std::uint32_t Id = 0;
    Environment Env{};
    double ActiveMinHours = 1.0;
    double ActiveMaxHours = 3.0;
    double TransitionMinHours = 0.05;
    double TransitionMaxHours = 0.25;
};

struct TransitionRule
{
    std::uint32_t FromId = 0;
    std::uint32_t ToId = 0;
    double Weight = 1.0;
    // Bit 0..3 = season 0..3. 0x0F means all four seasons.
    std::uint8_t SeasonMask = 0x0F;
    double MinimumBaseTemperatureC = -1000.0;
    double MaximumBaseTemperatureC = 1000.0;
};

struct Dataset
{
    std::vector<Preset> Presets;
    std::vector<TransitionRule> Rules;
};

struct Climate
{
    // Seasonal absolute outside-temperature bounds before weather offsets.
    std::array<double, 4> MinimumC{{-15.0, 5.0, 18.0, 2.0}};
    std::array<double, 4> MaximumC{{7.0, 22.0, 42.0, 18.0}};
    // Fraction of the seasonal half-range assigned to deterministic day-to-day variation.
    double DailyVariationFraction = 0.20;
    // Fraction assigned to the time-of-day cosine. Remaining range is headroom.
    double DiurnalFraction = 0.75;
    double PeakHour = 15.0;
};

struct Context
{
    std::uint8_t Season = 0;
    double BaseTemperatureC = 15.0; // global time/season temperature, before weather offset
};

struct State
{
    bool Initialized = false;
    std::uint32_t CurrentId = 0;
    std::uint32_t TargetId = 0;
    double TransitionElapsedHours = 0.0;
    double TransitionDurationHours = 0.0;
    double ActiveRemainingHours = 0.0;
    std::uint64_t TransitionSerial = 0;
    std::uint64_t LightningSerial = 0;
    double NextLightningHours = -1.0;
    double Wetness01 = 0.0;
    double SnowDepthCm = 0.0;
    double Dust01 = 0.0;
};

struct Events
{
    bool Changed = false;
    bool TransitionStarted = false;
    bool TransitionCompleted = false;
    bool WeatherChanged = false;
    std::uint32_t LightningCount = 0;
};

bool ValidateEnvironment(const Environment& Env, std::string* Error = nullptr);
bool ValidateDataset(const Dataset& Data, std::string* Error = nullptr);
bool ValidateState(const State& Weather, const Dataset& Data, std::string* Error = nullptr);

const Preset* FindPreset(const Dataset& Data, std::uint32_t Id);

// Initializes a deterministic schedule. StableSeed should be stable for the biome/world.
bool Initialize(State& Weather, const Dataset& Data, std::uint32_t InitialPresetId,
                std::uint64_t StableSeed, Events* OutEvents = nullptr);

// Advances the authoritative weather scheduler. The Context is assumed constant for DeltaHours;
// hosts should split at season or externally-calculated base-temperature boundaries when exact
// surface accumulation across those changes matters. On invalid input/failure, state is unchanged.
bool Advance(State& Weather, const Dataset& Data, const Context& WeatherContext,
             double DeltaHours, std::uint64_t StableSeed, Events& OutEvents);

// Forces a weather target for quests, scripted events, admin tools, or tests. A duration of
// zero applies immediately. On failure, state is unchanged.
bool ForceWeather(State& Weather, const Dataset& Data, std::uint32_t TargetPresetId,
                  double TransitionHours, std::uint64_t StableSeed, Events& OutEvents);

// Returns the visual environment. Cosmetic properties blend while a transition is active.
bool GetVisualEnvironment(const State& Weather, const Dataset& Data, Environment& OutEnvironment);

// Returns the authoritative gameplay weather. Gameplay switches only when the transition completes.
bool GetGameplayEnvironment(const State& Weather, const Dataset& Data, Environment& OutEnvironment);

double GetTransitionAlpha(const State& Weather);
double GetOutsideTemperatureC(const State& Weather, const Dataset& Data, double BaseTemperatureC);

// Deterministic global temperature for a day/hour, intended to be fed by the canonical game clock.
double ComputeGlobalTemperatureC(const Climate& ClimateProfile, std::uint8_t Season,
                                 std::int64_t DayIndex, double HourOfDay,
                                 std::uint64_t StableClimateSeed);

// Moves a local temperature toward a control area's target by at most OffsetMagnitudeC, then clamps.
double ApplyLocalTemperatureControl(double CurrentC, double TargetC, double OffsetMagnitudeC,
                                    double MinimumC, double MaximumC);

// Pure visual helpers.
double LerpAngleDegrees(double A, double B, double Alpha);
Environment BlendEnvironment(const Environment& A, const Environment& B, double Alpha);

} // namespace RB::Weather
