#include "RBFoundationRBAdapterSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "RBFoundationSaveProviders.h"

#include "RBFoundationSubsystem.h"
#include "RBItemEconomySubsystem.h"
#include "RBSaveSubsystem.h"
#include "RBRoutineWorldSubsystem.h"
#include "RBWeatherDirector.h"
#include "RBOptimizationGroup.h"

namespace
{
struct FRequiredPlugin
{
    const TCHAR* Name;
    const TCHAR* Version;
};

constexpr FRequiredPlugin FrozenPlugins[] = {
    { TEXT("RBSave"), TEXT("1.0.0") },
    { TEXT("RBItemEconomy"), TEXT("1.0.0") },
    { TEXT("RBRoutine"), TEXT("1.0.0") },
    { TEXT("RBWeather"), TEXT("1.0.1") },
    { TEXT("RBOptimization"), TEXT("1.0.0") },
    { TEXT("RefinedBadgerCombat"), TEXT("1.0.0") }
};

FRBFoundationOperationResult FoundationFailure(FGuid CorrelationId,
    const FString& Code, const FString& Message)
{
    FRBFoundationOperationResult Result;
    Result.CorrelationId = CorrelationId;
    Result.Code = Code;
    Result.Message = Message;
    return Result;
}
}
void URBFoundationRBAdapterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<URBFoundationSubsystem>();
    Collection.InitializeDependency<URBSaveSubsystem>();
    Collection.InitializeDependency<URBItemEconomySubsystem>();

    FString Error;
    if (!VerifyFrozenStack(Error) || !RegisterStackAuthorities(Error)
        || !RegisterSaveProviders(Error))
    {
        InitializationError = Error;
        return;
    }

    URBFoundationSubsystem* Foundation = GetFoundation();
    if (!Foundation || !Foundation->RegisterTimeConsumer(TEXT("RBStack"), this, Error))
    {
        InitializationError = Error.IsEmpty() ? TEXT("Unable to register RB stack time consumer.") : Error;
        return;
    }

    const TArray<FName> Required = {
        TEXT("Persistence"), TEXT("ItemEconomy"), TEXT("Routine"),
        TEXT("Combat"), TEXT("Optimization"), TEXT("Weather")
    };
    if (!Foundation->SetRequiredCapabilities(Required, Error)) InitializationError = Error;
}

void URBFoundationRBAdapterSubsystem::Deinitialize()
{
    if (URBFoundationSubsystem* Foundation = GetFoundation())
    {
        Foundation->UnregisterTimeConsumer(TEXT("RBStack"), this);
        Foundation->UnregisterCapabilitiesForOwner(this);
        Foundation->UnregisterAuthority(TEXT("Routine"), this);
        Foundation->UnregisterAuthority(TEXT("Combat"), this);
        Foundation->UnregisterAuthority(TEXT("Optimization"), this);
    }
    UnregisterSaveProviders();
    WeatherDirector.Reset();
    OptimizationGroups.Reset();
    Super::Deinitialize();
}

URBFoundationSubsystem* URBFoundationRBAdapterSubsystem::GetFoundation() const
{
    const UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetSubsystem<URBFoundationSubsystem>() : nullptr;
}

URBSaveSubsystem* URBFoundationRBAdapterSubsystem::GetSave() const
{
    const UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetSubsystem<URBSaveSubsystem>() : nullptr;
}

URBItemEconomySubsystem* URBFoundationRBAdapterSubsystem::GetItemEconomy() const
{
    const UGameInstance* GI = GetGameInstance();
    return GI ? GI->GetSubsystem<URBItemEconomySubsystem>() : nullptr;
}

URBRoutineWorldSubsystem* URBFoundationRBAdapterSubsystem::GetRoutine() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<URBRoutineWorldSubsystem>() : nullptr;
}
bool URBFoundationRBAdapterSubsystem::VerifyFrozenStack(FString& OutError) const
{
    URBFoundationSubsystem* Foundation = GetFoundation();
    if (!Foundation)
    {
        OutError = TEXT("RB Foundation subsystem unavailable.");
        return false;
    }
    for (const FRequiredPlugin& Required : FrozenPlugins)
    {
        if (!Foundation->VerifyPluginVersion(Required.Name, Required.Version, OutError))
            return false;
    }
    return true;
}

