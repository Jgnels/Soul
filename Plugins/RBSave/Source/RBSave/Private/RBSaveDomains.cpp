#include "RBSaveSubsystem.h"
#include "RBSaveCore.h"

#include "RBSaveDomainProvider.h"
#include "Async/Async.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

namespace {
using rb::save::Bytes;
using rb::save::DomainRecord;
using rb::save::Snapshot;
using rb::save::Value;

bool ValidateDomainSelection(const TArray<FName>* Selection,
    const TArray<FName>& Registered, FString& Error)
{
    if (!Selection) return true; // Existing all-domain contract.
    if (Selection->IsEmpty())
    {
        Error = TEXT("Selected domain list is empty.");
        return false;
    }
    TSet<FName> Seen;
    for (FName Id : *Selection)
    {
        if (Id.IsNone() || Seen.Contains(Id))
        {
            Error = TEXT("Selected domain list contains an empty or duplicate id.");
            return false;
        }
        if (!Registered.Contains(Id))
        {
            Error = FString::Printf(TEXT("Selected domain '%s' has no registered provider."), *Id.ToString());
            return false;
        }
        Seen.Add(Id);
    }
    return true;
}


std::string RBToUtf8(const FString& Text)
{
    FTCHARToUTF8 Converted(*Text);
    return std::string(Converted.Get(), Converted.Length());
}

FString RBFromUtf8(const std::string& Text)
{
    return FString(UTF8_TO_TCHAR(Text.c_str()));
}

FString RBSanitizeSlot(FString Slot)
{
    Slot.TrimStartAndEndInline();
    Slot.ReplaceInline(TEXT("/"), TEXT("_"));
    Slot.ReplaceInline(TEXT("\\"), TEXT("_"));
    Slot.ReplaceInline(TEXT(":"), TEXT("_"));    Slot.ReplaceInline(TEXT(".."), TEXT("_"));
    return Slot;
}

bool ToCoreValue(const FRBSaveField& Field, Value& Out, FString& Error)
{
    if (Field.Name.IsNone())
    {
        Error = TEXT("domain field name cannot be None");
        return false;
    }
    switch (Field.Type)
    {
    case ERBSaveFieldType::Integer:
        Out = static_cast<std::int64_t>(Field.IntegerValue);
        return true;
    case ERBSaveFieldType::Number:
        Out = Field.NumberValue;
        return true;
    case ERBSaveFieldType::Boolean:
        Out = Field.BooleanValue;
        return true;
    case ERBSaveFieldType::String:
        Out = RBToUtf8(Field.StringValue);
        return true;
    case ERBSaveFieldType::Bytes:
        Out = Bytes(Field.BytesValue.GetData(), Field.BytesValue.GetData() + Field.BytesValue.Num());
        return true;
    default:
        Error = TEXT("unsupported domain field type");        return false;
    }
}

FRBSaveField FromCoreField(const std::string& Name, const Value& Core)
{
    FRBSaveField Field;
    Field.Name = FName(RBFromUtf8(Name));
    if (const auto* IntValue = std::get_if<std::int64_t>(&Core))
    {
        Field.Type = ERBSaveFieldType::Integer;
        Field.IntegerValue = *IntValue;
    }
    else if (const auto* UIntValue = std::get_if<std::uint64_t>(&Core))
    {
        Field.Type = ERBSaveFieldType::Integer;
        Field.IntegerValue = static_cast<int64>(*UIntValue);
    }
    else if (const auto* NumberValue = std::get_if<double>(&Core))
    {
        Field.Type = ERBSaveFieldType::Number;
        Field.NumberValue = *NumberValue;
    }
    else if (const auto* BoolValue = std::get_if<bool>(&Core))
    {
        Field.Type = ERBSaveFieldType::Boolean;
        Field.BooleanValue = *BoolValue;
    }
    else if (const auto* StringValue = std::get_if<std::string>(&Core))
    {
        Field.Type = ERBSaveFieldType::String;
        Field.StringValue = RBFromUtf8(*StringValue);    }
    else if (const auto* BytesValue = std::get_if<Bytes>(&Core))
    {
        Field.Type = ERBSaveFieldType::Bytes;
        if (!BytesValue->empty()) Field.BytesValue.Append(BytesValue->data(), static_cast<int32>(BytesValue->size()));
    }
    return Field;
}

FRBSaveOperationResult DomainResult(bool bSuccess, FString Message, int64 Sequence = 0,
                                    int64 BytesWritten = 0, int64 Checksum = 0)
{
    FRBSaveOperationResult Result;
    Result.bSuccess = bSuccess;
    Result.Message = MoveTemp(Message);
    Result.Sequence = Sequence;
    Result.Bytes = BytesWritten;
    Result.Checksum = Checksum;
    return Result;
}

const DomainRecord* FindDomain(const Snapshot& SnapshotData, const FString& DomainId)
{
    const std::string Needle = RBToUtf8(DomainId);
    for (const DomainRecord& Domain : SnapshotData.domains)
        if (Domain.id == Needle) return &Domain;
    return nullptr;
}
} // namespace

