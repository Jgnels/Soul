#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RBAITypes.h"
#include "RBAIProfile.generated.h"

UCLASS(BlueprintType)
class RBAICORE_API URBAIProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Actions")
    TArray<FRBAIActionDefinition> Actions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.01"))
    float NearIntervalSeconds = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.01"))
    float MediumIntervalSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.01"))
    float FarIntervalSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.01"))
    float OutOfBoundsIntervalSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.0"))
    float NearDistance = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.0"))
    float MediumDistance = 7500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Scheduler", meta=(ClampMin="0.0"))
    float FarDistance = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Utility", meta=(ClampMin="0.0"))
    float MinimumActionScore = 0.000001f;
};
