#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "RBOptimizationGroup.h"
#include "RBOptimizationActor.h"
#include "RBOptimizationAudit.h"
#include "RBOptimizationReplication.h"
#include "RBOptimizationSource.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Misc/Guid.h"
namespace {
struct FTestWorld {
 UWorld* W=nullptr;
 FTestWorld() { const auto IV=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false); W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&IV); GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W); }
 ~FTestWorld() { if(W) { W->DestroyWorld(false); GEngine->DestroyWorldContext(W); } }
 ARBOptimizationGroup* Group() { auto* G=W->SpawnActor<ARBOptimizationGroup>(); G->MinimumActiveSeconds=0; auto* M=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")); if(!G->InitializeGroup(M,{},TEXT("BlockAllDynamic"))) return nullptr; return G; }
};
constexpr auto Flags=EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptRegistration,"RBOptimization.Runtime.Registration",Flags)
bool FRBOptRegistration::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!TestNotNull(TEXT("group"),G)) return false;
 TestTrue(TEXT("first"),G->RegisterItem(1,FTransform(FVector(0,0,0)),TEXT("health=50")));
 TestFalse(TEXT("duplicate"),G->RegisterItem(1,FTransform::Identity,TEXT("")));
 TestFalse(TEXT("zero"),G->RegisterItem(0,FTransform::Identity,TEXT("")));
 TestFalse(TEXT("negative scale"),G->RegisterItem(2,FTransform(FQuat::Identity,FVector::ZeroVector,FVector(-1,1,1)),TEXT("")));
 TestEqual(TEXT("proxy count"),G->Instances->GetInstanceCount(),1); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptSwap,"RBOptimization.Runtime.MaterializeAndPayload",Flags)
bool FRBOptSwap::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false;
 G->RegisterItem(21,FTransform(FVector(10,20,30)),TEXT("durability=7")); auto* A=G->ActivateItem(21);
 if(!TestNotNull(TEXT("actor"),A)) return false;
 TestEqual(TEXT("stable ID"),A->StableId,int64(21)); TestEqual(TEXT("state"),A->DomainPayload,FString(TEXT("durability=7")));
 TestEqual(TEXT("no duplicate proxy"),G->Instances->GetInstanceCount(),0); TestEqual(TEXT("active count"),G->GetActiveCount(),1);
 TestTrue(TEXT("position"),A->GetActorLocation().Equals(FVector(10,20,30))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptPool,"RBOptimization.Runtime.ActorPoolReuseAndBounds",Flags)
bool FRBOptPool::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->MaxPooledActors=1; G->SoftTransitionBudgetMs=100;
 TestEqual(TEXT("prewarm one"),G->PrewarmActorPool(1),1);
 TestEqual(TEXT("pool visible in diagnostics"),G->GetDiagnostics().PooledActors,1);
 G->RegisterItem(1,FTransform(FVector(0,0,0)),TEXT("one")); G->RegisterItem(2,FTransform(FVector(5000,0,0)),TEXT("two"));
 auto* A=G->ActivateItem(1,0.000000001); if(!TestNotNull(TEXT("first actor"),A)) return false;
 TestEqual(TEXT("pool consumed"),G->GetPooledActorCount(),0); TestEqual(TEXT("first pool hit"),G->GetDiagnostics().PoolHits,int64(1));
 // Outside both activation radii: retire without activating the other identity.
 G->UpdateViewers({FVector(2500,0,0)}); TestEqual(TEXT("first retired"),G->GetActiveCount(),0);
 TestEqual(TEXT("returned to pool"),G->GetPooledActorCount(),1);
 auto* B=G->ActivateItem(2,0.000000001); if(!TestNotNull(TEXT("second actor"),B)) return false;
 TestTrue(TEXT("same actor reused"),A==B); TestEqual(TEXT("new stable identity"),B->StableId,int64(2));
 TestEqual(TEXT("new payload restored"),B->DomainPayload,FString(TEXT("two"))); TestEqual(TEXT("second pool hit"),G->GetDiagnostics().PoolHits,int64(2));
 G->UpdateViewers({FVector(2500,0,0)}); TestEqual(TEXT("second retired"),G->GetActiveCount(),0);
 TestEqual(TEXT("bounded pool"),G->GetPooledActorCount(),1); TestEqual(TEXT("trimmed"),G->TrimActorPool(0),1);
 TestEqual(TEXT("pool empty after trim"),G->GetPooledActorCount(),0); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptPoolCollision,"RBOptimization.Runtime.PoolCollisionParity",Flags)
