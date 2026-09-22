#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RBMagicTypes.generated.h"

UENUM(BlueprintType)
enum class ERBMagicTargetMode : uint8
{
    Self,
    Unit,
    Ground,
    Direction,
    Global
};

UENUM(BlueprintType)
enum class ERBMagicDeliveryMode : uint8
{
    Instant,
    Projectile,
    Beam,
    Chain,
    Area,
    PersistentArea,
    Summon,
    Strategic
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicEntityRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FName Domain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGuid Id;

    bool IsValid() const { return !Domain.IsNone() && Id.IsValid(); }
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicResourceCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGameplayTag ResourceTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic", meta=(ClampMin="0.0"))
    float Amount = 0.0f;
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicEffectSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGameplayTag EffectTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    float Magnitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic", meta=(ClampMin="0.0"))
    float DurationSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic", meta=(ClampMin="0.0"))
    float Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic", meta=(ClampMin="0"))
    int32 MaxTargets = 1;
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicTarget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicEntityRef Entity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FVector Direction = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicCastRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGuid CastId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGameplayTag SpellTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicEntityRef Caster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicTarget Target;
};

USTRUCT(BlueprintType)
struct RBMAGICCORE_API FRBMagicEffectIntent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGuid CastId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FGameplayTag SpellTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicEntityRef Caster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicTarget Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Magic")
    FRBMagicEffectSpec Effect;
};
