#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

class UWorld;

struct RBSAVE_API FRBSaveWorldBackendResult
{
    bool bSuccess = false;
    FString Message;
    FString PhysicalSlot;
    FString ArtifactPath;
    int64 Bytes = 0;
    uint32 Checksum = 0;
};

using FRBSaveWorldBackendCompletion = TFunction<void(FRBSaveWorldBackendResult)>;

class RBSAVE_API IRBSaveWorldBackend
{
public:
    virtual ~IRBSaveWorldBackend() = default;
    virtual FName GetBackendId() const = 0;
    virtual bool IsAvailable(const UWorld* World, FString& Error) const = 0;
    virtual void Save(UWorld* World, const FString& PhysicalSlot,
                      FRBSaveWorldBackendCompletion Completion) = 0;
    virtual void Load(UWorld* World, const FString& PhysicalSlot,
                      FRBSaveWorldBackendCompletion Completion) = 0;
    virtual FString GetArtifactPath(const FString& PhysicalSlot) const = 0;
};
class RBSAVE_API FRBSaveWorldBackendRegistry
{
public:
    static bool Register(const TSharedRef<IRBSaveWorldBackend>& Backend, FString& Error);
    static void Unregister(FName BackendId);
    static TSharedPtr<IRBSaveWorldBackend> Find(FName BackendId);
    static TArray<FName> List();
};