bool FRBOptPoolCollision::RunTest(const FString&) {
 FTestWorld T;
 for(const auto Mode:{ECollisionEnabled::NoCollision,ECollisionEnabled::QueryOnly,ECollisionEnabled::QueryAndPhysics}) {
  auto* G=T.Group(); if(!TestNotNull(TEXT("group"),G)) return false;
  G->SoftTransitionBudgetMs=100; G->Instances->SetCollisionEnabled(Mode);
  G->RegisterItem(1,FTransform::Identity,TEXT("first"));
  auto* A=G->ActivateItem(1,0.000000001); if(!TestNotNull(TEXT("fresh actor accepts configured collision"),A)) return false;
  TestTrue(TEXT("fresh effective collision matches instance"),A->Mesh->GetCollisionEnabled()==Mode);
  G->UpdateViewers({FVector(5000,0,0)});
  TestEqual(TEXT("retired"),G->GetActiveCount(),0);
  TestTrue(TEXT("parked actor hidden and collision disabled"),A->IsHidden() && !A->GetActorEnableCollision());
  TestEqual(TEXT("parked identity cleared"),A->StableId,int64(0));
  TestTrue(TEXT("parked payload cleared"),A->DomainPayload.IsEmpty());
  FRBOptSnapshot Snapshot; FString Error;
  TestTrue(TEXT("snapshot with parked actor"),G->CaptureSnapshot(Snapshot,Error));
  TestEqual(TEXT("pool adds no durable identities"),Snapshot.Items.Num(),1);
  auto* B=G->ActivateItem(1); if(!TestNotNull(TEXT("pooled actor accepts configured collision"),B)) return false;
  TestTrue(TEXT("actual reuse"),A==B);
  TestTrue(TEXT("reused effective collision matches instance"),B->Mesh->GetCollisionEnabled()==Mode);
  TestTrue(TEXT("reused actor visible"),!B->IsHidden());
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptPoolLifetime,"RBOptimization.Runtime.PoolDisabledAndDestroyedEntries",Flags)
bool FRBOptPoolLifetime::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->SoftTransitionBudgetMs=100; G->MaxPooledActors=1;
 G->RegisterItem(1,FTransform::Identity,TEXT("saved"));
 auto* A=G->ActivateItem(1,0.000000001); if(!TestNotNull(TEXT("fresh"),A)) return false;
 G->UpdateViewers({FVector(5000,0,0)}); A->Destroy();
 auto* B=G->ActivateItem(1,0.000000001); if(!TestNotNull(TEXT("destroyed pool entry falls back to spawn"),B)) return false;
 TestTrue(TEXT("invalid entry discarded"),G->GetDiagnostics().PoolDiscards>=1);
 TestEqual(TEXT("state survived external pool destruction"),B->DomainPayload,FString(TEXT("saved")));
 G->UpdateViewers({FVector(5000,0,0)});
 G->MaxPooledActors=0;
 auto* C=G->ActivateItem(1,0.000000001); if(!TestNotNull(TEXT("disabled pool still spawns"),C)) return false;
 TestFalse(TEXT("existing pooled actor discarded when disabled"),IsValid(B));
 TestEqual(TEXT("disabled pool empty"),G->GetPooledActorCount(),0);
 G->UpdateViewers({FVector(5000,0,0)});
 TestFalse(TEXT("disabled pool destroys retired actor"),IsValid(C));
 TestEqual(TEXT("disabled prewarm"),G->PrewarmActorPool(10),0);
 G->MaxPooledActors=1;
 TestEqual(TEXT("prewarm bounded"),G->PrewarmActorPool(10),1);
 TestEqual(TEXT("trim all"),G->TrimActorPool(),1);
 TestNotNull(TEXT("proxy identity remains usable"),G->ActivateItem(1)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptFailure,"RBOptimization.Runtime.SpawnFailurePreservesProxy",Flags)
bool FRBOptFailure::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->RegisterItem(1,FTransform::Identity,TEXT("keep")); G->MaterializedClass=nullptr;
 TestNull(TEXT("no class"),G->ActivateItem(1)); TestEqual(TEXT("source preserved"),G->Instances->GetInstanceCount(),1); TestEqual(TEXT("no false active"),G->GetActiveCount(),0); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptHit,"RBOptimization.Runtime.StableIdentityAndStaleHits",Flags)
