#include "RBFoundationSubsystem.h"
#include "RBFoundationInterfaces.h"
#include "RBFoundationDeclarativeAdapter.h"

#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
constexpr int32 MaxFoundationTransactions = 100000;
constexpr int32 MaxFoundationTraceRecords = 10000;

FString ObjectLabel(const UObject* Object)
{
    return IsValid(Object) ? Object->GetPathName() : TEXT("<invalid>");
}
}

void URBFoundationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    OwnedProductAdapters.Reset();
    ProductAdapters.Reset();
    Authorities.Reset();
    Capabilities.Reset();
    RequiredCapabilities.Reset();
    CanonicalTimeSource.Reset();
    TimeConsumers.Reset();
    EventConsumers.Reset();
    Transactions.Reset();
    TraceRecords.Reset();
    NextTraceSequence = 1;
    LastTime = FRBFoundationTimeState{};
    bHasLastTime = false;
    bSynchronizingTime = false;
    ManifestRegistrationError.Reset();
    FString ManifestError;
    if (!RegisterInstalledDeclarativeProducts(ManifestError))
        ManifestRegistrationError = MoveTemp(ManifestError);
}

void URBFoundationSubsystem::Deinitialize()
{
    TArray<UObject*> Adapters;
    for (const auto& Pair : ProductAdapters)
        if (Pair.Value.Adapter.IsValid()) Adapters.Add(Pair.Value.Adapter.Get());
    for (UObject* Adapter : Adapters) UnregisterProductAdapter(Adapter);
    ProductAdapters.Reset();
    Authorities.Reset();
    Capabilities.Reset();
    RequiredCapabilities.Reset();
    CanonicalTimeSource.Reset();
    TimeConsumers.Reset();
    EventConsumers.Reset();
    Transactions.Reset();
    TraceRecords.Reset();
    NextTraceSequence = 1;
    ManifestRegistrationError.Reset();
    Super::Deinitialize();
}

void URBFoundationSubsystem::CompactInvalidRegistrations()
{
    for (auto It = ProductAdapters.CreateIterator(); It; ++It)
    {
        if (!It.Value().Adapter.IsValid()) It.RemoveCurrent();
    }
    for (auto It = Authorities.CreateIterator(); It; ++It)
    {
        if (!It.Value().Owner.IsValid()) It.RemoveCurrent();
    }
    for (auto It = Capabilities.CreateIterator(); It; ++It)
    {
        if (!It.Value().Owner.IsValid()) It.RemoveCurrent();
    }
    for (auto It = TimeConsumers.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid()) It.RemoveCurrent();
    }
    for (auto It = EventConsumers.CreateIterator(); It; ++It)
    {
        if (!It.Value().Consumer.IsValid()) It.RemoveCurrent();
    }
    if (!CanonicalTimeSource.IsValid()) CanonicalTimeSource.Reset();
}

FName URBFoundationSubsystem::FindProductIdForOwner(const UObject* Owner) const
{
    if (!IsValid(Owner)) return NAME_None;
    for (const auto& Pair : ProductAdapters)
    {
        if (Pair.Value.Adapter.Get() == Owner) return Pair.Key;
    }
    return NAME_None;
}

void URBFoundationSubsystem::AppendTrace(FRBFoundationTraceRecord Record)
{
    Record.Sequence = NextTraceSequence++;
    Record.TimestampSeconds = FPlatformTime::Seconds();
    if (TraceRecords.Num() >= MaxFoundationTraceRecords)
        TraceRecords.RemoveAt(0, TraceRecords.Num() - MaxFoundationTraceRecords + 1, EAllowShrinking::No);
    TraceRecords.Add(Record);
    OnTraceRecord.Broadcast(TraceRecords.Last());
}

bool URBFoundationSubsystem::RegisterProductAdapter(UObject* Adapter, FString& OutError)
{
    CompactInvalidRegistrations();
    if (!IsValid(Adapter) || !Adapter->GetClass()->ImplementsInterface(URBFoundationProductAdapter::StaticClass()))
    {
        OutError = TEXT("Product adapter must implement RBFoundationProductAdapter.");
        return false;
    }

    FRBFoundationProductDescriptor Descriptor;
    if (!IRBFoundationProductAdapter::Execute_GetFoundationProductDescriptor(Adapter, Descriptor, OutError))
        return false;
    if (!Descriptor.IsValid(&OutError)) return false;
    if (Descriptor.AdapterApiVersion != GetSupportedAdapterApiVersion())
    {
        OutError = FString::Printf(TEXT("Product '%s' uses adapter API %d; Foundation supports %d."),
            *Descriptor.ProductId.ToString(), Descriptor.AdapterApiVersion, GetSupportedAdapterApiVersion());
        return false;
    }
    if (const FProductAdapterEntry* Existing = ProductAdapters.Find(Descriptor.ProductId))
    {
        if (Existing->Adapter.Get() == Adapter) return true;
        OutError = FString::Printf(TEXT("Product ID '%s' is already registered by %s."),
            *Descriptor.ProductId.ToString(), *ObjectLabel(Existing->Adapter.Get()));
        return false;
    }

    TArray<FName> RegisteredDomains;
    for (FName Domain : Descriptor.AuthorityDomains)
    {
        if (!RegisterAuthority(Domain, Adapter, Descriptor.SemanticVersion, OutError))
        {
            for (FName Rollback : RegisteredDomains) UnregisterAuthority(Rollback, Adapter);
            return false;
        }
        RegisteredDomains.Add(Domain);
    }

    for (const FRBFoundationCapabilityDeclaration& Capability : Descriptor.Capabilities)
    {
        if (!RegisterCapability(Capability.Capability, Capability.AuthorityDomain, Adapter,
            Descriptor.SemanticVersion, Capability.bExclusive, OutError))
        {
            UnregisterCapabilitiesForOwner(Adapter);
            for (FName Rollback : RegisteredDomains) UnregisterAuthority(Rollback, Adapter);
            return false;
        }
    }

    FString StartupError;
    if (!IRBFoundationProductAdapter::Execute_FoundationAdapterStartup(Adapter, StartupError))
    {
        IRBFoundationProductAdapter::Execute_FoundationAdapterShutdown(Adapter);
        UnregisterCapabilitiesForOwner(Adapter);
        for (FName Rollback : RegisteredDomains) UnregisterAuthority(Rollback, Adapter);
        OutError = FString::Printf(TEXT("Product '%s' startup failed: %s"),
            *Descriptor.ProductId.ToString(), *StartupError);
        return false;
    }

    FProductAdapterEntry Entry;
    Entry.Adapter = Adapter;
    Entry.Descriptor = MoveTemp(Descriptor);
    Entry.bStarted = true;
    ProductAdapters.Add(Entry.Descriptor.ProductId, MoveTemp(Entry));
    return true;
}

