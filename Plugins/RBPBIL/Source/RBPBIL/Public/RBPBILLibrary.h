#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBPBILTypes.h"
#include "RBPBILLibrary.generated.h"

UCLASS()
class RBPBIL_API URBPBILLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="RB PBIL")
    static FName GetChannelTag(ERBPBILChannel Channel);

    UFUNCTION(BlueprintCallable, Category="RB PBIL",
        meta=(WorldContext="WorldContextObject"))
    static bool QueryBestLocationImmediate(
        UObject* WorldContextObject,
        const FRBPBILQueryRequest& Request,
        FRBPBILQueryResult& OutResult);
    UFUNCTION(BlueprintCallable, Category="RB PBIL",
        meta=(WorldContext="WorldContextObject"))
    static bool SampleChannelImmediate(
        UObject* WorldContextObject,
        ERBPBILChannel Channel,
        FVector WorldLocation,
        float& OutValue);
};
