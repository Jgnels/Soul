#include "RBWeatherDirector.h"

#include "Engine/World.h"

#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "RBWeather.h"
#include "RBWeatherBiomeVolume.h"
#include "RBWeatherSimulation.h"
#include "RBWeatherSubsystem.h"

namespace
{
uint64 StableStringHash(const FString& Text)
{
    uint64 Hash = UINT64_C(1469598103934665603);
    FTCHARToUTF8 Utf8(*Text);
    const uint8* Data = reinterpret_cast<const uint8*>(Utf8.Get());
    for (int32 Index = 0; Index < Utf8.Length(); ++Index)
    {
        Hash ^= static_cast<uint64>(Data[Index]);
        Hash *= UINT64_C(1099511628211);
    }
    return Hash;
}

int64 StableBiomeSeed(int64 WorldSeed, FName BiomeId, int64 SeedOffset)
{
    const uint64 Hash = StableStringHash(BiomeId.ToString());
    const uint64 Combined = static_cast<uint64>(WorldSeed) ^ Hash ^ static_cast<uint64>(SeedOffset);
    return static_cast<int64>(Combined & UINT64_C(0x7fffffffffffffff));
}

void MergeEvents(FRBWeatherSimulationEvents& Into, const FRBWeatherSimulationEvents& From)
{
    Into.bChanged |= From.bChanged;
    Into.bTransitionStarted |= From.bTransitionStarted;
    Into.bTransitionCompleted |= From.bTransitionCompleted;
    Into.bWeatherChanged |= From.bWeatherChanged;
    Into.LightningCount += From.LightningCount;
}
}

ARBWeatherDirector::ARBWeatherDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
}

void ARBWeatherDirector::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->RegisterDirector(this);
            if (System->GetDirector() != this)
            {
                UE_LOG(LogRBWeather, Error, TEXT("Multiple RBWeatherDirector actors detected. Only the first registered director is canonical."));
            }
        }
    }
    FString Error;
    if (HasAuthority() && !ValidateConfiguredData(Error))
    {
        UE_LOG(LogRBWeather, Warning, TEXT("RB Weather data is not ready: %s"), *Error);
    }
}

void ARBWeatherDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            System->UnregisterDirector(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void ARBWeatherDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARBWeatherDirector, GlobalState);
}

void ARBWeatherDirector::OnRep_GlobalState()
{
    OnGlobalWeatherStateChanged.Broadcast(GlobalState);
    if (LastObservedLightningSerial < 0)
    {
        LastObservedLightningSerial = GlobalState.LightningSerial;
    }
    else if (GlobalState.LightningSerial > LastObservedLightningSerial)
    {
        // Do not replay an entire skipped-time storm on a late packet/client. Surface the newest event.
        OnGlobalLightning.Broadcast(GlobalState.LightningSerial);
        LastObservedLightningSerial = GlobalState.LightningSerial;
    }
}

void ARBWeatherDirector::PublishGlobal(const FRBWeatherSimulationEvents& Events)
{
    ForceNetUpdate();
    OnGlobalWeatherStateChanged.Broadcast(GlobalState);
    if (Events.LightningCount > 0)
    {
        const int64 First = GlobalState.LightningSerial - Events.LightningCount + 1;
        for (int64 Serial = First; Serial <= GlobalState.LightningSerial; ++Serial)
        {
            OnGlobalLightning.Broadcast(Serial);
        }
    }
    LastObservedLightningSerial = GlobalState.LightningSerial;
}

bool ARBWeatherDirector::GetClimateProfile(FName OverrideRow, FRBWeatherClimateRow& OutClimate) const
{
    if (!ClimateTable || ClimateTable->GetRowStruct() != FRBWeatherClimateRow::StaticStruct()) return false;
    const FName RowName = OverrideRow.IsNone() ? ClimateProfileRow : OverrideRow;
    if (RowName.IsNone()) return false;
    const FRBWeatherClimateRow* Row = ClimateTable->FindRow<FRBWeatherClimateRow>(RowName, TEXT("RBWeather climate"), false);
    if (!Row) return false;
    OutClimate = *Row;
    return true;
}

