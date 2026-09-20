#include "RBOptimizationGroup.h"
#include "RBOptimizationActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace {
rbopt::Point ToPoint(const FVector& P) { return {P.X,P.Y,P.Z}; }
bool GoodTransform(const FTransform& T) {
    const FVector S=T.GetScale3D();
    return !T.ContainsNaN() && T.GetRotation().IsNormalized() && rbopt::valid(ToPoint(T.GetLocation()))
        && S.X>0 && S.Y>0 && S.Z>0 && S.X<=10000 && S.Y<=10000 && S.Z<=10000;
}
}
ARBOptimizationGroup::ARBOptimizationGroup()
{
    PrimaryActorTick.bCanEverTick=false;
    Instances=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instances"));
    SetRootComponent(Instances);
    Instances->SetMobility(EComponentMobility::Movable);
    Instances->SetAbsolute(true,true,true);
    Instances->SetRemoveSwap();
    MaterializedClass=ARBOptimizationActor::StaticClass();
}
bool ARBOptimizationGroup::CanOperate() const
{
    return GetWorld() && (GetNetMode()!=NM_Client || bReceivingAuthority) && IsInGameThread() && !bChanging
        && Instances->GetInstanceCount()==IndexToKey.Num();
}
double ARBOptimizationGroup::Now() const { return FPlatformTime::Seconds(); }
bool ARBOptimizationGroup::InitializeGroup(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, FName CollisionProfile)
{
    if(!CanOperate() || !Records.IsEmpty() || !Mesh || CollisionProfile.IsNone()
       || !FMath::IsFinite(SoftTransitionBudgetMs) || SoftTransitionBudgetMs<=0
       || MaxPooledActors<0 || MaxPooledActors>1024) return false;
    rbopt::Config C; C.enterRadius=ActivationRadius; C.exitRadius=RetirementRadius;
    C.minimumResidence=MinimumActiveSeconds; C.cellSize=RetirementRadius;
    C.maxChanges=MaxTransitionsPerUpdate>0?static_cast<size_t>(MaxTransitionsPerUpdate):0;
    auto NewPolicy=MakeUnique<rbopt::Registry>(C); if(!NewPolicy->usable()) return false;
    Policy=MoveTemp(NewPolicy); Profile=CollisionProfile;
    if(!CollectionId.IsValid()) CollectionId=FGuid::NewGuid();
    Instances->SetStaticMesh(Mesh); Instances->SetCollisionProfileName(Profile);
    for(int32 I=0;I<Materials.Num();++I) Instances->SetMaterial(I,Materials[I]);
    return true;
}
bool ARBOptimizationGroup::RegisterItem(int64 Id, const FTransform& Transform, const FString& Payload)
{
    if(!CanOperate() || !Policy || bMembershipFrozen || Id<=0 || !GoodTransform(Transform) || Records.Contains(Id)
       || Payload.Len()>1048576 || Policy->size()>=100000) return false;
    const FPrimitiveInstanceId Instance=Instances->AddInstanceById(Transform,true);
    if(!Instance.IsValid()) return false;
    ++InstanceRevision;
    if(!Policy->add(static_cast<rbopt::Id>(Id),ToPoint(Transform.GetLocation()))) {
        Instances->RemoveInstanceById(Instance); return false;
    }
    FRBOptimizationRecord Record; Record.Transform=Transform; Record.Payload=Payload; Record.Instance=Instance;
    Records.Add(Id,MoveTemp(Record)); IndexToKey.Add(Id); return true;
}
bool ARBOptimizationGroup::ApplyRepresentationPresentation(ARBOptimizationActor* A) const
{
    if(!IsValid(A) || !A->Mesh || !Instances || !Instances->GetStaticMesh()) return false;
    A->Mesh->SetStaticMesh(Instances->GetStaticMesh());
    for(int32 I=0;I<Instances->GetNumMaterials();++I) A->Mesh->SetMaterial(I,Instances->GetMaterial(I));
    A->Mesh->SetMobility(Instances->Mobility);
    A->Mesh->SetCastShadow(Instances->CastShadow);
    A->Mesh->SetCollisionProfileName(Profile);
    // The component setter compares the actor-masked mode and can skip NoCollision.
    // Configure the body while the actor remains safely disabled during staging.
    A->Mesh->BodyInstance.SetCollisionEnabled(Instances->GetCollisionEnabled());
    A->Mesh->SetCollisionObjectType(Instances->GetCollisionObjectType());
    A->Mesh->SetCollisionResponseToChannels(Instances->GetCollisionResponseToChannels());
    A->Mesh->SetGenerateOverlapEvents(Instances->GetGenerateOverlapEvents());
    A->Mesh->SetCanEverAffectNavigation(Instances->CanEverAffectNavigation());
    A->Mesh->SetReceivesDecals(Instances->bReceivesDecals);
    A->Mesh->SetRenderCustomDepth(Instances->bRenderCustomDepth);
    A->Mesh->SetCustomDepthStencilValue(Instances->CustomDepthStencilValue);
    A->Mesh->LightingChannels=Instances->LightingChannels;
    return true;
}
ARBOptimizationActor* ARBOptimizationGroup::SpawnFreshActor(int64 Id,const FString& Payload,const FTransform& Transform)
{
    FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn; P.bDeferConstruction=true;
    auto* A=GetWorld()->SpawnActor<ARBOptimizationActor>(MaterializedClass,Transform,P);
    if(!IsValid(A)) return nullptr;
    A->StableId=Id; A->DomainPayload=Payload; A->bGameplayPinned=false;
    if(A->GetClass()==ARBOptimizationActor::StaticClass()) A->bAllowReturnToInstance=true;
    A->SetActorHiddenInGame(true); A->SetActorEnableCollision(false);
    if(!ApplyRepresentationPresentation(A)) { A->Destroy(); return nullptr; }
    A->FinishSpawning(Transform);
    return IsValid(A)?A:nullptr;
}
ARBOptimizationActor* ARBOptimizationGroup::AcquirePooledActor(int64 Id,const FString& Payload,const FTransform& Transform)
{
    const int32 Limit=FMath::Clamp(MaxPooledActors,0,1024);
    while(ActorPool.Num()>Limit) {
        if(auto* A=ActorPool.Pop(EAllowShrinking::No).Get();IsValid(A)) A->Destroy();
        ++Diagnostics.PoolDiscards;
    }
    while(!ActorPool.IsEmpty()) {
        ARBOptimizationActor* A=ActorPool.Pop(EAllowShrinking::No);
        if(!IsValid(A)) { ++Diagnostics.PoolDiscards; continue; }
        if(A->GetWorld()!=GetWorld() || A->GetClass()!=MaterializedClass.Get()) { A->Destroy(); ++Diagnostics.PoolDiscards; continue; }
        A->SetActorHiddenInGame(true); A->SetActorEnableCollision(false);
        A->SetActorTransform(Transform,false,nullptr,ETeleportType::TeleportPhysics);
        A->StableId=Id; A->DomainPayload=Payload; A->bGameplayPinned=false;
        if(A->GetClass()==ARBOptimizationActor::StaticClass()) A->bAllowReturnToInstance=true;
        if(!ApplyRepresentationPresentation(A) || !A->RestoreFromPool(Id,Payload) || !IsValid(A)) { if(IsValid(A)) A->Destroy(); ++Diagnostics.PoolDiscards; continue; }
        ++Diagnostics.PoolHits; return A;
    }
    ++Diagnostics.PoolMisses; return nullptr;
}
bool ARBOptimizationGroup::ReturnActorToPool(ARBOptimizationActor* A)
{
    if(!IsValid(A)) return false;
    const int32 Limit=FMath::Clamp(MaxPooledActors,0,1024);
    A->SetActorEnableCollision(false); A->SetActorHiddenInGame(true);
    if(Limit<=0 || ActorPool.Num()>=Limit || !A->PrepareForPool() || !IsValid(A)) { if(IsValid(A)) A->Destroy(); ++Diagnostics.PoolDiscards; return false; }
    A->SetActorEnableCollision(false); A->SetActorHiddenInGame(true);
    A->SetActorTickEnabled(false);
    A->StableId=0; A->DomainPayload.Reset(); A->bGameplayPinned=false;
    ActorPool.Add(A); ++Diagnostics.PoolReturns; return true;
}
int32 ARBOptimizationGroup::PrewarmActorPool(int32 TargetPooledActors)
{
    if(!CanOperate() || !Policy || !MaterializedClass || MaterializedClass->HasAnyClassFlags(CLASS_Abstract)) return ActorPool.Num();
    TGuardValue<bool> Lock(bChanging,true);
    const int32 Target=FMath::Clamp(TargetPooledActors,0,FMath::Clamp(MaxPooledActors,0,1024));
    while(ActorPool.Num()<Target) {
        auto* A=SpawnFreshActor(0,FString(),FTransform::Identity);
        if(!IsValid(A) || !ReturnActorToPool(A)) break;
    }
    return ActorPool.Num();
}
int32 ARBOptimizationGroup::TrimActorPool(int32 KeepActors)
{
    if(!CanOperate()) return 0;
    TGuardValue<bool> Lock(bChanging,true);
    const int32 Keep=FMath::Clamp(KeepActors,0,ActorPool.Num()); int32 Removed=0;
    while(ActorPool.Num()>Keep) { if(ARBOptimizationActor* A=ActorPool.Pop(EAllowShrinking::No).Get();IsValid(A)) A->Destroy(); ++Removed; }
    return Removed;
}
bool ARBOptimizationGroup::Materialize(int64 Id)
{
    auto* R=Records.Find(Id); const auto* N=Policy?Policy->get(static_cast<rbopt::Id>(Id)):nullptr;
    if(!R || !N || N->representation!=rbopt::Representation::Instance || !R->Instance.IsValid()
       || !MaterializedClass || MaterializedClass->HasAnyClassFlags(CLASS_Abstract)) return false;
    const rbopt::Change Change{static_cast<rbopt::Id>(Id),N->revision,rbopt::Representation::Actor};
    TGuardValue<bool> Lock(bChanging,true);
    ARBOptimizationActor* A=AcquirePooledActor(Id,R->Payload,R->Transform);
    if(!A) A=SpawnFreshActor(Id,R->Payload,R->Transform);
    if(!IsValid(A)) return false;
    bool Compatible=A->Mesh && A->Mesh->GetStaticMesh()==Instances->GetStaticMesh() && A->GetActorTransform().Equals(R->Transform,0.001)
        && A->Mesh->GetNumMaterials()==Instances->GetNumMaterials() && A->Mesh->Mobility==Instances->Mobility
        // The staging actor is collision-disabled until commit. Compare its configured
        // body mode, not the effective mode masked by that temporary actor override.
        && A->Mesh->CastShadow==Instances->CastShadow && A->Mesh->BodyInstance.GetCollisionEnabled(false)==Instances->GetCollisionEnabled()
        && A->Mesh->GetCollisionObjectType()==Instances->GetCollisionObjectType()
        && A->Mesh->GetCollisionResponseToChannels()==Instances->GetCollisionResponseToChannels()
        && A->Mesh->GetGenerateOverlapEvents()==Instances->GetGenerateOverlapEvents()
        && A->Mesh->CanEverAffectNavigation()==Instances->CanEverAffectNavigation()
        && A->Mesh->bReceivesDecals==Instances->bReceivesDecals && A->Mesh->bRenderCustomDepth==Instances->bRenderCustomDepth
        && A->Mesh->CustomDepthStencilValue==Instances->CustomDepthStencilValue
        && A->Mesh->LightingChannels.bChannel0==Instances->LightingChannels.bChannel0
        && A->Mesh->LightingChannels.bChannel1==Instances->LightingChannels.bChannel1
        && A->Mesh->LightingChannels.bChannel2==Instances->LightingChannels.bChannel2;
    if(Compatible) for(int32 I=0;I<Instances->GetNumMaterials();++I) Compatible=Compatible && A->Mesh->GetMaterial(I)==Instances->GetMaterial(I);
    if(!Compatible) { A->Destroy(); ++Diagnostics.PoolDiscards; return false; }
    // No source instance is removed until the replacement exists.
    if(!Policy->commit(Change,ToPoint(R->Transform.GetLocation()),Now())) { A->Destroy(); ++Diagnostics.PoolDiscards; return false; }
    RemoveProxy(*R); R->Actor=A;
    A->SetActorHiddenInGame(false); A->SetActorEnableCollision(true); return true;
}
ARBOptimizationActor* ARBOptimizationGroup::ActivateItem(int64 Id, double HoldSeconds)
{
    if(!CanOperate() || !Policy || !FMath::IsFinite(HoldSeconds) || HoldSeconds<=0) return nullptr;
    auto* R=Records.Find(Id); if(!R || !Policy->request(static_cast<rbopt::Id>(Id),Now(),HoldSeconds)) return nullptr;
    if(IsValid(R->Actor)) return R->Actor;
    if(!Materialize(Id)) return nullptr;
    ARBOptimizationActor* Result=R->Actor;
    OnRepresentationChanged.Broadcast(Id,ERBOptRepresentation::Actor);
    return IsValid(Result)?Result:nullptr;
}
ARBOptimizationActor* ARBOptimizationGroup::ActivateHit(const FHitResult& Hit, int64 ExpectedInstanceRevision, double HoldSeconds)
{
    if(!bHitActivation || !CanOperate() || ExpectedInstanceRevision!=InstanceRevision || Hit.GetComponent()!=Instances
        || !IndexToKey.IsValidIndex(Hit.Item)) return nullptr;
    // Capture revision BEFORE tracing. Refuse an old hit after any proxy list change.
    const int64 Id=IndexToKey[Hit.Item];
    const FRBOptimizationRecord* R=Records.Find(Id);
    if(!R || Instances->GetInstanceIndexForId(R->Instance)!=Hit.Item) return nullptr;
    return ActivateItem(Id,HoldSeconds);
}
bool ARBOptimizationGroup::SetPinned(int64 Id, bool bPinned)
{
    if(!CanOperate() || !Policy || !Records.Contains(Id)) return false;
    const auto* N=Policy->get(static_cast<rbopt::Id>(Id));
    if(!N || N->representation==rbopt::Representation::Consumed) return false;
    if(bPinned) DomainPins.Add(Id); else DomainPins.Remove(Id);
    Policy->guard(static_cast<rbopt::Id>(Id),bPinned?1:0,false);
    if(bPinned && N->representation==rbopt::Representation::Instance) return ActivateItem(Id)!=nullptr;
    return true;
}
bool ARBOptimizationGroup::ConsumeItem(int64 Id)
{
    if(!CanOperate() || !Policy) return false; auto* R=Records.Find(Id); if(!R) return false;
    if(IsValid(R->Actor)) {
        if(!GoodTransform(R->Actor->GetActorTransform()) || R->Actor->DomainPayload.Len()>1048576) return false;
        R->Transform=R->Actor->GetActorTransform(); R->Payload=R->Actor->DomainPayload;
    }
    if(!Policy->consume(static_cast<rbopt::Id>(Id),Now())) return false;
    { TGuardValue<bool> Lock(bChanging,true);
      if(R->Instance.IsValid()) RemoveProxy(*R);
      if(IsValid(R->Actor)) R->Actor->Destroy(); R->Actor=nullptr; DomainPins.Remove(Id); }
    OnRepresentationChanged.Broadcast(Id,ERBOptRepresentation::Consumed);
    return true; // Tombstone remains; duplicate registration cannot resurrect consumed content.
}
bool ARBOptimizationGroup::Retire(int64 Id,const rbopt::Change& Change)
{
    auto* R=Records.Find(Id); if(!R || !IsValid(R->Actor) || !R->Actor->IsQuiescent()) return false;
    ARBOptimizationActor* A=R->Actor; if(A->Mesh->GetStaticMesh()!=Instances->GetStaticMesh()) return false;
    if(A->Mesh->GetNumMaterials()!=Instances->GetNumMaterials()) return false;
    for(int32 I=0;I<Instances->GetNumMaterials();++I) if(A->Mesh->GetMaterial(I)!=Instances->GetMaterial(I)) return false;
    if(A->Mesh->Mobility!=Instances->Mobility || A->Mesh->CastShadow!=Instances->CastShadow
       || A->Mesh->GetCollisionEnabled()!=Instances->GetCollisionEnabled()
       || A->Mesh->GetCollisionObjectType()!=Instances->GetCollisionObjectType()
       || A->Mesh->GetGenerateOverlapEvents()!=Instances->GetGenerateOverlapEvents()
       || A->Mesh->CanEverAffectNavigation()!=Instances->CanEverAffectNavigation()
       || A->Mesh->GetCollisionResponseToChannels()!=Instances->GetCollisionResponseToChannels()
       || A->Mesh->bReceivesDecals!=Instances->bReceivesDecals || A->Mesh->bRenderCustomDepth!=Instances->bRenderCustomDepth
       || A->Mesh->CustomDepthStencilValue!=Instances->CustomDepthStencilValue
       || A->Mesh->LightingChannels.bChannel0!=Instances->LightingChannels.bChannel0
       || A->Mesh->LightingChannels.bChannel1!=Instances->LightingChannels.bChannel1
       || A->Mesh->LightingChannels.bChannel2!=Instances->LightingChannels.bChannel2) return false;
    TGuardValue<bool> Lock(bChanging,true); FString Payload;
    if(!A->ExportForReturn(Payload) || !IsValid(A) || !A->IsQuiescent() || Payload.Len()>1048576) return false;
    const FTransform Transform=A->GetActorTransform(); if(!GoodTransform(Transform)) return false;
    const FPrimitiveInstanceId NewInstance=Instances->AddInstanceById(Transform,true);
    if(!NewInstance.IsValid()) return false;
    ++InstanceRevision;
    if(!Policy->commit(Change,ToPoint(Transform.GetLocation()),Now())) { Instances->RemoveInstanceById(NewInstance); return false; }
    R->Transform=Transform; R->Payload=Payload; R->Instance=NewInstance; IndexToKey.Add(Id);
    R->Actor=nullptr; ReturnActorToPool(A); return true;
}
void ARBOptimizationGroup::UpdateViewers(const TArray<FVector>& Locations)
{
    if(!CanOperate() || !Policy || Locations.Num()>32) return;
    std::vector<rbopt::Point> Views; for(const FVector& V:Locations) Views.push_back(ToPoint(V));
    for(const rbopt::Point& V:Views) if(!rbopt::valid(V)) return;
    for(const rbopt::Id Id:Policy->activeIds()) {
        auto* R=Records.Find(static_cast<int64>(Id));
        if(!R || !IsValid(R->Actor)) { if(Policy->consume(Id,Now())) OnRepresentationChanged.Broadcast(static_cast<int64>(Id),ERBOptRepresentation::Consumed); continue; }
        // Only active actors are polled. Dormant populations are spatially queried.
        Policy->updatePosition(Id,ToPoint(R->Actor->GetActorLocation()));
        Policy->guard(Id,DomainPins.Contains(static_cast<int64>(Id))||R->Actor->bGameplayPinned?1:0,R->Actor->IsQuiescent());
    }
    Policy->expireRequests(Now());
    const auto Plan=Policy->plan(Views,Now()); if(!Plan.validInput) return;
    Diagnostics.LastInspected=static_cast<int32>(Plan.inspected); Diagnostics.LastTransitions=0;
    const double Start=FPlatformTime::Seconds();
    for(const auto& Change:Plan.changes) {
        if((FPlatformTime::Seconds()-Start)*1000.0>=SoftTransitionBudgetMs) break;
        const bool Changed=Change.to==rbopt::Representation::Actor ? Materialize(static_cast<int64>(Change.id)) : Retire(static_cast<int64>(Change.id),Change);
        if(Changed) { ++Diagnostics.LastTransitions; OnRepresentationChanged.Broadcast(static_cast<int64>(Change.id),Change.to==rbopt::Representation::Actor?ERBOptRepresentation::Actor:ERBOptRepresentation::Instance); }
    }
    Diagnostics.LastUpdateMs=(FPlatformTime::Seconds()-Start)*1000.0;
}
void ARBOptimizationGroup::PollPlayers()
{
    if(!bAutomaticProximity) return; TArray<FVector> Views;
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It) {
        const APlayerController* PC=It->Get(); if(PC && PC->PlayerCameraManager) Views.Add(PC->PlayerCameraManager->GetCameraLocation());
    }
    UpdateViewers(Views);
}
void ARBOptimizationGroup::BeginPlay() { Super::BeginPlay(); if(GetNetMode()!=NM_Client) GetWorldTimerManager().SetTimer(Timer,this,&ARBOptimizationGroup::PollPlayers,0.1f,true); }
void ARBOptimizationGroup::EndPlay(const EEndPlayReason::Type Reason) {
    GetWorldTimerManager().ClearTimer(Timer); bChanging=true;
    for(auto& Entry:Records) if(IsValid(Entry.Value.Actor)) Entry.Value.Actor->Destroy();
    for(const TObjectPtr<ARBOptimizationActor>& Pooled:ActorPool) if(IsValid(Pooled.Get())) Pooled->Destroy();
    ActorPool.Empty();
    Super::EndPlay(Reason);
}
int32 ARBOptimizationGroup::GetActiveCount() const { return Policy?static_cast<int32>(Policy->activeCount()):0; }
int32 ARBOptimizationGroup::GetRegisteredCount() const { return Policy?static_cast<int32>(Policy->size()):0; }

void ARBOptimizationGroup::RemoveProxy(FRBOptimizationRecord& Record)
{
    const int32 Index=Instances->GetInstanceIndexForId(Record.Instance);
    check(IndexToKey.IsValidIndex(Index));
    Instances->RemoveInstanceById(Record.Instance);
    IndexToKey.RemoveAtSwap(Index,1,EAllowShrinking::No);
    Record.Instance=FPrimitiveInstanceId(); ++InstanceRevision;
}
