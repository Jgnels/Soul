#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RBRoutineTypes.generated.h"

UENUM(BlueprintType)
enum class ERBRoutineFactType : uint8
{
    Bool,
    Integer,
    Number,
    Name
};

UENUM(BlueprintType)
enum class ERBRoutineCompareOp : uint8
{
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual
};

UENUM(BlueprintType)
enum class ERBRoutineSimulationLOD : uint8
{
    Full,
    Reduced,
    Dormant
};

UENUM(BlueprintType)
enum class ERBRoutineStatus : uint8
{
    Idle,
    Planning,
    Executing,
    Interrupted,
    Blocked
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineFactValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERBRoutineFactType Type = ERBRoutineFactType::Bool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 IntegerValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double NumberValue = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName NameValue = NAME_None;

    static FRBRoutineFactValue FromBool(bool Value);
    static FRBRoutineFactValue FromInteger(int64 Value);
    static FRBRoutineFactValue FromNumber(double Value);
    static FRBRoutineFactValue FromName(FName Value);
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineFactCondition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Key = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERBRoutineCompareOp Operator = ERBRoutineCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineFactValue Value;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineFactEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Key = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineFactValue Value;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineUtilityTerm
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactKey = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Weight = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Baseline = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bClampInput = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bClampInput"))
    double MinInput = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bClampInput"))
    double MaxInput = 1.0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineGoalDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName GoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag GoalTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double BasePriority = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactCondition> Eligibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactCondition> Desired;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineUtilityTerm> UtilityTerms;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineActionDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactCondition> Preconditions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactEffect> PredictedEffects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.001"))
    double BaseCost = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bInterruptible = true;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineScheduleEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EntryId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="127"))
    int32 DayMask = 127;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1439"))
    int32 StartMinuteOfDay = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="1440"))
    int32 EndMinuteOfDay = 1440;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName PreferredGoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double PriorityBoost = 25.0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineMemoryRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OtherAgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    double Salience = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-1.0", ClampMax="1.0"))
    double Valence = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 WorldMinute = 0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineRelationshipState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OtherAgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-1.0", ClampMax="1.0"))
    double Affinity = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-1.0", ClampMax="1.0"))
    double Trust = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    double Fear = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 LastChangedWorldMinute = 0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineInterruptRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag ReasonTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ForcedGoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 ExpireAtWorldMinute = -1;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, FRBRoutineFactValue> Facts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineMemoryRecord> Memories;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineRelationshipState> Relationships;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 CapturedWorldMinute = 0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutinePerceptionEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName EventId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SourceAgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRecordMemory = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
    double Salience = 0.5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="-1.0", ClampMax="1.0"))
    double Valence = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactEffect> RoutineFactUpdates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequestInterrupt = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InterruptPriority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ForcedGoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag InterruptReasonTag;
};
