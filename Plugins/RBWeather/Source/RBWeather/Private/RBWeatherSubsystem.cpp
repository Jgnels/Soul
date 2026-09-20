#include "RBWeatherSubsystem.h"

#include "RBWeather.h"
#include "RBWeatherBiomeVolume.h"
#include "RBWeatherControlVolume.h"
#include "RBWeatherDirector.h"
#include "RBWeatherSimulation.h"
#include "RBWeatherTemperatureSourceComponent.h"

namespace
{
template <typename T>
void AddUniqueWeak(TArray<TWeakObjectPtr<T>>& Items, T* Item)
{
    if (!IsValid(Item)) return;
    Items.RemoveAll([](const TWeakObjectPtr<T>& Existing) { return !Existing.IsValid(); });
    Items.AddUnique(Item);
}

template <typename T>
void RemoveWeak(TArray<TWeakObjectPtr<T>>& Items, T* Item)
{
    Items.RemoveAll([Item](const TWeakObjectPtr<T>& Existing) { return !Existing.IsValid() || Existing.Get() == Item; });
}
}

void URBWeatherSubsystem::RegisterDirector(ARBWeatherDirector* InDirector)
{
    if (!IsValid(InDirector)) return;
    if (!Director.IsValid() || Director.Get() == InDirector)
    {
        Director = InDirector;
    }
}

void URBWeatherSubsystem::UnregisterDirector(ARBWeatherDirector* InDirector)
{
    if (Director.Get() == InDirector) Director.Reset();
}

void URBWeatherSubsystem::RegisterBiome(ARBWeatherBiomeVolume* Biome)
{
    if (!IsValid(Biome) || Biome->BiomeId.IsNone()) return;
    for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : Biomes)
    {
        const ARBWeatherBiomeVolume* Other = Weak.Get();
        if (IsValid(Other) && Other != Biome && Other->BiomeId == Biome->BiomeId)
        {
            UE_LOG(LogRBWeather, Error, TEXT("Duplicate active BiomeId '%s' rejected."), *Biome->BiomeId.ToString());
            return;
        }
    }

    FRBWeatherCanonicalBiomeRecord* Existing = CanonicalBiomes.Find(Biome->BiomeId);
    if (!Existing)
    {
        FRBWeatherCanonicalBiomeRecord Record;
        Record.BiomeId = Biome->BiomeId;
        Record.ClimateProfileOverride = Biome->ClimateProfileOverride;
        Record.InitialWeather = Biome->InitialWeather;
        Record.SeedOffset = Biome->SeedOffset;
        Existing = &CanonicalBiomes.Add(Biome->BiomeId, Record);
    }
    else if (Existing->ClimateProfileOverride != Biome->ClimateProfileOverride ||
             Existing->InitialWeather != Biome->InitialWeather || Existing->SeedOffset != Biome->SeedOffset)
    {
        UE_LOG(LogRBWeather, Error, TEXT("BiomeId '%s' re-registered with a different definition; actor rejected."),
            *Biome->BiomeId.ToString());
        return;
    }

    AddUniqueWeak(Biomes, Biome);
    if (Existing->Weather.bInitialized) Biome->ApplyCanonicalState(Existing->Weather, true);
}

void URBWeatherSubsystem::UnregisterBiome(ARBWeatherBiomeVolume* Biome)
{
    // CanonicalBiomes intentionally survives actor streaming/unregistration.
    RemoveWeak(Biomes, Biome);
}
void URBWeatherSubsystem::RegisterControlArea(ARBWeatherControlVolume* Area) { AddUniqueWeak(ControlAreas, Area); }
void URBWeatherSubsystem::UnregisterControlArea(ARBWeatherControlVolume* Area) { RemoveWeak(ControlAreas, Area); }
void URBWeatherSubsystem::RegisterTemperatureSource(URBWeatherTemperatureSourceComponent* Source) { AddUniqueWeak(TemperatureSources, Source); }
void URBWeatherSubsystem::UnregisterTemperatureSource(URBWeatherTemperatureSourceComponent* Source) { RemoveWeak(TemperatureSources, Source); }