bool URBFoundationSubsystem::RegisterDeclarativeProduct(
    const FRBFoundationProductDescriptor& Descriptor, FString& OutError)
{
    URBFoundationDeclarativeProductAdapter* Adapter =
        NewObject<URBFoundationDeclarativeProductAdapter>(this);
    if (!Adapter->Configure(Descriptor, OutError)) return false;
    if (!RegisterProductAdapter(Adapter, OutError)) return false;
    OwnedProductAdapters.Add(Adapter);
    return true;
}

bool URBFoundationSubsystem::UnregisterProduct(FName ProductId)
{
    FProductAdapterEntry* Entry = ProductAdapters.Find(ProductId);
    if (!Entry || !Entry->Adapter.IsValid()) return false;
    UObject* Adapter = Entry->Adapter.Get();
    UnregisterProductAdapter(Adapter);
    OwnedProductAdapters.Remove(Adapter);
    return true;
}

void URBFoundationSubsystem::UnregisterProductAdapter(UObject* Adapter)
{
    if (!IsValid(Adapter)) return;
    FName ProductId = NAME_None;
    FRBFoundationProductDescriptor Descriptor;
    for (const auto& Pair : ProductAdapters)
    {
        if (Pair.Value.Adapter.Get() == Adapter)
        {
            ProductId = Pair.Key;
            Descriptor = Pair.Value.Descriptor;
            break;
        }
    }
    if (ProductId.IsNone()) return;
    IRBFoundationProductAdapter::Execute_FoundationAdapterShutdown(Adapter);
    UnregisterCapabilitiesForOwner(Adapter);
    for (FName Domain : Descriptor.AuthorityDomains) UnregisterAuthority(Domain, Adapter);
    ProductAdapters.Remove(ProductId);
}

TArray<FRBFoundationProductRecord> URBFoundationSubsystem::GetProducts() const
{
    TArray<FRBFoundationProductRecord> Result;
    for (const auto& Pair : ProductAdapters)
    {
        if (!Pair.Value.Adapter.IsValid()) continue;
        FRBFoundationProductRecord Record;
        Record.Descriptor = Pair.Value.Descriptor;
        Record.AdapterName = ObjectLabel(Pair.Value.Adapter.Get());
        Record.bStarted = Pair.Value.bStarted;
        Result.Add(MoveTemp(Record));
    }
    Result.Sort([](const auto& A, const auto& B)
    {
        return A.Descriptor.ProductId.LexicalLess(B.Descriptor.ProductId);
    });
    return Result;
}

bool URBFoundationSubsystem::GetProduct(FName ProductId, FRBFoundationProductRecord& OutProduct) const
{
    const FProductAdapterEntry* Entry = ProductAdapters.Find(ProductId);
    if (!Entry || !Entry->Adapter.IsValid()) return false;
    OutProduct.Descriptor = Entry->Descriptor;
    OutProduct.AdapterName = ObjectLabel(Entry->Adapter.Get());
    OutProduct.bStarted = Entry->bStarted;
    return true;
}

TArray<FRBFoundationAuthorityRecord> URBFoundationSubsystem::GetAuthorities() const
{
    TArray<FRBFoundationAuthorityRecord> Result;
    for (const auto& Pair : Authorities)
    {
        if (!Pair.Value.Owner.IsValid()) continue;
        FRBFoundationAuthorityRecord Record;
        Record.Domain = Pair.Key;
        Record.Version = Pair.Value.Version;
        Record.OwnerName = ObjectLabel(Pair.Value.Owner.Get());
        for (const auto& Product : ProductAdapters)
        {
            if (Product.Value.Adapter.Get() == Pair.Value.Owner.Get())
            {
                Record.ProductId = Product.Key;
                break;
            }
        }
        Result.Add(MoveTemp(Record));
    }
    Result.Sort([](const auto& A, const auto& B) { return A.Domain.LexicalLess(B.Domain); });
    return Result;
}

bool URBFoundationSubsystem::RegisterAuthority(FName Domain, UObject* Owner,
    const FString& Version, FString& OutError)
{
    CompactInvalidRegistrations();
    if (Domain.IsNone() || !IsValid(Owner))
    {
        OutError = TEXT("Authority registration requires a domain and valid owner.");
        return false;
    }
    if (FAuthorityEntry* Existing = Authorities.Find(Domain))
    {
        if (Existing->Owner.Get() == Owner)
        {
            Existing->Version = Version;
            return true;
        }
        OutError = FString::Printf(TEXT("Duplicate authority '%s': %s already owns it."),
            *Domain.ToString(), *ObjectLabel(Existing->Owner.Get()));
        return false;
    }
    FAuthorityEntry Entry;
    Entry.Owner = Owner;
    Entry.Version = Version;
    Authorities.Add(Domain, MoveTemp(Entry));
    return true;
}

void URBFoundationSubsystem::UnregisterAuthority(FName Domain, UObject* Owner)
{
    if (FAuthorityEntry* Existing = Authorities.Find(Domain))
    {
        if (Existing->Owner.Get() == Owner) Authorities.Remove(Domain);
    }
}