bool ARBWeatherDirector::InitializeGlobalIfNeeded(FName Season, int64 DayIndex, double HourOfDay, FString& OutError)
{
    if (GlobalState.bInitialized) return true;
    FRBWeatherClimateRow Climate;
    if (!GetClimateProfile(NAME_None, Climate))
    {
        OutError = TEXT("Global climate profile could not be resolved.");
        return false;
    }
    FRBWeatherSimulationEvents Events;
    if (!FRBWeatherSimulation::Initialize(GlobalState, WeatherPresetTable, WeatherTransitionTable, Climate,
        InitialWeather, Season, DayIndex, HourOfDay, StableSeed, Events, OutError)) return false;
    PublishGlobal(Events);
    return true;
}

bool ARBWeatherDirector::AdvanceExternalTime(double DeltaGameHours, FName Season, int64 DayIndex, double HourOfDay)
{
    if (!HasAuthority() || !FMath::IsFinite(DeltaGameHours) || DeltaGameHours < 0.0) return false;

    FString Error;
    FRBWeatherClimateRow GlobalClimate;
    if (!GetClimateProfile(NAME_None, GlobalClimate)) return false;

    FRBWeatherRuntimeState CandidateGlobal = GlobalState;
    FRBWeatherSimulationEvents GlobalEvents{};
    if (!CandidateGlobal.bInitialized)
    {
        FRBWeatherSimulationEvents InitEvents;
        if (!FRBWeatherSimulation::Initialize(CandidateGlobal, WeatherPresetTable, WeatherTransitionTable,
            GlobalClimate, InitialWeather, Season, DayIndex, HourOfDay, StableSeed, InitEvents, Error))
        {
            UE_LOG(LogRBWeather, Error, TEXT("Weather initialization failed: %s"), *Error);
            return false;
        }
        MergeEvents(GlobalEvents, InitEvents);
    }
    if (DeltaGameHours > 0.0 || GlobalState.bInitialized)
    {
        FRBWeatherSimulationEvents AdvanceEvents;
        if (!FRBWeatherSimulation::Advance(CandidateGlobal, WeatherPresetTable, WeatherTransitionTable,
            GlobalClimate, Season, DayIndex, HourOfDay, DeltaGameHours, StableSeed, AdvanceEvents, Error))
        {
            UE_LOG(LogRBWeather, Error, TEXT("Global weather advance failed: %s"), *Error);
            return false;
        }
        MergeEvents(GlobalEvents, AdvanceEvents);
    }

    URBWeatherSubsystem* System = GetWorld() ? GetWorld()->GetSubsystem<URBWeatherSubsystem>() : nullptr;
    TMap<FName, FRBWeatherRuntimeState> CandidateBiomes;
    if (System)
    {
        for (const auto& Pair : System->GetCanonicalBiomes())
        {
            const FRBWeatherCanonicalBiomeRecord& Record = Pair.Value;
            FRBWeatherClimateRow Climate;
            if (!GetClimateProfile(Record.ClimateProfileOverride, Climate))
            {
                UE_LOG(LogRBWeather, Error, TEXT("Biome '%s' climate profile could not be resolved."), *Pair.Key.ToString());
                return false;
            }
            FRBWeatherRuntimeState Candidate = Record.Weather;
            FRBWeatherSimulationEvents Events;
            const int64 Seed = StableBiomeSeed(StableSeed, Pair.Key, Record.SeedOffset);
            if (!Candidate.bInitialized)
            {
                if (!FRBWeatherSimulation::Initialize(Candidate, WeatherPresetTable, WeatherTransitionTable, Climate,
                    Record.InitialWeather, Season, DayIndex, HourOfDay, Seed, Events, Error))
                {
                    UE_LOG(LogRBWeather, Error, TEXT("Biome '%s' initialization failed: %s"), *Pair.Key.ToString(), *Error);
                    return false;
                }
            }
            if (DeltaGameHours > 0.0 || Record.Weather.bInitialized)
            {
                if (!FRBWeatherSimulation::Advance(Candidate, WeatherPresetTable, WeatherTransitionTable, Climate,
                    Season, DayIndex, HourOfDay, DeltaGameHours, Seed, Events, Error))
                {
                    UE_LOG(LogRBWeather, Error, TEXT("Biome '%s' advance failed; no weather state was committed: %s"),
                        *Pair.Key.ToString(), *Error);
                    return false;
                }
            }
            CandidateBiomes.Add(Pair.Key, Candidate);
        }
    }

    // Commit only after every candidate succeeds.
    GlobalState = CandidateGlobal;
    if (System)
    {
        for (auto& Pair : System->GetCanonicalBiomes())
        {
            if (const FRBWeatherRuntimeState* Candidate = CandidateBiomes.Find(Pair.Key)) Pair.Value.Weather = *Candidate;
        }
        for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
        {
            ARBWeatherBiomeVolume* Biome = Weak.Get();
            if (!IsValid(Biome)) continue;
            if (const FRBWeatherRuntimeState* Candidate = CandidateBiomes.Find(Biome->BiomeId))
                Biome->ApplyCanonicalState(*Candidate, false);
        }
    }
    PublishGlobal(GlobalEvents);
    if (System)
    {
        for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
        {
            ARBWeatherBiomeVolume* Biome = Weak.Get();
            if (!IsValid(Biome)) continue;
            if (const FRBWeatherRuntimeState* Candidate = CandidateBiomes.Find(Biome->BiomeId))
                Biome->ApplyCanonicalState(*Candidate, true);
        }
    }
    return true;
}

