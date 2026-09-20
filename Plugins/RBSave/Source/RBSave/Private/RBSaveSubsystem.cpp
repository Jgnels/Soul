#include "RBSaveSubsystem.h"

#include "RBSaveCore.h"
#include "RBSaveIdentityComponent.h"
#include "Async/Async.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/SoftObjectPath.h"

namespace {
using rb::save::Bytes;
using rb::save::DomainRecord;
using rb::save::Snapshot;
using rb::save::Value;

std::string ToUtf8(const FString& Value)
{
    FTCHARToUTF8 Converted(*Value);
    return std::string(Converted.Get(), Converted.Length());
}

FString FromUtf8(const std::string& Value)
{
    return FString(UTF8_TO_TCHAR(Value.c_str()));
}

std::string ActorDomainId(const FString& StableId)
{
    return "world.actor." + ToUtf8(StableId);
}

const Value* FindField(const DomainRecord& Domain, const char* Name)
{
    const auto It = Domain.fields.find(Name);
    return It == Domain.fields.end() ? nullptr : &It->second;
}

FString NormalizeLevelName(const AActor* Actor)
{
    if (!Actor || !Actor->GetLevel()) return FString();
    return UWorld::RemovePIEPrefix(Actor->GetLevel()->GetOutermost()->GetName());
}

FString StableActorId(AActor* Actor, bool bAllowCreateId, bool& bRuntimeSpawned, bool& bStable)
{
    bRuntimeSpawned = false;
    bStable = false;
    if (!Actor) return FString();
    if (auto* Identity = Actor->FindComponentByClass<URBSaveIdentityComponent>())
    {
        const FGuid Guid = bAllowCreateId ? Identity->GetOrCreatePersistentId() : Identity->PersistentId;
        if (!Guid.IsValid()) return FString();
        bRuntimeSpawned = !Actor->HasAnyFlags(RF_WasLoaded);
        bStable = true;
        return FString::Printf(TEXT("guid:%s"), *Guid.ToString(EGuidFormats::DigitsWithHyphensLower));
    }

    if (!Actor->HasAnyFlags(RF_WasLoaded))
    {
        bRuntimeSpawned = true;
        return FString();
    }

    const FString LevelName = NormalizeLevelName(Actor);
    if (LevelName.IsEmpty()) return FString();
    bStable = true;
    return FString::Printf(TEXT("placed:%s:%s"), *LevelName, *Actor->GetFName().ToString());
}

Bytes ToBytes(const TArray<uint8>& Data)
{
    return Bytes(Data.GetData(), Data.GetData() + Data.Num());
}

TArray<uint8> ToArray(const Bytes& Data)
{
    TArray<uint8> Result;
    if (!Data.empty()) Result.Append(Data.data(), static_cast<int32>(Data.size()));
    return Result;
}

bool SerializeActor(AActor* Actor, Bytes& OutBytes)
{
    TArray<uint8> Buffer;
    FMemoryWriter Writer(Buffer, true);
    FObjectAndNameAsStringProxyArchive Archive(Writer, false);
    Archive.ArIsSaveGame = true;
    Archive.ArNoDelta = true;
    Actor->Serialize(Archive);
    Writer.Flush();
    if (Writer.IsError()) return false;
    OutBytes = ToBytes(Buffer);
    return true;
}

bool DeserializeActor(AActor* Actor, const Bytes& Data)
{
    TArray<uint8> Buffer = ToArray(Data);
    FMemoryReader Reader(Buffer, true);
    FObjectAndNameAsStringProxyArchive Archive(Reader, true);
    Archive.ArIsSaveGame = true;
    Archive.ArNoDelta = true;
    Actor->Serialize(Archive);
    return !Reader.IsError();
}

bool CaptureWorld(UWorld* World, const FString& Slot, Snapshot& OutSnapshot,
                  int32& OutActors, int32& OutSkipped, FString& OutError)
{
    if (!World) { OutError = TEXT("world unavailable"); return false; }
    OutSnapshot.sequence = static_cast<std::uint64_t>(FDateTime::UtcNow().GetTicks());
    OutSnapshot.createdUtcMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000LL;
    OutSnapshot.slot = ToUtf8(Slot);

    DomainRecord Meta;
    Meta.id = "world.meta";
    Meta.schemaVersion = 1;
    Meta.fields["Map"] = ToUtf8(UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()));
    OutSnapshot.domains.push_back(std::move(Meta));

