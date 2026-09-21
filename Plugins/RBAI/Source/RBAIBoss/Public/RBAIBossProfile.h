#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RBAIBossProfile.generated.h"

USTRUCT(BlueprintType)
struct RBAIBOSS_API FRBAIBossActionSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss", meta=(ClampMin="0.0"))
    float TelegraphSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss", meta=(ClampMin="0.0"))
    float CooldownSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct RBAIBOSS_API FRBAIBossPhaseSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    FGameplayTag PhaseTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss", meta=(ClampMin="0.0", ClampMax="1.0"))
    float EnterAtOrBelowHealth = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    TArray<FRBAIBossActionSpec> Actions;
};

UCLASS(BlueprintType)
class RBAIBOSS_API URBAIBossProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Boss")
    TArray<FRBAIBossPhaseSpec> Phases;

    UFUNCTION(BlueprintPure, Category="RB AI|Boss")
    int32 FindPhaseForHealth(float Health01) const;
};