bool URBSaveSubsystem::RegisterDomainProvider(UObject* Provider, FString& Error)
{
    Error.Reset();    if (!IsValid(Provider) || !Provider->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass()))
    {
        Error = TEXT("provider must implement RBSaveDomainProvider");
        return false;
    }
    const FName DomainId = IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Provider);
    const int32 Schema = IRBSaveDomainProvider::Execute_GetRBSaveSchemaVersion(Provider);
    if (DomainId.IsNone() || Schema <= 0)
    {
        Error = TEXT("provider returned invalid domain id or schema version");
        return false;
    }
    for (const TWeakObjectPtr<UObject>& ExistingWeak : DomainProviders)
    {
        UObject* Existing = ExistingWeak.Get();
        if (!Existing) continue;
        if (Existing == Provider) return true;
        if (Existing->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass()) &&
            IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Existing) == DomainId)
        {
            Error = FString::Printf(TEXT("domain '%s' is already registered"), *DomainId.ToString());
            return false;
        }
    }
    DomainProviders.RemoveAll([](const TWeakObjectPtr<UObject>& P) { return !P.IsValid(); });
    DomainProviders.Add(Provider);
    return true;
}

void URBSaveSubsystem::UnregisterDomainProvider(UObject* Provider)
{
    DomainProviders.RemoveAll([Provider](const TWeakObjectPtr<UObject>& P) { return !P.IsValid() || P.Get() == Provider; });
}
TArray<FName> URBSaveSubsystem::GetRegisteredDomainIds() const
{
    TArray<FName> Result;
    for (const TWeakObjectPtr<UObject>& Weak : DomainProviders)
    {
        UObject* Provider = Weak.Get();
        if (!Provider || !Provider->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass())) continue;
        const FName Id = IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Provider);
        if (!Id.IsNone()) Result.AddUnique(Id);
    }
    Result.Sort(FNameLexicalLess());
    return Result;
}

FString URBSaveSubsystem::GetDomainSlotPath(const FString& Slot) const
{
    const FString Safe = RBSanitizeSlot(Slot);
    if (Safe.IsEmpty()) return FString();
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RBSave"), TEXT("Domains"), Safe + TEXT(".domain.rbsave"));
}

