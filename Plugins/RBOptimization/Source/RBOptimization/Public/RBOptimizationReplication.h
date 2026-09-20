#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RBOptimizationState.h"
#include "RBOptimizationReplication.generated.h"
class ARBOptimizationGroup;
class ARBOptimizationReplicator;
class APawn;

// Only public presentation travels over this channel. Inventory/save payload stays authoritative.
USTRUCT(BlueprintType)
struct FRBOptNetItem : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Network") int64 Id=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Network") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Network") FTransform Transform;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Network") ERBOptRepresentation State=ERBOptRepresentation::Instance;
};
USTRUCT()
struct FRBOptNetList : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY() TArray<FRBOptNetItem> Items;
    TWeakObjectPtr<ARBOptimizationReplicator> Owner;
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& Params) {
        return FastArrayDeltaSerialize<FRBOptNetItem,FRBOptNetList>(Items,Params,*this);
    }
    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Params);
};
template<> struct TStructOpsTypeTraits<FRBOptNetList> : TStructOpsTypeTraitsBase2<FRBOptNetList> {
    enum { WithNetDeltaSerializer=true };
};
USTRUCT()
struct FRBOptNetConfig {
    GENERATED_BODY()
    UPROPERTY() FGuid Collection;
    UPROPERTY() TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY() TEnumAsByte<EComponentMobility::Type> Mobility=EComponentMobility::Movable;
    UPROPERTY() FName CollisionProfile=TEXT("BlockAllDynamic");
    UPROPERTY() TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled=ECollisionEnabled::QueryAndPhysics;
    UPROPERTY() TEnumAsByte<ECollisionChannel> CollisionObjectType=ECC_WorldStatic;
    UPROPERTY() FCollisionResponseContainer CollisionResponses;
    UPROPERTY() bool bGenerateOverlapEvents=false;
    UPROPERTY() bool bCanEverAffectNavigation=false;
    UPROPERTY() bool bCastShadow=true;
    UPROPERTY() bool bReceivesDecals=true;
    UPROPERTY() bool bRenderCustomDepth=false;
    UPROPERTY() int32 CustomDepthStencilValue=0;
    UPROPERTY() bool bLightingChannel0=true;
    UPROPERTY() bool bLightingChannel1=false;
    UPROPERTY() bool bLightingChannel2=false;
    UPROPERTY() bool bReady=false;
};
// Bounded 512-item shards for the native simple-prop representation only.
// Does not pretend to replicate arbitrary Blueprint variables, skeletal animation or Chaos state.
UCLASS(BlueprintType)
class RBOPTIMIZATION_API ARBOptimizationReplicator : public AActor {
    GENERATED_BODY()
public:
    ARBOptimizationReplicator();
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Network")
    bool AttachAuthoritativeGroup(ARBOptimizationGroup* Group,FString& Error);
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Network")
    bool DetachAuthoritativeGroup(FString& Error);
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Network") void Synchronize();
    UFUNCTION(BlueprintPure,Category="RB Optimization|Network") TArray<FRBOptNetItem> GetItems() const { return Wire.Items; }
    UFUNCTION(BlueprintPure,Category="RB Optimization|Network") ARBOptimizationGroup* GetClientView() const { return ClientView; }
    UPROPERTY(BlueprintReadOnly,Category="RB Optimization|Network") FString LastFault;
    UPROPERTY(EditAnywhere,Category="RB Optimization|Network") double MaximumRequestDistance=500;
    bool RequestActivation(APawn* Pawn,int64 Id,int64 ExpectedRevision);
    void ReconcileClient();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void BeginPlay() override;
private:
    UPROPERTY(ReplicatedUsing=OnRep_Config) FRBOptNetConfig Config;
    UPROPERTY(Replicated) FRBOptNetList Wire;
    UPROPERTY(Transient) TObjectPtr<ARBOptimizationGroup> Source;
    UPROPERTY(Transient) TObjectPtr<ARBOptimizationGroup> ClientView;
    TMap<int64,int32> Index;
    TMap<int64,int64> Applied;
    TSet<int64> Dirty;
    FTimerHandle Timer;
    bool bSynchronizing=false;
    UFUNCTION() void OnRep_Config();
    UFUNCTION() void SourceDestroyed(AActor* Destroyed);
    UFUNCTION() void Changed(int64 Id,ERBOptRepresentation State);
};
// Place on an owned pawn/controller. UE validates RPC ownership; the server independently
// validates revision, pawn position, request rate and feature permission.
UCLASS(ClassGroup=(RefinedBadger),meta=(BlueprintSpawnableComponent))
class RBOPTIMIZATION_API URBOptRequestComponent : public UActorComponent {
    GENERATED_BODY()
public:
    URBOptRequestComponent();
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Network")
    void RequestActivation(ARBOptimizationReplicator* Target,int64 Id,int64 ExpectedRevision);
private:
    double LastRequest=-100;
    UFUNCTION(Server,Reliable) void ServerRequest(ARBOptimizationReplicator* Target,int64 Id,int64 ExpectedRevision);
};
