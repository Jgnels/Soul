#include "RBOptimizationSource.h"
#include "RBOptimizationGroup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace {
FString SourcePresentationSignature(const UStaticMeshComponent* C) {
    FString R=FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d:%d:%d:%s"),int32(C->Mobility),int32(C->CastShadow),int32(C->GetCollisionEnabled()),
        int32(C->bReceivesDecals),int32(C->bRenderCustomDepth),C->CustomDepthStencilValue,
        int32(C->LightingChannels.bChannel0),int32(C->LightingChannels.bChannel1),int32(C->LightingChannels.bChannel2),*C->GetCollisionProfileName().ToString());
    R+=FString::Printf(TEXT(":%d:%d:%d"),int32(C->GetCollisionObjectType()),int32(C->GetGenerateOverlapEvents()),int32(C->CanEverAffectNavigation()));
    for(int32 Channel=0;Channel<ECC_MAX;++Channel) R+=FString::Printf(TEXT(":%d"),int32(C->GetCollisionResponseToChannel(ECollisionChannel(Channel))));
    return R;
}
bool Inspect(UStaticMeshComponent* Source,FGuid Collection,FRBOptSourceManifest& Out,FString& Error,bool bAllowHidden) {
    if(!IsInGameThread() || !IsValid(Source) || !IsValid(Source->GetOwner()) || !Source->GetWorld() || Source->GetWorld()->GetNetMode()==NM_Client
       || !Source->ComponentHasTag(TEXT("RBOpt.FrozenSource")) || !Collection.IsValid() || !Source->GetStaticMesh()
       || Source->IsSimulatingPhysics() || (!bAllowHidden && (!Source->IsVisible() || Source->bHiddenInGame || Source->GetOwner()->IsHidden()))) {
        Error=TEXT("Visible, non-simulating, authority-side frozen source and stable collection ID required"); return false;
    }
    FRBOptSourceManifest M; M.Source=Source;M.CollectionId=Collection;M.Mesh=Source->GetStaticMesh();M.SourcePath=Source->GetPathName();M.PresentationSignature=SourcePresentationSignature(Source);
    for(int32 I=0;I<Source->GetNumMaterials();++I) M.Materials.Add(Source->GetMaterial(I));
    if(const auto* ISM=Cast<UInstancedStaticMeshComponent>(Source)) {
        M.bInstanced=true;
        if(ISM->GetInstanceCount()<1 || ISM->GetInstanceCount()>10000 || ISM->NumCustomDataFloats!=0) {
            Error=TEXT("This bounded importer requires 1..10000 instances without per-instance custom data");return false;
        }
        for(int32 I=0;I<ISM->GetInstanceCount();++I) {FTransform T; if(!ISM->GetInstanceTransform(I,T,true)){Error=TEXT("Instance transform read failed");return false;} M.Transforms.Add(T);}
    } else M.Transforms.Add(Source->GetComponentTransform());
    for(const auto& T:M.Transforms) {
        const FVector P=T.GetLocation(),S=T.GetScale3D();
        if(T.ContainsNaN() || !T.GetRotation().IsNormalized() || !rbopt::valid({P.X,P.Y,P.Z}) || S.GetMin()<=0 || S.GetMax()>10000) {
            Error=TEXT("Unsupported transform");return false;
        }
    }
    Out=MoveTemp(M);Error.Reset();return true;
}
FString ManifestDifference(const FRBOptSourceManifest& A,const FRBOptSourceManifest& B) {
    if(A.Source!=B.Source) return TEXT("source object");
    if(A.CollectionId!=B.CollectionId) return TEXT("collection identity");
    if(A.Mesh!=B.Mesh) return TEXT("mesh");
    if(A.Materials!=B.Materials) return TEXT("materials");
    if(A.SourcePath!=B.SourcePath) return TEXT("source path");
    if(A.PresentationSignature!=B.PresentationSignature) return FString::Printf(TEXT("presentation signature expected{%s} actual{%s}"),*A.PresentationSignature,*B.PresentationSignature);
    if(A.bInstanced!=B.bInstanced || A.Transforms.Num()!=B.Transforms.Num()) return TEXT("instance topology");
    for(int32 I=0;I<A.Transforms.Num();++I) if(!A.Transforms[I].Equals(B.Transforms[I],0.01)) return FString::Printf(TEXT("transform[%d]"),I);
    return {};
}
bool Matches(const FRBOptSourceManifest& A,const FRBOptSourceManifest& B) { return ManifestDifference(A,B).IsEmpty(); }
}
bool URBOptimizationSource::InspectFrozenSource(UStaticMeshComponent* Source,FGuid Collection,FRBOptSourceManifest& Manifest,FString& Error) {
    return Inspect(Source,Collection,Manifest,Error,false);
}
bool URBOptimizationSource::AdoptReviewedSource(const FRBOptSourceManifest& M,int64 FirstId,bool Reviewed,FRBOptSourceReceipt& Out,FString& Error) {
    FRBOptSourceManifest Live;
    if(!Reviewed) { Error=TEXT("Explicit visual/material parity review required"); return false; }
    if(FirstId<=0 || FirstId>MAX_int64-M.Transforms.Num()) { Error=TEXT("Invalid stable-ID range"); return false; }
    if(!Inspect(M.Source,M.CollectionId,Live,Error,false)) return false;
    if(const FString Difference=ManifestDifference(M,Live); !Difference.IsEmpty()) { Error=TEXT("Frozen source changed after review: ")+Difference; return false; }
    FActorSpawnParameters Params;Params.OverrideLevel=M.Source->GetOwner()->GetLevel();
    auto* G=M.Source->GetWorld()->SpawnActor<ARBOptimizationGroup>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
    if(!G){Error=TEXT("Group spawn failed");return false;}
    G->CollectionId=M.CollectionId; TArray<UMaterialInterface*> Materials;for(const auto& V:M.Materials)Materials.Add(V.Get());
    G->Instances->SetMobility(M.Source->Mobility);
    if(!G->InitializeGroup(M.Mesh,Materials,M.Source->GetCollisionProfileName())) {G->Destroy();Error=TEXT("Source profile unsupported");return false;}
    G->Instances->SetCastShadow(M.Source->CastShadow);
    G->Instances->SetCollisionEnabled(M.Source->GetCollisionEnabled());
    G->Instances->SetCollisionObjectType(M.Source->GetCollisionObjectType());
    G->Instances->SetCollisionResponseToChannels(M.Source->GetCollisionResponseToChannels());
    G->Instances->SetGenerateOverlapEvents(M.Source->GetGenerateOverlapEvents());
    G->Instances->SetCanEverAffectNavigation(M.Source->CanEverAffectNavigation());
    G->Instances->SetReceivesDecals(M.Source->bReceivesDecals);
    G->Instances->SetRenderCustomDepth(M.Source->bRenderCustomDepth);
    G->Instances->SetCustomDepthStencilValue(M.Source->CustomDepthStencilValue);
    G->Instances->LightingChannels=M.Source->LightingChannels;
    for(int32 I=0;I<M.Transforms.Num();++I) if(!G->RegisterItem(FirstId+I,M.Transforms[I],TEXT(""))) {
        G->Destroy();Error=TEXT("Staging failed; source was not hidden or deleted");return false;
    }
    FRBOptSourceReceipt R;R.Original=M;R.Group=G;R.FirstId=FirstId;R.OriginalCollision=M.Source->GetCollisionEnabled();
    R.OriginalCollisionProfile=M.Source->GetCollisionProfileName(); R.OriginalObjectType=M.Source->GetCollisionObjectType();
    R.OriginalResponses=M.Source->GetCollisionResponseToChannels();
    R.bOriginalGenerateOverlapEvents=M.Source->GetGenerateOverlapEvents(); R.bOriginalCanEverAffectNavigation=M.Source->CanEverAffectNavigation(); R.bAdopted=true;
    M.Source->SetHiddenInGame(true);M.Source->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    R.AdoptedPresentationSignature=SourcePresentationSignature(M.Source);
    Out=MoveTemp(R);Error.Reset();return true;
}
bool URBOptimizationSource::RestoreOriginalIfUnchanged(FRBOptSourceReceipt& R,FString& Error) {
    FRBOptSourceManifest Live; FRBOptSnapshot Current;
    if(!R.bAdopted || !IsValid(R.Group) || !Inspect(R.Original.Source,R.Original.CollectionId,Live,Error,true)
       || Live.PresentationSignature!=R.AdoptedPresentationSignature || !R.Original.Source->bHiddenInGame || R.Original.Source->GetCollisionEnabled()!=ECollisionEnabled::NoCollision
       || !R.Group->CaptureSnapshot(Current,Error) || Current.Items.Num()!=R.Original.Transforms.Num()) {
        Error=TEXT("Source or managed state changed; refusing unsafe rollback");return false;
    }
    FRBOptSourceManifest Comparable=Live; Comparable.PresentationSignature=R.Original.PresentationSignature;
    if(const FString Difference=ManifestDifference(R.Original,Comparable); !Difference.IsEmpty()) { Error=TEXT("Source changed during takeover: ")+Difference;return false; }
    for(int32 I=0;I<Current.Items.Num();++I) {
        const auto& S=Current.Items[I];
        if(S.Id!=R.FirstId+I || S.State!=ERBOptRepresentation::Instance || !S.Payload.IsEmpty() || S.bPinned
           || S.RemainingHoldSeconds>0 || S.RespawnRemainingSeconds>=0 || !S.Transform.Equals(R.Original.Transforms[I],0.0)) {
            Error=TEXT("Rollback would discard gameplay state, active objects or consumed identities");return false;
        }
    }
    R.Group->Destroy(); R.Group=nullptr;
    R.Original.Source->SetCollisionProfileName(R.OriginalCollisionProfile);
    R.Original.Source->SetCollisionObjectType(R.OriginalObjectType);
    R.Original.Source->SetCollisionResponseToChannels(R.OriginalResponses);
    R.Original.Source->SetCollisionEnabled(R.OriginalCollision); R.Original.Source->SetCollisionProfileName(R.OriginalCollisionProfile);
    R.Original.Source->SetGenerateOverlapEvents(R.bOriginalGenerateOverlapEvents);
    R.Original.Source->SetCanEverAffectNavigation(R.bOriginalCanEverAffectNavigation); R.Original.Source->SetHiddenInGame(false);
    R.bAdopted=false;Error.Reset();return true;
}