bool FRBOptHit::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false;
 for(int64 Id=1;Id<=3;++Id) G->RegisterItem(Id,FTransform(FVector(Id*100,0,0)),TEXT(""));
 const int64 Old=G->GetInstanceRevision(); FHitResult Hit; Hit.Component=G->Instances; Hit.Item=0;
 TestTrue(TEXT("consume first"),G->ConsumeItem(1)); TestNull(TEXT("old hit refused"),G->ActivateHit(Hit,Old));
 auto* A=G->ActivateHit(Hit,G->GetInstanceRevision()); if(!TestNotNull(TEXT("new hit"),A)) return false;
 TestEqual(TEXT("swap-removed index maps correct ID"),A->StableId,int64(3));
 TestFalse(TEXT("consumed cannot re-register"),G->RegisterItem(1,FTransform::Identity,TEXT(""))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptReturn,"RBOptimization.Runtime.RetirementAndMovedState",Flags)
bool FRBOptReturn::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false;
 G->RegisterItem(8,FTransform::Identity,TEXT("initial")); G->UpdateViewers({FVector::ZeroVector});
 auto* A=G->ActivateItem(8,0.000000001); if(!A) return false;
 A->DomainPayload=TEXT("changed"); A->SetActorLocation(FVector(5000,0,0));
 G->UpdateViewers({FVector(5000,0,0)}); TestEqual(TEXT("moved actor stays by viewer"),G->GetActiveCount(),1);
 G->UpdateViewers({FVector::ZeroVector}); TestEqual(TEXT("retired far away"),G->GetActiveCount(),0);
 auto* B=G->ActivateItem(8); if(!TestNotNull(TEXT("rematerialized"),B)) return false;
 TestEqual(TEXT("preserved changed payload"),B->DomainPayload,FString(TEXT("changed"))); TestTrue(TEXT("preserved position"),B->GetActorLocation().Equals(FVector(5000,0,0))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptSafety,"RBOptimization.Runtime.PinsAttachmentsAndMissingViews",Flags)
bool FRBOptSafety::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false;
 G->RegisterItem(1,FTransform::Identity,TEXT("")); auto* A=G->ActivateItem(1,0.000000001); if(!A) return false;
 G->SetPinned(1,true); G->UpdateViewers({FVector(5000,0,0)}); TestEqual(TEXT("pin"),G->GetActiveCount(),1);
 G->SetPinned(1,false); G->UpdateViewers({}); TestEqual(TEXT("no viewers is not retirement permission"),G->GetActiveCount(),1);
 A->bAllowReturnToInstance=false; G->UpdateViewers({FVector(5000,0,0)}); TestEqual(TEXT("consent required"),G->GetActiveCount(),1);
 A->bAllowReturnToInstance=true; auto* Child=T.W->SpawnActor<ARBOptimizationActor>(); Child->AttachToActor(A,FAttachmentTransformRules::KeepWorldTransform);
 G->UpdateViewers({FVector(5000,0,0)}); TestEqual(TEXT("attachment pin"),G->GetActiveCount(),1); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptDestroy,"RBOptimization.Runtime.NoResurrection",Flags)