bool URBFoundationRBAdapterSubsystem::RegisterStackAuthorities(FString& OutError)
{
    URBFoundationSubsystem* Foundation = GetFoundation();
    URBSaveSubsystem* Save = GetSave();
    URBItemEconomySubsystem* Items = GetItemEconomy();
    if (!Foundation || !Save || !Items)
    {
        OutError = TEXT("Required RB GameInstance subsystem unavailable.");
        return false;
    }
    auto Register = [Foundation, &OutError](FName Domain, UObject* Owner,
        const TCHAR* Version, FName Capability) -> bool
    {
        if (!Foundation->RegisterAuthority(Domain, Owner, Version, OutError)) return false;
        if (!Foundation->RegisterCapability(Capability, Domain, Owner, Version, true, OutError))
        {
            Foundation->UnregisterAuthority(Domain, Owner);
            return false;
        }
        return true;
    };

    if (!Register(TEXT("Persistence"), Save, TEXT("1.0.0"), TEXT("Persistence"))) return false;
    if (!Register(TEXT("ItemEconomy"), Items, TEXT("1.0.0"), TEXT("ItemEconomy"))) return false;
    if (!Register(TEXT("Routine"), this, TEXT("1.0.0"), TEXT("Routine"))) return false;
    if (!Register(TEXT("Combat"), this, TEXT("1.0.0"), TEXT("Combat"))) return false;
    if (!Register(TEXT("Optimization"), this, TEXT("1.0.0"), TEXT("Optimization"))) return false;
    return true;
}

bool URBFoundationRBAdapterSubsystem::RegisterSaveProviders(FString& OutError)
{
    URBSaveSubsystem* Save = GetSave();
    if (!Save) { OutError = TEXT("RB Save subsystem unavailable."); return false; }

    auto AddProvider = [this, Save, &OutError](URBFoundationSaveProviderBase* Provider) -> bool
    {
        Provider->Adapter = this;
        if (!Save->RegisterDomainProvider(Provider, OutError)) return false;
        SaveProviders.Add(Provider);
        return true;
    };
    if (!AddProvider(NewObject<URBFoundationCoreSaveProvider>(this))) return false;
    if (!AddProvider(NewObject<URBFoundationItemEconomySaveProvider>(this))) return false;
    if (!AddProvider(NewObject<URBFoundationOptimizationSaveProvider>(this))) return false;
    if (!AddProvider(NewObject<URBFoundationRoutineSaveProvider>(this))) return false;
    if (!AddProvider(NewObject<URBFoundationWeatherSaveProvider>(this))) return false;
    return true;
}

void URBFoundationRBAdapterSubsystem::UnregisterSaveProviders()
{
    if (URBSaveSubsystem* Save = GetSave())
    {
        for (UObject* Provider : SaveProviders)
        {
            if (IsValid(Provider)) Save->UnregisterDomainProvider(Provider);
        }
    }
    SaveProviders.Reset();
}

bool URBFoundationRBAdapterSubsystem::RegisterWeatherDirector(
    ARBWeatherDirector* Director, FString& OutError)
{
    if (!IsValid(Director)) { OutError = TEXT("Weather director is invalid."); return false; }
    if (WeatherDirector.IsValid() && WeatherDirector.Get() != Director)
    {
        OutError = TEXT("A different RB Weather director is already registered.");
        return false;
    }
    URBFoundationSubsystem* Foundation = GetFoundation();
    if (!Foundation) { OutError = TEXT("RB Foundation subsystem unavailable."); return false; }
    if (!Foundation->RegisterAuthority(TEXT("Weather"), Director, TEXT("1.0.1"), OutError)) return false;
    if (!Foundation->RegisterCapability(TEXT("Weather"), TEXT("Weather"), Director,
        TEXT("1.0.1"), true, OutError))
    {
        Foundation->UnregisterAuthority(TEXT("Weather"), Director);
        return false;
    }
    WeatherDirector = Director;
    bWeatherTimeApplied = false;
    return true;
}

void URBFoundationRBAdapterSubsystem::UnregisterWeatherDirector(ARBWeatherDirector* Director)
{
    if (WeatherDirector.Get() != Director) return;
    if (URBFoundationSubsystem* Foundation = GetFoundation())
    {
        Foundation->UnregisterCapabilitiesForOwner(Director);
        Foundation->UnregisterAuthority(TEXT("Weather"), Director);
    }
    WeatherDirector.Reset();
    bWeatherTimeApplied = false;
}