bool URBFoundationSubsystem::RegisterCapability(FName Capability, FName AuthorityDomain,
    UObject* Owner, const FString& Version, bool bExclusive, FString& OutError)
{
    CompactInvalidRegistrations();
    if (Capability.IsNone() || AuthorityDomain.IsNone() || !IsValid(Owner))
    {
        OutError = TEXT("Capability registration requires capability, authority domain and owner.");
        return false;
    }
    TArray<FCapabilityEntry> Existing;
    Capabilities.MultiFind(Capability, Existing);
    for (const FCapabilityEntry& Entry : Existing)
    {
        if (Entry.Owner.Get() == Owner && Entry.AuthorityDomain == AuthorityDomain) return true;
        if (bExclusive || Entry.bExclusive)
        {
            OutError = FString::Printf(TEXT("Exclusive capability '%s' already registered by %s."),
                *Capability.ToString(), *ObjectLabel(Entry.Owner.Get()));
            return false;
        }
    }
    FCapabilityEntry Entry;
    Entry.AuthorityDomain = AuthorityDomain;
    Entry.Owner = Owner;
    Entry.Version = Version;
    Entry.bExclusive = bExclusive;
    Capabilities.Add(Capability, MoveTemp(Entry));
    return true;
}

void URBFoundationSubsystem::UnregisterCapabilitiesForOwner(UObject* Owner)
{
    for (auto It = Capabilities.CreateIterator(); It; ++It)
    {
        if (It.Value().Owner.Get() == Owner) It.RemoveCurrent();
    }
}

bool URBFoundationSubsystem::HasCapability(FName Capability) const
{
    TArray<FCapabilityEntry> Entries;
    Capabilities.MultiFind(Capability, Entries);
    for (const FCapabilityEntry& Entry : Entries)
    {
        if (Entry.Owner.IsValid()) return true;
    }
    return false;
}

TArray<FRBFoundationCapabilityRecord> URBFoundationSubsystem::GetCapabilities() const
{
    TArray<FRBFoundationCapabilityRecord> Result;
    for (auto It = Capabilities.CreateConstIterator(); It; ++It)
    {
        const FCapabilityEntry& Entry = It.Value();
        if (!Entry.Owner.IsValid()) continue;
        FRBFoundationCapabilityRecord Record;
        Record.Capability = It.Key();
        Record.AuthorityDomain = Entry.AuthorityDomain;
        Record.Version = Entry.Version;
        Record.bExclusive = Entry.bExclusive;
        Record.OwnerName = ObjectLabel(Entry.Owner.Get());
        Result.Add(MoveTemp(Record));
    }
    Result.Sort([](const auto& A, const auto& B)
    {
        return A.Capability.LexicalLess(B.Capability);
    });
    return Result;
}

bool URBFoundationSubsystem::SetRequiredCapabilities(const TArray<FName>& Required, FString& OutError)
{
    TSet<FName> Candidate;
    for (FName Capability : Required)
    {
        if (Capability.IsNone()) { OutError = TEXT("Required capability cannot be None."); return false; }
        Candidate.Add(Capability);
    }
    RequiredCapabilities = MoveTemp(Candidate);
    return true;
}

bool URBFoundationSubsystem::RegisterTimeSource(UObject* Source, FString& OutError)
{
    CompactInvalidRegistrations();
    if (!IsValid(Source) || !Source->GetClass()->ImplementsInterface(URBFoundationTimeSource::StaticClass()))
    {
        OutError = TEXT("Canonical time source must implement RBFoundationTimeSource.");
        return false;
    }
    if (CanonicalTimeSource.IsValid() && CanonicalTimeSource.Get() != Source)
    {
        OutError = FString::Printf(TEXT("Canonical time source already registered: %s."),
            *ObjectLabel(CanonicalTimeSource.Get()));
        return false;
    }
    if (!RegisterAuthority(TEXT("CanonicalTime"), Source, TEXT("host"), OutError)) return false;
    if (!RegisterCapability(TEXT("CanonicalTime"), TEXT("CanonicalTime"), Source,
        TEXT("host"), true, OutError))
    {
        UnregisterAuthority(TEXT("CanonicalTime"), Source);
        return false;
    }
    CanonicalTimeSource = Source;
    return true;
}

void URBFoundationSubsystem::UnregisterTimeSource(UObject* Source)
{
    if (CanonicalTimeSource.Get() != Source) return;
    CanonicalTimeSource.Reset();
    UnregisterCapabilitiesForOwner(Source);
    UnregisterAuthority(TEXT("CanonicalTime"), Source);
    ResetCanonicalTimeBaseline();
}

bool URBFoundationSubsystem::RegisterTimeConsumer(FName ConsumerId, UObject* Consumer,
    FString& OutError)
{
    CompactInvalidRegistrations();
    if (ConsumerId.IsNone() || !IsValid(Consumer)
        || !Consumer->GetClass()->ImplementsInterface(URBFoundationTimeConsumer::StaticClass()))
    {
        OutError = TEXT("Time consumer requires an ID and RBFoundationTimeConsumer implementation.");
        return false;
    }
    if (TWeakObjectPtr<UObject>* Existing = TimeConsumers.Find(ConsumerId))
    {
        if (Existing->Get() == Consumer) return true;
        OutError = FString::Printf(TEXT("Time consumer ID '%s' is already registered."), *ConsumerId.ToString());
        return false;
    }
    TimeConsumers.Add(ConsumerId, Consumer);
    return true;
}

void URBFoundationSubsystem::UnregisterTimeConsumer(FName ConsumerId, UObject* Consumer)
{
    if (TWeakObjectPtr<UObject>* Existing = TimeConsumers.Find(ConsumerId))
    {
        if (Existing->Get() == Consumer) TimeConsumers.Remove(ConsumerId);
    }
}