ARBWeatherDirector* URBWeatherSubsystem::GetDirector() const
{
    return Director.Get();
}

ARBWeatherBiomeVolume* URBWeatherSubsystem::FindBiomeById(FName BiomeId) const
{
    if (BiomeId.IsNone()) return nullptr;
    for (const auto& Weak : Biomes)
    {
        ARBWeatherBiomeVolume* B = Weak.Get();
        if (IsValid(B) && B->BiomeId == BiomeId) return B;
    }
    return nullptr;
}

FRBWeatherCanonicalBiomeRecord* URBWeatherSubsystem::FindCanonicalBiome(FName BiomeId)
{
    return BiomeId.IsNone() ? nullptr : CanonicalBiomes.Find(BiomeId);
}

const FRBWeatherCanonicalBiomeRecord* URBWeatherSubsystem::FindCanonicalBiome(FName BiomeId) const
{
    return BiomeId.IsNone() ? nullptr : CanonicalBiomes.Find(BiomeId);
}

void URBWeatherSubsystem::SyncCanonicalBiomeFromVolume(ARBWeatherBiomeVolume* Biome, const FRBWeatherRuntimeState& State)
{
    if (!IsValid(Biome) || Biome->BiomeId.IsNone()) return;
    FRBWeatherCanonicalBiomeRecord* Record = CanonicalBiomes.Find(Biome->BiomeId);
    if (!Record)
    {
        FRBWeatherCanonicalBiomeRecord NewRecord;
        NewRecord.BiomeId = Biome->BiomeId;
        NewRecord.ClimateProfileOverride = Biome->ClimateProfileOverride;
        NewRecord.InitialWeather = Biome->InitialWeather;
        NewRecord.SeedOffset = Biome->SeedOffset;
        Record = &CanonicalBiomes.Add(Biome->BiomeId, NewRecord);
    }
    Record->Weather = State;
}