    OutActors = 0;
    OutSkipped = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->HasAnyFlags(RF_Transient) || Actor->IsTemplate())
            continue;

        bool bRuntime = false;
        bool bStable = false;
        const FString StableId = StableActorId(Actor, true, bRuntime, bStable);
        if (!bStable)
        {
            ++OutSkipped;
            continue;
        }

        Bytes SaveData;
        if (!SerializeActor(Actor, SaveData))
        {
            OutError = FString::Printf(TEXT("failed to serialize %s"), *Actor->GetName());
            return false;
        }

        const FTransform Transform = Actor->GetActorTransform();
        const FVector Location = Transform.GetLocation();
        const FRotator Rotation = Transform.Rotator();
        const FVector Scale = Transform.GetScale3D();
        DomainRecord Record;
        Record.id = ActorDomainId(StableId);
        Record.schemaVersion = 1;
        Record.fields["StableId"] = ToUtf8(StableId);
        Record.fields["Class"] = ToUtf8(Actor->GetClass()->GetPathName());
        Record.fields["Level"] = ToUtf8(NormalizeLevelName(Actor));
        Record.fields["RuntimeSpawned"] = bRuntime;
        Record.fields["X"] = Location.X;
        Record.fields["Y"] = Location.Y;
        Record.fields["Z"] = Location.Z;
        Record.fields["Pitch"] = Rotation.Pitch;
        Record.fields["Yaw"] = Rotation.Yaw;
        Record.fields["Roll"] = Rotation.Roll;
        Record.fields["ScaleX"] = Scale.X;
        Record.fields["ScaleY"] = Scale.Y;
        Record.fields["ScaleZ"] = Scale.Z;
        Record.fields["SaveGameData"] = std::move(SaveData);
        OutSnapshot.domains.push_back(std::move(Record));
        ++OutActors;
    }
    return true;
}

bool GetDouble(const DomainRecord& Record, const char* Name, double& Out)
{
    const Value* Field = FindField(Record, Name);
    if (!Field) return false;
    if (const auto* D = std::get_if<double>(Field)) { Out = *D; return true; }
    return false;
}