bool URBFoundationSubsystem::SynchronizeCanonicalTime(FString& OutError)
{
    CompactInvalidRegistrations();
    UObject* Source = CanonicalTimeSource.Get();
    if (!IsValid(Source)) { OutError = TEXT("No canonical time source is registered."); return false; }
    if (bSynchronizingTime) { OutError = TEXT("Canonical time synchronization is already in progress."); return false; }

    FRBFoundationTimeState Candidate;
    if (!IRBFoundationTimeSource::Execute_GetFoundationTimeState(Source, Candidate, OutError)) return false;
    if (!Candidate.IsValid(&OutError)) return false;
    if (bHasLastTime && Candidate.GameSeconds < LastTime.GameSeconds)
    {
        OutError = TEXT("Canonical game time regressed without resetting the Foundation baseline.");
        return false;
    }

    TArray<FName> ConsumerIds;
    TimeConsumers.GetKeys(ConsumerIds);
    ConsumerIds.Sort(FNameLexicalLess());
    for (FName ConsumerId : ConsumerIds)
    {
        UObject* Consumer = TimeConsumers.FindRef(ConsumerId).Get();
        if (!IsValid(Consumer)) continue;
        FString ConsumerError;
        if (!IRBFoundationTimeConsumer::Execute_ValidateFoundationTime(Consumer, Candidate,
            LastTime, bHasLastTime, ConsumerError))
        {
            OutError = FString::Printf(TEXT("Time consumer '%s' rejected time: %s"),
                *ConsumerId.ToString(), *ConsumerError);
            return false;
        }
    }

    TGuardValue<bool> SynchronizingGuard(bSynchronizingTime, true);
    for (FName ConsumerId : ConsumerIds)
    {
        UObject* Consumer = TimeConsumers.FindRef(ConsumerId).Get();
        if (!IsValid(Consumer)) continue;
        FString ConsumerError;
        if (!IRBFoundationTimeConsumer::Execute_ApplyFoundationTime(Consumer, Candidate,
            LastTime, bHasLastTime, ConsumerError))
        {
            OutError = FString::Printf(TEXT("Time consumer '%s' failed to apply time: %s"),
                *ConsumerId.ToString(), *ConsumerError);
            return false;
        }
    }
    LastTime = Candidate;
    bHasLastTime = true;
    return true;
}

void URBFoundationSubsystem::ResetCanonicalTimeBaseline()
{
    LastTime = FRBFoundationTimeState{};
    bHasLastTime = false;
}

ERBFoundationBeginResult URBFoundationSubsystem::BeginTransaction(const FGuid& CorrelationId,
    FName Operation, const FString& PayloadFingerprint, FRBFoundationTransactionRecord& OutRecord,
    FString& OutError)
{
    return BeginTransactionEx(CorrelationId, CorrelationId, Operation, PayloadFingerprint, OutRecord, OutError);
}

ERBFoundationBeginResult URBFoundationSubsystem::BeginTransactionEx(FGuid TransactionId,
    FGuid CorrelationId, FName Operation, const FString& PayloadFingerprint,
    FRBFoundationTransactionRecord& OutRecord, FString& OutError)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBFoundation_BeginTransaction);
    if (!TransactionId.IsValid() || !CorrelationId.IsValid() || Operation.IsNone())
    {
        OutError = TEXT("Transaction requires valid transaction/correlation IDs and an operation.");
        return ERBFoundationBeginResult::Invalid;
    }

    if (const FRBFoundationTransactionRecord* Existing = Transactions.Find(TransactionId))
    {
        OutRecord = *Existing;
        FRBFoundationTraceRecord Trace;
        Trace.CorrelationId = CorrelationId;
        Trace.TransactionId = TransactionId;
        Trace.Stage = TEXT("TransactionReplay");
        Trace.Source = TEXT("Foundation");
        Trace.Operation = Operation;
        Trace.bSuccess = true;
        if (Existing->CorrelationId != CorrelationId || Existing->Operation != Operation
            || Existing->PayloadFingerprint != PayloadFingerprint)
        {
            Trace.Stage = TEXT("TransactionConflict");
            Trace.Code = TEXT("ReplayConflict");
            Trace.Message = TEXT("Transaction ID replay used a different correlation, operation or payload.");
            Trace.bSuccess = false;
            AppendTrace(MoveTemp(Trace));
            OutError = TEXT("Transaction ID replay used a different correlation, operation or payload.");
            return ERBFoundationBeginResult::Conflict;
        }
        ERBFoundationBeginResult Result = ERBFoundationBeginResult::ResumePending;
        if (Existing->Status == ERBFoundationTransactionStatus::Committed)
            Result = ERBFoundationBeginResult::ReplayCommitted;
        else if (Existing->Status == ERBFoundationTransactionStatus::Failed)
            Result = ERBFoundationBeginResult::ReplayFailed;
        Trace.Code = StaticEnum<ERBFoundationBeginResult>()->GetNameStringByValue(static_cast<int64>(Result));
        AppendTrace(MoveTemp(Trace));
        return Result;
    }
    if (Transactions.Num() >= MaxFoundationTransactions)
    {
        OutError = TEXT("Foundation transaction ledger is full; refusing to evict idempotency history.");
        return ERBFoundationBeginResult::Invalid;
    }

    FRBFoundationTransactionRecord Record;
    Record.TransactionId = TransactionId;
    Record.CorrelationId = CorrelationId;
    Record.Operation = Operation;
    Record.PayloadFingerprint = PayloadFingerprint;
    Record.Status = ERBFoundationTransactionStatus::Pending;
    Record.CreatedGameSeconds = bHasLastTime ? LastTime.GameSeconds : 0;
    Transactions.Add(TransactionId, Record);
    OutRecord = Record;

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("TransactionBegin");
    Trace.Source = TEXT("Foundation");
    Trace.Operation = Operation;
    AppendTrace(MoveTemp(Trace));
    return ERBFoundationBeginResult::NewTransaction;
}

bool URBFoundationSubsystem::CommitTransaction(const FGuid& CorrelationId,
    int64 DomainRevision, FString& OutError)
{
    return CommitTransactionById(CorrelationId, DomainRevision, OutError);
}

bool URBFoundationSubsystem::CommitTransactionById(FGuid TransactionId,
    int64 DomainRevision, FString& OutError)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBFoundation_CommitTransaction);
    FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId);
    if (!Record) { OutError = TEXT("Unknown Foundation transaction."); return false; }
    if (Record->Status == ERBFoundationTransactionStatus::Failed)
    {
        OutError = TEXT("A failed transaction cannot be committed under the same transaction ID.");
        return false;
    }
    if (Record->Status == ERBFoundationTransactionStatus::Committed)
    {
        if (Record->DomainRevision == DomainRevision) return true;
        OutError = TEXT("Committed transaction replay used a different domain revision.");
        return false;
    }
    const FRBFoundationTransactionParticipantResult* FailedParticipant = Record->Participants.FindByPredicate(
        [](const FRBFoundationTransactionParticipantResult& Participant) { return !Participant.bSuccess; });
    if (FailedParticipant)
    {
        OutError = FString::Printf(TEXT("Transaction cannot commit because participant '%s' failed."),
            *FailedParticipant->ParticipantId.ToString());
        return false;
    }
    Record->Status = ERBFoundationTransactionStatus::Committed;
    Record->DomainRevision = DomainRevision;
    Record->Error.Reset();

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = Record->CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("TransactionCommitted");
    Trace.Source = TEXT("Foundation");
    Trace.Operation = Record->Operation;
    Trace.Revision = DomainRevision;
    AppendTrace(MoveTemp(Trace));
    return true;
}