bool FRBOptDestroy::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->RegisterItem(1,FTransform::Identity,TEXT("")); auto* A=G->ActivateItem(1); if(!A) return false;
 A->Destroy(); G->UpdateViewers({FVector::ZeroVector}); TestEqual(TEXT("external destruction tombstoned"),G->GetActiveCount(),0);
 TestNull(TEXT("cannot resurrect"),G->ActivateItem(1)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptAudit,"RBOptimization.Runtime.ReadOnlyAudit",Flags)
bool FRBOptAudit::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->RegisterItem(1,FTransform::Identity,TEXT(""));
 const FString Name=TEXT("test-")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"); FString Path;
 TestTrue(TEXT("audit write"),URBOptimizationAudit::WriteLoadedWorldAudit(T.W,Name,Path));
 TestEqual(TEXT("unchanged count"),G->Instances->GetInstanceCount(),1); TestEqual(TEXT("unchanged active"),G->GetActiveCount(),0);
 FString Ignored; TestFalse(TEXT("refuse overwrite"),URBOptimizationAudit::WriteLoadedWorldAudit(T.W,Name,Ignored));
 TestFalse(TEXT("refuse traversal"),URBOptimizationAudit::WriteLoadedWorldAudit(T.W,TEXT("../bad.json"),Ignored));
 FString Content; TestTrue(TEXT("read audit"),FFileHelper::LoadFileToString(Content,*Path)); TestTrue(TEXT("scope"),Content.Contains(TEXT("loaded world only")));
 IFileManager::Get().Delete(*Path); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptSave,"RBOptimization.State.RoundTripAndIdentity",Flags)
bool FRBOptSave::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false;
 G->SoftTransitionBudgetMs=100; G->RegisterItem(1,FTransform(FVector(10,0,0)),TEXT("int64=9007199254740993"));
 G->RegisterItem(9007199254740993LL,FTransform(FVector(2000,0,0)),TEXT("dead"));
 G->RegisterItem(3,FTransform(FVector(5000,0,0)),TEXT("alive"));
 auto* A=G->ActivateItem(3); if(!A) return false; A->DomainPayload=TEXT("changed");
 G->ConsumeItem(9007199254740993LL); G->ScheduleRespawn(9007199254740993LL,12);
 FRBOptSnapshot S; FString Error; TestTrue(TEXT("snapshot"),G->CaptureSnapshot(S,Error));
 auto* H=T.Group(); if(!H) return false; TestFalse(TEXT("foreign collection rejected"),H->RestoreIntoFreshGroup(S,Error));
 H->CollectionId=S.CollectionId; TestTrue(TEXT("restore"),H->RestoreIntoFreshGroup(S,Error));
 TestEqual(TEXT("live actors"),H->GetActiveCount(),1); TestEqual(TEXT("instances"),H->Instances->GetInstanceCount(),1);
 FRBOptSavedItem Item; TestTrue(TEXT("exact int64 lookup"),H->GetItemState(9007199254740993LL,Item));
 TestTrue(TEXT("tombstone retained"),Item.State==ERBOptRepresentation::Consumed);
 TestEqual(TEXT("respawn remainder"),Item.RespawnRemainingSeconds,12.0);
 TestTrue(TEXT("active state"),H->GetItemState(3,Item)); TestEqual(TEXT("payload retained"),Item.Payload,FString(TEXT("changed")));
 TestFalse(TEXT("existing group never overwritten"),H->RestoreIntoFreshGroup(S,Error)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptSaveInvalid,"RBOptimization.State.InvalidAndUnsafeSnapshots",Flags)
