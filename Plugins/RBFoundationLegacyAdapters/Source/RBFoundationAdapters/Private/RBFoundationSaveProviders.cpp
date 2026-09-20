#include "RBFoundationSaveProviders.h"
#include "RBFoundationRBAdapterSubsystem.h"

#include "JsonObjectConverter.h"
#include "RBFoundationSubsystem.h"
#include "RBItemEconomySubsystem.h"
#include "RBRoutineWorldSubsystem.h"
#include "RBWeatherDirector.h"
#include "RBOptimizationGroup.h"

namespace
{
constexpr const TCHAR* PayloadField = TEXT("Payload");

bool SetBytesDomain(FName DomainId, int32 SchemaVersion, const TArray<uint8>& Bytes,
    FRBSaveDomainState& OutState)
{
    OutState = FRBSaveDomainState{};
    OutState.DomainId = DomainId;
    OutState.SchemaVersion = SchemaVersion;
    FRBSaveField Field;
    Field.Name = PayloadField;
    Field.Type = ERBSaveFieldType::Bytes;
    Field.BytesValue = Bytes;
    OutState.Fields.Add(MoveTemp(Field));
    return true;
}

bool GetBytesDomain(const FRBSaveDomainState& State, FName ExpectedDomain,
    int32 ExpectedSchema, TArray<uint8>& OutBytes, FString& OutError)
{
    if (State.DomainId != ExpectedDomain || State.SchemaVersion != ExpectedSchema
        || State.Fields.Num() != 1 || State.Fields[0].Name != PayloadField
        || State.Fields[0].Type != ERBSaveFieldType::Bytes)
    {
        OutError = TEXT("RB Foundation domain envelope mismatch.");
        return false;
    }
    OutBytes = State.Fields[0].BytesValue;
    return true;
}

template <typename T>
bool StructToBytes(const T& Value, TArray<uint8>& OutBytes, FString& OutError)
{
    FString Json;
    if (!FJsonObjectConverter::UStructToJsonObjectString(T::StaticStruct(), &Value, Json, 0, 0))
    {
        OutError = TEXT("Failed to serialize Foundation domain to JSON.");
        return false;
    }
    FTCHARToUTF8 Utf8(*Json);
    OutBytes.SetNumUninitialized(Utf8.Length());
    if (Utf8.Length() > 0) FMemory::Memcpy(OutBytes.GetData(), Utf8.Get(), Utf8.Length());
    return true;
}

template <typename T>
bool BytesToStruct(const TArray<uint8>& Bytes, T& OutValue, FString& OutError)
{
    if (Bytes.Num() > 64 * 1024 * 1024)
    {
        OutError = TEXT("Foundation domain payload exceeds 64 MiB bound.");
        return false;
    }
    FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    const FString Json(Converted.Length(), Converted.Get());
    T Candidate{};
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Candidate, 0, 0))
    {
        OutError = TEXT("Failed to parse Foundation domain JSON.");
        return false;
    }
    OutValue = MoveTemp(Candidate);
    return true;
}
}

FName URBFoundationCoreSaveProvider::GetRBSaveDomainId_Implementation() const
{
    return TEXT("RBFoundation");
}

int32 URBFoundationCoreSaveProvider::GetRBSaveSchemaVersion_Implementation() const
{
    return 1;
}