bool URBFoundationSubsystem::FailTransaction(const FGuid& CorrelationId,
    const FString& Error, FString& OutError)
{
    return FailTransactionById(CorrelationId, Error, OutError);
}

bool URBFoundationSubsystem::FailTransactionById(FGuid TransactionId,
    const FString& Error, FString& OutError)
{
    FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId);
    if (!Record) { OutError = TEXT("Unknown Foundation transaction."); return false; }
    if (Record->Status == ERBFoundationTransactionStatus::Committed)
    {
        OutError = TEXT("Committed transaction cannot be failed.");
        return false;
    }
    if (Record->Status == ERBFoundationTransactionStatus::Failed)
        return Record->Error == Error;
    Record->Status = ERBFoundationTransactionStatus::Failed;
    Record->Error = Error;

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = Record->CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("TransactionFailed");
    Trace.Source = TEXT("Foundation");
    Trace.Operation = Record->Operation;
    Trace.Code = TEXT("Failed");
    Trace.Message = Error;
    Trace.bSuccess = false;
    AppendTrace(MoveTemp(Trace));
    return true;
}

bool URBFoundationSubsystem::RecordTransactionParticipantResult(FGuid TransactionId,
    FName ParticipantId, bool bSuccess, const FString& Code, const FString& Message,
    double DurationMs, FString& OutError)
{
    if (!TransactionId.IsValid() || ParticipantId.IsNone() || !FMath::IsFinite(DurationMs) || DurationMs < 0.0)
    {
        OutError = TEXT("Participant result requires valid transaction/participant IDs and finite non-negative duration.");
        return false;
    }
    FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId);
    if (!Record) { OutError = TEXT("Unknown Foundation transaction."); return false; }
    if (Record->Status != ERBFoundationTransactionStatus::Pending)
    {
        OutError = TEXT("Participant results can only be added to pending transactions.");
        return false;
    }
    if (const FRBFoundationTransactionParticipantResult* Existing = Record->Participants.FindByPredicate(
        [ParticipantId](const auto& Item) { return Item.ParticipantId == ParticipantId; }))
    {
        if (Existing->bSuccess == bSuccess && Existing->Code == Code && Existing->Message == Message)
            return true;
        OutError = FString::Printf(TEXT("Participant '%s' replayed a conflicting result."), *ParticipantId.ToString());
        return false;
    }
    FRBFoundationTransactionParticipantResult Result;
    Result.ParticipantId = ParticipantId;
    Result.bSuccess = bSuccess;
    Result.Code = Code;
    Result.Message = Message;
    Result.DurationMs = DurationMs;
    Record->Participants.Add(Result);

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = Record->CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("ParticipantResult");
    Trace.Source = ParticipantId;
    Trace.Operation = Record->Operation;
    Trace.Code = Code;
    Trace.Message = Message;
    Trace.bSuccess = bSuccess;
    Trace.DurationMs = DurationMs;
    AppendTrace(MoveTemp(Trace));
    return true;
}

bool URBFoundationSubsystem::RecordTransactionCompensation(FGuid TransactionId,
    FName ParticipantId, bool bSucceeded, const FString& Message, FString& OutError)
{
    FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId);
    if (!Record) { OutError = TEXT("Unknown Foundation transaction."); return false; }
    FRBFoundationTransactionParticipantResult* Participant = Record->Participants.FindByPredicate(
        [ParticipantId](const auto& Item) { return Item.ParticipantId == ParticipantId; });
    if (!Participant) { OutError = TEXT("Cannot compensate an unknown transaction participant."); return false; }
    if (Participant->bSuccess) { OutError = TEXT("Successful participant does not require compensation."); return false; }
    if (Participant->bCompensationAttempted)
    {
        if (Participant->bCompensationSucceeded == bSucceeded && Participant->CompensationMessage == Message)
            return true;
        OutError = TEXT("Compensation replay conflicts with the recorded compensation result.");
        return false;
    }
    Participant->bCompensationAttempted = true;
    Participant->bCompensationSucceeded = bSucceeded;
    Participant->CompensationMessage = Message;

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = Record->CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("ParticipantCompensation");
    Trace.Source = ParticipantId;
    Trace.Operation = Record->Operation;
    Trace.Code = bSucceeded ? TEXT("Compensated") : TEXT("CompensationFailed");
    Trace.Message = Message;
    Trace.bSuccess = bSucceeded;
    AppendTrace(MoveTemp(Trace));
    return true;
}

bool URBFoundationSubsystem::RecordTransactionPersistence(FGuid TransactionId,
    FName DomainId, int64 Revision, FString& OutError)
{
    if (DomainId.IsNone()) { OutError = TEXT("Persistence touch requires a domain ID."); return false; }
    FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId);
    if (!Record) { OutError = TEXT("Unknown Foundation transaction."); return false; }
    if (const FRBFoundationPersistenceTouch* Existing = Record->PersistenceTouches.FindByPredicate(
        [DomainId](const auto& Touch) { return Touch.DomainId == DomainId; }))
    {
        if (Existing->Revision == Revision) return true;
        OutError = FString::Printf(TEXT("Persistence domain '%s' replayed a different revision."), *DomainId.ToString());
        return false;
    }
    FRBFoundationPersistenceTouch Touch;
    Touch.DomainId = DomainId;
    Touch.Revision = Revision;
    Record->PersistenceTouches.Add(Touch);

    FRBFoundationTraceRecord Trace;
    Trace.CorrelationId = Record->CorrelationId;
    Trace.TransactionId = TransactionId;
    Trace.Stage = TEXT("PersistenceTouched");
    Trace.Source = DomainId;
    Trace.Operation = Record->Operation;
    Trace.Revision = Revision;
    AppendTrace(MoveTemp(Trace));
    return true;
}

