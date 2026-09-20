#include "RBSaveSubsystem.h"

#include "RBSaveCore.h"
#include "RBSaveGeneration.h"
#include "RBSaveSettings.h"
#include "RBSaveWorldBackend.h"
#include "Async/Async.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

namespace {
using rb::save::GenerationArtifact;
using rb::save::GenerationManifest;
using rb::save::Snapshot;

constexpr const TCHAR* NativeBackendId = TEXT("Native");

std::string ToUtf8Coord(const FString& Text)
{
    FTCHARToUTF8 Converted(*Text);
    return std::string(Converted.Get(), Converted.Length());
}

FString FromUtf8Coord(const std::string& Text)
{
    return FString(UTF8_TO_TCHAR(Text.c_str()));
}
FString SanitizeCoordSlot(FString Slot)
{
    Slot.TrimStartAndEndInline();
    Slot.ReplaceInline(TEXT("/"), TEXT("_"));
    Slot.ReplaceInline(TEXT("\\"), TEXT("_"));
    Slot.ReplaceInline(TEXT(":"), TEXT("_"));
    Slot.ReplaceInline(TEXT(".."), TEXT("_"));
    return Slot;
}

FRBSaveOperationResult CoordResult(bool bSuccess, FString Message)
{
    FRBSaveOperationResult Result;
    Result.bSuccess = bSuccess;
    Result.Message = MoveTemp(Message);
    return Result;
}

FString GenerationName(uint64 Generation)
{
    return FString::Printf(TEXT("%020llu"),
        static_cast<unsigned long long>(Generation));
}

FString PhysicalWorldSlot(const FString& Slot, uint64 Generation)
{
    return FString::Printf(TEXT("RB_%s_g%s"), *Slot, *GenerationName(Generation));
}
bool MakePortableSavedPath(const FString& AbsolutePath, FString& OutPath, FString& Error)
{
    FString SavedRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FString FullPath = FPaths::ConvertRelativePathToFull(AbsolutePath);
    FPaths::NormalizeDirectoryName(SavedRoot);
    FPaths::NormalizeFilename(FullPath);
    FString Prefix = SavedRoot;
    if (!Prefix.EndsWith(TEXT("/"))) Prefix += TEXT("/");
    if (!FullPath.StartsWith(Prefix, ESearchCase::IgnoreCase))
    {
        Error = TEXT("artifact path is outside Project/Saved");
        return false;
    }
    OutPath = FullPath.RightChop(Prefix.Len());
    FPaths::NormalizeFilename(OutPath);
    if (OutPath.IsEmpty() || FPaths::IsRelative(OutPath) == false || OutPath.Contains(TEXT("..")))
    {
        Error = TEXT("unable to derive safe portable artifact path");
        return false;
    }
    return true;
}

bool ResolvePortableSavedPath(const std::string& Portable, FString& OutPath, FString& Error)
{
    const FString Relative = FromUtf8Coord(Portable);
    if (Relative.IsEmpty() || !FPaths::IsRelative(Relative) || Relative.Contains(TEXT("..")))
    {
        Error = TEXT("generation artifact path is not a safe relative path");
        return false;
    }
    FString SavedRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FPaths::NormalizeDirectoryName(SavedRoot);
    OutPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(SavedRoot, Relative));
    FPaths::NormalizeFilename(OutPath);
    FString Prefix = SavedRoot;
    FPaths::NormalizeFilename(Prefix);
    if (!Prefix.EndsWith(TEXT("/"))) Prefix += TEXT("/");
    if (!OutPath.StartsWith(Prefix, ESearchCase::IgnoreCase))
    {
        Error = TEXT("resolved generation artifact escapes Project/Saved");
        return false;
    }
    return true;
}

rb::save::ArtifactVerifier MakeArtifactVerifier()
{
    return [](const GenerationArtifact& Artifact, std::string& CoreError)
    {
        const std::string& Portable = Artifact.path.empty() ? Artifact.locator : Artifact.path;
        FString Absolute;
        FString Error;
        if (!ResolvePortableSavedPath(Portable, Absolute, Error))
        {
            CoreError = ToUtf8Coord(Error);
            return false;
        }
        return rb::save::VerifyFileFingerprint(Artifact,
            std::filesystem::path(*Absolute), CoreError);
    };
}
const GenerationArtifact* FindArtifact(const GenerationManifest& Manifest, const char* Role)
{
    for (const GenerationArtifact& Artifact : Manifest.artifacts)
        if (Artifact.role == Role) return &Artifact;
    return nullptr;
}