bool ARBWeatherDirector::ForceGlobalWeather(FName WeatherId, double TransitionGameHours)
{
    if (!HasAuthority() || !GlobalState.bInitialized) return false;
    FRBWeatherClimateRow Climate;
    if (!GetClimateProfile(NAME_None, Climate)) return false;
    FRBWeatherSimulationEvents Events;
    FString Error;
    const bool bOk = FRBWeatherSimulation::ForceWeather(GlobalState, WeatherPresetTable, WeatherTransitionTable,
        Climate, WeatherId, TransitionGameHours, StableSeed, Events, Error);
    if (!bOk)
    {
        UE_LOG(LogRBWeather, Error, TEXT("ForceGlobalWeather failed: %s"), *Error);
        return false;
    }
    PublishGlobal(Events);
    return true;
}

bool ARBWeatherDirector::ForceBiomeWeather(FName BiomeId, FName WeatherId, double TransitionGameHours)
{
    if (!HasAuthority() || BiomeId.IsNone()) return false;
    URBWeatherSubsystem* System = GetWorld() ? GetWorld()->GetSubsystem<URBWeatherSubsystem>() : nullptr;
    if (!System) return false;
    FRBWeatherCanonicalBiomeRecord* Record = System->FindCanonicalBiome(BiomeId);
    if (!Record || !Record->Weather.bInitialized) return false;

    FRBWeatherClimateRow Climate;
    if (!GetClimateProfile(Record->ClimateProfileOverride, Climate)) return false;
    FRBWeatherRuntimeState Candidate = Record->Weather;
    FRBWeatherSimulationEvents Events;
    FString Error;
    const int64 Seed = StableBiomeSeed(StableSeed, BiomeId, Record->SeedOffset);
    if (!FRBWeatherSimulation::ForceWeather(Candidate, WeatherPresetTable, WeatherTransitionTable,
        Climate, WeatherId, TransitionGameHours, Seed, Events, Error))
    {
        UE_LOG(LogRBWeather, Error, TEXT("ForceBiomeWeather failed for '%s': %s"), *BiomeId.ToString(), *Error);
        return false;
    }

    Record->Weather = Candidate;
    for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
    {
        ARBWeatherBiomeVolume* Biome = Weak.Get();
        if (IsValid(Biome) && Biome->BiomeId == BiomeId) Biome->ApplyCanonicalState(Candidate, false);
    }
    for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
    {
        ARBWeatherBiomeVolume* Biome = Weak.Get();
        if (IsValid(Biome) && Biome->BiomeId == BiomeId) Biome->ApplyCanonicalState(Candidate, true);
    }
    return true;
}

