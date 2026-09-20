#include "RBOptimizationActor.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

ARBOptimizationActor::ARBOptimizationActor()
{
    PrimaryActorTick.bCanEverTick=false;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Representation"));
    SetRootComponent(Mesh);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
bool ARBOptimizationActor::ExportForReturn_Implementation(FString& Payload) const
{
    Payload=DomainPayload;
    // A Blueprint class needs its own explicit consent, never automatic generic serialization.
    return bAllowReturnToInstance;
}
bool ARBOptimizationActor::PrepareForPool_Implementation()
{
    // Custom subclasses must explicitly implement their reset contract.
    return GetClass()==ARBOptimizationActor::StaticClass() && IsQuiescent();
}
bool ARBOptimizationActor::RestoreFromPool_Implementation(int64 NewStableId, const FString& Payload)
{
    // Custom subclasses must explicitly rebuild any transient state on reuse.
    if(GetClass()!=ARBOptimizationActor::StaticClass()) return false;
    StableId=NewStableId;
    DomainPayload=Payload;
    bGameplayPinned=false;
    bAllowReturnToInstance=true;
    return true;
}
bool ARBOptimizationActor::IsQuiescent() const
{
    if(!bAllowReturnToInstance || bGameplayPinned || GetAttachParentActor()) return false;
    TArray<AActor*> Attached; GetAttachedActors(Attached); if(!Attached.IsEmpty()) return false;
    TArray<UPhysicsConstraintComponent*> Constraints; GetComponents(Constraints);
    if(!Constraints.IsEmpty()) return false;
    TArray<UPrimitiveComponent*> Primitives; GetComponents(Primitives);
    for(const UPrimitiveComponent* P:Primitives)
        if(P->IsSimulatingPhysics()) return false; // Even sleeping bodies need a domain-specific handoff.
    return true;
}