bool URBFoundationSubsystem::GetTransaction(const FGuid& CorrelationId,
    FRBFoundationTransactionRecord& OutRecord) const
{
    if (const FRBFoundationTransactionRecord* Record = Transactions.Find(CorrelationId))
    {
        OutRecord = *Record;
        return true;
    }
    const TArray<FRBFoundationTransactionRecord> Matches = GetTransactionsForCorrelation(CorrelationId);
    if (Matches.Num() == 1) { OutRecord = Matches[0]; return true; }
    return false;
}

bool URBFoundationSubsystem::GetTransactionById(FGuid TransactionId,
    FRBFoundationTransactionRecord& OutRecord) const
{
    if (const FRBFoundationTransactionRecord* Record = Transactions.Find(TransactionId))
    {
        OutRecord = *Record;
        return true;
    }
    return false;
}

TArray<FRBFoundationTransactionRecord> URBFoundationSubsystem::GetTransactionsForCorrelation(
    FGuid CorrelationId) const
{
    TArray<FRBFoundationTransactionRecord> Result;
    for (const auto& Pair : Transactions)
        if (Pair.Value.CorrelationId == CorrelationId) Result.Add(Pair.Value);
    Result.Sort([](const auto& A, const auto& B)
    {
        return A.TransactionId.ToString(EGuidFormats::Digits) < B.TransactionId.ToString(EGuidFormats::Digits);
    });
    return Result;
}

bool URBFoundationSubsystem::RegisterEventConsumer(FName ConsumerId, FName ProductId,
    UObject* Consumer, const TArray<FName>& EventTypes, FString& OutError)
{
    CompactInvalidRegistrations();
    if (ConsumerId.IsNone() || ProductId.IsNone() || !IsValid(Consumer)
        || !Consumer->GetClass()->ImplementsInterface(URBFoundationEventConsumer::StaticClass()))
    {
        OutError = TEXT("Event consumer requires consumer/product IDs and RBFoundationEventConsumer implementation.");
        return false;
    }
    const FProductAdapterEntry* Product = ProductAdapters.Find(ProductId);
    if (!Product || !Product->Adapter.IsValid())
    {
        OutError = FString::Printf(TEXT("Event consumer product '%s' is not registered."), *ProductId.ToString());
        return false;
    }
    if (EventTypes.IsEmpty()) { OutError = TEXT("Event consumer requires at least one event type."); return false; }
    if (const FEventConsumerEntry* Existing = EventConsumers.Find(ConsumerId))
    {
        if (Existing->Consumer.Get() == Consumer) return true;
        OutError = FString::Printf(TEXT("Event consumer ID '%s' is already registered."), *ConsumerId.ToString());
        return false;
    }
    TSet<FName> Unique;
    for (FName EventType : EventTypes)
    {
        if (EventType.IsNone()) { OutError = TEXT("Event consumer event type cannot be None."); return false; }
        if (!Product->Descriptor.ConsumesEvents.Contains(EventType))
        {
            OutError = FString::Printf(TEXT("Product '%s' did not declare consumed event '%s'."),
                *ProductId.ToString(), *EventType.ToString());
            return false;
        }
        Unique.Add(EventType);
    }
    FEventConsumerEntry Entry;
    Entry.ProductId = ProductId;
    Entry.Consumer = Consumer;
    Entry.EventTypes = MoveTemp(Unique);
    EventConsumers.Add(ConsumerId, MoveTemp(Entry));
    return true;
}

void URBFoundationSubsystem::UnregisterEventConsumer(FName ConsumerId, UObject* Consumer)
{
    if (FEventConsumerEntry* Existing = EventConsumers.Find(ConsumerId))
        if (Existing->Consumer.Get() == Consumer) EventConsumers.Remove(ConsumerId);
}

bool URBFoundationSubsystem::PublishDomainEvent(FGuid CorrelationId, FName SourceDomain,
    FName EventType, const FString& Payload, int64 Revision, FString& OutError)
{
    return PublishDomainEventWithTransaction(CorrelationId, FGuid(), SourceDomain,
        EventType, Payload, Revision, OutError);
}