bool FRBOptSaveInvalid::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->RegisterItem(1,FTransform::Identity,TEXT("x"));
 FRBOptSnapshot S; FString Error; if(!G->CaptureSnapshot(S,Error)) return false;
 auto* H=T.Group(); if(!H) return false; H->CollectionId=S.CollectionId;
 const FRBOptSavedItem Duplicate=S.Items[0]; S.Items.Add(Duplicate); TestFalse(TEXT("duplicate rejected"),H->RestoreIntoFreshGroup(S,Error)); TestEqual(TEXT("atomic"),H->GetRegisteredCount(),0);
 S.Items.RemoveAt(1); S.Items[0].Transform.SetScale3D(FVector(-1,1,1)); TestFalse(TEXT("bad transform"),H->RestoreIntoFreshGroup(S,Error));
 S.Items[0].Transform=FTransform::Identity; S.Items[0].RemainingHoldSeconds=std::numeric_limits<double>::quiet_NaN();
 TestFalse(TEXT("NaN refused"),H->RestoreIntoFreshGroup(S,Error)); TestEqual(TEXT("still empty"),H->GetRegisteredCount(),0);
 auto* A=G->ActivateItem(1); if(!A) return false; A->bAllowReturnToInstance=false;
 FRBOptSnapshot Sentinel; Sentinel.SchemaVersion=999; TestFalse(TEXT("unsafe active save refused"),G->CaptureSnapshot(Sentinel,Error));
 TestEqual(TEXT("output untouched on failure"),Sentinel.SchemaVersion,999); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptRespawn,"RBOptimization.State.ExternalClockRespawn",Flags)