bool RestoreWorld(UWorld* World, const Snapshot& SnapshotData,
                  int32& OutActors, int32& OutSkipped, FString& OutError)
{
    if (!World) { OutError = TEXT("world unavailable"); return false; }
    TMap<FString, AActor*> Existing;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) continue;
        bool bRuntime = false;
        bool bStable = false;
        const FString StableId = StableActorId(Actor, false, bRuntime, bStable);
        if (bStable) Existing.Add(StableId, Actor);
    }

    OutActors = 0;
    OutSkipped = 0;
    for (const DomainRecord& Record : SnapshotData.domains)
    {
        if (!Record.id.starts_with("world.actor.")) continue;
        const Value* StableValue = FindField(Record, "StableId");
        const auto* StableString = StableValue ? std::get_if<std::string>(StableValue) : nullptr;
        if (!StableString) { ++OutSkipped; continue; }
        const FString StableId = FromUtf8(*StableString);
        AActor* Actor = Existing.FindRef(StableId);

        const Value* RuntimeValue = FindField(Record, "RuntimeSpawned");
        const bool bRuntime = RuntimeValue && std::get_if<bool>(RuntimeValue) && *std::get_if<bool>(RuntimeValue);
        if (!Actor && bRuntime)
        {
            const Value* ClassValue = FindField(Record, "Class");
            const auto* ClassString = ClassValue ? std::get_if<std::string>(ClassValue) : nullptr;
            if (!ClassString) { ++OutSkipped; continue; }
            double X=0,Y=0,Z=0,Pitch=0,Yaw=0,Roll=0,SX=1,SY=1,SZ=1;
            if (!GetDouble(Record,"X",X) || !GetDouble(Record,"Y",Y) || !GetDouble(Record,"Z",Z) ||
                !GetDouble(Record,"Pitch",Pitch) || !GetDouble(Record,"Yaw",Yaw) || !GetDouble(Record,"Roll",Roll) ||
                !GetDouble(Record,"ScaleX",SX) || !GetDouble(Record,"ScaleY",SY) || !GetDouble(Record,"ScaleZ",SZ))
            { ++OutSkipped; continue; }
            const FTransform SpawnTransform(FRotator(Pitch,Yaw,Roll), FVector(X,Y,Z), FVector(SX,SY,SZ));
            const FSoftClassPath ClassPath(FromUtf8(*ClassString));
            UClass* ActorClass = ClassPath.TryLoadClass<AActor>();
            if (!ActorClass) { ++OutSkipped; continue; }
            Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform);
            if (Actor && StableId.StartsWith(TEXT("guid:")))
            {
                FGuid Guid;
                if (FGuid::Parse(StableId.RightChop(5), Guid))
                    if (auto* Identity = Actor->FindComponentByClass<URBSaveIdentityComponent>()) Identity->PersistentId = Guid;
            }
        }
        if (!Actor)
        {
            // Likely a stable actor in a streamed level that is not currently loaded.
            ++OutSkipped;
            continue;
        }

        double X=0,Y=0,Z=0,Pitch=0,Yaw=0,Roll=0,SX=1,SY=1,SZ=1;
        if (!GetDouble(Record,"X",X) || !GetDouble(Record,"Y",Y) || !GetDouble(Record,"Z",Z) ||
            !GetDouble(Record,"Pitch",Pitch) || !GetDouble(Record,"Yaw",Yaw) || !GetDouble(Record,"Roll",Roll) ||
            !GetDouble(Record,"ScaleX",SX) || !GetDouble(Record,"ScaleY",SY) || !GetDouble(Record,"ScaleZ",SZ))
        { ++OutSkipped; continue; }
        const FTransform SavedTransform(FRotator(Pitch,Yaw,Roll), FVector(X,Y,Z), FVector(SX,SY,SZ));
        if (!Actor->GetActorTransform().Equals(SavedTransform, 0.1))
        {
            Actor->SetActorTransform(SavedTransform, false, nullptr, ETeleportType::TeleportPhysics);
        }

        const Value* DataValue = FindField(Record, "SaveGameData");
        const auto* Data = DataValue ? std::get_if<Bytes>(DataValue) : nullptr;
        if (!Data || !DeserializeActor(Actor, *Data))
        {
            OutError = FString::Printf(TEXT("failed to restore %s"), *StableId);
            return false;
        }
        ++OutActors;
    }
    return true;
}

FRBSaveOperationResult MakeResult(bool bSuccess, FString Message, int64 Sequence = 0,
                                  int64 BytesWritten = 0, int64 Checksum = 0,
                                  int32 Actors = 0, int32 Skipped = 0)
{
    FRBSaveOperationResult Result;
    Result.bSuccess = bSuccess;
    Result.Message = MoveTemp(Message);
    Result.Sequence = Sequence;
    Result.Bytes = BytesWritten;
    Result.Checksum = Checksum;
    Result.ActorRecords = Actors;
    Result.SkippedUnstableActors = Skipped;
    return Result;
}

FString SanitizeSlot(FString Slot)
{
    Slot.TrimStartAndEndInline();
    Slot.ReplaceInline(TEXT("/"), TEXT("_"));
    Slot.ReplaceInline(TEXT("\\"), TEXT("_"));
    Slot.ReplaceInline(TEXT(":"), TEXT("_"));
    Slot.ReplaceInline(TEXT(".."), TEXT("_"));
    return Slot;
}
} // namespace

FString URBSaveSubsystem::GetSlotPath(const FString& Slot) const
{
    const FString Safe = SanitizeSlot(Slot);
    if (Safe.IsEmpty()) return FString();
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RBSave"), Safe + TEXT(".rbsave"));
}

