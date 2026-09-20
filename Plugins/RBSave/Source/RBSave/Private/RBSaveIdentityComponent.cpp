#include "RBSaveIdentityComponent.h"

URBSaveIdentityComponent::URBSaveIdentityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FGuid URBSaveIdentityComponent::GetOrCreatePersistentId()
{
    if (!PersistentId.IsValid())
    {
        PersistentId = FGuid::NewGuid();
    }
    return PersistentId;
}