bool ARBWeatherDirector::GetGlobalSnapshot(FRBWeatherSnapshotAtLocation& OutSnapshot) const
{
    OutSnapshot = {};
    if (!GlobalState.bInitialized) return false;
    FRBWeatherClimateRow Climate;
    if (!GetClimateProfile(NAME_None, Climate)) return false;
    FString Error;
    double Alpha = 1.0;
    if (!FRBWeatherSimulation::GetVisualEnvironment(GlobalState, WeatherPresetTable, WeatherTransitionTable,
        Climate, OutSnapshot.Environment, Alpha, Error)) return false;
    if (!FRBWeatherSimulation::GetGameplayEnvironment(GlobalState, WeatherPresetTable, WeatherTransitionTable,
        Climate, OutSnapshot.GameplayEnvironment, Error)) return false;
    bool bTempValid = false;
    const double Outside = FRBWeatherSimulation::GetOutsideTemperatureC(GlobalState, WeatherPresetTable,
        WeatherTransitionTable, Climate, bTempValid);
    if (!bTempValid) return false;
    OutSnapshot.BiomeId = TEXT("Global");
    OutSnapshot.BiomeBlendWeight = 0.0;
    OutSnapshot.CurrentWeather = GlobalState.CurrentWeather;
    OutSnapshot.TargetWeather = GlobalState.TargetWeather;
    OutSnapshot.TransitionAlpha = Alpha;
    OutSnapshot.LightningSerial = GlobalState.LightningSerial;
    OutSnapshot.Wetness = GlobalState.Wetness;
    OutSnapshot.SnowDepthCm = GlobalState.SnowDepthCm;
    OutSnapshot.Dust = GlobalState.Dust;
    OutSnapshot.VisualWetness = GlobalState.Wetness;
    OutSnapshot.VisualSnowDepthCm = GlobalState.SnowDepthCm;
    OutSnapshot.VisualDust = GlobalState.Dust;
    OutSnapshot.BaseTemperatureC = GlobalState.BaseTemperatureC;
    OutSnapshot.OutsideTemperatureC = Outside;
    OutSnapshot.VisualOutsideTemperatureC = Outside;
    OutSnapshot.LocalTemperatureC = Outside;
    return true;
}