bool URBSaveSubsystem::CaptureRegisteredDomains(const FString& Slot, Snapshot& OutSnapshot,
                                                FString& OutError, const TArray<FName>* SelectedDomains) const
{
    OutError.Reset();
    OutSnapshot = Snapshot{};
    if (!ValidateDomainSelection(SelectedDomains, GetRegisteredDomainIds(), OutError)) return false;
    const FString SafeSlot = RBSanitizeSlot(Slot);
    if (SafeSlot.IsEmpty())
    {
        OutError = TEXT("invalid slot");
        return false;
    }
    OutSnapshot.sequence = static_cast<std::uint64_t>(FDateTime::UtcNow().GetTicks());
    OutSnapshot.createdUtcMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000LL;
    OutSnapshot.slot = RBToUtf8(SafeSlot);
    struct FProviderRow { FString Id; UObject* Object = nullptr; int32 Schema = 0; };
    TArray<FProviderRow> Providers;
    for (const TWeakObjectPtr<UObject>& Weak : DomainProviders)
    {
        UObject* Provider = Weak.Get();
        if (!Provider || !Provider->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass())) continue;
        const FName Id = IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Provider);
        if (SelectedDomains && !SelectedDomains->Contains(Id)) continue;
        const int32 Schema = IRBSaveDomainProvider::Execute_GetRBSaveSchemaVersion(Provider);
        if (Id.IsNone() || Schema <= 0)
        {
            OutError = TEXT("registered provider became invalid");
            return false;
        }
        Providers.Add({Id.ToString(), Provider, Schema});
    }
    Providers.Sort([](const FProviderRow& A, const FProviderRow& B) { return A.Id < B.Id; });

    TSet<FString> DomainIds;
    for (const FProviderRow& Row : Providers)
    {
        FRBSaveDomainState State;
        FString ProviderError;
        if (!IRBSaveDomainProvider::Execute_CaptureRBSaveDomain(Row.Object, State, ProviderError))
        {
            OutError = FString::Printf(TEXT("capture failed for '%s': %s"), *Row.Id, *ProviderError);
            return false;
        }
        if (State.DomainId.IsNone()) State.DomainId = FName(Row.Id);
        if (State.SchemaVersion <= 0) State.SchemaVersion = Row.Schema;
        if (State.DomainId.ToString() != Row.Id || State.SchemaVersion != Row.Schema)
        {
            OutError = FString::Printf(TEXT("provider '%s' returned mismatched id/schema"), *Row.Id);
            return false;
        }
        if (DomainIds.Contains(Row.Id))
        {
            OutError = FString::Printf(TEXT("duplicate domain '%s'"), *Row.Id);
            return false;
        }
        DomainIds.Add(Row.Id);
        DomainRecord Core;
        Core.id = RBToUtf8(Row.Id);
        Core.schemaVersion = static_cast<std::uint32_t>(Row.Schema);
        TSet<FName> FieldNames;
        for (const FRBSaveField& Field : State.Fields)
        {
            if (Field.Name.IsNone() || FieldNames.Contains(Field.Name))
            {
                OutError = FString::Printf(TEXT("domain '%s' contains missing/duplicate field name"), *Row.Id);
                return false;
            }
            FieldNames.Add(Field.Name);
            Value CoreValue;
            if (!ToCoreValue(Field, CoreValue, OutError))
            {
                OutError = FString::Printf(TEXT("domain '%s': %s"), *Row.Id, *OutError);
                return false;
            }
            Core.fields.emplace(RBToUtf8(Field.Name.ToString()), MoveTemp(CoreValue));
        }
        OutSnapshot.domains.push_back(MoveTemp(Core));
    }
    return true;
}