bool URBFoundationRBAdapterSubsystem::RegisterOptimizationGroup(
    FName GroupId, ARBOptimizationGroup* Group, FString& OutError)
{
    if (GroupId.IsNone() || !IsValid(Group))
    {
        OutError = TEXT("Optimization group registration requires ID and valid group.");
        return false;
    }
    if (TWeakObjectPtr<ARBOptimizationGroup>* Existing = OptimizationGroups.Find(GroupId))
    {
        if (Existing->Get() == Group) return true;
        OutError = TEXT("Optimization group ID is already registered to another group.");
        return false;
    }
    OptimizationGroups.Add(GroupId, Group);
    if (URBFoundationSubsystem* Foundation = GetFoundation())
    {
        const FName Capability(*FString::Printf(TEXT("OptimizationGroup.%s"), *GroupId.ToString()));
        if (!Foundation->RegisterCapability(Capability, TEXT("Optimization"), this,
            TEXT("1.0.0"), false, OutError))
        {
            OptimizationGroups.Remove(GroupId);
            return false;
        }
    }
    return true;
}

void URBFoundationRBAdapterSubsystem::UnregisterOptimizationGroup(
    FName GroupId, ARBOptimizationGroup* Group)
{
    if (TWeakObjectPtr<ARBOptimizationGroup>* Existing = OptimizationGroups.Find(GroupId))
    {
        if (Existing->Get() == Group) OptimizationGroups.Remove(GroupId);
    }
}

ARBOptimizationGroup* URBFoundationRBAdapterSubsystem::GetOptimizationGroup(FName GroupId) const
{
    return OptimizationGroups.FindRef(GroupId).Get();
}

FRBFoundationOperationResult URBFoundationRBAdapterSubsystem::CommitRoutineProductionAward(
    FGuid CorrelationId, FName AgentId, int64 DestinationInventoryId,
    FName DefinitionId, int64 Quantity, int32 Quality)
{
    if (!CorrelationId.IsValid() || AgentId.IsNone() || DestinationInventoryId <= 0
        || DefinitionId.IsNone() || Quantity <= 0 || Quality < 0 || Quality > 10000)
    {
        return FoundationFailure(CorrelationId, TEXT("InvalidRequest"),
            TEXT("Production award request contains invalid identity, destination or quantity."));
    }
    URBFoundationSubsystem* Foundation = GetFoundation();
    URBItemEconomySubsystem* Items = GetItemEconomy();
    URBRoutineWorldSubsystem* Routine = GetRoutine();
    if (!Foundation || !Items || !Routine)
        return FoundationFailure(CorrelationId, TEXT("MissingAuthority"), TEXT("Required RB authority unavailable."));

    FRBRoutineLogicalPersonRecord Person;
    if (!Routine->GetLogicalPerson(AgentId, Person))
        return FoundationFailure(CorrelationId, TEXT("UnknownPerson"), TEXT("Routine person does not exist."));

    const FString Fingerprint = FString::Printf(TEXT("%s|%lld|%s|%lld|%d"),
        *AgentId.ToString(), static_cast<long long>(DestinationInventoryId),
        *DefinitionId.ToString(), static_cast<long long>(Quantity), Quality);
    FRBFoundationTransactionRecord Existing;
    FString Error;
    const ERBFoundationBeginResult Begin = Foundation->BeginTransaction(
        CorrelationId, TEXT("RoutineProductionAward"), Fingerprint, Existing, Error);
    if (Begin == ERBFoundationBeginResult::Conflict || Begin == ERBFoundationBeginResult::Invalid)
        return FoundationFailure(CorrelationId, TEXT("TransactionRejected"), Error);
    if (Begin == ERBFoundationBeginResult::ReplayFailed)
        return FoundationFailure(CorrelationId, TEXT("PreviouslyFailed"), Existing.Error);
    if (Begin == ERBFoundationBeginResult::ReplayCommitted)
    {
        FRBFoundationOperationResult Result;
        Result.bSuccess = true;
        Result.bReplayed = true;
        Result.Code = TEXT("CommittedReplay");
        Result.Message = TEXT("Foundation transaction was already committed.");
        Result.Revision = Existing.DomainRevision;
        Result.CorrelationId = CorrelationId;
        return Result;
    }

    const FName EventId(*CorrelationId.ToString(EGuidFormats::Digits));
    const FRBItemOperationResult ItemResult = Items->CommitProductionAward(
        EventId, DestinationInventoryId, DefinitionId, Quantity, Quality);
    if (!ItemResult.bSuccess)
        return FoundationFailure(CorrelationId,
            ItemResult.Code.IsEmpty() ? TEXT("ItemEconomyRejected") : ItemResult.Code,
            ItemResult.Message);
    if (!Foundation->CommitTransaction(CorrelationId, ItemResult.Revision, Error))
        return FoundationFailure(CorrelationId, TEXT("FoundationCommitFailed"), Error);

    FString EventError;
    Foundation->PublishDomainEvent(CorrelationId, TEXT("ItemEconomy"),
        TEXT("ProductionAwardCommitted"), Fingerprint, ItemResult.Revision, EventError);

    FRBFoundationOperationResult Result;
    Result.bSuccess = true;
    Result.bReplayed = ItemResult.bReplayed || Begin == ERBFoundationBeginResult::ResumePending;
    Result.Code = ItemResult.Code.IsEmpty() ? TEXT("Committed") : ItemResult.Code;
    Result.Message = ItemResult.Message;
    Result.Revision = ItemResult.Revision;
    Result.CorrelationId = CorrelationId;
    return Result;
}

