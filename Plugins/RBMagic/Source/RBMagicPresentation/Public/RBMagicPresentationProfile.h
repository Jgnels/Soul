#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RBMagicPresentationProfile.generated.h"

class UNiagaraSystem;
class USoundBase;
class UTexture2D;

UCLASS(BlueprintType)
class RBMAGICPRESENTATION_API URBMagicPresentationProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
    FGameplayTag SpellTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spellbook")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> CastSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> ProjectileSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> ImpactSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
    TSoftObjectPtr<UNiagaraSystem> PersistentSystem;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> CastSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> ImpactSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundBase> LoopSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icon Capture", meta=(ClampMin="0.0"))
    float IconCaptureTimeSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icon Capture", meta=(ClampMin="0.01"))
    float IconCaptureScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Icon Capture")
    FVector IconCaptureOffset = FVector::ZeroVector;
};