TSharedPtr<IRBSaveWorldBackend> SelectBackend(UWorld* World, FName& OutId, FString& Error)
{
    const URBSaveSettings* Settings = GetDefault<URBSaveSettings>();
    const FName Preferred = Settings ? Settings->PreferredWorldBackend : NAME_None;
    if (!Preferred.IsNone() && Preferred != FName(NativeBackendId))
    {
        TSharedPtr<IRBSaveWorldBackend> Backend = FRBSaveWorldBackendRegistry::Find(Preferred);
        FString AvailabilityError;
        if (Backend.IsValid() && Backend->IsAvailable(World, AvailabilityError))
        {
            OutId = Preferred;
            return Backend;
        }
        if (Settings && !Settings->bAllowNativeFallback)
        {
            Error = AvailabilityError.IsEmpty()
                ? FString::Printf(TEXT("preferred backend '%s' unavailable"), *Preferred.ToString())
                : AvailabilityError;
            return nullptr;
        }
    }
    OutId = FName(NativeBackendId);
    return nullptr;
}
FString ManifestArtifactPath(const GenerationArtifact& Artifact)
{
    FString Absolute;
    FString Error;
    const std::string& Portable = Artifact.path.empty() ? Artifact.locator : Artifact.path;
    if (!ResolvePortableSavedPath(Portable, Absolute, Error)) return FString();
    return Absolute;
}

void FillGenerationInfo(const GenerationManifest& Manifest, FRBSaveGenerationInfo& OutInfo)
{
    OutInfo = FRBSaveGenerationInfo{};
    OutInfo.Generation = static_cast<int64>(Manifest.generation);
    OutInfo.LogicalSlot = FromUtf8Coord(Manifest.logicalSlot);
    OutInfo.CreatedUtcMs = Manifest.createdUtcMs;
    OutInfo.ArtifactCount = static_cast<int32>(Manifest.artifacts.size());
    if (const GenerationArtifact* World = FindArtifact(Manifest, "world"))
        OutInfo.WorldBackend = FName(FromUtf8Coord(World->backend));
    for (const GenerationArtifact& Artifact : Manifest.artifacts)
    {
        FRBSaveArtifactInfo Info;
        Info.Role = FName(FromUtf8Coord(Artifact.role));
        Info.BackendId = FName(FromUtf8Coord(Artifact.backend));
        Info.Locator = FromUtf8Coord(Artifact.locator);
        Info.Path = FromUtf8Coord(Artifact.path.empty() ? Artifact.locator : Artifact.path);
        Info.Bytes = static_cast<int64>(Artifact.bytes);
        Info.Checksum = static_cast<int64>(Artifact.checksum);
        OutInfo.Artifacts.Add(MoveTemp(Info));
    }
}
} // namespace

FString URBSaveSubsystem::GetGenerationRoot(const FString& Slot) const
{
    const FString Safe = SanitizeCoordSlot(Slot);
    return Safe.IsEmpty() ? FString() : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RBSave"), TEXT("Slots"), Safe);
}
TArray<FName> URBSaveSubsystem::GetAvailableWorldBackends() const
{
    TArray<FName> Result = FRBSaveWorldBackendRegistry::List();
    Result.AddUnique(FName(NativeBackendId));
    Result.Sort(FNameLexicalLess());
    return Result;
}

FName URBSaveSubsystem::GetEffectiveWorldBackend() const
{
    FName BackendId = NAME_None;
    FString Error;
    SelectBackend(GetWorld(), BackendId, Error);
    return Error.IsEmpty() ? BackendId : NAME_None;
}

bool URBSaveSubsystem::GetActiveGeneration(const FString& Slot,
                                           FRBSaveGenerationInfo& OutInfo,
                                           FString& Error) const
{
    Error.Reset();
    const FString Root = GetGenerationRoot(Slot);
    if (Root.IsEmpty()) { Error = TEXT("invalid slot"); return false; }
    std::string CoreError;
    const auto Active = rb::save::ReadActiveGeneration(
        std::filesystem::path(*Root), CoreError, MakeArtifactVerifier());
    if (!Active)
    {
        Error = FromUtf8Coord(CoreError);
        return false;
    }
    FillGenerationInfo(*Active, OutInfo);
    return true;
}
bool URBSaveSubsystem::ValidateActiveGeneration(const FString& Slot, FString& Error) const
{
    FRBSaveGenerationInfo Info;
    return GetActiveGeneration(Slot, Info, Error);
}

