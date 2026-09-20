#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/RBRepresentationPolicy.h"
#include "RBOptimizationState.h"
#include "RBOptimizationGroup.generated.h"
class ARBOptimizationActor;
class UMaterialInterface;
class ARBOptimizationReplicator;

USTRUCT()
struct FRBOptimizationRecord
{
    GENERATED_BODY()
    UPROPERTY() FTransform Transform;
    UPROPERTY() FString Payload;
    UPROPERTY() TObjectPtr<ARBOptimizationActor> Actor;
    double RespawnRemainingSeconds=-1;
    FPrimitiveInstanceId Instance;
};

// Standalone prototype only. Fails closed in every networked net mode.
// Does not harvest, edit or replace existing actors, foliage, PCG, or vendor assets.
UCLASS(Blueprintable)
class RBOPTIMIZATION_API ARBOptimizationGroup : public AActor
{
    GENERATED_BODY()
public:
    ARBOptimizationGroup();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization") FGuid CollectionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization") bool bHitActivation=true;
    UPROPERTY(BlueprintAssignable, Category="RB Optimization") FRBOptTransitionEvent OnRepresentationChanged;
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool InitializeFromTable(UDataTable* Table, FName RowName, FString& Error);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool CaptureSnapshot(FRBOptSnapshot& Snapshot, FString& Error);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool RestoreIntoFreshGroup(const FRBOptSnapshot& Snapshot, FString& Error);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool ScheduleRespawn(int64 Id, double DelayGameSeconds);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    int32 AdvanceRespawnTime(double DeltaGameSeconds);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool RespawnItem(int64 Id, const FTransform& Transform, const FString& Payload);
    UFUNCTION(BlueprintPure, Category="RB Optimization")
    FRBOptDiagnostics GetDiagnostics() const;
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool GetItemState(int64 Id, FRBOptSavedItem& State) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RB Optimization")
    TObjectPtr<UInstancedStaticMeshComponent> Instances;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    TSubclassOf<ARBOptimizationActor> MaterializedClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    double ActivationRadius=500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    double RetirementRadius=750;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    double MinimumActiveSeconds=2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    int32 MaxTransitionsPerUpdate=8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    double SoftTransitionBudgetMs=2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Pooling", meta=(ClampMin="0",ClampMax="1024"))
    int32 MaxPooledActors=64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    bool bAutomaticProximity=false;
    UFUNCTION(BlueprintCallable, Category="RB Optimization|Pooling")
    int32 PrewarmActorPool(int32 TargetPooledActors);
    UFUNCTION(BlueprintCallable, Category="RB Optimization|Pooling")
    int32 TrimActorPool(int32 KeepActors=0);
    UFUNCTION(BlueprintPure, Category="RB Optimization|Pooling")
    int32 GetPooledActorCount() const { return ActorPool.Num(); }
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool InitializeGroup(UStaticMesh* Mesh, const TArray<UMaterialInterface*>& Materials, FName CollisionProfile);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool RegisterItem(int64 Id, const FTransform& Transform, const FString& Payload);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    ARBOptimizationActor* ActivateItem(int64 Id, double HoldSeconds=2);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    ARBOptimizationActor* ActivateHit(const FHitResult& Hit, int64 ExpectedInstanceRevision, double HoldSeconds=2);
    UFUNCTION(BlueprintPure, Category="RB Optimization")
    int64 GetInstanceRevision() const { return InstanceRevision; }
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool SetPinned(int64 Id, bool bPinned);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    bool ConsumeItem(int64 Id);
    UFUNCTION(BlueprintCallable, Category="RB Optimization")
    void UpdateViewers(const TArray<FVector>& Locations);
    UFUNCTION(BlueprintPure, Category="RB Optimization")
    int32 GetActiveCount() const;
    UFUNCTION(BlueprintPure, Category="RB Optimization")
    int32 GetRegisteredCount() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TMap<int64,FRBOptimizationRecord> Records;
    UPROPERTY(Transient) TArray<TObjectPtr<ARBOptimizationActor>> ActorPool;
    TUniquePtr<rbopt::Registry> Policy;
    TArray<int64> IndexToKey;
    int64 InstanceRevision=1;
    TSet<int64> DomainPins;
    FName Profile=NAME_None;
    FTimerHandle Timer;
    bool bChanging=false;
    bool bReceivingAuthority=false;
    bool bMembershipFrozen=false;
    friend class ARBOptimizationReplicator;
    bool ApplyAuthorityItem(int64 Id, const FTransform& Transform, ERBOptRepresentation State);
    FRBOptDiagnostics Diagnostics;
    bool CanOperate() const;
    bool Materialize(int64 Id);
    bool Retire(int64 Id, const rbopt::Change& Change);
    ARBOptimizationActor* AcquirePooledActor(int64 Id, const FString& Payload, const FTransform& Transform);
    bool ReturnActorToPool(ARBOptimizationActor* Actor);
    ARBOptimizationActor* SpawnFreshActor(int64 Id, const FString& Payload, const FTransform& Transform);
    bool ApplyRepresentationPresentation(ARBOptimizationActor* Actor) const;
    void PollPlayers();
    double Now() const;
    void RemoveProxy(FRBOptimizationRecord& Record);
};
