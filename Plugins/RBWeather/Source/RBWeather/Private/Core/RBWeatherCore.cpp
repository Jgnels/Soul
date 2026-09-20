#include "RBWeatherCore.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace RB::Weather
{
namespace
{
constexpr double Epsilon = 1e-9;
constexpr double Pi = 3.141592653589793238462643383279502884;

bool Fail(std::string* Error, const char* Message)
{
    if (Error) { *Error = Message; }
    return false;
}

bool Finite(double V) { return std::isfinite(V); }
bool FiniteRange(double V, double Min, double Max)
{
    return Finite(V) && V >= Min && V <= Max;
}

double Clamp01(double V) { return std::clamp(V, 0.0, 1.0); }

std::uint64_t Mix(std::uint64_t X)
{
    X += UINT64_C(0x9e3779b97f4a7c15);
    X = (X ^ (X >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    X = (X ^ (X >> 27)) * UINT64_C(0x94d049bb133111eb);
    return X ^ (X >> 31);
}

double Unit(std::uint64_t X)
{
    // 53 deterministic mantissa bits mapped to [0,1).
    return static_cast<double>(Mix(X) >> 11) * (1.0 / 9007199254740992.0);
}

double DrawRange(double Min, double Max, std::uint64_t Key)
{
    if (Max <= Min) { return Min; }
    return Min + (Max - Min) * Unit(Key);
}

std::uint64_t ScheduleKey(std::uint64_t StableSeed, std::uint64_t Serial, std::uint64_t Salt)
{
    return Mix(StableSeed ^ Mix(Serial + UINT64_C(0x7f4a7c159e3779b9)) ^ Salt);
}

const Preset* FindPresetInternal(const Dataset& Data, std::uint32_t Id)
{
    for (const auto& P : Data.Presets)
    {
        if (P.Id == Id) { return &P; }
    }
    return nullptr;
}

bool SeasonAllowed(std::uint8_t Mask, std::uint8_t Season)
{
    if (Season > 3) { return false; }
    return (Mask & static_cast<std::uint8_t>(1u << Season)) != 0;
}

bool PickNextPreset(const State& S, const Dataset& Data, const Context& C,
                    std::uint64_t StableSeed, std::uint32_t& OutId)
{
    double Total = 0.0;
    for (const auto& Rule : Data.Rules)
    {
        if (Rule.FromId != S.CurrentId || Rule.Weight <= 0.0 ||
            !SeasonAllowed(Rule.SeasonMask, C.Season) ||
            C.BaseTemperatureC < Rule.MinimumBaseTemperatureC ||
            C.BaseTemperatureC > Rule.MaximumBaseTemperatureC)
        {
            continue;
        }
        Total += Rule.Weight;
    }

    if (!(Total > 0.0) || !Finite(Total))
    {
        OutId = S.CurrentId;
        return true;
    }

    double Needle = Unit(ScheduleKey(StableSeed, S.TransitionSerial, UINT64_C(0x13579bdf2468ace0))) * Total;
    const TransitionRule* Last = nullptr;
    for (const auto& Rule : Data.Rules)
    {
        if (Rule.FromId != S.CurrentId || Rule.Weight <= 0.0 ||
            !SeasonAllowed(Rule.SeasonMask, C.Season) ||
            C.BaseTemperatureC < Rule.MinimumBaseTemperatureC ||
            C.BaseTemperatureC > Rule.MaximumBaseTemperatureC)
        {
            continue;
        }
        Last = &Rule;
        if (Needle < Rule.Weight)
        {
            OutId = Rule.ToId;
            return true;
        }
        Needle -= Rule.Weight;
    }
    OutId = Last ? Last->ToId : S.CurrentId;
    return true;
}

double DrawActiveDuration(const Preset& P, std::uint64_t Seed, std::uint64_t Serial)
{
    return DrawRange(P.ActiveMinHours, P.ActiveMaxHours,
                     ScheduleKey(Seed, Serial, UINT64_C(0x6a09e667f3bcc909)));
}

double DrawTransitionDuration(const Preset& P, std::uint64_t Seed, std::uint64_t Serial)
{
    return DrawRange(P.TransitionMinHours, P.TransitionMaxHours,
                     ScheduleKey(Seed, Serial, UINT64_C(0xbb67ae8584caa73b)));
}

double DrawLightningInterval(double RatePerHour, std::uint64_t Seed,
                             std::uint64_t TransitionSerial, std::uint64_t LightningSerial)
{
    if (!(RatePerHour > 0.0)) { return -1.0; }
    // Bounded jitter avoids pathological zero intervals while remaining deterministic.
    const auto K = Mix(ScheduleKey(Seed, TransitionSerial, UINT64_C(0x3c6ef372fe94f82b)) ^
                       Mix(LightningSerial + UINT64_C(0xa54ff53a5f1d36f1)));
    const double Jitter = 0.55 + 0.90 * Unit(K); // 0.55..1.45 of mean interval.
    return std::max(1e-6, Jitter / RatePerHour);
}

void ResetLightning(State& S, const Preset& Current, std::uint64_t StableSeed)
{
    if (Current.Env.LightningRatePerHour > 0.0)
    {
        S.NextLightningHours = DrawLightningInterval(Current.Env.LightningRatePerHour, StableSeed,
                                                      S.TransitionSerial, S.LightningSerial);
    }
    else
    {
        S.NextLightningHours = -1.0;
    }
}

void ApplySurface(State& S, const Environment& Env, const Context& C, double Step)
{
    const bool IsWetPrecip = Env.PrecipitationType == Precipitation::Rain ||
                             Env.PrecipitationType == Precipitation::Sleet ||
                             Env.PrecipitationType == Precipitation::Hail;
    const double WetGain = IsWetPrecip ? Env.WetnessGainPerHour * Env.PrecipitationIntensity : 0.0;
    const double Dry = IsWetPrecip ? 0.0 : Env.DryingPerHour;
    S.Wetness01 = Clamp01(S.Wetness01 + (WetGain - Dry) * Step);

    const double ActualTemperature = C.BaseTemperatureC + Env.TemperatureOffsetC;
    const bool IsSnow = Env.PrecipitationType == Precipitation::Snow ||
                        Env.PrecipitationType == Precipitation::Sleet;
    const double SnowGain = (IsSnow && ActualTemperature <= Env.SnowMeltStartC + Epsilon)
        ? Env.SnowGainCmPerHour * Env.PrecipitationIntensity : 0.0;
    const double Melt = ActualTemperature > Env.SnowMeltStartC
        ? (ActualTemperature - Env.SnowMeltStartC) * Env.SnowMeltCmPerHourPerC : 0.0;
    S.SnowDepthCm = std::clamp(S.SnowDepthCm + (SnowGain - Melt) * Step, 0.0, MaxSurfaceSnowCm);

    const bool IsDust = Env.PrecipitationType == Precipitation::Dust;
    const double DustGain = IsDust ? Env.DustGainPerHour * Env.PrecipitationIntensity : 0.0;
    const double DustWash = IsWetPrecip ? Env.DustWashPerHour * Env.PrecipitationIntensity : 0.0;
    S.Dust01 = Clamp01(S.Dust01 + (DustGain - DustWash) * Step);
}

bool CompleteTransition(State& S, const Dataset& Data, std::uint64_t StableSeed, Events& E)
{
    const Preset* Target = FindPresetInternal(Data, S.TargetId);
    if (!Target) { return false; }
    const bool ChangedId = S.CurrentId != S.TargetId;
    S.CurrentId = S.TargetId;
    S.TransitionElapsedHours = 0.0;
    S.TransitionDurationHours = 0.0;
    S.ActiveRemainingHours = DrawActiveDuration(*Target, StableSeed, S.TransitionSerial);
    ResetLightning(S, *Target, StableSeed);
    E.TransitionCompleted = true;
    E.WeatherChanged = E.WeatherChanged || ChangedId;
    E.Changed = true;
    return true;
}

bool BeginNext(State& S, const Dataset& Data, const Context& C,
               std::uint64_t StableSeed, Events& E)
{
    std::uint32_t NextId = S.CurrentId;
    if (!PickNextPreset(S, Data, C, StableSeed, NextId)) { return false; }
    const Preset* Next = FindPresetInternal(Data, NextId);
    const Preset* Current = FindPresetInternal(Data, S.CurrentId);
    if (!Next || !Current) { return false; }

    ++S.TransitionSerial;
    if (NextId == S.CurrentId)
    {
        S.TargetId = S.CurrentId;
        S.ActiveRemainingHours = DrawActiveDuration(*Current, StableSeed, S.TransitionSerial);
        E.Changed = true;
        return true;
    }

    S.TargetId = NextId;
    S.TransitionElapsedHours = 0.0;
    S.TransitionDurationHours = DrawTransitionDuration(*Next, StableSeed, S.TransitionSerial);
    S.ActiveRemainingHours = 0.0;
    E.TransitionStarted = true;
    E.Changed = true;
    if (S.TransitionDurationHours <= Epsilon)
    {
        return CompleteTransition(S, Data, StableSeed, E);
    }
    return true;
}

} // namespace

bool ValidateEnvironment(const Environment& E, std::string* Error)
{
    if (static_cast<std::uint8_t>(E.PrecipitationType) > static_cast<std::uint8_t>(Precipitation::Dust))
        return Fail(Error, "Precipitation type invalid");
    if (!FiniteRange(E.CloudCoverage, 0.0, 1.0)) return Fail(Error, "CloudCoverage must be in [0,1]");
    if (!FiniteRange(E.CloudDensity, 0.0, 1.0)) return Fail(Error, "CloudDensity must be in [0,1]");
    if (!FiniteRange(E.SunIntensityScale, 0.0, 1000.0)) return Fail(Error, "SunIntensityScale invalid");
    for (double V : {E.SunLightColor.R, E.SunLightColor.G, E.SunLightColor.B, E.SunLightColor.A})
        if (!FiniteRange(V, 0.0, 100.0)) return Fail(Error, "SunLightColor invalid");
    if (!FiniteRange(E.SkyLightIntensityScale, 0.0, 1000.0)) return Fail(Error, "SkyLightIntensityScale invalid");
    if (!FiniteRange(E.FogDensity, 0.0, 10.0)) return Fail(Error, "FogDensity invalid");
    for (double V : {E.FogInscatteringColor.R, E.FogInscatteringColor.G, E.FogInscatteringColor.B, E.FogInscatteringColor.A})
        if (!FiniteRange(V, 0.0, 100.0)) return Fail(Error, "FogInscatteringColor invalid");
    if (!FiniteRange(E.FogHeightFalloff, 0.0, 100.0)) return Fail(Error, "FogHeightFalloff invalid");
    if (!FiniteRange(E.VisibilityKm, 0.001, 100000.0)) return Fail(Error, "VisibilityKm invalid");
    if (!FiniteRange(E.WindSpeedMps, 0.0, 1000.0)) return Fail(Error, "WindSpeedMps invalid");
    if (!FiniteRange(E.WindGustMps, 0.0, 2000.0)) return Fail(Error, "WindGustMps invalid");
    if (!Finite(E.WindDirectionDegrees)) return Fail(Error, "WindDirectionDegrees must be finite");
    if (!FiniteRange(E.PrecipitationIntensity, 0.0, 1.0)) return Fail(Error, "PrecipitationIntensity invalid");
    if (!FiniteRange(E.LightningRatePerHour, 0.0, 1000000.0)) return Fail(Error, "LightningRatePerHour invalid");
    if (!FiniteRange(E.TemperatureOffsetC, -1000.0, 1000.0)) return Fail(Error, "TemperatureOffsetC invalid");
    if (!FiniteRange(E.Humidity01, 0.0, 1.0)) return Fail(Error, "Humidity01 invalid");
    if (!FiniteRange(E.WetnessGainPerHour, 0.0, 1000000.0)) return Fail(Error, "WetnessGainPerHour invalid");
    if (!FiniteRange(E.DryingPerHour, 0.0, 1000000.0)) return Fail(Error, "DryingPerHour invalid");
    if (!FiniteRange(E.SnowGainCmPerHour, 0.0, 1000000.0)) return Fail(Error, "SnowGainCmPerHour invalid");
    if (!FiniteRange(E.SnowMeltCmPerHourPerC, 0.0, 1000000.0)) return Fail(Error, "SnowMelt rate invalid");
    if (!FiniteRange(E.SnowMeltStartC, -1000.0, 1000.0)) return Fail(Error, "SnowMeltStartC invalid");
    if (!FiniteRange(E.DustGainPerHour, 0.0, 1000000.0)) return Fail(Error, "DustGainPerHour invalid");
    if (!FiniteRange(E.DustWashPerHour, 0.0, 1000000.0)) return Fail(Error, "DustWashPerHour invalid");
    if (!FiniteRange(E.LensWetness, 0.0, 1.0) || !FiniteRange(E.LensFrost, 0.0, 1.0) ||
        !FiniteRange(E.LensDust, 0.0, 1.0) || !FiniteRange(E.AudioIntensity, 0.0, 1.0))
        return Fail(Error, "Lens/audio values must be in [0,1]");
    return true;
}

bool ValidateDataset(const Dataset& Data, std::string* Error)
{
    if (Data.Presets.empty()) { return Fail(Error, "Dataset requires at least one preset"); }
    for (std::size_t I = 0; I < Data.Presets.size(); ++I)
    {
        const auto& P = Data.Presets[I];
        if (P.Id == 0) { return Fail(Error, "Preset Id 0 is reserved/invalid"); }
        if (!ValidateEnvironment(P.Env, Error)) { return false; }
        if (!FiniteRange(P.ActiveMinHours, 1e-6, MaxDeltaHours) ||
            !FiniteRange(P.ActiveMaxHours, P.ActiveMinHours, MaxDeltaHours))
            return Fail(Error, "Preset active duration invalid");
        if (!FiniteRange(P.TransitionMinHours, 0.0, MaxDeltaHours) ||
            !FiniteRange(P.TransitionMaxHours, P.TransitionMinHours, MaxDeltaHours))
            return Fail(Error, "Preset transition duration invalid");
        for (std::size_t J = I + 1; J < Data.Presets.size(); ++J)
        {
            if (Data.Presets[J].Id == P.Id) { return Fail(Error, "Duplicate preset Id"); }
        }
    }
    for (const auto& R : Data.Rules)
    {
        if (!FindPresetInternal(Data, R.FromId) || !FindPresetInternal(Data, R.ToId))
            return Fail(Error, "Transition rule references missing preset");
        if (!FiniteRange(R.Weight, 0.0, 1e12)) return Fail(Error, "Transition weight invalid");
        if ((R.SeasonMask & 0x0F) == 0) return Fail(Error, "Transition season mask selects no season");
        if (!Finite(R.MinimumBaseTemperatureC) || !Finite(R.MaximumBaseTemperatureC) ||
            R.MinimumBaseTemperatureC > R.MaximumBaseTemperatureC)
            return Fail(Error, "Transition temperature bounds invalid");
    }
    return true;
}

bool ValidateState(const State& S, const Dataset& Data, std::string* Error)
{
    if (!S.Initialized) { return Fail(Error, "Weather state is not initialized"); }
    if (!FindPresetInternal(Data, S.CurrentId) || !FindPresetInternal(Data, S.TargetId))
        return Fail(Error, "State references missing preset");
    if (!FiniteRange(S.TransitionElapsedHours, 0.0, MaxDeltaHours) ||
        !FiniteRange(S.TransitionDurationHours, 0.0, MaxDeltaHours) ||
        S.TransitionElapsedHours > S.TransitionDurationHours + Epsilon)
        return Fail(Error, "Transition timing invalid");
    if (!FiniteRange(S.ActiveRemainingHours, 0.0, MaxDeltaHours))
        return Fail(Error, "Active remaining time invalid");
    if (S.TransitionDurationHours <= Epsilon &&
        (S.TransitionElapsedHours > Epsilon || S.CurrentId != S.TargetId))
        return Fail(Error, "Inactive transition state is inconsistent");
    if (S.NextLightningHours < 0.0)
    {
        if (S.NextLightningHours != -1.0) return Fail(Error, "NextLightningHours invalid sentinel");
    }
    else if (!FiniteRange(S.NextLightningHours, 0.0, MaxDeltaHours))
        return Fail(Error, "NextLightningHours invalid");
    if (!FiniteRange(S.Wetness01, 0.0, 1.0) || !FiniteRange(S.Dust01, 0.0, 1.0) ||
        !FiniteRange(S.SnowDepthCm, 0.0, MaxSurfaceSnowCm))
        return Fail(Error, "Surface state invalid");
    return true;
}

const Preset* FindPreset(const Dataset& Data, std::uint32_t Id)
{
    return FindPresetInternal(Data, Id);
}

bool Initialize(State& S, const Dataset& Data, std::uint32_t InitialPresetId,
                std::uint64_t StableSeed, Events* OutEvents)
{
    std::string Error;
    if (!ValidateDataset(Data, &Error)) { return false; }
    const Preset* Initial = FindPresetInternal(Data, InitialPresetId);
    if (!Initial) { return false; }
    State Candidate{};
    Candidate.Initialized = true;
    Candidate.CurrentId = InitialPresetId;
    Candidate.TargetId = InitialPresetId;
    Candidate.ActiveRemainingHours = DrawActiveDuration(*Initial, StableSeed, 0);
    ResetLightning(Candidate, *Initial, StableSeed);
    if (!ValidateState(Candidate, Data, nullptr)) { return false; }
    S = Candidate;
    if (OutEvents)
    {
        *OutEvents = {};
        OutEvents->Changed = true;
        OutEvents->WeatherChanged = true;
    }
    return true;
}

bool Advance(State& Weather, const Dataset& Data, const Context& C,
             double DeltaHours, std::uint64_t StableSeed, Events& OutEvents)
{
    OutEvents = {};
    if (!FiniteRange(DeltaHours, 0.0, MaxDeltaHours) || C.Season > 3 ||
        !FiniteRange(C.BaseTemperatureC, -1000.0, 1000.0) ||
        !ValidateDataset(Data, nullptr) || !ValidateState(Weather, Data, nullptr))
    {
        return false;
    }
    if (DeltaHours <= 0.0) { return true; }

    State S = Weather;
    Events E{};
    double Remaining = DeltaHours;

    for (std::size_t Boundary = 0; Boundary < MaxTransitionsPerAdvance; ++Boundary)
    {
        const Preset* Current = FindPresetInternal(Data, S.CurrentId);
        if (!Current) { return false; }

        // Process events exactly on the boundary before declaring a zero-remaining advance complete.
        if (Current->Env.LightningRatePerHour > 0.0 && S.NextLightningHours >= 0.0 &&
            S.NextLightningHours <= Epsilon)
        {
            ++S.LightningSerial;
            ++E.LightningCount;
            E.Changed = true;
            S.NextLightningHours = DrawLightningInterval(Current->Env.LightningRatePerHour,
                StableSeed, S.TransitionSerial, S.LightningSerial);
            continue;
        }

        const bool Transitioning = S.TransitionDurationHours > Epsilon;
        if (Transitioning && S.TransitionElapsedHours + Epsilon >= S.TransitionDurationHours)
        {
            if (!CompleteTransition(S, Data, StableSeed, E)) { return false; }
            continue;
        }
        if (!Transitioning && S.ActiveRemainingHours <= Epsilon)
        {
            if (!BeginNext(S, Data, C, StableSeed, E)) { return false; }
            continue;
        }
        if (Remaining <= Epsilon)
        {
            Weather = S;
            OutEvents = E;
            return true;
        }

        Current = FindPresetInternal(Data, S.CurrentId);
        if (!Current) { return false; }

        double Step = Remaining;
        if (S.TransitionDurationHours > Epsilon)
        {
            Step = std::min(Step, S.TransitionDurationHours - S.TransitionElapsedHours);
        }
        else
        {
            Step = std::min(Step, S.ActiveRemainingHours);
        }
        if (Current->Env.LightningRatePerHour > 0.0 && S.NextLightningHours >= 0.0)
        {
            Step = std::min(Step, S.NextLightningHours);
        }
        if (!Finite(Step) || Step < 0.0) { return false; }
        if (Step <= Epsilon)
        {
            if (Current->Env.LightningRatePerHour > 0.0 && S.NextLightningHours <= Epsilon)
            {
                ++S.LightningSerial;
                ++E.LightningCount;
                E.Changed = true;
                S.NextLightningHours = DrawLightningInterval(Current->Env.LightningRatePerHour,
                    StableSeed, S.TransitionSerial, S.LightningSerial);
                continue;
            }
            return false;
        }

        // Surface memory follows the authoritative current weather. Visual transition blending is
        // cosmetic; this choice keeps gameplay integration exact and delta-size invariant.
        ApplySurface(S, Current->Env, C, Step);
        if (S.TransitionDurationHours > Epsilon)
        {
            S.TransitionElapsedHours += Step;
        }
        else
        {
            S.ActiveRemainingHours = std::max(0.0, S.ActiveRemainingHours - Step);
        }
        if (S.NextLightningHours >= 0.0)
        {
            S.NextLightningHours = std::max(0.0, S.NextLightningHours - Step);
        }
        Remaining -= Step;
        E.Changed = true;
    }
    return false; // Pathological schedule: caller state is intentionally unchanged.
}

bool ForceWeather(State& Weather, const Dataset& Data, std::uint32_t TargetPresetId,
                  double TransitionHours, std::uint64_t StableSeed, Events& OutEvents)
{
    OutEvents = {};
    if (!FiniteRange(TransitionHours, 0.0, MaxDeltaHours) ||
        !ValidateDataset(Data, nullptr) || !ValidateState(Weather, Data, nullptr))
        return false;
    const Preset* Target = FindPresetInternal(Data, TargetPresetId);
    if (!Target) return false;
    State S = Weather;
    Events E{};
    if (TargetPresetId == S.CurrentId && TransitionHours <= Epsilon)
    {
        S.TargetId = S.CurrentId;
        S.TransitionElapsedHours = 0.0;
        S.TransitionDurationHours = 0.0;
        Weather = S;
        return true;
    }
    ++S.TransitionSerial;
    S.TargetId = TargetPresetId;
    S.TransitionElapsedHours = 0.0;
    S.TransitionDurationHours = TransitionHours;
    S.ActiveRemainingHours = 0.0;
    E.TransitionStarted = TargetPresetId != S.CurrentId;
    E.Changed = true;
    if (TransitionHours <= Epsilon)
    {
        if (!CompleteTransition(S, Data, StableSeed, E)) return false;
    }
    Weather = S;
    OutEvents = E;
    return true;
}

double GetTransitionAlpha(const State& S)
{
    if (S.TransitionDurationHours <= Epsilon) { return 1.0; }
    return Clamp01(S.TransitionElapsedHours / S.TransitionDurationHours);
}

double LerpAngleDegrees(double A, double B, double Alpha)
{
    Alpha = Clamp01(Alpha);
    auto Wrap = [](double V)
    {
        V = std::fmod(V, 360.0);
        if (V < 0.0) V += 360.0;
        return V;
    };
    A = Wrap(A);
    B = Wrap(B);
    double Delta = B - A;
    if (Delta > 180.0) Delta -= 360.0;
    if (Delta < -180.0) Delta += 360.0;
    return Wrap(A + Delta * Alpha);
}

Environment BlendEnvironment(const Environment& A, const Environment& B, double Alpha)
{
    Alpha = Clamp01(Alpha);
    const auto L = [Alpha](double X, double Y) { return X + (Y - X) * Alpha; };
    Environment R{};
    R.CloudCoverage = L(A.CloudCoverage, B.CloudCoverage);
    R.CloudDensity = L(A.CloudDensity, B.CloudDensity);
    R.SunIntensityScale = L(A.SunIntensityScale, B.SunIntensityScale);
    R.SunLightColor = {L(A.SunLightColor.R, B.SunLightColor.R), L(A.SunLightColor.G, B.SunLightColor.G),
                       L(A.SunLightColor.B, B.SunLightColor.B), L(A.SunLightColor.A, B.SunLightColor.A)};
    R.SkyLightIntensityScale = L(A.SkyLightIntensityScale, B.SkyLightIntensityScale);
    R.FogDensity = L(A.FogDensity, B.FogDensity);
    R.FogInscatteringColor = {L(A.FogInscatteringColor.R, B.FogInscatteringColor.R),
                              L(A.FogInscatteringColor.G, B.FogInscatteringColor.G),
                              L(A.FogInscatteringColor.B, B.FogInscatteringColor.B),
                              L(A.FogInscatteringColor.A, B.FogInscatteringColor.A)};
    R.FogHeightFalloff = L(A.FogHeightFalloff, B.FogHeightFalloff);
    R.VisibilityKm = L(A.VisibilityKm, B.VisibilityKm);
    R.WindSpeedMps = L(A.WindSpeedMps, B.WindSpeedMps);
    R.WindGustMps = L(A.WindGustMps, B.WindGustMps);
    R.WindDirectionDegrees = LerpAngleDegrees(A.WindDirectionDegrees, B.WindDirectionDegrees, Alpha);
    // Discrete precipitation switches at halfway for presentation asset selection; intensity blends.
    R.PrecipitationType = Alpha < 0.5 ? A.PrecipitationType : B.PrecipitationType;
    R.PrecipitationIntensity = L(A.PrecipitationIntensity, B.PrecipitationIntensity);
    R.LightningRatePerHour = L(A.LightningRatePerHour, B.LightningRatePerHour);
    R.TemperatureOffsetC = L(A.TemperatureOffsetC, B.TemperatureOffsetC);
    R.Humidity01 = L(A.Humidity01, B.Humidity01);
    R.WetnessGainPerHour = L(A.WetnessGainPerHour, B.WetnessGainPerHour);
    R.DryingPerHour = L(A.DryingPerHour, B.DryingPerHour);
    R.SnowGainCmPerHour = L(A.SnowGainCmPerHour, B.SnowGainCmPerHour);
    R.SnowMeltCmPerHourPerC = L(A.SnowMeltCmPerHourPerC, B.SnowMeltCmPerHourPerC);
    R.SnowMeltStartC = L(A.SnowMeltStartC, B.SnowMeltStartC);
    R.DustGainPerHour = L(A.DustGainPerHour, B.DustGainPerHour);
    R.DustWashPerHour = L(A.DustWashPerHour, B.DustWashPerHour);
    R.LensWetness = L(A.LensWetness, B.LensWetness);
    R.LensFrost = L(A.LensFrost, B.LensFrost);
    R.LensDust = L(A.LensDust, B.LensDust);
    R.AudioIntensity = L(A.AudioIntensity, B.AudioIntensity);
    return R;
}

bool GetVisualEnvironment(const State& S, const Dataset& Data, Environment& Out)
{
    if (!ValidateState(S, Data, nullptr)) { return false; }
    const auto* A = FindPresetInternal(Data, S.CurrentId);
    const auto* B = FindPresetInternal(Data, S.TargetId);
    if (!A || !B) { return false; }
    Out = S.TransitionDurationHours > Epsilon
        ? BlendEnvironment(A->Env, B->Env, GetTransitionAlpha(S)) : A->Env;
    return true;
}

bool GetGameplayEnvironment(const State& S, const Dataset& Data, Environment& Out)
{
    if (!ValidateState(S, Data, nullptr)) { return false; }
    const auto* A = FindPresetInternal(Data, S.CurrentId);
    if (!A) { return false; }
    Out = A->Env;
    return true;
}

double GetOutsideTemperatureC(const State& S, const Dataset& Data, double BaseTemperatureC)
{
    Environment E{};
    if (!Finite(BaseTemperatureC) || !GetGameplayEnvironment(S, Data, E))
        return std::numeric_limits<double>::quiet_NaN();
    return BaseTemperatureC + E.TemperatureOffsetC;
}

double ComputeGlobalTemperatureC(const Climate& C, std::uint8_t Season,
                                 std::int64_t DayIndex, double HourOfDay,
                                 std::uint64_t StableClimateSeed)
{
    if (Season > 3 || !Finite(HourOfDay) || !Finite(C.MinimumC[Season]) ||
        !Finite(C.MaximumC[Season]) || C.MinimumC[Season] > C.MaximumC[Season] ||
        !FiniteRange(C.DailyVariationFraction, 0.0, 1.0) ||
        !FiniteRange(C.DiurnalFraction, 0.0, 1.0) || !Finite(C.PeakHour))
        return std::numeric_limits<double>::quiet_NaN();

    HourOfDay = std::fmod(HourOfDay, 24.0);
    if (HourOfDay < 0.0) HourOfDay += 24.0;
    const double Min = C.MinimumC[Season];
    const double Max = C.MaximumC[Season];
    const double Mid = (Min + Max) * 0.5;
    const double HalfRange = (Max - Min) * 0.5;
    const auto DayBits = static_cast<std::uint64_t>(DayIndex) ^ UINT64_C(0xd1b54a32d192ed03);
    const double DailySigned = Unit(Mix(StableClimateSeed ^ Mix(DayBits))) * 2.0 - 1.0;
    const double Daily = DailySigned * HalfRange * C.DailyVariationFraction;
    const double Phase = std::cos((HourOfDay - C.PeakHour) * (2.0 * Pi / 24.0));
    const double Diurnal = Phase * HalfRange * C.DiurnalFraction;
    return std::clamp(Mid + Daily + Diurnal, Min, Max);
}

double ApplyLocalTemperatureControl(double CurrentC, double TargetC, double OffsetMagnitudeC,
                                    double MinimumC, double MaximumC)
{
    if (!Finite(CurrentC) || !Finite(TargetC) || !Finite(OffsetMagnitudeC) ||
        !Finite(MinimumC) || !Finite(MaximumC) || OffsetMagnitudeC < 0.0 || MinimumC > MaximumC)
        return std::numeric_limits<double>::quiet_NaN();
    const double Delta = TargetC - CurrentC;
    const double Applied = std::clamp(Delta, -OffsetMagnitudeC, OffsetMagnitudeC);
    return std::clamp(CurrentC + Applied, MinimumC, MaximumC);
}

} // namespace RB::Weather