void URBSaveSubsystem::SaveGameAsync(const FString& Slot,
                                     const FRBSaveOperationDelegate& Completion)
{
    const FString SafeSlot = SanitizeCoordSlot(Slot);
    const FString Root = GetGenerationRoot(SafeSlot);
    if (SafeSlot.IsEmpty() || Root.IsEmpty())
    {
        Completion.ExecuteIfBound(CoordResult(false, TEXT("invalid slot")));
        return;
    }

    const URBSaveSettings* Settings = GetDefault<URBSaveSettings>();
    if (Settings && Settings->bRequireDomainProviders && GetRegisteredDomainIds().IsEmpty())
    {
        Completion.ExecuteIfBound(CoordResult(false, TEXT("at least one domain provider is required")));
        return;
    }

    Snapshot Domains;
    FString DomainError;
    if (!CaptureRegisteredDomains(SafeSlot, Domains, DomainError))
    {
        Completion.ExecuteIfBound(CoordResult(false, DomainError));
        return;
    }
    std::string NumberError;
    const int32 MaxScan = Settings ? Settings->MaxGenerationScan : 1000;
    const uint64 Generation = rb::save::NextGenerationNumber(
        std::filesystem::path(*Root), NumberError, static_cast<std::size_t>(MaxScan));
    if (Generation == 0)
    {
        Completion.ExecuteIfBound(CoordResult(false,
            FString::Printf(TEXT("unable to allocate generation: %s"), *FromUtf8Coord(NumberError))));
        return;
    }

    Domains.sequence = Generation;
    Domains.slot = ToUtf8Coord(SafeSlot);
    const FString GenerationDir = FPaths::Combine(
        Root, TEXT("generations"), GenerationName(Generation));
    const FString DomainPath = FPaths::Combine(GenerationDir, TEXT("domain.rbsave"));
    const FString WorldPhysicalSlot = PhysicalWorldSlot(SafeSlot, Generation);

    FName BackendId = NAME_None;
    FString BackendError;
    TSharedPtr<IRBSaveWorldBackend> Backend = SelectBackend(GetWorld(), BackendId, BackendError);
    if (!BackendError.IsEmpty())
    {
        Completion.ExecuteIfBound(CoordResult(false, BackendError));
        return;
    }

    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    FRBSaveOperationDelegate Callback = Completion;
    auto FinalizeWorldSave =
        [WeakThis, Callback, Root, SafeSlot, Generation, GenerationDir, DomainPath,
         Domains = MoveTemp(Domains), BackendId, WorldPhysicalSlot]
        (FRBSaveWorldBackendResult WorldResult) mutable
    {
        if (!WorldResult.bSuccess)
        {
            FRBSaveOperationResult Result = CoordResult(false,
                TEXT("world backend save failed: ") + WorldResult.Message);
            Result.BackendId = BackendId;
            Result.Generation = static_cast<int64>(Generation);
            Callback.ExecuteIfBound(Result);
            return;
        }
        if (WorldResult.ArtifactPath.IsEmpty())
        {
            FRBSaveOperationResult Result = CoordResult(false,
                TEXT("world backend returned no artifact path"));
            Result.BackendId = BackendId;
            Result.Generation = static_cast<int64>(Generation);
            Callback.ExecuteIfBound(Result);
            return;
        }

        FString WorldPortable;
        FString DomainPortable;
        FString PathError;
        if (!MakePortableSavedPath(WorldResult.ArtifactPath, WorldPortable, PathError) ||
            !MakePortableSavedPath(DomainPath, DomainPortable, PathError))
        {
            Callback.ExecuteIfBound(CoordResult(false, PathError));
            return;
        }
        const rb::save::ArtifactVerifier Verifier = MakeArtifactVerifier();
        Async(EAsyncExecution::ThreadPool,
            [WeakThis, Callback, Root, SafeSlot, Generation, GenerationDir, DomainPath,
             Domains = MoveTemp(Domains), BackendId, WorldPhysicalSlot,
             WorldPath = WorldResult.ArtifactPath, WorldPortable, DomainPortable, Verifier]() mutable
        {
            std::error_code DirectoryError;
            std::filesystem::create_directories(std::filesystem::path(*GenerationDir), DirectoryError);
            if (DirectoryError)
            {
                FRBSaveOperationResult Result = CoordResult(false,
                    TEXT("unable to create generation directory"));
                AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable
                {
                    Callback.ExecuteIfBound(Result);
                });
                return;
            }

            const auto DomainWrite = rb::save::WriteAtomic(
                std::filesystem::path(*DomainPath), Domains);
            if (!DomainWrite.ok)
            {
                FRBSaveOperationResult Result = CoordResult(false,
                    TEXT("domain write failed: ") + FromUtf8Coord(DomainWrite.error));
                AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable
                {
                    Callback.ExecuteIfBound(Result);
                });
                return;
            }
            const auto DomainFingerprint = rb::save::FingerprintFile(
                std::filesystem::path(*DomainPath));
            const auto WorldFingerprint = rb::save::FingerprintFile(
                std::filesystem::path(*WorldPath));
            if (!DomainFingerprint.ok || !WorldFingerprint.ok)
            {
                FRBSaveOperationResult Result = CoordResult(false,
                    TEXT("artifact fingerprint failed"));
                AsyncTask(ENamedThreads::GameThread, [Callback, Result]() mutable
                {
                    Callback.ExecuteIfBound(Result);
                });
                return;
            }

            GenerationManifest Manifest;
            Manifest.formatVersion = 2;
            Manifest.generation = Generation;
            Manifest.createdUtcMs = Domains.createdUtcMs;
            Manifest.logicalSlot = ToUtf8Coord(SafeSlot);
            Manifest.artifacts.emplace_back("domain", "RBSave", "domain",
                ToUtf8Coord(DomainPortable), DomainFingerprint.bytes,
                DomainFingerprint.checksum);
            Manifest.artifacts.emplace_back("world", ToUtf8Coord(BackendId.ToString()),
                ToUtf8Coord(WorldPhysicalSlot), ToUtf8Coord(WorldPortable),
                WorldFingerprint.bytes, WorldFingerprint.checksum);

            const auto Commit = rb::save::CommitGeneration(
                std::filesystem::path(*Root), Manifest, Verifier);
            FRBSaveOperationResult Result = CoordResult(Commit.ok,
                Commit.ok ? TEXT("coordinated save complete")
                          : TEXT("generation commit failed: ") + FromUtf8Coord(Commit.error));
            Result.Generation = static_cast<int64>(Generation);
            Result.Sequence = static_cast<int64>(Generation);
            Result.BackendId = BackendId;
            Result.ArtifactPath = WorldPath;
            Result.Bytes = static_cast<int64>(DomainFingerprint.bytes + WorldFingerprint.bytes);
            Result.Checksum = static_cast<int64>(Commit.manifestChecksum);
            AsyncTask(ENamedThreads::GameThread, [WeakThis, Callback, Result]() mutable
            {
                if (!WeakThis.IsValid())
                {
                    FRBSaveOperationResult Dead = CoordResult(false, TEXT("save subsystem destroyed"));
                    Callback.ExecuteIfBound(Dead);
                    return;
                }
                Callback.ExecuteIfBound(Result);
            });
        });
    };

    if (Backend.IsValid())
    {
        Backend->Save(GetWorld(), WorldPhysicalSlot, MoveTemp(FinalizeWorldSave));
        return;
    }

    SaveWorldCpp(WorldPhysicalSlot,
        [this, WorldPhysicalSlot, FinalizeWorldSave = MoveTemp(FinalizeWorldSave)]
        (FRBSaveOperationResult NativeResult) mutable
    {
        FRBSaveWorldBackendResult Result;
        Result.bSuccess = NativeResult.bSuccess;
        Result.Message = NativeResult.Message;
        Result.PhysicalSlot = WorldPhysicalSlot;
        Result.ArtifactPath = GetSlotPath(WorldPhysicalSlot);
        Result.Bytes = NativeResult.Bytes;
        Result.Checksum = static_cast<uint32>(NativeResult.Checksum);
        FinalizeWorldSave(MoveTemp(Result));
    });
}

