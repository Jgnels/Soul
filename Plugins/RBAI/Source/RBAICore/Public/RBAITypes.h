#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RBAITypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ERBAIResponseCurve : uint8
{
    Linear,
    InverseLinear,
    Quadratic,
    InverseQuadratic,
    Step
};

USTRUCT(BlueprintType)
struct RBAICORE_API FRBAIConsiderationDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility")
    FGameplayTag SignalTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility")
    float MinValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility")
    float MaxValue = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility")
    ERBAIResponseCurve Curve = ERBAIResponseCurve::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility", meta=(ClampMin="0.001"))
    float Exponent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Utility", meta=(ClampMin="0.0"))
    float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct RBAICORE_API FRBAIActionDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    FGameplayTag ActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    int32 PriorityGroup = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action", meta=(ClampMin="0.0"))
    float BaseScore = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action", meta=(ClampMin="0.0"))
    float InertiaBonus = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    bool bInterruptible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    FGameplayTagContainer BlockedTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Action")
    TArray<FRBAIConsiderationDefinition> Considerations;
};

USTRUCT(BlueprintType)
struct RBAICORE_API FRBAIContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Context")
    FName ContextId = TEXT("Self");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Context")
    TObjectPtr<AActor> ContextActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Context")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Context")
    FGameplayTagContainer Tags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Context")
    TMap<FGameplayTag, float> Signals;
};

USTRUCT(BlueprintType)
struct RBAICORE_API FRBAIBrainSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Snapshot")
    int32 Version = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Snapshot")
    FGameplayTag CurrentActionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Snapshot")
    FName CurrentContextId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Snapshot")
    int64 DecisionRevision = 0;
};

USTRUCT(BlueprintType)
struct RBAICORE_API FRBAIReplicatedDecision
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Decision")
    FGameplayTag ActionTag;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Decision")
    FName ContextId;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Decision")
    TObjectPtr<AActor> ContextActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Decision")
    int64 Revision = 0;
};
