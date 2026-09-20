#pragma once
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Engine/DataTable.h"
#include "RBOptimizationState.generated.h"
class ARBOptimizationActor;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ERBOptRepresentation : uint8 { Instance, Actor, Consumed };

// Typed save fragment. RB Save or the game's existing persistence system owns storage.
// No UObject paths from a save are loaded or executed by the restore operation.
USTRUCT(BlueprintType)
struct RBOPTIMIZATION_API FRBOptSavedItem {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") int64 Id=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FTransform Transform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FString Payload;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") ERBOptRepresentation State=ERBOptRepresentation::Instance;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") bool bPinned=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") double RemainingHoldSeconds=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") double RespawnRemainingSeconds=-1;
};
USTRUCT(BlueprintType)
struct RBOPTIMIZATION_API FRBOptSnapshot {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") int32 SchemaVersion=3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FGuid CollectionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FString PresentationSignature;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FString MeshPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FString ClassPath;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") FName CollisionProfile;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") TArray<FString> MaterialPaths;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="RB Optimization|Save") TArray<FRBOptSavedItem> Items;
};
USTRUCT(BlueprintType)
struct RBOPTIMIZATION_API FRBOptPresetRow : public FTableRowBase {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") TSubclassOf<ARBOptimizationActor> ActorClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") FName CollisionProfile=TEXT("BlockAllDynamic");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") bool bProximityActivation=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") bool bHitActivation=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") double ActivationRadius=500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Optimization|Preset") double RetirementRadius=750;
};
USTRUCT(BlueprintType)
struct RBOPTIMIZATION_API FRBOptDiagnostics {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 Registered=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 Active=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 Instances=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 Consumed=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 PendingRespawns=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 PooledActors=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int64 PoolHits=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int64 PoolMisses=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int64 PoolReturns=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int64 PoolDiscards=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 LastInspected=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") int32 LastTransitions=0;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Diagnostics") double LastUpdateMs=0;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBOptTransitionEvent,int64,Id,ERBOptRepresentation,Representation);