namespace {
using FCoordinatorCompletion = TFunction<void(FRBSaveOperationResult)>;

void CompleteOnGameThread(FCoordinatorCompletion Completion, FRBSaveOperationResult Result)
{
    if (IsInGameThread())
    {
        if (Completion) Completion(MoveTemp(Result));
        return;
    }
    AsyncTask(ENamedThreads::GameThread,
        [Completion = MoveTemp(Completion), Result = MoveTemp(Result)]() mutable
    {
        if (Completion) Completion(MoveTemp(Result));
    });
}

bool LoadManifestAtGeneration(const FString& Root, uint64 Generation,
                              GenerationManifest& OutManifest, FString& Error)
{
    const auto Loaded = rb::save::ReadFile(rb::save::GenerationManifestPath(
        std::filesystem::path(*Root), Generation));
    if (!Loaded.ok)
    {
        Error = TEXT("unable to read generation manifest: ") + FromUtf8Coord(Loaded.error);
        return false;
    }
    std::string CoreError;
    if (!rb::save::ManifestFromSnapshot(Loaded.snapshot, OutManifest, CoreError))
    {
        Error = TEXT("invalid generation manifest: ") + FromUtf8Coord(CoreError);
        return false;
    }
    if (OutManifest.generation != Generation)
    {
        Error = TEXT("generation manifest number mismatch");
        return false;
    }
    const rb::save::ArtifactVerifier Verifier = MakeArtifactVerifier();
    for (const GenerationArtifact& Artifact : OutManifest.artifacts)
    {
        std::string VerifyError;
        if (!Verifier(Artifact, VerifyError))
        {
            Error = FString::Printf(TEXT("artifact '%s' invalid: %s"),
                *FromUtf8Coord(Artifact.role), *FromUtf8Coord(VerifyError));
            return false;
        }
    }
    return true;
}
void ExecuteGenerationLoad(URBSaveSubsystem* Subsystem, const FString& Root,
                           GenerationManifest Manifest, bool bActivateAfterLoad,
                           FCoordinatorCompletion Completion)
{
    if (!IsValid(Subsystem))
    {
        CompleteOnGameThread(MoveTemp(Completion),
            CoordResult(false, TEXT("save subsystem unavailable")));
        return;
    }
    const GenerationArtifact* WorldArtifact = FindArtifact(Manifest, "world");
    const GenerationArtifact* DomainArtifact = FindArtifact(Manifest, "domain");
    if (!WorldArtifact || !DomainArtifact)
    {
        CompleteOnGameThread(MoveTemp(Completion),
            CoordResult(false, TEXT("generation must contain world and domain artifacts")));
        return;
    }

    const FString DomainPath = ManifestArtifactPath(*DomainArtifact);
    const FString WorldPath = ManifestArtifactPath(*WorldArtifact);
    if (DomainPath.IsEmpty() || WorldPath.IsEmpty())
    {
        CompleteOnGameThread(MoveTemp(Completion),
            CoordResult(false, TEXT("generation contains invalid artifact path")));
        return;
    }
    const FName BackendId(FromUtf8Coord(WorldArtifact->backend));
    const FString PhysicalSlot = FromUtf8Coord(WorldArtifact->locator);
    const uint64 Generation = Manifest.generation;
    const int64 TotalBytes = static_cast<int64>(WorldArtifact->bytes + DomainArtifact->bytes);
    TWeakObjectPtr<URBSaveSubsystem> WeakThis(Subsystem);
    auto AfterWorldLoad =
        [WeakThis, Root, Manifest = MoveTemp(Manifest), DomainPath, WorldPath,
         BackendId, Generation, TotalBytes, bActivateAfterLoad,
         Completion = MoveTemp(Completion)](FRBSaveWorldBackendResult WorldResult) mutable
    {
        if (!WorldResult.bSuccess)
        {
            FRBSaveOperationResult Result = CoordResult(false,
                TEXT("world backend load failed: ") + WorldResult.Message);
            Result.BackendId = BackendId;
            Result.Generation = static_cast<int64>(Generation);
            CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
            return;
        }

        Async(EAsyncExecution::ThreadPool,
            [WeakThis, Root, Manifest = MoveTemp(Manifest), DomainPath, WorldPath,
             BackendId, Generation, TotalBytes, bActivateAfterLoad,
             Completion = MoveTemp(Completion)]() mutable
        {
            auto Domains = rb::save::ReadFile(std::filesystem::path(*DomainPath));
            if (!Domains.ok)
            {
                FRBSaveOperationResult Result = CoordResult(false,
                    TEXT("domain read failed: ") + FromUtf8Coord(Domains.error));
                Result.BackendId = BackendId;
                Result.Generation = static_cast<int64>(Generation);
                CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
                return;
            }
            AsyncTask(ENamedThreads::GameThread,
                [WeakThis, Root, Manifest = MoveTemp(Manifest), Domains = MoveTemp(Domains),
                 WorldPath, BackendId, Generation, TotalBytes, bActivateAfterLoad,
                 Completion = MoveTemp(Completion)]() mutable
            {
                if (!WeakThis.IsValid())
                {
                    CompleteOnGameThread(MoveTemp(Completion),
                        CoordResult(false, TEXT("save subsystem destroyed")));
                    return;
                }
                FString RestoreError;
                if (!WeakThis->RestoreRegisteredDomains(Domains.snapshot, RestoreError))
                {
                    FRBSaveOperationResult Result = CoordResult(false,
                        TEXT("domain restore failed: ") + RestoreError);
                    Result.BackendId = BackendId;
                    Result.Generation = static_cast<int64>(Generation);
                    CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
                    return;
                }

                if (!bActivateAfterLoad)
                {
                    FRBSaveOperationResult Result = CoordResult(true, TEXT("coordinated load complete"));
                    Result.BackendId = BackendId;
                    Result.Generation = static_cast<int64>(Generation);
                    Result.Sequence = static_cast<int64>(Generation);
                    Result.ArtifactPath = WorldPath;
                    Result.Bytes = TotalBytes;
                    CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
                    return;
                }
                const rb::save::ArtifactVerifier Verifier = MakeArtifactVerifier();
                Async(EAsyncExecution::ThreadPool,
                    [Root, WorldPath, BackendId, Generation, TotalBytes,
                     Completion = MoveTemp(Completion), Verifier]() mutable
                {
                    const auto Activated = rb::save::ActivateGeneration(
                        std::filesystem::path(*Root), Generation, Verifier);
                    FRBSaveOperationResult Result = CoordResult(Activated.ok,
                        Activated.ok ? TEXT("rollback load complete")
                                     : TEXT("rollback activation failed: ") + FromUtf8Coord(Activated.error));
                    Result.BackendId = BackendId;
                    Result.Generation = static_cast<int64>(Generation);
                    Result.Sequence = static_cast<int64>(Generation);
                    Result.ArtifactPath = WorldPath;
                    Result.Bytes = TotalBytes;
                    Result.Checksum = static_cast<int64>(Activated.manifestChecksum);
                    CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
                });
            });
        });
    };

    if (BackendId == FName(NativeBackendId))
    {
        Subsystem->LoadWorldCpp(PhysicalSlot,
            [AfterWorldLoad = MoveTemp(AfterWorldLoad), PhysicalSlot, WorldPath]
            (FRBSaveOperationResult NativeResult) mutable
        {
            FRBSaveWorldBackendResult Result;
            Result.bSuccess = NativeResult.bSuccess;
            Result.Message = NativeResult.Message;
            Result.PhysicalSlot = PhysicalSlot;
            Result.ArtifactPath = WorldPath;
            Result.Bytes = NativeResult.Bytes;
            Result.Checksum = static_cast<uint32>(NativeResult.Checksum);
            AfterWorldLoad(MoveTemp(Result));
        });
        return;
    }
    TSharedPtr<IRBSaveWorldBackend> Backend = FRBSaveWorldBackendRegistry::Find(BackendId);
    FString AvailabilityError;
    if (!Backend.IsValid() || !Backend->IsAvailable(Subsystem->GetWorld(), AvailabilityError))
    {
        FRBSaveOperationResult Result = CoordResult(false,
            AvailabilityError.IsEmpty()
                ? FString::Printf(TEXT("saved backend '%s' unavailable"), *BackendId.ToString())
                : AvailabilityError);
        Result.BackendId = BackendId;
        Result.Generation = static_cast<int64>(Generation);
        CompleteOnGameThread(MoveTemp(Completion), MoveTemp(Result));
        return;
    }
    Backend->Load(Subsystem->GetWorld(), PhysicalSlot, MoveTemp(AfterWorldLoad));
}
} // namespace