void URBSaveSubsystem::SaveWorldCpp(const FString& Slot, FRBSaveCppCompletion Completion)
{
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty())
    {
        if (Completion) Completion(MakeResult(false, TEXT("invalid slot")));
        return;
    }

    Snapshot Captured;
    int32 Actors = 0;
    int32 Skipped = 0;
    FString Error;
    if (!CaptureWorld(GetWorld(), Slot, Captured, Actors, Skipped, Error))
    {
        if (Completion) Completion(MakeResult(false, Error, 0, 0, 0, Actors, Skipped));
        return;
    }

    const int64 Sequence = static_cast<int64>(Captured.sequence);
    Async(EAsyncExecution::ThreadPool,
        [Path, Captured = MoveTemp(Captured), Actors, Skipped, Sequence,
         Callback = MoveTemp(Completion)]() mutable
    {
        const auto Io = rb::save::WriteAtomic(std::filesystem::path(*Path), Captured);
        FRBSaveOperationResult Result = MakeResult(Io.ok,
            Io.ok ? TEXT("save complete") : FromUtf8(Io.error), Sequence,
            static_cast<int64>(Io.bytesWritten), static_cast<int64>(Io.checksum), Actors, Skipped);
        AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
        {
            if (Callback) Callback(Result);
        });
    });
}

void URBSaveSubsystem::SaveWorldAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion)
{
    FRBSaveOperationDelegate Callback = Completion;
    SaveWorldCpp(Slot, [Callback](FRBSaveOperationResult Result) mutable
    {
        Callback.ExecuteIfBound(Result);
    });
}

void URBSaveSubsystem::LoadWorldCpp(const FString& Slot, FRBSaveCppCompletion Completion)
{
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty())
    {
        if (Completion) Completion(MakeResult(false, TEXT("invalid slot")));
        return;
    }

    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool, [WeakThis, Path, Callback = MoveTemp(Completion)]() mutable
    {
        auto Loaded = rb::save::ReadFile(std::filesystem::path(*Path));
        if (!Loaded.ok)
        {
            FRBSaveOperationResult Result = MakeResult(false, FromUtf8(Loaded.error));
            AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
            {
                if (Callback) Callback(Result);
            });
            return;
        }
        AsyncTask(ENamedThreads::GameThread,
            [WeakThis, Callback = MoveTemp(Callback), Loaded = MoveTemp(Loaded)]() mutable
        {
            if (!WeakThis.IsValid())
            {
                if (Callback) Callback(MakeResult(false, TEXT("save subsystem destroyed")));
                return;
            }
            int32 Actors = 0;
            int32 Skipped = 0;
            FString Error;
            const bool bOk = RestoreWorld(WeakThis->GetWorld(), Loaded.snapshot, Actors, Skipped, Error);
            if (Callback) Callback(MakeResult(bOk, bOk ? TEXT("load complete") : Error,
                static_cast<int64>(Loaded.snapshot.sequence), 0, 0, Actors, Skipped));
        });
    });
}

void URBSaveSubsystem::LoadWorldAsync(const FString& Slot, const FRBSaveOperationDelegate& Completion)
{
    FRBSaveOperationDelegate Callback = Completion;
    LoadWorldCpp(Slot, [Callback](FRBSaveOperationResult Result) mutable
    {
        Callback.ExecuteIfBound(Result);
    });
}

FString URBSaveSubsystem::InspectSlot(const FString& Slot, bool& bValid, FString& Error) const
{
    bValid = false;
    Error.Reset();
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty()) { Error = TEXT("invalid slot"); return FString(); }
    const auto Loaded = rb::save::ReadFile(std::filesystem::path(*Path));
    if (!Loaded.ok) { Error = FromUtf8(Loaded.error); return FString(); }
    bValid = true;
    return FromUtf8(rb::save::ToJson(Loaded.snapshot));
}

bool URBSaveSubsystem::RestoreQuickBackup(const FString& Slot, FString& Error) const
{
    const FString Path = GetSlotPath(Slot);
    if (Path.IsEmpty()) { Error = TEXT("invalid slot"); return false; }
    std::string CoreError;
    const bool bOk = rb::save::RestoreBackup(std::filesystem::path(*Path), CoreError);
    Error = bOk ? FString() : FromUtf8(CoreError);
    return bOk;
}
