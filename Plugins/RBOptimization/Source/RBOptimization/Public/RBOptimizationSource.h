#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBOptimizationSource.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class ARBOptimizationGroup;

// A runtime takeover, not an editor asset conversion. Source meshes/instance arrays are never deleted.
USTRUCT(BlueprintType)
struct FRBOptSourceManifest {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") TObjectPtr<UStaticMeshComponent> Source;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") FGuid CollectionId;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") TArray<TObjectPtr<UMaterialInterface>> Materials;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") TArray<FTransform> Transforms;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") FString SourcePath;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") FString PresentationSignature;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") bool bInstanced=false;
};
USTRUCT(BlueprintType)
struct FRBOptSourceReceipt {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") FRBOptSourceManifest Original;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") TObjectPtr<ARBOptimizationGroup> Group;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") int64 FirstId=1;
    UPROPERTY(BlueprintReadOnly, Category="RB Optimization|Source") bool bAdopted=false;
    UPROPERTY() TEnumAsByte<ECollisionEnabled::Type> OriginalCollision=ECollisionEnabled::NoCollision;
    UPROPERTY() FName OriginalCollisionProfile=NAME_None;
    UPROPERTY() TEnumAsByte<ECollisionChannel> OriginalObjectType=ECC_WorldStatic;
    UPROPERTY() FCollisionResponseContainer OriginalResponses;
    UPROPERTY() bool bOriginalGenerateOverlapEvents=false;
    UPROPERTY() bool bOriginalCanEverAffectNavigation=false;
    UPROPERTY() FString AdoptedPresentationSignature;
};
UCLASS()
class RBOPTIMIZATION_API URBOptimizationSource : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    // Producer must tag the component RBOpt.FrozenSource after stopping regeneration/mutation.
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Source")
    static bool InspectFrozenSource(UStaticMeshComponent* Source,FGuid CollectionId,FRBOptSourceManifest& Manifest,FString& Error);
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Source")
    static bool AdoptReviewedSource(const FRBOptSourceManifest& Manifest,int64 FirstId,bool bVisualParityReviewed,FRBOptSourceReceipt& Receipt,FString& Error);
    // Refuses to resurrect consumed objects or discard movement/payload/state changes.
    UFUNCTION(BlueprintCallable,Category="RB Optimization|Source")
    static bool RestoreOriginalIfUnchanged(FRBOptSourceReceipt& Receipt,FString& Error);
};