bool URBWeatherSubsystem::GetWeatherAtLocation(FVector WorldLocation, FRBWeatherSnapshotAtLocation& OutSnapshot) const
{
    OutSnapshot = {};
    ARBWeatherDirector* D = Director.Get();
    if (!IsValid(D) || !D->GetGlobalSnapshot(OutSnapshot)) return false;

    ARBWeatherBiomeVolume* BestBiome = nullptr;
    double BestWeight = 0.0;
    int32 BestPriority = MIN_int32;
    for (const auto& Weak : Biomes)
    {
        ARBWeatherBiomeVolume* B = Weak.Get();
        if (!IsValid(B)) continue;
        const double Weight = B->GetLocationWeight(WorldLocation);
        if (Weight <= 0.0) continue;
        if (!BestBiome || B->Priority > BestPriority ||
            (B->Priority == BestPriority && Weight > BestWeight + KINDA_SMALL_NUMBER) ||
            (B->Priority == BestPriority && FMath::Abs(Weight - BestWeight) <= KINDA_SMALL_NUMBER && B->BiomeId.LexicalLess(BestBiome->BiomeId)))
        {
            BestBiome = B;
            BestWeight = Weight;
            BestPriority = B->Priority;
        }
    }

    if (BestBiome)
    {
        FRBWeatherSnapshotAtLocation Local{};
        FString Error;
        if (BestBiome->BuildSnapshot(D, Local, Error))
        {
            const double A = FMath::Clamp(BestWeight, 0.0, 1.0);
            const FRBWeatherSnapshotAtLocation Global = OutSnapshot;
            OutSnapshot.Environment = FRBWeatherSimulation::BlendEnvironments(Global.Environment, Local.Environment, A);
            OutSnapshot.VisualWetness = FMath::Lerp(Global.VisualWetness, Local.VisualWetness, A);
            OutSnapshot.VisualSnowDepthCm = FMath::Lerp(Global.VisualSnowDepthCm, Local.VisualSnowDepthCm, A);
            OutSnapshot.VisualDust = FMath::Lerp(Global.VisualDust, Local.VisualDust, A);
            OutSnapshot.VisualOutsideTemperatureC = FMath::Lerp(Global.VisualOutsideTemperatureC, Local.VisualOutsideTemperatureC, A);
            OutSnapshot.BiomeBlendWeight = A;
            if (A >= 0.5)
            {
                // Gameplay authority changes discretely; presentation remains spatially blended.
                OutSnapshot.BiomeId = Local.BiomeId;
                OutSnapshot.CurrentWeather = Local.CurrentWeather;
                OutSnapshot.TargetWeather = Local.TargetWeather;
                OutSnapshot.TransitionAlpha = Local.TransitionAlpha;
                OutSnapshot.LightningSerial = Local.LightningSerial;
                OutSnapshot.GameplayEnvironment = Local.GameplayEnvironment;
                OutSnapshot.Wetness = Local.Wetness;
                OutSnapshot.SnowDepthCm = Local.SnowDepthCm;
                OutSnapshot.Dust = Local.Dust;
                OutSnapshot.BaseTemperatureC = Local.BaseTemperatureC;
                OutSnapshot.OutsideTemperatureC = Local.OutsideTemperatureC;
            }
            OutSnapshot.LocalTemperatureC = OutSnapshot.OutsideTemperatureC;
        }
    }

    ARBWeatherControlVolume* BestControl = nullptr;
    double BestControlWeight = 0.0;
    int32 BestControlPriority = MIN_int32;
    for (const auto& Weak : ControlAreas)
    {
        ARBWeatherControlVolume* Area = Weak.Get();
        if (!IsValid(Area)) continue;
        const double Weight = Area->GetLocationWeight(WorldLocation);
        if (Weight <= 0.0) continue;
        if (!BestControl || Area->Priority > BestControlPriority ||
            (Area->Priority == BestControlPriority && Weight > BestControlWeight + KINDA_SMALL_NUMBER) ||
            (Area->Priority == BestControlPriority && FMath::Abs(Weight - BestControlWeight) <= KINDA_SMALL_NUMBER &&
             Area->ControlAreaId.LexicalLess(BestControl->ControlAreaId)))
        {
            BestControl = Area;
            BestControlWeight = Weight;
            BestControlPriority = Area->Priority;
        }
    }

    OutSnapshot.LocalTemperatureC = OutSnapshot.OutsideTemperatureC;
    if (BestControl)
    {
        const double W = FMath::Clamp(BestControlWeight, 0.0, 1.0);
        OutSnapshot.ControlAreaId = BestControl->ControlAreaId;
        OutSnapshot.ControlAreaBlendWeight = W;
        OutSnapshot.PrecipitationVisualMultiplier = FMath::Lerp(1.0, BestControl->PrecipitationVisualMultiplier, W);
        OutSnapshot.LensEffectMultiplier = FMath::Lerp(1.0, BestControl->LensEffectMultiplier, W);
        OutSnapshot.WeatherAudioMultiplier = FMath::Lerp(1.0, BestControl->WeatherAudioMultiplier, W);
        OutSnapshot.LocalWindMultiplier = FMath::Lerp(1.0, BestControl->WindMultiplier, W);
        if (BestControl->bControlTemperature)
        {
            bool bValid = false;
            const double Controlled = FRBWeatherSimulation::ApplyLocalTemperatureControl(
                OutSnapshot.LocalTemperatureC, BestControl->TargetTemperatureC,
                BestControl->TemperatureAdjustmentLimitC, BestControl->MinimumTemperatureC,
                BestControl->MaximumTemperatureC, bValid);
            if (bValid)
            {
                OutSnapshot.LocalTemperatureC = FMath::Lerp(OutSnapshot.OutsideTemperatureC, Controlled, W);
            }
        }
    }

    for (const auto& Weak : TemperatureSources)
    {
        const URBWeatherTemperatureSourceComponent* Source = Weak.Get();
        if (IsValid(Source))
        {
            OutSnapshot.LocalTemperatureC += Source->GetTemperatureOffsetAt(WorldLocation);
        }
    }
    return FMath::IsFinite(OutSnapshot.LocalTemperatureC);
}