bool ARBWeatherDirector::ValidateConfiguredData(FString& OutError) const
{
    if (!ClimateTable || ClimateTable->GetRowStruct() != FRBWeatherClimateRow::StaticStruct())
    {
        OutError = TEXT("ClimateTable must use FRBWeatherClimateRow.");
        return false;
    }
    FRBWeatherClimateRow Climate;
    if (!GetClimateProfile(NAME_None, Climate))
    {
        OutError = TEXT("Configured climate profile row was not found.");
        return false;
    }
    if (!FRBWeatherSimulation::ValidateData(WeatherPresetTable, WeatherTransitionTable, Climate, OutError)) return false;
    if (!WeatherPresetTable->GetRowNames().Contains(InitialWeather))
    {
        OutError = TEXT("InitialWeather must match a weather preset DataTable row name.");
        return false;
    }
    if (UWorld* World = GetWorld())
    {
        if (const URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            TSet<FName> Ids;
            for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
            {
                const ARBWeatherBiomeVolume* Biome = Weak.Get();
                if (!IsValid(Biome)) continue;
                if (Biome->BiomeId.IsNone())
                {
                    OutError = TEXT("Every biome must have a non-None BiomeId for save/restore and deterministic seeding.");
                    return false;
                }
                if (Ids.Contains(Biome->BiomeId))
                {
                    OutError = FString::Printf(TEXT("Duplicate BiomeId '%s'."), *Biome->BiomeId.ToString());
                    return false;
                }
                Ids.Add(Biome->BiomeId);
                FRBWeatherClimateRow BiomeClimate;
                if (!GetClimateProfile(Biome->ClimateProfileOverride, BiomeClimate))
                {
                    OutError = FString::Printf(TEXT("Biome '%s' has an invalid climate override."), *Biome->BiomeId.ToString());
                    return false;
                }
                if (!WeatherPresetTable->GetRowNames().Contains(Biome->InitialWeather))
                {
                    OutError = FString::Printf(TEXT("Biome '%s' has an unknown InitialWeather."), *Biome->BiomeId.ToString());
                    return false;
                }
            }
        }
    }
    OutError.Reset();
    return true;
}

int64 ARBWeatherDirector::GetBiomeStableSeed(const ARBWeatherBiomeVolume* Biome) const
{
    return IsValid(Biome) ? StableBiomeSeed(StableSeed, Biome->BiomeId, Biome->SeedOffset) : StableSeed;
}

