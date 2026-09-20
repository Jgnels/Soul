#include "RBOptimizationReplication.h"
#include "RBOptimizationGroup.h"
#include "RBOptimizationActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

void FRBOptNetList::PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters&) {
    if(Owner.IsValid()) Owner->ReconcileClient();
}
ARBOptimizationReplicator::ARBOptimizationReplicator() {
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    bReplicates=true; bAlwaysRelevant=false; SetReplicateMovement(false);
    PrimaryActorTick.bCanEverTick=false; Wire.Owner=this;
}
void ARBOptimizationReplicator::BeginPlay() {
    Super::BeginPlay(); Wire.Owner=this;
    if(HasAuthority()) GetWorldTimerManager().SetTimer(Timer,this,&ARBOptimizationReplicator::Synchronize,0.1f,true);
    else ReconcileClient();
}
void ARBOptimizationReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ARBOptimizationReplicator,Config); DOREPLIFETIME(ARBOptimizationReplicator,Wire);
}
bool ARBOptimizationReplicator::AttachAuthoritativeGroup(ARBOptimizationGroup* G,FString& Error) {
    Error.Reset();
    if(!HasAuthority() || GetNetMode()==NM_Client || Source || !IsValid(G) || G->GetWorld()!=GetWorld()
       || !G->Policy || G->MaterializedClass!=ARBOptimizationActor::StaticClass() || G->GetRegisteredCount()>512
       || G->GetRegisteredCount()==0 || !G->Instances->GetStaticMesh() || G->Instances->GetNumMaterials()>32) {
        Error=TEXT("Authority, a new bridge, native representation and 1..512 registered items required"); return false;
    }
    for(const auto& Pair:G->Records) if(IsValid(Pair.Value.Actor) && !Pair.Value.Actor->IsQuiescent()) {
        Error=TEXT("Physics/attachments/custom active state need a different network adapter");return false;
    }
    Source=G; Source->bMembershipFrozen=true; SetActorLocation(G->GetActorLocation());
    Config.Collection=G->CollectionId; Config.Mesh=G->Instances->GetStaticMesh(); Config.Mobility=G->Instances->Mobility;
    Config.CollisionProfile=G->Instances->GetCollisionProfileName(); Config.CollisionEnabled=G->Instances->GetCollisionEnabled(); Config.CollisionObjectType=G->Instances->GetCollisionObjectType();
    Config.bGenerateOverlapEvents=G->Instances->GetGenerateOverlapEvents(); Config.bCanEverAffectNavigation=G->Instances->CanEverAffectNavigation();
    Config.CollisionResponses=G->Instances->GetCollisionResponseToChannels();
    Config.bCastShadow=G->Instances->CastShadow; Config.bReceivesDecals=G->Instances->bReceivesDecals;
    Config.bRenderCustomDepth=G->Instances->bRenderCustomDepth; Config.CustomDepthStencilValue=G->Instances->CustomDepthStencilValue;
    Config.bLightingChannel0=G->Instances->LightingChannels.bChannel0; Config.bLightingChannel1=G->Instances->LightingChannels.bChannel1; Config.bLightingChannel2=G->Instances->LightingChannels.bChannel2;
    for(int32 I=0;I<G->Instances->GetNumMaterials();++I) Config.Materials.Add(G->Instances->GetMaterial(I));
    TArray<int64> Keys; G->Records.GetKeys(Keys); Keys.Sort();
    for(const int64 Id:Keys) { FRBOptNetItem Item; Item.Id=Id; Index.Add(Id,Wire.Items.Add(Item)); Dirty.Add(Id); }
    Source->OnRepresentationChanged.AddDynamic(this,&ARBOptimizationReplicator::Changed);
    Source->OnDestroyed.AddDynamic(this,&ARBOptimizationReplicator::SourceDestroyed);
    Config.bReady=true; Synchronize(); ForceNetUpdate(); return true;
}
bool ARBOptimizationReplicator::DetachAuthoritativeGroup(FString& Error) {
    Error.Reset();
    if(!HasAuthority() || GetNetMode()==NM_Client || !IsValid(Source)) { Error=TEXT("Attached authoritative group required"); return false; }
    Source->OnRepresentationChanged.RemoveDynamic(this,&ARBOptimizationReplicator::Changed);
    Source->OnDestroyed.RemoveDynamic(this,&ARBOptimizationReplicator::SourceDestroyed);
    Source->bMembershipFrozen=false; Source=nullptr; Index.Empty(); Dirty.Empty();
    Wire.Items.Empty(); Wire.MarkArrayDirty(); Config=FRBOptNetConfig(); Applied.Empty(); ForceNetUpdate();
    return true;
}
void ARBOptimizationReplicator::Changed(int64 Id,ERBOptRepresentation) { Dirty.Add(Id); Synchronize(); }
void ARBOptimizationReplicator::SourceDestroyed(AActor*) { Destroy(); }
void ARBOptimizationReplicator::Synchronize() {
    if(!HasAuthority() || !IsValid(Source) || bSynchronizing) return;
    TGuardValue<bool> Lock(bSynchronizing,true);
    // Correctness does not depend on delegate delivery. Shards are bounded to 512 identities;
    // only state/transform differences are marked dirty for actual FastArray transmission.
    TArray<int64> Pending; Pending.Reserve(Wire.Items.Num());
    for(const auto& Existing:Wire.Items) Pending.Add(Existing.Id);
    Dirty.Empty();
    for(const int64 Id:Pending) {
        const int32* Position=Index.Find(Id); if(!Position) {LastFault=TEXT("Membership changed outside the frozen contract");continue;}
        auto& Item=Wire.Items[*Position]; const auto* R=Source->Records.Find(Id);
        const auto* N=Source->Policy->get(static_cast<rbopt::Id>(Id));
        if(!R || !N) {LastFault=TEXT("Invalid source identity");continue;}
        FTransform Transform=R->Transform;
        ERBOptRepresentation State=N->representation==rbopt::Representation::Instance?ERBOptRepresentation::Instance:
            N->representation==rbopt::Representation::Actor?ERBOptRepresentation::Actor:ERBOptRepresentation::Consumed;
        if(State==ERBOptRepresentation::Actor) {
            if(!IsValid(R->Actor) || R->Actor->Mesh->IsSimulatingPhysics()) {LastFault=TEXT("Unsupported active lifetime or physics");continue;}
            Transform=R->Actor->GetActorTransform();
        }
        if(Item.Revision==0 || Item.State!=State || !Item.Transform.Equals(Transform,0.0)) {
            Item.State=State; Item.Transform=Transform; ++Item.Revision; Wire.MarkItemDirty(Item);
        }
    }
}
void ARBOptimizationReplicator::OnRep_Config() { Wire.Owner=this; ReconcileClient(); }
void ARBOptimizationReplicator::ReconcileClient() {
    if(GetNetMode()!=NM_Client) return;
    if(!Config.bReady) { if(IsValid(ClientView)) ClientView->Destroy(); ClientView=nullptr; Applied.Empty(); return; }
    if(!Config.Mesh || !Config.Collection.IsValid()) return;
    if(!ClientView) {
        ClientView=GetWorld()->SpawnActor<ARBOptimizationGroup>(); if(!ClientView){LastFault=TEXT("Client group spawn failed");return;}
        ClientView->CollectionId=Config.Collection;
        TArray<UMaterialInterface*> Materials; for(const auto& M:Config.Materials) Materials.Add(M.Get());
        TGuardValue<bool> Receive(ClientView->bReceivingAuthority,true);
        ClientView->Instances->SetMobility(Config.Mobility);
        if(!ClientView->InitializeGroup(Config.Mesh,Materials,Config.CollisionProfile)) {
            ClientView->Destroy(); ClientView=nullptr;LastFault=TEXT("Client configuration failed");return;
        }
        ClientView->Instances->SetCastShadow(Config.bCastShadow); ClientView->Instances->SetCollisionEnabled(Config.CollisionEnabled); ClientView->Instances->SetCollisionObjectType(Config.CollisionObjectType);
        ClientView->Instances->SetGenerateOverlapEvents(Config.bGenerateOverlapEvents); ClientView->Instances->SetCanEverAffectNavigation(Config.bCanEverAffectNavigation);
        ClientView->Instances->SetCollisionResponseToChannels(Config.CollisionResponses);
        ClientView->Instances->SetReceivesDecals(Config.bReceivesDecals); ClientView->Instances->SetRenderCustomDepth(Config.bRenderCustomDepth);
        ClientView->Instances->SetCustomDepthStencilValue(Config.CustomDepthStencilValue);
        ClientView->Instances->LightingChannels.bChannel0=Config.bLightingChannel0; ClientView->Instances->LightingChannels.bChannel1=Config.bLightingChannel1; ClientView->Instances->LightingChannels.bChannel2=Config.bLightingChannel2;
    }
    for(const auto& Item:Wire.Items) {
        if(Item.Id<=0 || Item.Revision<=Applied.FindRef(Item.Id)) continue;
        if(ClientView->ApplyAuthorityItem(Item.Id,Item.Transform,Item.State)) Applied.Add(Item.Id,Item.Revision);
        else LastFault=TEXT("Client rejected invalid authoritative item");
    }
}
bool ARBOptimizationReplicator::RequestActivation(APawn* Pawn,int64 Id,int64 ExpectedRevision) {
    if(!HasAuthority() || GetNetMode()==NM_Client || !IsValid(Source) || !IsValid(Pawn) || !Pawn->GetController()
       || Pawn->GetWorld()!=GetWorld() || !Source->bHitActivation || !FMath::IsFinite(MaximumRequestDistance)
       || MaximumRequestDistance<=0 || MaximumRequestDistance>5000) return false;
    Synchronize(); const int32* Pos=Index.Find(Id);
    if(!Pos || Wire.Items[*Pos].Revision!=ExpectedRevision || Wire.Items[*Pos].State==ERBOptRepresentation::Consumed) return false;
    if(FVector::DistSquared(Pawn->GetActorLocation(),Wire.Items[*Pos].Transform.GetLocation())>FMath::Square(MaximumRequestDistance)) return false;
    return Source->ActivateItem(Id,2)!=nullptr;
}
void ARBOptimizationReplicator::EndPlay(const EEndPlayReason::Type Reason) {
    GetWorldTimerManager().ClearTimer(Timer);
    if(IsValid(Source)) {Source->OnRepresentationChanged.RemoveDynamic(this,&ARBOptimizationReplicator::Changed);Source->bMembershipFrozen=false;Source->OnDestroyed.RemoveDynamic(this,&ARBOptimizationReplicator::SourceDestroyed);}
    if(IsValid(ClientView)) ClientView->Destroy();
    Super::EndPlay(Reason);
}
URBOptRequestComponent::URBOptRequestComponent() { SetIsReplicatedByDefault(true); PrimaryComponentTick.bCanEverTick=false; }
void URBOptRequestComponent::RequestActivation(ARBOptimizationReplicator* Target,int64 Id,int64 ExpectedRevision) {
    if(IsValid(Target) && Id>0 && ExpectedRevision>0) ServerRequest(Target,Id,ExpectedRevision);
}
void URBOptRequestComponent::ServerRequest_Implementation(ARBOptimizationReplicator* Target,int64 Id,int64 ExpectedRevision) {
    const double Time=FPlatformTime::Seconds(); if(Time-LastRequest<0.1) return; LastRequest=Time;
    APawn* Pawn=Cast<APawn>(GetOwner()); if(auto* C=Cast<AController>(GetOwner())) Pawn=C->GetPawn();
    if(IsValid(Target)) Target->RequestActivation(Pawn,Id,ExpectedRevision);
}
// Client-only reconciler, inaccessible from Blueprint. Server state is never accepted from a client.
bool ARBOptimizationGroup::ApplyAuthorityItem(int64 Id,const FTransform& Transform,ERBOptRepresentation State) {
    if(GetNetMode()!=NM_Client || Id<=0 || Transform.ContainsNaN() || !Transform.GetRotation().IsNormalized()) return false;
    const FVector P=Transform.GetLocation(),Scale=Transform.GetScale3D();
    if(!rbopt::valid({P.X,P.Y,P.Z}) || Scale.GetMin()<=0 || Scale.GetMax()>10000) return false;
    if(State!=ERBOptRepresentation::Instance && State!=ERBOptRepresentation::Actor && State!=ERBOptRepresentation::Consumed) return false;
    TGuardValue<bool> Receive(bReceivingAuthority,true);
    if(!Records.Contains(Id) && !RegisterItem(Id,Transform,TEXT(""))) return false;
    auto& R=Records.FindChecked(Id); const auto* N=Policy->get(static_cast<rbopt::Id>(Id));
    if(State==ERBOptRepresentation::Consumed) return N->representation==rbopt::Representation::Consumed || ConsumeItem(Id);
    if(N->representation==rbopt::Representation::Consumed) {
        if(!RespawnItem(Id,Transform,TEXT(""))) return false;
    }
    if(State==ERBOptRepresentation::Instance && IsValid(R.Actor)) {
        if(!ConsumeItem(Id) || !RespawnItem(Id,Transform,TEXT(""))) return false;
    } else if(State==ERBOptRepresentation::Actor && !IsValid(R.Actor)) {
        if(!Materialize(Id)) return false;
    }
    if(IsValid(R.Actor)) { R.Actor->SetActorTransform(Transform); }
    else { Instances->UpdateInstanceTransformById(R.Instance,Transform,true); }
    R.Transform=Transform; Policy->updatePosition(static_cast<rbopt::Id>(Id),{P.X,P.Y,P.Z}); return true;
}