bool URBFoundationRBAdapterSubsystem::ValidateFoundationTime_Implementation(
    const FRBFoundationTimeState& NewTime, const FRBFoundationTimeState& PreviousTime,
    bool bHasPreviousTime, FString& OutError) const
{
    if (!NewTime.IsValid(&OutError)) return false;
    if (bHasPreviousTime && NewTime.GameSeconds < PreviousTime.GameSeconds)
    {
        OutError = TEXT("RB stack refuses regressing canonical time.");
        return false;
    }
    if (!GetRoutine() || !GetItemEconomy())
    {
        OutError = TEXT("Routine and Item Economy must exist before time synchronization.");
        return false;
    }
    return true;
}

bool URBFoundationRBAdapterSubsystem::ApplyFoundationTime_Implementation(
    const FRBFoundationTimeState& NewTime, const FRBFoundationTimeState& PreviousTime,
    bool bHasPreviousTime, FString& OutError)
{
    URBItemEconomySubsystem* Items = GetItemEconomy();
    URBRoutineWorldSubsystem* Routine = GetRoutine();
    ARBWeatherDirector* Weather = GetWeatherDirector();
    if (!Items || !Routine || !Weather)
    {
        OutError = TEXT("RB time bridge requires Item Economy, Routine and a registered Weather director.");
        return false;
    }

    const int64 DeltaSeconds = bHasPreviousTime
        ? NewTime.GameSeconds - PreviousTime.GameSeconds : 0;
    const double DeltaHours = static_cast<double>(DeltaSeconds) / 3600.0;
    const FRBWeatherSaveSnapshot WeatherBefore = Weather->MakeSnapshot();
    if (!Weather->AdvanceExternalTime(DeltaHours, NewTime.Season,
        NewTime.DayIndex, NewTime.HourOfDay))
    {
        OutError = TEXT("RB Weather rejected canonical time.");
        return false;
    }
    const FRBItemOperationResult ItemTime = Items->AdvanceCanonicalTime(NewTime.GameSeconds);
    if (!ItemTime.bSuccess)
    {
        Weather->RestoreSnapshot(WeatherBefore);
        OutError = FString::Printf(TEXT("RB Item Economy rejected canonical time: %s"),
            *ItemTime.Message);
        return false;
    }

    Routine->SetExternalTime(NewTime.DayIndex, NewTime.MinuteOfDay);
    if (DeltaSeconds > 0)
    {
        for (auto It = OptimizationGroups.CreateIterator(); It; ++It)
        {
            ARBOptimizationGroup* Group = It.Value().Get();
            if (!IsValid(Group)) { It.RemoveCurrent(); continue; }
            Group->AdvanceRespawnTime(static_cast<double>(DeltaSeconds));
        }
    }
    LastWeatherGameSeconds = NewTime.GameSeconds;
    bWeatherTimeApplied = true;
    return true;
}

void URBFoundationRBAdapterSubsystem::ResetTimeBridgeAfterRestore()
{
    bWeatherTimeApplied = false;
    LastWeatherGameSeconds = 0;
}