bool URBSaveSubsystem::RestoreRegisteredDomains(const Snapshot& SnapshotData, FString& OutError,
    const TArray<FName>* SelectedDomains)
{
    OutError.Reset();
    if (!ValidateDomainSelection(SelectedDomains, GetRegisteredDomainIds(), OutError)) return false;
    // Reject a missing selected domain before restoring any provider.
    if (SelectedDomains)
    {
        for (FName Id : *SelectedDomains)
        {
            if (!FindDomain(SnapshotData, Id.ToString()))
            {
                OutError = FString::Printf(TEXT("Selected domain '%s' is missing from snapshot."), *Id.ToString());
                return false;
            }
        }
    }

    TSet<FString> RegisteredIds;
    for (const TWeakObjectPtr<UObject>& Weak : DomainProviders)
    {
        UObject* Provider = Weak.Get();
        if (!Provider || !Provider->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass())) continue;
        const FName DomainId = IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Provider);
        if (!DomainId.IsNone()) RegisteredIds.Add(DomainId.ToString());
    }
    for (const DomainRecord& Domain : SnapshotData.domains)
    {
        const FString DomainId = RBFromUtf8(Domain.id);
        if (SelectedDomains && !SelectedDomains->Contains(FName(*DomainId))) continue;
        if (!RegisteredIds.Contains(DomainId))
        {
            OutError = FString::Printf(TEXT("saved domain '%s' has no registered provider"), *DomainId);
            return false;
        }
    }
    for (const TWeakObjectPtr<UObject>& Weak : DomainProviders)
    {
        UObject* Provider = Weak.Get();
        if (!Provider || !Provider->GetClass()->ImplementsInterface(URBSaveDomainProvider::StaticClass())) continue;
        const FName DomainId = IRBSaveDomainProvider::Execute_GetRBSaveDomainId(Provider);
        if (SelectedDomains && !SelectedDomains->Contains(DomainId)) continue;
        const int32 ExpectedSchema = IRBSaveDomainProvider::Execute_GetRBSaveSchemaVersion(Provider);
        const DomainRecord* Core = FindDomain(SnapshotData, DomainId.ToString());
        if (!Core)
        {
            OutError = FString::Printf(TEXT("registered domain '%s' is missing from snapshot"), *DomainId.ToString());
            return false;
        }
        if (ExpectedSchema <= 0 || Core->schemaVersion != static_cast<std::uint32_t>(ExpectedSchema))
        {
            OutError = FString::Printf(TEXT("domain '%s' schema %u cannot restore into provider schema %d; migrate first"),
                *DomainId.ToString(), Core->schemaVersion, ExpectedSchema);
            return false;
        }
        FRBSaveDomainState State;
        State.DomainId = DomainId;
        State.SchemaVersion = ExpectedSchema;
        for (const auto& [Name, CoreValue] : Core->fields)
            State.Fields.Add(FromCoreField(Name, CoreValue));
        FString ProviderError;
        if (!IRBSaveDomainProvider::Execute_RestoreRBSaveDomain(Provider, State, ProviderError))
        {
            OutError = FString::Printf(TEXT("restore failed for '%s': %s"), *DomainId.ToString(), *ProviderError);
            return false;
        }
    }
    return true;
}

void URBSaveSubsystem::SaveDomainsAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion)
{
    const FString Path = GetDomainSlotPath(Slot);
    if (Path.IsEmpty())
    {
        Completion.ExecuteIfBound(DomainResult(false, TEXT("invalid slot")));
        return;
    }
    Snapshot Captured;
    FString Error;
    if (!CaptureRegisteredDomains(Slot, Captured, Error))
    {
        Completion.ExecuteIfBound(DomainResult(false, Error));
        return;
    }
    const int64 Sequence = static_cast<int64>(Captured.sequence);
    FRBSaveOperationDelegate Callback = Completion;
    Async(EAsyncExecution::ThreadPool, [Path, Captured = MoveTemp(Captured), Sequence, Callback]() mutable
    {
        const auto Io = rb::save::WriteAtomic(std::filesystem::path(*Path), Captured);
        FRBSaveOperationResult Result = DomainResult(Io.ok,
            Io.ok ? TEXT("domain save complete") : RBFromUtf8(Io.error), Sequence,
            static_cast<int64>(Io.bytesWritten), static_cast<int64>(Io.checksum));
        AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable { Callback.ExecuteIfBound(Result); });
    });
}