FRBWeatherSaveSnapshot ARBWeatherDirector::MakeSnapshot() const
{
    FRBWeatherSaveSnapshot Snapshot;
    Snapshot.SchemaVersion = 2;
    Snapshot.GlobalWeather = GlobalState;
    if (const UWorld* World = GetWorld())
    {
        if (const URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
        {
            TArray<FName> Ids;
            System->GetCanonicalBiomes().GetKeys(Ids);
            Ids.Sort([](FName A, FName B) { return A.LexicalLess(B); });
            for (FName Id : Ids)
            {
                const FRBWeatherCanonicalBiomeRecord* Record = System->FindCanonicalBiome(Id);
                if (!Record) continue;
                FRBWeatherBiomeSaveState Entry;
                Entry.BiomeId = Id;
                Entry.ClimateProfileOverride = Record->ClimateProfileOverride;
                Entry.InitialWeather = Record->InitialWeather;
                Entry.SeedOffset = Record->SeedOffset;
                Entry.Weather = Record->Weather;
                Snapshot.Biomes.Add(Entry);
            }
        }
    }
    return Snapshot;
}

bool ARBWeatherDirector::RestoreSnapshot(const FRBWeatherSaveSnapshot& Snapshot)
{
    if (!HasAuthority() || (Snapshot.SchemaVersion != 1 && Snapshot.SchemaVersion != 2) ||
        !Snapshot.GlobalWeather.bInitialized) return false;

    FRBWeatherClimateRow GlobalClimate;
    if (!GetClimateProfile(NAME_None, GlobalClimate)) return false;
    FString Error;
    FRBWeatherEnvironment Dummy;
    double Alpha = 0.0;
    if (!FMath::IsFinite(Snapshot.GlobalWeather.BaseTemperatureC) || !FMath::IsFinite(Snapshot.GlobalWeather.HourOfDay) ||
        !FRBWeatherSimulation::GetVisualEnvironment(Snapshot.GlobalWeather, WeatherPresetTable, WeatherTransitionTable,
            GlobalClimate, Dummy, Alpha, Error)) return false;
    bool bGlobalTimeValid = false;
    FRBWeatherSimulation::ComputeBaseTemperatureC(GlobalClimate, Snapshot.GlobalWeather.CurrentSeason,
        Snapshot.GlobalWeather.DayIndex, Snapshot.GlobalWeather.HourOfDay, StableSeed, bGlobalTimeValid);
    if (!bGlobalTimeValid) return false;

    URBWeatherSubsystem* System = GetWorld() ? GetWorld()->GetSubsystem<URBWeatherSubsystem>() : nullptr;
    if (!System) return Snapshot.Biomes.IsEmpty();

    TSet<FName> SavedIds;
    TMap<FName, FRBWeatherCanonicalBiomeRecord> Pending;
    for (const FRBWeatherBiomeSaveState& Entry : Snapshot.Biomes)
    {
        if (Entry.BiomeId.IsNone() || SavedIds.Contains(Entry.BiomeId) || !Entry.Weather.bInitialized) return false;
        SavedIds.Add(Entry.BiomeId);

        const FRBWeatherCanonicalBiomeRecord* Existing = System->FindCanonicalBiome(Entry.BiomeId);
        FRBWeatherCanonicalBiomeRecord Record;
        if (Snapshot.SchemaVersion == 1)
        {
            if (!Existing) return false;
            Record = *Existing;
        }
        else
        {
            Record.BiomeId = Entry.BiomeId;
            Record.ClimateProfileOverride = Entry.ClimateProfileOverride;
            Record.InitialWeather = Entry.InitialWeather;
            Record.SeedOffset = Entry.SeedOffset;
            if (Record.InitialWeather.IsNone() || !WeatherPresetTable ||
                !WeatherPresetTable->GetRowNames().Contains(Record.InitialWeather)) return false;
            if (Existing && (Existing->ClimateProfileOverride != Record.ClimateProfileOverride ||
                Existing->InitialWeather != Record.InitialWeather || Existing->SeedOffset != Record.SeedOffset)) return false;
        }
        Record.Weather = Entry.Weather;

        FRBWeatherClimateRow Climate;
        if (!GetClimateProfile(Record.ClimateProfileOverride, Climate)) return false;
        if (!FMath::IsFinite(Record.Weather.BaseTemperatureC) || !FMath::IsFinite(Record.Weather.HourOfDay)) return false;
        bool bTimeValid = false;
        FRBWeatherSimulation::ComputeBaseTemperatureC(Climate, Record.Weather.CurrentSeason,
            Record.Weather.DayIndex, Record.Weather.HourOfDay,
            StableBiomeSeed(StableSeed, Record.BiomeId, Record.SeedOffset), bTimeValid);
        if (!bTimeValid || !FRBWeatherSimulation::GetVisualEnvironment(Record.Weather, WeatherPresetTable,
            WeatherTransitionTable, Climate, Dummy, Alpha, Error)) return false;
        Pending.Add(Record.BiomeId, Record);
    }

    // A snapshot is a complete authoritative world snapshot, never a partial patch.
    for (const auto& Pair : System->GetCanonicalBiomes())
    {
        if (!SavedIds.Contains(Pair.Key)) return false;
    }
    if (Snapshot.SchemaVersion == 1 && Pending.Num() != System->GetCanonicalBiomes().Num()) return false;

    // All validation completed before any authoritative mutation or observable delegate.
    GlobalState = Snapshot.GlobalWeather;
    System->GetCanonicalBiomes() = Pending;
    for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
    {
        ARBWeatherBiomeVolume* Biome = Weak.Get();
        if (!IsValid(Biome)) continue;
        if (const FRBWeatherCanonicalBiomeRecord* Record = System->FindCanonicalBiome(Biome->BiomeId))
            Biome->ApplyCanonicalState(Record->Weather, false);
    }
    FRBWeatherSimulationEvents Events;
    Events.bChanged = true;
    PublishGlobal(Events);
    for (const TWeakObjectPtr<ARBWeatherBiomeVolume>& Weak : System->GetBiomes())
    {
        ARBWeatherBiomeVolume* Biome = Weak.Get();
        if (!IsValid(Biome)) continue;
        if (const FRBWeatherCanonicalBiomeRecord* Record = System->FindCanonicalBiome(Biome->BiomeId))
            Biome->ApplyCanonicalState(Record->Weather, true);
    }
    return true;
}
