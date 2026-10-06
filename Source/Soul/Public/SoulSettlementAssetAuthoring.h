#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SoulSettlementAssetAuthoring.generated.h"

class UMaterialInstanceConstant;
class UMaterialFunctionInterface;
class UMaterialFunctionInstance;
class UWorld;

// Editor-only access to UE's material-layer update API, which Python does not
// expose. Asset duplication, provenance and saving remain explicit authoring steps.
UCLASS()
class SOUL_API USoulSettlementAssetAuthoring : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Soul|Settlement|Authoring", meta=(DevelopmentOnly))
    static bool RebindUnloadedOwnedSublevel(UWorld* World, FName SourcePackage, FName OwnedReplacement);

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement|Authoring", meta=(DevelopmentOnly))
    static bool ReparentOwnedMaterialFunction(UMaterialFunctionInstance* Function, UMaterialFunctionInterface* Parent);

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement|Authoring", meta=(DevelopmentOnly))
    static bool RemapOwnedMaterialLayers(UMaterialInstanceConstant* Material,
        const TMap<UMaterialFunctionInterface*, UMaterialFunctionInterface*>& Replacements);
};