bool FRBOptRespawn::RunTest(const FString&) {
 FTestWorld T; auto* G=T.Group(); if(!G) return false; G->SoftTransitionBudgetMs=100; G->MaxTransitionsPerUpdate=1;
 for(int64 Id=1;Id<=3;++Id){G->RegisterItem(Id,FTransform(FVector(Id*100,0,0)),TEXT("seed")); G->ConsumeItem(Id);}
 TestFalse(TEXT("no implicit resurrection"),G->RegisterItem(1,FTransform::Identity,TEXT("")));
 TestTrue(TEXT("schedule first"),G->ScheduleRespawn(1,10)); TestTrue(TEXT("schedule second"),G->ScheduleRespawn(2,10));
 TestEqual(TEXT("five seconds"),G->AdvanceRespawnTime(5),0); TestEqual(TEXT("bad time"),G->AdvanceRespawnTime(-1),0);
 TestEqual(TEXT("bounded first respawn"),G->AdvanceRespawnTime(5),1); TestEqual(TEXT("budgeted continuation"),G->AdvanceRespawnTime(0),1);
 TestEqual(TEXT("unrequested stays dead"),G->GetDiagnostics().Consumed,1);
 TestFalse(TEXT("live cannot respawn"),G->RespawnItem(1,FTransform::Identity,TEXT("")));
 TestTrue(TEXT("explicit new generation"),G->RespawnItem(3,FTransform(FVector(700,0,0)),TEXT("new")));
 auto* A=G->ActivateItem(3); if(!A) return false; TestEqual(TEXT("new payload"),A->DomainPayload,FString(TEXT("new"))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptTable,"RBOptimization.Runtime.TablePresetAndHitToggle",Flags)
bool FRBOptTable::RunTest(const FString&) {
 FTestWorld T; auto* G=T.W->SpawnActor<ARBOptimizationGroup>(); UDataTable* D=NewObject<UDataTable>(); D->RowStruct=FRBOptPresetRow::StaticStruct();
 FRBOptPresetRow R; R.Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")); R.ActorClass=ARBOptimizationActor::StaticClass();
 R.bHitActivation=false; R.bProximityActivation=false; D->AddRow(TEXT("crate"),R); FString Error;
 TestFalse(TEXT("missing row"),G->InitializeFromTable(D,TEXT("missing"),Error)); TestTrue(TEXT("preset"),G->InitializeFromTable(D,TEXT("crate"),Error));
 G->RegisterItem(1,FTransform::Identity,TEXT("")); FHitResult Hit; Hit.Component=G->Instances; Hit.Item=0;
 TestNull(TEXT("disabled hit"),G->ActivateHit(Hit,G->GetInstanceRevision()));
 G->bHitActivation=true; TestNotNull(TEXT("enabled hit"),G->ActivateHit(Hit,G->GetInstanceRevision()));
 TestFalse(TEXT("populated group cannot reinitialize"),G->InitializeFromTable(D,TEXT("crate"),Error)); return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptNetState,"RBOptimization.Network.AuthorityDeltaAndTombstones",Flags)
bool FRBOptNetState::RunTest(const FString&) {
 FTestWorld T;auto* G=T.Group();if(!G)return false;
 for(int64 Id=1;Id<=4;++Id)G->RegisterItem(Id,FTransform(FVector(Id*100,0,0)),TEXT("private-inventory"));
 auto* R=T.W->SpawnActor<ARBOptimizationReplicator>();FString Error;
 TestTrue(TEXT("bind authority"),R->AttachAuthoritativeGroup(G,Error));
 TestFalse(TEXT("membership locked"),G->RegisterItem(5,FTransform::Identity,TEXT("")));
 TestEqual(TEXT("initial delta identities"),R->GetItems().Num(),4);
 const auto Before=R->GetItems();R->Synchronize();TestEqual(TEXT("no redundant dirty update"),R->GetItems()[0].Revision,Before[0].Revision);
 TestNotNull(TEXT("source activation"),G->ActivateItem(1));R->Synchronize();TestTrue(TEXT("activation published"),R->GetItems()[0].State==ERBOptRepresentation::Actor);
 TestTrue(TEXT("source consumption"),G->ConsumeItem(2));R->Synchronize();TestTrue(TEXT("consumption published"),R->GetItems()[1].State==ERBOptRepresentation::Consumed);
 TestTrue(TEXT("explicit respawn"),G->RespawnItem(2,FTransform(FVector(400,0,0)),TEXT("new-private")));R->Synchronize();
 TestTrue(TEXT("new generation published"),R->GetItems()[1].Revision>Before[1].Revision);
 TestTrue(TEXT("wire schema has no payload"),FRBOptNetItem::StaticStruct()->FindPropertyByName(TEXT("Payload"))==nullptr);
 TestTrue(TEXT("explicit detach"),R->DetachAuthoritativeGroup(Error));TestTrue(TEXT("membership released"),G->RegisterItem(5,FTransform::Identity,TEXT("")));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptNetRequest,"RBOptimization.Network.RequestValidation",Flags)
bool FRBOptNetRequest::RunTest(const FString&) {
 FTestWorld T;auto* G=T.Group();if(!G)return false;G->RegisterItem(1,FTransform(FVector(100,0,0)),TEXT(""));
 auto* R=T.W->SpawnActor<ARBOptimizationReplicator>();FString Error;if(!R->AttachAuthoritativeGroup(G,Error))return false;
 auto* Pawn=T.W->SpawnActor<APawn>();auto* PC=T.W->SpawnActor<APlayerController>();
 const int64 Revision=R->GetItems()[0].Revision;
 TestFalse(TEXT("unpossessed rejected"),R->RequestActivation(Pawn,1,Revision));PC->Possess(Pawn);
 TestFalse(TEXT("wrong generation"),R->RequestActivation(Pawn,1,Revision+1));
 TestFalse(TEXT("unknown ID"),R->RequestActivation(Pawn,99,Revision));
 R->MaximumRequestDistance=1;TestFalse(TEXT("distance validated on server"),R->RequestActivation(Pawn,1,Revision));
 R->MaximumRequestDistance=500;G->bHitActivation=false;TestFalse(TEXT("permission disabled"),R->RequestActivation(Pawn,1,Revision));
 G->bHitActivation=true;TestTrue(TEXT("near possessed request"),R->RequestActivation(Pawn,1,Revision));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptImport,"RBOptimization.Source.HISMAdoptionAndRollback",Flags)
bool FRBOptImport::RunTest(const FString&) {
 FTestWorld T;auto* Owner=T.W->SpawnActor<AActor>();auto* H=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
 Owner->AddInstanceComponent(H);Owner->SetRootComponent(H);H->SetMobility(EComponentMobility::Movable);H->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 H->SetCollisionProfileName(TEXT("BlockAllDynamic"));H->SetGenerateOverlapEvents(true);H->SetCanEverAffectNavigation(false);H->RegisterComponent();H->AddInstance(FTransform(FVector(10,0,0)));H->AddInstance(FTransform(FVector(20,0,0)));
 FRBOptSourceManifest M;FRBOptSourceReceipt Receipt;FString Error;const FGuid Id=FGuid::NewGuid();
 TestFalse(TEXT("producer consent required"),URBOptimizationSource::InspectFrozenSource(H,Id,M,Error));H->ComponentTags.Add(TEXT("RBOpt.FrozenSource"));
 TestTrue(TEXT("HISM manifest"),URBOptimizationSource::InspectFrozenSource(H,Id,M,Error));
 const bool OriginalShadow=H->CastShadow;H->SetCastShadow(!OriginalShadow);
 TestFalse(TEXT("stale presentation review refused"),URBOptimizationSource::AdoptReviewedSource(M,100,true,Receipt,Error));H->SetCastShadow(OriginalShadow);
 TestFalse(TEXT("visual review required"),URBOptimizationSource::AdoptReviewedSource(M,100,false,Receipt,Error));
 TestTrue(TEXT("adopt"),URBOptimizationSource::AdoptReviewedSource(M,100,true,Receipt,Error));
 TestEqual(TEXT("original instances never deleted"),H->GetInstanceCount(),2);TestTrue(TEXT("source hidden"),H->bHiddenInGame!=0);
 TestEqual(TEXT("replacement count"),Receipt.Group->Instances->GetInstanceCount(),2);
 TestTrue(TEXT("mobility preserved"),Receipt.Group->Instances->Mobility==H->Mobility);
 TestTrue(TEXT("shadow preserved"),Receipt.Group->Instances->CastShadow==H->CastShadow);
 TestTrue(TEXT("collision preserved"),Receipt.Group->Instances->GetCollisionEnabled()==Receipt.OriginalCollision);
 TestEqual(TEXT("object type preserved"),Receipt.Group->Instances->GetCollisionObjectType(),Receipt.OriginalObjectType);
 TestTrue(TEXT("overlap setting preserved"),Receipt.Group->Instances->GetGenerateOverlapEvents()==Receipt.bOriginalGenerateOverlapEvents);
 TestTrue(TEXT("nav relevance preserved"),Receipt.Group->Instances->CanEverAffectNavigation()==Receipt.bOriginalCanEverAffectNavigation);
 TestTrue(TEXT("safe rollback"),URBOptimizationSource::RestoreOriginalIfUnchanged(Receipt,Error));TestFalse(TEXT("visible again"),H->bHiddenInGame!=0);
 TestEqual(TEXT("profile restored"),H->GetCollisionProfileName(),FName(TEXT("BlockAllDynamic")));
 const bool SecondAdoption=URBOptimizationSource::AdoptReviewedSource(M,100,true,Receipt,Error);
 TestTrue(TEXT("second adoption"),SecondAdoption); if(!SecondAdoption){AddError(TEXT("second adoption detail: ")+Error);return false;}
 TestTrue(TEXT("consume after second adoption"),Receipt.Group->ConsumeItem(100));
 TestFalse(TEXT("cannot resurrect consumed prop"),URBOptimizationSource::RestoreOriginalIfUnchanged(Receipt,Error));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBOptImportStale,"RBOptimization.Source.StaleManifestRefused",Flags)
bool FRBOptImportStale::RunTest(const FString&) {
 FTestWorld T;auto* A=T.W->SpawnActor<ARBOptimizationActor>();A->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
 A->Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));A->Mesh->ComponentTags.Add(TEXT("RBOpt.FrozenSource"));
 FRBOptSourceManifest M;FRBOptSourceReceipt Receipt;FString Error;
 TestTrue(TEXT("static component inspect"),URBOptimizationSource::InspectFrozenSource(A->Mesh,FGuid::NewGuid(),M,Error));
 A->SetActorLocation(FVector(100,0,0));
 TestFalse(TEXT("stale pose refused"),URBOptimizationSource::AdoptReviewedSource(M,1,true,Receipt,Error));
 TestFalse(TEXT("source still visible"),A->Mesh->bHiddenInGame!=0);return true;
}

#endif