bool URBFoundationSubsystem::PublishDomainEventWithTransaction(FGuid CorrelationId,
    FGuid TransactionId, FName SourceDomain, FName EventType, const FString& Payload,
    int64 Revision, FString& OutError)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBFoundation_PublishDomainEvent);
    CompactInvalidRegistrations();
    if (!CorrelationId.IsValid() || SourceDomain.IsNone() || EventType.IsNone())
    {
        OutError = TEXT("Domain event requires correlation ID, source domain and event type.");
        return false;
    }
    const FAuthorityEntry* Authority = Authorities.Find(SourceDomain);
    if (!Authority || !Authority->Owner.IsValid())
    {
        OutError = FString::Printf(TEXT("No registered authority for source domain '%s'."),
            *SourceDomain.ToString());
        return false;
    }
    if (TransactionId.IsValid())
    {
        const FRBFoundationTransactionRecord* Transaction = Transactions.Find(TransactionId);
        if (!Transaction) { OutError = TEXT("Domain event references unknown transaction ID."); return false; }
        if (Transaction->CorrelationId != CorrelationId)
        {
            OutError = TEXT("Domain event transaction/correlation IDs do not match.");
            return false;
        }
    }
    const FName SourceProduct = FindProductIdForOwner(Authority->Owner.Get());
    if (!SourceProduct.IsNone())
    {
        const FProductAdapterEntry* Product = ProductAdapters.Find(SourceProduct);
        if (Product && !Product->Descriptor.EmitsEvents.Contains(EventType))
        {
            OutError = FString::Printf(TEXT("Product '%s' did not declare emitted event '%s'."),
                *SourceProduct.ToString(), *EventType.ToString());
            return false;
        }
    }

    FRBFoundationDomainEvent Event;
    Event.EventId = FGuid::NewGuid();
    Event.CorrelationId = CorrelationId;
    Event.TransactionId = TransactionId;
    Event.SourceDomain = SourceDomain;
    Event.EventType = EventType;
    Event.Payload = Payload;
    Event.Revision = Revision;

    FRBFoundationTraceRecord Emitted;
    Emitted.CorrelationId = CorrelationId;
    Emitted.TransactionId = TransactionId;
    Emitted.EventId = Event.EventId;
    Emitted.Stage = TEXT("EventEmitted");
    Emitted.Source = SourceProduct.IsNone() ? SourceDomain : SourceProduct;
    Emitted.Operation = EventType;
    Emitted.Revision = Revision;
    AppendTrace(MoveTemp(Emitted));

    OnDomainEvent.Broadcast(Event);

    TArray<FName> ConsumerIds;
    EventConsumers.GetKeys(ConsumerIds);
    ConsumerIds.Sort(FNameLexicalLess());
    TArray<FString> DispatchErrors;
    for (FName ConsumerId : ConsumerIds)
    {
        FEventConsumerEntry* ConsumerEntry = EventConsumers.Find(ConsumerId);
        if (!ConsumerEntry || !ConsumerEntry->Consumer.IsValid()
            || !ConsumerEntry->EventTypes.Contains(EventType)) continue;
        TRACE_CPUPROFILER_EVENT_SCOPE(RBFoundation_ConsumeDomainEvent);
        const double Start = FPlatformTime::Seconds();
        FRBFoundationEventConsumerResult Result;
        const bool bHandled = IRBFoundationEventConsumer::Execute_ConsumeFoundationEvent(
            ConsumerEntry->Consumer.Get(), Event, Result);
        const double DurationMs = (FPlatformTime::Seconds() - Start) * 1000.0;

        bool bTraceLinkValid = true;
        if (Result.TransactionId.IsValid())
        {
            const FRBFoundationTransactionRecord* Linked = Transactions.Find(Result.TransactionId);
            bTraceLinkValid = Linked && Linked->CorrelationId == CorrelationId;
        }

        FRBFoundationTraceRecord Consumed;
        Consumed.CorrelationId = CorrelationId;
        Consumed.TransactionId = bTraceLinkValid && Result.TransactionId.IsValid()
            ? Result.TransactionId : TransactionId;
        Consumed.EventId = Event.EventId;
        Consumed.Stage = (!bHandled || !bTraceLinkValid) ? TEXT("EventConsumerError")
            : (Result.bAccepted ? TEXT("EventConsumed") : TEXT("EventRejected"));
        Consumed.Source = EventType;
        Consumed.Target = ConsumerId;
        Consumed.Operation = EventType;
        Consumed.Code = bTraceLinkValid ? Result.Code : TEXT("InvalidTransactionLink");
        Consumed.Message = bTraceLinkValid ? Result.Message
            : TEXT("Consumer returned an unknown or different-correlation transaction ID.");
        Consumed.bSuccess = bHandled && bTraceLinkValid && Result.bAccepted;
        Consumed.DurationMs = DurationMs;
        Consumed.Revision = Revision;
        AppendTrace(MoveTemp(Consumed));
        if (!bHandled || !bTraceLinkValid)
        {
            DispatchErrors.Add(FString::Printf(TEXT("Consumer '%s' failed event routing contract: %s"),
                *ConsumerId.ToString(), bTraceLinkValid ? TEXT("handler failure") : TEXT("invalid transaction link")));
        }
    }
    if (!DispatchErrors.IsEmpty())
    {
        OutError = FString::Join(DispatchErrors, TEXT(" "));
        return false;
    }
    return true;
}

TArray<FRBFoundationTraceRecord> URBFoundationSubsystem::GetTraceForCorrelation(FGuid CorrelationId) const
{
    TArray<FRBFoundationTraceRecord> Result;
    for (const FRBFoundationTraceRecord& Record : TraceRecords)
        if (Record.CorrelationId == CorrelationId) Result.Add(Record);
    return Result;
}

TArray<FRBFoundationTraceRecord> URBFoundationSubsystem::GetRecentTrace(int32 MaxRecords) const
{
    TArray<FRBFoundationTraceRecord> Result;
    if (MaxRecords <= 0 || TraceRecords.IsEmpty()) return Result;
    const int32 Count = FMath::Min(MaxRecords, TraceRecords.Num());
    Result.Append(TraceRecords.GetData() + TraceRecords.Num() - Count, Count);
    return Result;
}

void URBFoundationSubsystem::ClearTrace()
{
    TraceRecords.Reset();
    NextTraceSequence = 1;
}