void URBSaveSubsystem::LoadDomainsAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion)
{
    const FString Path = GetDomainSlotPath(Slot);
    if (Path.IsEmpty())
    {
        Completion.ExecuteIfBound(DomainResult(false, TEXT("invalid slot")));
        return;
    }
    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    FRBSaveOperationDelegate Callback = Completion;
    Async(EAsyncExecution::ThreadPool, [WeakThis, Path, Callback]() mutable
    {
        auto Loaded = rb::save::ReadFile(std::filesystem::path(*Path));
        if (!Loaded.ok)
        {
            FRBSaveOperationResult Result = DomainResult(false, RBFromUtf8(Loaded.error));
            AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable { Callback.ExecuteIfBound(Result); });
            return;
        }
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, Loaded = MoveTemp(Loaded)]() mutable
        {
            if (!WeakThis.IsValid())
            {
                Callback.ExecuteIfBound(DomainResult(false, TEXT("save subsystem destroyed")));
                return;
            }
            FString Error;
            const bool bOk = WeakThis->RestoreRegisteredDomains(Loaded.snapshot, Error);
            Callback.ExecuteIfBound(DomainResult(bOk, bOk ? TEXT("domain load complete") : Error,
                static_cast<int64>(Loaded.snapshot.sequence)));
        });
    });
}

void URBSaveSubsystem::SaveSelectedDomainsAsync(const FString& Slot, const TArray<FName>& DomainIds, const FRBSaveOperationDelegate& Completion)
{
    const FString Path = GetDomainSlotPath(Slot);
    if (Path.IsEmpty())
    {
        Completion.ExecuteIfBound(DomainResult(false, TEXT("invalid slot")));
        return;
    }
    Snapshot Captured;
    FString Error;
    if (!CaptureRegisteredDomains(Slot, Captured, Error, &DomainIds))
    {
        Completion.ExecuteIfBound(DomainResult(false, Error));
        return;
    }
    const int64 Sequence = static_cast<int64>(Captured.sequence);
    FRBSaveOperationDelegate Callback = Completion;
    Async(EAsyncExecution::ThreadPool, [Path, Captured = MoveTemp(Captured), Sequence, Callback]() mutable
    {
        const auto Io = rb::save::WriteAtomic(std::filesystem::path(*Path), Captured);
        FRBSaveOperationResult Result = DomainResult(Io.ok,
            Io.ok ? TEXT("domain save complete") : RBFromUtf8(Io.error), Sequence,
            static_cast<int64>(Io.bytesWritten), static_cast<int64>(Io.checksum));
        AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable { Callback.ExecuteIfBound(Result); });
    });
}

void URBSaveSubsystem::LoadSelectedDomainsAsync(const FString& Slot, const TArray<FName>& DomainIds, const FRBSaveOperationDelegate& Completion)
{
    FString SelectionError;
    if (!ValidateDomainSelection(&DomainIds, GetRegisteredDomainIds(), SelectionError))
    {
        Completion.ExecuteIfBound(DomainResult(false, SelectionError));
        return;
    }
    const FString Path = GetDomainSlotPath(Slot);
    if (Path.IsEmpty())
    {
        Completion.ExecuteIfBound(DomainResult(false, TEXT("invalid slot")));
        return;
    }
    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    FRBSaveOperationDelegate Callback = Completion;
    Async(EAsyncExecution::ThreadPool, [WeakThis, Path, Callback, DomainIds]() mutable
    {
        auto Loaded = rb::save::ReadFile(std::filesystem::path(*Path));
        if (!Loaded.ok)
        {
            FRBSaveOperationResult Result = DomainResult(false, RBFromUtf8(Loaded.error));
            AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable { Callback.ExecuteIfBound(Result); });
            return;
        }
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, DomainIds, Loaded = MoveTemp(Loaded)]() mutable
        {
            if (!WeakThis.IsValid())
            {
                Callback.ExecuteIfBound(DomainResult(false, TEXT("save subsystem destroyed")));
                return;
            }
            FString Error;
            const bool bOk = WeakThis->RestoreRegisteredDomains(Loaded.snapshot, Error, &DomainIds);
            Callback.ExecuteIfBound(DomainResult(bOk, bOk ? TEXT("domain load complete") : Error,
                static_cast<int64>(Loaded.snapshot.sequence)));
        });
    });
}
