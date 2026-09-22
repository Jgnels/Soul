#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RBMagicTypes.h"
#include "RBMagicSpellDefinition.generated.h"

UCLASS(BlueprintType)
class RBMAGICCORE_API URBMagicSpellDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FGameplayTag SpellTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FGameplayTag SchoolTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FGameplayTagContainer SpellTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display", meta=(MultiLine="true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast")
    ERBMagicTargetMode TargetMode = ERBMagicTargetMode::Unit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast")
    ERBMagicDeliveryMode DeliveryMode = ERBMagicDeliveryMode::Instant;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast", meta=(ClampMin="0.0"))
    float CastTimeSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast", meta=(ClampMin="0.0"))
    float CooldownSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast", meta=(ClampMin="0.0"))
    float Range = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cast", meta=(ClampMin="0.0"))
    float AreaRadius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Economy")
    TArray<FRBMagicResourceCost> Costs;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects")
    TArray<FRBMagicEffectSpec> Effects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI", meta=(ClampMin="0.0"))
    float AIBaseUtility = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rules")
    bool bStrategicOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rules")
    bool bBattleOnly = false;
};
