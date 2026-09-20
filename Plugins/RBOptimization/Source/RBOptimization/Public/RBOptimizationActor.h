#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RBOptimizationActor.generated.h"
class UStaticMeshComponent;

// Only explicitly registered RB-owned representations are materialized.
// Game systems retain the authoritative health, inventory, quest and save state.
UCLASS(Blueprintable)
class RBOPTIMIZATION_API ARBOptimizationActor : public AActor
{
    GENERATED_BODY()
public:
    ARBOptimizationActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="RB Optimization")
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization")
    int64 StableId=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    FString DomainPayload;
    // Custom actors default to refusing retirement. Their owner must explicitly consent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    bool bAllowReturnToInstance=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization")
    bool bGameplayPinned=false;
    // A successful callback must export every transient state the host needs.
    UFUNCTION(BlueprintNativeEvent, Category="RB Optimization")
    bool ExportForReturn(FString& Payload) const;
    virtual bool ExportForReturn_Implementation(FString& Payload) const;

    // Pooling is opt-in for custom subclasses. The native base actor is reusable by default.
    UFUNCTION(BlueprintNativeEvent, Category="RB Optimization|Pooling")
    bool PrepareForPool();
    virtual bool PrepareForPool_Implementation();
    UFUNCTION(BlueprintNativeEvent, Category="RB Optimization|Pooling")
    bool RestoreFromPool(int64 NewStableId, const FString& Payload);
    virtual bool RestoreFromPool_Implementation(int64 NewStableId, const FString& Payload);

    bool IsQuiescent() const;
};
