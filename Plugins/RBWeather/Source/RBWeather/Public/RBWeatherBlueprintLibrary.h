#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBWeatherTypes.h"
#include "RBWeatherBlueprintLibrary.generated.h"

class ARBWeatherDirector;

UCLASS()
class RBWEATHER_API URBWeatherBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="RB Weather", meta=(WorldContext="WorldContextObject"))
    static bool GetWeatherAtLocation(const UObject* WorldContextObject, FVector WorldLocation,
                                     FRBWeatherSnapshotAtLocation& OutSnapshot);

    UFUNCTION(BlueprintPure, Category="RB Weather", meta=(WorldContext="WorldContextObject"))
    static ARBWeatherDirector* GetWeatherDirector(const UObject* WorldContextObject);
};