void URBSaveSubsystem::LoadGameAsync(const FString& Slot,
                                     const FRBSaveOperationDelegate& Completion)
{
    const FString SafeSlot = SanitizeCoordSlot(Slot);
    const FString Root = GetGenerationRoot(SafeSlot);
    if (SafeSlot.IsEmpty() || Root.IsEmpty())
    {
        Completion.ExecuteIfBound(CoordResult(false, TEXT("invalid slot")));
        return;
    }
    FRBSaveOperationDelegate Callback = Completion;
    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    const rb::save::ArtifactVerifier Verifier = MakeArtifactVerifier();
    Async(EAsyncExecution::ThreadPool,
        [WeakThis, Callback, Root, SafeSlot, Verifier]() mutable
    {
        std::string CoreError;
        auto Active = rb::save::ReadActiveGeneration(
            std::filesystem::path(*Root), CoreError, Verifier);
        if (!Active)
        {
            CompleteOnGameThread(
                [Callback](FRBSaveOperationResult Result) mutable { Callback.ExecuteIfBound(Result); },
                CoordResult(false, TEXT("active generation invalid: ") + FromUtf8Coord(CoreError)));
            return;
        }
        if (Active->logicalSlot != ToUtf8Coord(SafeSlot))
        {
            CompleteOnGameThread(
                [Callback](FRBSaveOperationResult Result) mutable { Callback.ExecuteIfBound(Result); },
                CoordResult(false, TEXT("active generation logical slot mismatch")));
            return;
        }
        AsyncTask(ENamedThreads::GameThread,
            [WeakThis, Callback, Root, Manifest = MoveTemp(*Active)]() mutable
        {
            if (!WeakThis.IsValid())
            {
                Callback.ExecuteIfBound(CoordResult(false, TEXT("save subsystem destroyed")));
                return;
            }
            ExecuteGenerationLoad(WeakThis.Get(), Root, MoveTemp(Manifest), false,
                [Callback](FRBSaveOperationResult Result) mutable
                {
                    Callback.ExecuteIfBound(Result);
                });
        });
    });
}
void URBSaveSubsystem::RollbackToGenerationAsync(const FString& Slot, int64 Generation,
                                                 const FRBSaveOperationDelegate& Completion)
{
    const FString SafeSlot = SanitizeCoordSlot(Slot);
    const FString Root = GetGenerationRoot(SafeSlot);
    if (SafeSlot.IsEmpty() || Root.IsEmpty() || Generation <= 0)
    {
        Completion.ExecuteIfBound(CoordResult(false, TEXT("invalid slot or generation")));
        return;
    }

    FRBSaveOperationDelegate Callback = Completion;
    TWeakObjectPtr<URBSaveSubsystem> WeakThis(this);
    Async(EAsyncExecution::ThreadPool,
        [WeakThis, Callback, Root, SafeSlot, Generation]() mutable
    {
        GenerationManifest Manifest;
        FString Error;
        if (!LoadManifestAtGeneration(Root, static_cast<uint64>(Generation), Manifest, Error))
        {
            CompleteOnGameThread(
                [Callback](FRBSaveOperationResult Result) mutable { Callback.ExecuteIfBound(Result); },
                CoordResult(false, Error));
            return;
        }
        if (Manifest.logicalSlot != ToUtf8Coord(SafeSlot))
        {
            CompleteOnGameThread(
                [Callback](FRBSaveOperationResult Result) mutable { Callback.ExecuteIfBound(Result); },
                CoordResult(false, TEXT("rollback generation belongs to another logical slot")));
            return;
        }
        AsyncTask(ENamedThreads::GameThread,
            [WeakThis, Callback, Root, Manifest = MoveTemp(Manifest)]() mutable
        {
            if (!WeakThis.IsValid())
            {
                Callback.ExecuteIfBound(CoordResult(false, TEXT("save subsystem destroyed")));
                return;
            }
            ExecuteGenerationLoad(WeakThis.Get(), Root, MoveTemp(Manifest), true,
                [Callback](FRBSaveOperationResult Result) mutable
                {
                    Callback.ExecuteIfBound(Result);
                });
        });
    });
}
