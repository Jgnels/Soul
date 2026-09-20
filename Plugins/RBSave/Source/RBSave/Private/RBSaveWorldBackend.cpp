#include "RBSaveWorldBackend.h"

#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"

namespace {
FCriticalSection GBackendMutex;
TMap<FName, TSharedPtr<IRBSaveWorldBackend>> GBackends;
}

bool FRBSaveWorldBackendRegistry::Register(const TSharedRef<IRBSaveWorldBackend>& Backend, FString& Error)
{
    Error.Reset();
    const FName Id = Backend->GetBackendId();
    if (Id.IsNone())
    {
        Error = TEXT("backend id cannot be None");
        return false;
    }
    FScopeLock Lock(&GBackendMutex);
    if (GBackends.Contains(Id))
    {
        Error = FString::Printf(TEXT("backend '%s' is already registered"), *Id.ToString());
        return false;
    }
    GBackends.Add(Id, Backend);
    return true;
}

void FRBSaveWorldBackendRegistry::Unregister(FName BackendId)
{
    FScopeLock Lock(&GBackendMutex);
    GBackends.Remove(BackendId);
}
TSharedPtr<IRBSaveWorldBackend> FRBSaveWorldBackendRegistry::Find(FName BackendId)
{
    FScopeLock Lock(&GBackendMutex);
    if (const TSharedPtr<IRBSaveWorldBackend>* Found = GBackends.Find(BackendId)) return *Found;
    return nullptr;
}

TArray<FName> FRBSaveWorldBackendRegistry::List()
{
    FScopeLock Lock(&GBackendMutex);
    TArray<FName> Result;
    GBackends.GetKeys(Result);
    Result.Sort(FNameLexicalLess());
    return Result;
}