bool URBFoundationCoreSaveProvider::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState, FString& OutError) const
{
    URBFoundationSubsystem* Foundation = Adapter ? Adapter->GetFoundation() : nullptr;
    if (!Foundation) { OutError = TEXT("RB Foundation subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    const FRBFoundationSnapshot Snapshot = Foundation->MakeSnapshot();
    if (!StructToBytes(Snapshot, Bytes, OutError)) return false;
    return SetBytesDomain(GetRBSaveDomainId_Implementation(), 1, Bytes, OutState);
}

bool URBFoundationCoreSaveProvider::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State, FString& OutError)
{
    URBFoundationSubsystem* Foundation = Adapter ? Adapter->GetFoundation() : nullptr;
    if (!Foundation) { OutError = TEXT("RB Foundation subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    if (!GetBytesDomain(State, GetRBSaveDomainId_Implementation(), 1, Bytes, OutError)) return false;
    FRBFoundationSnapshot Snapshot;
    if (!BytesToStruct(Bytes, Snapshot, OutError)) return false;
    if (!Foundation->RestoreSnapshot(Snapshot, OutError)) return false;
    Adapter->ResetTimeBridgeAfterRestore();
    return true;
}

FName URBFoundationItemEconomySaveProvider::GetRBSaveDomainId_Implementation() const
{
    return TEXT("RBItemEconomy");
}

int32 URBFoundationItemEconomySaveProvider::GetRBSaveSchemaVersion_Implementation() const
{
    return 1;
}

bool URBFoundationItemEconomySaveProvider::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState, FString& OutError) const
{
    URBItemEconomySubsystem* Items = Adapter ? Adapter->GetItemEconomy() : nullptr;
    if (!Items) { OutError = TEXT("RB Item Economy subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    if (!Items->CaptureEconomy(Bytes, OutError)) return false;
    return SetBytesDomain(GetRBSaveDomainId_Implementation(), 1, Bytes, OutState);
}

bool URBFoundationItemEconomySaveProvider::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State, FString& OutError)
{
    URBItemEconomySubsystem* Items = Adapter ? Adapter->GetItemEconomy() : nullptr;
    if (!Items) { OutError = TEXT("RB Item Economy subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    if (!GetBytesDomain(State, GetRBSaveDomainId_Implementation(), 1, Bytes, OutError)) return false;
    return Items->RestoreEconomy(Bytes, OutError);
}

FName URBFoundationRoutineSaveProvider::GetRBSaveDomainId_Implementation() const
{
    return TEXT("RBRoutinePopulation");
}

int32 URBFoundationRoutineSaveProvider::GetRBSaveSchemaVersion_Implementation() const
{
    return 1;
}

bool URBFoundationRoutineSaveProvider::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState, FString& OutError) const
{
    URBRoutineWorldSubsystem* Routine = Adapter ? Adapter->GetRoutine() : nullptr;
    if (!Routine) { OutError = TEXT("RB Routine world subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    const FRBRoutinePopulationSnapshot Snapshot = Routine->CreatePopulationSnapshot();
    if (!StructToBytes(Snapshot, Bytes, OutError)) return false;
    return SetBytesDomain(GetRBSaveDomainId_Implementation(), 1, Bytes, OutState);
}

bool URBFoundationRoutineSaveProvider::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State, FString& OutError)
{
    URBRoutineWorldSubsystem* Routine = Adapter ? Adapter->GetRoutine() : nullptr;
    if (!Routine) { OutError = TEXT("RB Routine world subsystem unavailable."); return false; }
    TArray<uint8> Bytes;
    if (!GetBytesDomain(State, GetRBSaveDomainId_Implementation(), 1, Bytes, OutError)) return false;
    FRBRoutinePopulationSnapshot Snapshot;
    if (!BytesToStruct(Bytes, Snapshot, OutError)) return false;
    return Routine->RestorePopulationSnapshot(Snapshot, OutError);
}

FName URBFoundationWeatherSaveProvider::GetRBSaveDomainId_Implementation() const
{
    return TEXT("RBWeather");
}

int32 URBFoundationWeatherSaveProvider::GetRBSaveSchemaVersion_Implementation() const
{
    return 1;
}

bool URBFoundationWeatherSaveProvider::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState, FString& OutError) const
{
    ARBWeatherDirector* Weather = Adapter ? Adapter->GetWeatherDirector() : nullptr;
    if (!Weather) { OutError = TEXT("RB Weather director is not registered with Foundation."); return false; }
    TArray<uint8> Bytes;
    const FRBWeatherSaveSnapshot Snapshot = Weather->MakeSnapshot();
    if (!StructToBytes(Snapshot, Bytes, OutError)) return false;
    return SetBytesDomain(GetRBSaveDomainId_Implementation(), 1, Bytes, OutState);
}

bool URBFoundationWeatherSaveProvider::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State, FString& OutError)
{
    ARBWeatherDirector* Weather = Adapter ? Adapter->GetWeatherDirector() : nullptr;
    if (!Weather) { OutError = TEXT("RB Weather director is not registered with Foundation."); return false; }
    TArray<uint8> Bytes;
    if (!GetBytesDomain(State, GetRBSaveDomainId_Implementation(), 1, Bytes, OutError)) return false;
    FRBWeatherSaveSnapshot Snapshot;
    if (!BytesToStruct(Bytes, Snapshot, OutError)) return false;
    if (!Weather->RestoreSnapshot(Snapshot)) { OutError = TEXT("RB Weather rejected restored snapshot."); return false; }
    return true;
}

namespace
{
bool ValidOptimizationItem(const FRBOptSavedItem& Item, TSet<int64>& Seen,
    int64& PayloadBytes, FString& OutError)
{
    const FVector Scale = Item.Transform.GetScale3D();
    if (Item.Id <= 0 || Seen.Contains(Item.Id) || Item.Transform.ContainsNaN()
        || !Item.Transform.GetRotation().IsNormalized()
        || Scale.X <= 0 || Scale.Y <= 0 || Scale.Z <= 0
        || Scale.X > 10000 || Scale.Y > 10000 || Scale.Z > 10000)
    {
        OutError = TEXT("Optimization snapshot contains invalid identity or transform.");
        return false;
    }
    PayloadBytes += static_cast<int64>(Item.Payload.Len()) * sizeof(TCHAR);
    if (Item.Payload.Len() > 1048576 || PayloadBytes > 64LL * 1024LL * 1024LL
        || !FMath::IsFinite(Item.RemainingHoldSeconds) || Item.RemainingHoldSeconds < 0
        || !FMath::IsFinite(Item.RespawnRemainingSeconds) || Item.RespawnRemainingSeconds < -1)
    {
        OutError = TEXT("Optimization snapshot exceeds bounded payload/time limits.");
        return false;
    }
    Seen.Add(Item.Id);
    return true;
}

bool SameOptimizationPresentationRC1(const FString& Current, const FString& Saved)
{
    if (Current == Saved) return true;
    TArray<FString> CurrentTokens;
    TArray<FString> SavedTokens;
    Current.ParseIntoArray(CurrentTokens, TEXT(":"), false);
    Saved.ParseIntoArray(SavedTokens, TEXT(":"), false);
    if (CurrentTokens.Num() != SavedTokens.Num() || CurrentTokens.Num() < 2) return false;

    // RB Optimization 0.1.0-rc1 appends ECC_OverlapAll_Deprecated after the last
    // serialized collision channel. UE marks that channel transient/nonserialized;
    // reading its response produces a process-local garbage tail byte. All serialized
    // presentation tokens must still match exactly.
    for (int32 Index = 0; Index < CurrentTokens.Num() - 1; ++Index)
        if (CurrentTokens[Index] != SavedTokens[Index]) return false;
    return true;
}

bool SameOptimizationIdentity(const FRBOptSnapshot& Current, const FRBOptSnapshot& Saved)
{
    return Current.CollectionId == Saved.CollectionId
        && SameOptimizationPresentationRC1(Current.PresentationSignature, Saved.PresentationSignature)
        && Current.MeshPath == Saved.MeshPath && Current.ClassPath == Saved.ClassPath
        && Current.CollisionProfile == Saved.CollisionProfile
        && Current.MaterialPaths == Saved.MaterialPaths;
}
}
FName URBFoundationOptimizationSaveProvider::GetRBSaveDomainId_Implementation() const
{
    return TEXT("RBOptimization");
}

int32 URBFoundationOptimizationSaveProvider::GetRBSaveSchemaVersion_Implementation() const
{
    return 1;
}

bool URBFoundationOptimizationSaveProvider::CaptureRBSaveDomain_Implementation(
    FRBSaveDomainState& OutState, FString& OutError) const
{
    if (!Adapter) { OutError = TEXT("RB Foundation adapter unavailable."); return false; }
    FRBFoundationOptimizationSnapshot Bundle;
    TArray<FName> GroupIds;
    Adapter->GetOptimizationGroups().GetKeys(GroupIds);
    GroupIds.Sort(FNameLexicalLess());

    for (FName GroupId : GroupIds)
    {
        ARBOptimizationGroup* Group = Adapter->GetOptimizationGroup(GroupId);
        if (!IsValid(Group)) { OutError = TEXT("Registered Optimization group expired."); return false; }
        FRBFoundationOptimizationGroupSnapshot Entry;
        Entry.GroupId = GroupId;
        if (!Group->CaptureSnapshot(Entry.Snapshot, OutError)) return false;
        Bundle.Groups.Add(MoveTemp(Entry));
    }
    TArray<uint8> Bytes;
    if (!StructToBytes(Bundle, Bytes, OutError)) return false;
    return SetBytesDomain(GetRBSaveDomainId_Implementation(), 1, Bytes, OutState);
}

bool URBFoundationOptimizationSaveProvider::RestoreRBSaveDomain_Implementation(
    const FRBSaveDomainState& State, FString& OutError)
{
    if (!Adapter) { OutError = TEXT("RB Foundation adapter unavailable."); return false; }
    TArray<uint8> Bytes;
    if (!GetBytesDomain(State, GetRBSaveDomainId_Implementation(), 1, Bytes, OutError)) return false;
    FRBFoundationOptimizationSnapshot Bundle;
    if (!BytesToStruct(Bytes, Bundle, OutError)) return false;
    if (Bundle.SchemaVersion != 1 || Bundle.Groups.Num() > 4096)
    {
        OutError = TEXT("Unsupported or oversized Optimization bundle.");
        return false;
    }

    TSet<FName> SeenGroups;
    for (FRBFoundationOptimizationGroupSnapshot& Entry : Bundle.Groups)
    {
        if (Entry.GroupId.IsNone() || SeenGroups.Contains(Entry.GroupId))
        {
            OutError = TEXT("Optimization bundle contains missing/duplicate group ID.");
            return false;
        }
        SeenGroups.Add(Entry.GroupId);
        ARBOptimizationGroup* Group = Adapter->GetOptimizationGroup(Entry.GroupId);
        if (!IsValid(Group) || Group->GetRegisteredCount() != 0)
        {
            OutError = FString::Printf(TEXT("Optimization group '%s' is missing or not fresh."),
                *Entry.GroupId.ToString());
            return false;
        }
        if (Entry.Snapshot.SchemaVersion != 3 || Entry.Snapshot.Items.Num() > 100000)
        {
            OutError = TEXT("Optimization group snapshot has unsupported schema or size.");
            return false;
        }
        FRBOptSnapshot Current;
        FString CurrentError;
        if (!Group->CaptureSnapshot(Current, CurrentError))
        {
            OutError = FString::Printf(TEXT("Optimization group '%s' preflight capture failed: %s"),
                *Entry.GroupId.ToString(), *CurrentError);
            return false;
        }
        if (!SameOptimizationIdentity(Current, Entry.Snapshot))
        {
            OutError = FString::Printf(TEXT("Optimization group '%s' identity mismatch [collection=%d presentation=%d mesh=%d class=%d collision=%d materials=%d]. Current collection=%s mesh=%s class=%s collision=%s; Saved collection=%s mesh=%s class=%s collision=%s."),
                *Entry.GroupId.ToString(), Current.CollectionId == Entry.Snapshot.CollectionId,
                Current.PresentationSignature == Entry.Snapshot.PresentationSignature,
                Current.MeshPath == Entry.Snapshot.MeshPath, Current.ClassPath == Entry.Snapshot.ClassPath,
                Current.CollisionProfile == Entry.Snapshot.CollisionProfile,
                Current.MaterialPaths == Entry.Snapshot.MaterialPaths,
                *Current.CollectionId.ToString(), *Current.MeshPath, *Current.ClassPath, *Current.CollisionProfile.ToString(),
                *Entry.Snapshot.CollectionId.ToString(), *Entry.Snapshot.MeshPath, *Entry.Snapshot.ClassPath,
                *Entry.Snapshot.CollisionProfile.ToString());
            OutError += FString::Printf(TEXT(" CurrentSignature=%s SavedSignature=%s"),
                *Current.PresentationSignature, *Entry.Snapshot.PresentationSignature);
            return false;
        }
        // RC1 includes one transient deprecated collision-channel response in its
        // presentation signature. Once all serialized identity fields and all prior
        // signature tokens match, normalize only that tail token to this fresh process.
        if (Current.PresentationSignature != Entry.Snapshot.PresentationSignature)
            Entry.Snapshot.PresentationSignature = Current.PresentationSignature;
        TSet<int64> SeenItems;
        int64 PayloadBytes = 0;
        for (const FRBOptSavedItem& Item : Entry.Snapshot.Items)
        {
            const bool bConsumed = Item.State == ERBOptRepresentation::Consumed;
            const bool bKnownState = Item.State == ERBOptRepresentation::Instance
                || Item.State == ERBOptRepresentation::Actor || bConsumed;
            if (!bKnownState || (Item.RespawnRemainingSeconds >= 0 && !bConsumed)
                || (bConsumed && (Item.bPinned || Item.RemainingHoldSeconds > 0))
                || !ValidOptimizationItem(Item, SeenItems, PayloadBytes, OutError))
                return false;
        }
    }
    for (const auto& Pair : Adapter->GetOptimizationGroups())
    {
        if (!Pair.Value.IsValid() || !SeenGroups.Contains(Pair.Key))
        {
            OutError = TEXT("Registered Optimization group set does not match saved bundle.");
            return false;
        }
    }

    TArray<FRBFoundationOptimizationGroupSnapshot> Ordered = Bundle.Groups;
    Ordered.Sort([](const auto& A, const auto& B)
    {
        return A.GroupId.LexicalLess(B.GroupId);
    });
    for (const FRBFoundationOptimizationGroupSnapshot& Entry : Ordered)
    {
        ARBOptimizationGroup* Group = Adapter->GetOptimizationGroup(Entry.GroupId);
        FString RestoreError;
        if (!Group->RestoreIntoFreshGroup(Entry.Snapshot, RestoreError))
        {
            OutError = FString::Printf(TEXT("Optimization group '%s' restore failed: %s"),
                *Entry.GroupId.ToString(), *RestoreError);
            return false;
        }
    }
    return true;
}

