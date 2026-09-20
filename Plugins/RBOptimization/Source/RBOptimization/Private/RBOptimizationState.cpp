#include "RBOptimizationState.h"
#include "RBOptimizationGroup.h"
#include "RBOptimizationActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace {
FString PresentationSignature(const UInstancedStaticMeshComponent* C) {
    FString Result=FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d:%d:%d"),int32(C->Mobility),int32(C->CastShadow),int32(C->GetCollisionEnabled()),
        int32(C->bReceivesDecals),int32(C->bRenderCustomDepth),C->CustomDepthStencilValue,
        int32(C->LightingChannels.bChannel0),int32(C->LightingChannels.bChannel1),int32(C->LightingChannels.bChannel2));
    Result+=FString::Printf(TEXT(":%d:%d:%d:%d"),int32(C->Mobility),int32(C->GetCollisionObjectType()),int32(C->GetGenerateOverlapEvents()),int32(C->CanEverAffectNavigation()));
    for(int32 Channel=0;Channel<ECC_MAX;++Channel) Result+=FString::Printf(TEXT(":%d"),int32(C->GetCollisionResponseToChannel(ECollisionChannel(Channel))));
    return Result;
}

bool StateTransformValid(const FTransform& T) {
    const FVector S=T.GetScale3D(),P=T.GetLocation();
    return !T.ContainsNaN() && T.GetRotation().IsNormalized() && rbopt::valid({P.X,P.Y,P.Z})
        && S.X>0 && S.Y>0 && S.Z>0 && S.X<=10000 && S.Y<=10000 && S.Z<=10000;
}
ERBOptRepresentation ToSaved(rbopt::Representation R) {
    return R==rbopt::Representation::Instance?ERBOptRepresentation::Instance:
        R==rbopt::Representation::Actor?ERBOptRepresentation::Actor:ERBOptRepresentation::Consumed;
}
}
bool ARBOptimizationGroup::InitializeFromTable(UDataTable* Table,FName RowName,FString& Error)
{
    Error.Reset();
    if(!CanOperate() || !Records.IsEmpty() || !Table || Table->GetRowStruct()!=FRBOptPresetRow::StaticStruct()) {
        Error=TEXT("A fresh group and the exact preset row type are required"); return false;
    }
    const FRBOptPresetRow* Row=Table->FindRow<FRBOptPresetRow>(RowName,TEXT("RB Optimization"),false);
    if(!Row || !Row->Mesh || !Row->ActorClass || Row->ActorClass->HasAnyClassFlags(CLASS_Abstract)) {
        Error=TEXT("Missing row, mesh or concrete actor class"); return false;
    }
    const double OldEnter=ActivationRadius,OldExit=RetirementRadius;
    ActivationRadius=Row->ActivationRadius; RetirementRadius=Row->RetirementRadius;
    TArray<UMaterialInterface*> Materials; for(const auto& M:Row->Materials) Materials.Add(M.Get());
    if(!InitializeGroup(Row->Mesh,Materials,Row->CollisionProfile)) {
        ActivationRadius=OldEnter; RetirementRadius=OldExit; Error=TEXT("Invalid preset parameters"); return false;
    }
    MaterializedClass=Row->ActorClass; bAutomaticProximity=Row->bProximityActivation; bHitActivation=Row->bHitActivation;
    return true;
}
bool ARBOptimizationGroup::GetItemState(int64 Id,FRBOptSavedItem& Out) const
{
    if(!IsInGameThread() || !GetWorld() || bChanging || !Policy) return false;
    const auto* R=Records.Find(Id); const auto* N=Policy->get(static_cast<rbopt::Id>(Id));
    if(!R || !N) return false;
    FRBOptSavedItem S; S.Id=Id; S.Transform=R->Transform; S.Payload=R->Payload; S.State=ToSaved(N->representation);
    S.bPinned=DomainPins.Contains(Id); S.RemainingHoldSeconds=FMath::Max(0.0,N->keepUntil-Now());
    S.RespawnRemainingSeconds=R->RespawnRemainingSeconds;
    if(S.State==ERBOptRepresentation::Actor) {
        if(!IsValid(R->Actor)) return false;
        S.Transform=R->Actor->GetActorTransform(); S.Payload=R->Actor->DomainPayload;
        S.bPinned=S.bPinned || R->Actor->bGameplayPinned;
        S.RemainingHoldSeconds=FMath::Max(S.RemainingHoldSeconds,FMath::Max(0.0,N->lastChange+MinimumActiveSeconds-Now()));
    }
    Out=MoveTemp(S); return true;
}
bool ARBOptimizationGroup::CaptureSnapshot(FRBOptSnapshot& Out,FString& Error)
{
    Error.Reset();
    if(!CanOperate() || !Policy || !MaterializedClass || !Instances->GetStaticMesh()) {
        Error=TEXT("Initialized group required"); return false;
    }
    FRBOptSnapshot S; S.CollectionId=CollectionId; S.MeshPath=Instances->GetStaticMesh()->GetPathName();
    S.ClassPath=MaterializedClass->GetPathName(); S.CollisionProfile=Profile; S.PresentationSignature=PresentationSignature(Instances);
    for(int32 I=0;I<Instances->GetNumMaterials();++I) S.MaterialPaths.Add(GetPathNameSafe(Instances->GetMaterial(I)));
    TArray<int64> Keys; Records.GetKeys(Keys); Keys.Sort(); int64 PayloadBytes=0;
    for(const int64 Id:Keys) {
        FRBOptSavedItem Item; if(!GetItemState(Id,Item)) {Error=TEXT("Invalid actor lifetime; synchronize group first"); return false;}
        if(Item.State==ERBOptRepresentation::Actor) {
            // Arbitrary physics, attachments, latent state and custom gameplay are not silently flattened.
            auto* A=Records.FindChecked(Id).Actor.Get(); FString Export;
            if(!A->IsQuiescent()) {Error=TEXT("Active actor has state that this save adapter cannot represent"); return false;}
            { TGuardValue<bool> Lock(bChanging,true);
              if(!A->ExportForReturn(Export) || !IsValid(A) || !A->IsQuiescent()) {Error=TEXT("Actor declined snapshot export"); return false;} }
            Item.Payload=MoveTemp(Export); Item.Transform=A->GetActorTransform();
        }
        PayloadBytes+=static_cast<int64>(Item.Payload.Len())*sizeof(TCHAR);
        if(!StateTransformValid(Item.Transform) || Item.Payload.Len()>1048576 || PayloadBytes>64*1024*1024) {
            Error=TEXT("Snapshot exceeds valid transform or bounded payload limits"); return false;
        }
        S.Items.Add(MoveTemp(Item));
    }
    Out=MoveTemp(S); return true;
}
bool ARBOptimizationGroup::RestoreIntoFreshGroup(const FRBOptSnapshot& S,FString& Error)
{
    Error.Reset();
    if(!CanOperate() || !Policy || !Records.IsEmpty() || S.SchemaVersion!=3 || !S.CollectionId.IsValid()
       || S.CollectionId!=CollectionId || !Instances->GetStaticMesh() || !MaterializedClass
       || S.MeshPath!=Instances->GetStaticMesh()->GetPathName() || S.ClassPath!=MaterializedClass->GetPathName()
       || S.PresentationSignature!=PresentationSignature(Instances) || S.CollisionProfile!=Profile || S.MaterialPaths.Num()!=Instances->GetNumMaterials() || S.Items.Num()>100000) {
        Error=TEXT("Fresh initialized group, schema and exact collection/asset identity required"); return false;
    }
    for(int32 I=0;I<S.MaterialPaths.Num();++I) if(S.MaterialPaths[I]!=GetPathNameSafe(Instances->GetMaterial(I))) {
        Error=TEXT("Material identity mismatch"); return false;
    }
    TSet<int64> Seen; int64 Bytes=0;
    for(const auto& Item:S.Items) {
        Bytes+=static_cast<int64>(Item.Payload.Len())*sizeof(TCHAR);
        const bool Consumed=Item.State==ERBOptRepresentation::Consumed;
        if(Item.Id<=0 || Seen.Contains(Item.Id) || !StateTransformValid(Item.Transform) || Item.Payload.Len()>1048576
           || Bytes>64*1024*1024 || !FMath::IsFinite(Item.RemainingHoldSeconds) || Item.RemainingHoldSeconds<0
           || Item.RemainingHoldSeconds>315576000 || !FMath::IsFinite(Item.RespawnRemainingSeconds)
           || Item.RespawnRemainingSeconds < -1 || Item.RespawnRemainingSeconds>315576000
           || (Item.RespawnRemainingSeconds>=0 && !Consumed)
           || (Consumed && (Item.bPinned || Item.RemainingHoldSeconds>0))
           || (Item.State!=ERBOptRepresentation::Instance && Item.State!=ERBOptRepresentation::Actor && !Consumed)) {
            Error=TEXT("Invalid, duplicate, contradictory or oversized item; no state changed"); return false;
        }
        Seen.Add(Item.Id);
    }
    // Validate before mutation, and roll back only this previously empty collection on staging failure.
    // Custom actor BeginPlay side effects are outside this transaction: opt-in actors must be staging-safe.
    auto Rollback=[this]() {
        TGuardValue<bool> Lock(bChanging,true);
        for(auto& Pair:Records) if(IsValid(Pair.Value.Actor)) Pair.Value.Actor->Destroy();
        Records.Empty(); DomainPins.Empty(); IndexToKey.Empty(); Instances->ClearInstances(); ++InstanceRevision;
        rbopt::Config C; C.enterRadius=ActivationRadius; C.exitRadius=RetirementRadius; C.cellSize=RetirementRadius;
        C.minimumResidence=MinimumActiveSeconds; C.maxChanges=static_cast<size_t>(MaxTransitionsPerUpdate);
        Policy=MakeUnique<rbopt::Registry>(C);
    };
    for(const auto& Item:S.Items) {
        if(!RegisterItem(Item.Id,Item.Transform,Item.Payload)) {Rollback();Error=TEXT("Registration failed; fresh group rolled back");return false;}
        if(Item.State==ERBOptRepresentation::Consumed) {
            if(!ConsumeItem(Item.Id)){Rollback();Error=TEXT("Tombstone restore failed");return false;}
            Records.FindChecked(Item.Id).RespawnRemainingSeconds=Item.RespawnRemainingSeconds;
        } else {
            if(Item.State==ERBOptRepresentation::Actor && !Materialize(Item.Id)) {Rollback();Error=TEXT("Actor restore failed; fresh group rolled back");return false;}
            if(Item.RemainingHoldSeconds>0) Policy->request(static_cast<rbopt::Id>(Item.Id),Now(),Item.RemainingHoldSeconds);
            if(Item.bPinned) { DomainPins.Add(Item.Id); Policy->guard(static_cast<rbopt::Id>(Item.Id),1,false); }
        }
    }
    return true;
}
bool ARBOptimizationGroup::ScheduleRespawn(int64 Id,double Delay)
{
    if(!CanOperate() || !Policy || !FMath::IsFinite(Delay) || Delay<0 || Delay>315576000) return false;
    const auto* N=Policy->get(static_cast<rbopt::Id>(Id)); auto* R=Records.Find(Id);
    if(!R || !N || N->representation!=rbopt::Representation::Consumed) return false;
    R->RespawnRemainingSeconds=Delay; return true;
}
bool ARBOptimizationGroup::RespawnItem(int64 Id,const FTransform& Transform,const FString& Payload)
{
    if(!CanOperate() || !Policy || !StateTransformValid(Transform) || Payload.Len()>1048576) return false;
    auto* R=Records.Find(Id); const auto* N=Policy->get(static_cast<rbopt::Id>(Id));
    if(!R || !N || N->representation!=rbopt::Representation::Consumed) return false;
    const FPrimitiveInstanceId NewId=Instances->AddInstanceById(Transform,true);
    if(!NewId.IsValid()) return false;
    if(!Policy->revive(static_cast<rbopt::Id>(Id),{Transform.GetLocation().X,Transform.GetLocation().Y,Transform.GetLocation().Z},Now())) {
        Instances->RemoveInstanceById(NewId); ++InstanceRevision; return false;
    }
    R->Transform=Transform; R->Payload=Payload; R->Instance=NewId; R->RespawnRemainingSeconds=-1;
    IndexToKey.Add(Id); ++InstanceRevision;
    OnRepresentationChanged.Broadcast(Id,ERBOptRepresentation::Instance); return true;
}
int32 ARBOptimizationGroup::AdvanceRespawnTime(double Delta)
{
    if(!CanOperate() || !Policy || !FMath::IsFinite(Delta) || Delta<0 || Delta>315576000) return 0;
    TArray<int64> Due;
    for(auto& Pair:Records) if(Pair.Value.RespawnRemainingSeconds>=0) {
        Pair.Value.RespawnRemainingSeconds=FMath::Max(0.0,Pair.Value.RespawnRemainingSeconds-Delta);
        if(Pair.Value.RespawnRemainingSeconds==0) Due.Add(Pair.Key);
    }
    Due.Sort(); int32 Count=0; const double Start=Now();
    for(int64 Id:Due) {
        if(Count>=MaxTransitionsPerUpdate || (Now()-Start)*1000>=SoftTransitionBudgetMs) break;
        const auto* R=Records.Find(Id); if(!R) continue;
        const FTransform Transform=R->Transform; const FString Payload=R->Payload;
        if(RespawnItem(Id,Transform,Payload)) ++Count;
    }
    return Count;
}
FRBOptDiagnostics ARBOptimizationGroup::GetDiagnostics() const
{
    FRBOptDiagnostics Result=Diagnostics; Result.Registered=GetRegisteredCount(); Result.Active=GetActiveCount();
    Result.Instances=Instances?Instances->GetInstanceCount():0; Result.Consumed=0; Result.PendingRespawns=0;
    Result.PooledActors=ActorPool.Num();
    if(Policy) for(const auto& Pair:Records) {
        const auto* N=Policy->get(static_cast<rbopt::Id>(Pair.Key));
        if(N && N->representation==rbopt::Representation::Consumed) ++Result.Consumed;
        if(Pair.Value.RespawnRemainingSeconds>=0) ++Result.PendingRespawns;
    }
    return Result;
}
