#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBOptimizationAudit.generated.h"

UCLASS()
class RBOPTIMIZATION_API URBOptimizationAudit : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Explicit, read-only inventory of the supplied world's LOADED actors.
    // Findings are candidates, not measured timings or safe automatic mutations.
    UFUNCTION(BlueprintCallable, Category="RB Optimization", meta=(WorldContext="WorldContextObject"))
    static bool WriteLoadedWorldAudit(const UObject* WorldContextObject, const FString& FileName, FString& OutputPath);
};