FRBFoundationHealthReport URBFoundationSubsystem::EvaluateHealth() const
{
    FRBFoundationHealthReport Report;
    for (const auto& Pair : ProductAdapters) if (Pair.Value.Adapter.IsValid()) ++Report.ProductCount;
    for (const auto& Pair : Authorities) if (Pair.Value.Owner.IsValid()) ++Report.AuthorityCount;
    for (auto It = Capabilities.CreateConstIterator(); It; ++It)
        if (It.Value().Owner.IsValid()) ++Report.CapabilityCount;
    Report.TransactionCount = Transactions.Num();
    for (const auto& Pair : EventConsumers) if (Pair.Value.Consumer.IsValid()) ++Report.EventConsumerCount;
    Report.TraceCount = TraceRecords.Num();
    Report.bHasCanonicalTimeSource = CanonicalTimeSource.IsValid()
        && CanonicalTimeSource->GetClass()->ImplementsInterface(URBFoundationTimeSource::StaticClass());

    auto AddIssue = [&Report](FName Code, const FString& Message, FName OwnerDomain)
    {
        FRBFoundationHealthIssue Issue;
        Issue.Code = Code;
        Issue.Message = Message;
        Issue.OwnerDomain = OwnerDomain;
        Issue.bBlocking = true;
        Report.Issues.Add(MoveTemp(Issue));
    };

    if (!Report.bHasCanonicalTimeSource)
        AddIssue(TEXT("MissingCanonicalTime"), TEXT("Exactly one canonical time source is required."), TEXT("CanonicalTime"));

    if (!ManifestRegistrationError.IsEmpty())
        AddIssue(TEXT("ManifestRegistrationFailed"), ManifestRegistrationError, TEXT("FoundationManifest"));

    for (FName Required : RequiredCapabilities)
    {
        if (!HasCapability(Required))
            AddIssue(TEXT("MissingCapability"), FString::Printf(TEXT("Required capability '%s' is missing."),
                *Required.ToString()), Required);
    }

    for (const auto& Pair : ProductAdapters)
    {
        UObject* Adapter = Pair.Value.Adapter.Get();
        if (!IsValid(Adapter))
        {
            AddIssue(TEXT("InvalidAdapter"), FString::Printf(TEXT("Product adapter '%s' is no longer valid."),
                *Pair.Key.ToString()), Pair.Key);
            continue;
        }
        TArray<FRBFoundationHealthIssue> AdapterIssues;
        IRBFoundationProductAdapter::Execute_ProbeFoundationAdapterHealth(Adapter, AdapterIssues);
        for (FRBFoundationHealthIssue& Issue : AdapterIssues)
        {
            if (Issue.OwnerDomain.IsNone()) Issue.OwnerDomain = Pair.Key;
            Report.Issues.Add(MoveTemp(Issue));
        }
    }

    int32 Pending = 0;
    int32 Failed = 0;
    for (const auto& Pair : Transactions)
    {
        if (Pair.Value.Status == ERBFoundationTransactionStatus::Pending) ++Pending;
        else if (Pair.Value.Status == ERBFoundationTransactionStatus::Failed) ++Failed;
    }
    if (Pending > 0)
        AddIssue(TEXT("PendingTransactions"), FString::Printf(TEXT("%d transaction(s) are unresolved."), Pending), TEXT("Transactions"));
    if (Failed > 0)
        AddIssue(TEXT("FailedTransactions"), FString::Printf(TEXT("%d transaction(s) failed and require an explicit new correlation ID or recovery decision."), Failed), TEXT("Transactions"));

    Report.bReady = !Report.Issues.ContainsByPredicate([](const FRBFoundationHealthIssue& Issue)
    {
        return Issue.bBlocking;
    });
    return Report;
}

bool URBFoundationSubsystem::VerifyPluginVersion(const FString& PluginName,
    const FString& ExpectedVersion, FString& OutError) const
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
    if (!Plugin.IsValid())
    {
        OutError = FString::Printf(TEXT("Required plugin '%s' is not installed."), *PluginName);
        return false;
    }
    const FString Actual = Plugin->GetDescriptor().VersionName;
    if (Actual != ExpectedVersion)
    {
        OutError = FString::Printf(TEXT("Plugin '%s' version mismatch: expected '%s', found '%s'."),
            *PluginName, *ExpectedVersion, *Actual);
        return false;
    }
    return true;
}

FRBFoundationSnapshot URBFoundationSubsystem::MakeSnapshot() const
{
    FRBFoundationSnapshot Snapshot;
    Snapshot.SchemaVersion = 2;
    Transactions.GenerateValueArray(Snapshot.Transactions);
    Snapshot.Transactions.Sort([](const auto& A, const auto& B)
    {
        return A.TransactionId.ToString(EGuidFormats::Digits)
            < B.TransactionId.ToString(EGuidFormats::Digits);
    });
    Snapshot.LastTime = LastTime;
    Snapshot.bHasLastTime = bHasLastTime;
    return Snapshot;
}

bool URBFoundationSubsystem::RestoreSnapshot(const FRBFoundationSnapshot& Snapshot,
    FString& OutError)
{
    if (Snapshot.SchemaVersion != 1 && Snapshot.SchemaVersion != 2)
    {
        OutError = TEXT("Unsupported RB Foundation snapshot schema.");
        return false;
    }
    if (Snapshot.Transactions.Num() > MaxFoundationTransactions)
    {
        OutError = TEXT("Foundation snapshot exceeds transaction ledger bound.");
        return false;
    }
    if (Snapshot.bHasLastTime && !Snapshot.LastTime.IsValid(&OutError)) return false;

    TMap<FGuid, FRBFoundationTransactionRecord> Candidate;
    for (FRBFoundationTransactionRecord Record : Snapshot.Transactions)
    {
        if (Snapshot.SchemaVersion == 1 && !Record.TransactionId.IsValid())
            Record.TransactionId = Record.CorrelationId;
        if (!Record.TransactionId.IsValid() || !Record.CorrelationId.IsValid() || Record.Operation.IsNone())
        {
            OutError = TEXT("Foundation snapshot contains an invalid transaction record.");
            return false;
        }
        if (Candidate.Contains(Record.TransactionId))
        {
            OutError = TEXT("Foundation snapshot contains duplicate transaction IDs.");
            return false;
        }
        const int64 RawStatus = static_cast<int64>(Record.Status);
        if (!StaticEnum<ERBFoundationTransactionStatus>()->IsValidEnumValue(RawStatus))
        {
            OutError = TEXT("Foundation snapshot contains an invalid transaction status.");
            return false;
        }

        TSet<FName> ParticipantIds;
        for (const FRBFoundationTransactionParticipantResult& Participant : Record.Participants)
        {
            if (Participant.ParticipantId.IsNone() || ParticipantIds.Contains(Participant.ParticipantId)
                || !FMath::IsFinite(Participant.DurationMs) || Participant.DurationMs < 0.0
                || (Participant.bCompensationSucceeded && !Participant.bCompensationAttempted))
            {
                OutError = TEXT("Foundation snapshot contains an invalid participant result.");
                return false;
            }
            ParticipantIds.Add(Participant.ParticipantId);
        }
        TSet<FName> PersistenceDomains;
        for (const FRBFoundationPersistenceTouch& Touch : Record.PersistenceTouches)
        {
            if (Touch.DomainId.IsNone() || PersistenceDomains.Contains(Touch.DomainId))
            {
                OutError = TEXT("Foundation snapshot contains an invalid/duplicate persistence touch.");
                return false;
            }
            PersistenceDomains.Add(Touch.DomainId);
        }
        Candidate.Add(Record.TransactionId, MoveTemp(Record));
    }

    Transactions = MoveTemp(Candidate);
    LastTime = Snapshot.LastTime;
    bHasLastTime = Snapshot.bHasLastTime;
    return true;
}
