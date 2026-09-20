#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RBRoutineTypes.h"
#include "RBRoutinePopulationTypes.generated.h"

UENUM(BlueprintType)
enum class ERBRoutineRepresentationState : uint8
{
    LogicalOnly,
    Embodied
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineCoarseLocation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AreaId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AnchorId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasWorldLocation = false;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineCoarseTravelState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OriginAreaId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OriginAnchorId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector OriginWorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasOriginWorldLocation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName DestinationAreaId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName DestinationAnchorId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector DestinationWorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasDestinationWorldLocation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 StartedWorldMinute = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 ArrivalWorldMinute = 0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutinePersonSocialContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName HouseholdId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FactionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CommunityId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName WorkplaceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RoleId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ImportantAgentIds;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineLogicalPersonRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSoftObjectPath ProfilePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineSnapshot RoutineState;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERBRoutineSimulationLOD SimulationLOD = ERBRoutineSimulationLOD::Dormant;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERBRoutineRepresentationState RepresentationState = ERBRoutineRepresentationState::LogicalOnly;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineCoarseLocation Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineCoarseTravelState Travel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutinePersonSocialContext Social;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CurrentIntentionGoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag CurrentCoarseActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 LastSimulatedWorldMinute = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNeedsAuthoritativeReconciliation = false;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutinePopulationSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SchemaVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int64 CapturedWorldMinute = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineLogicalPersonRecord> People;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutinePopulationMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 LogicalPeople = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EmbodiedPeople = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RecordsVisitedLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CatchUpsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 FullEvaluationsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ReducedEvaluationsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DormantEvaluationsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PromotionRequestsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DemotionRequestsLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int64 CatchUpWorldMinutesAppliedLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int64 EstimatedAllocatedBytes = 0;

    UPROPERTY(BlueprintReadOnly)
    double SchedulerMillisecondsLastTick = 0.0;

    UPROPERTY(BlueprintReadOnly)
    double CatchUpMillisecondsLastTick = 0.0;

    UPROPERTY(BlueprintReadOnly)
    int32 AgentUpdatesVisitedLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DecisionsExecutedLastTick = 0;

    UPROPERTY(BlueprintReadOnly)
    double DecisionMillisecondsLastTick = 0.0;
};

UENUM(BlueprintType)
enum class ERBRoutineItemIntent : uint8
{
    Equip,
    Unequip,
    Use,
    Carry
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineItemAuthorityRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid RequestId;

    UPROPERTY(BlueprintReadOnly)
    FName AgentId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    ERBRoutineItemIntent Intent = ERBRoutineItemIntent::Equip;

    UPROPERTY(BlueprintReadOnly)
    FString StableItemRef;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag ActivityTag;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineItemAuthorityResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid RequestId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSucceeded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ResultCode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactEffect> RoutineFactUpdates;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineSocialReservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid ReservationId;

    UPROPERTY(BlueprintReadOnly)
    FName AgentA = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName AgentB = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag ActivityTag;

    UPROPERTY(BlueprintReadOnly)
    FName SiteId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int64 ReservedWorldMinute = 0;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineDialogueContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName AgentId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName OtherAgentId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName CurrentIntentionGoalId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag CurrentActivityTag;

    UPROPERTY(BlueprintReadOnly)
    FRBRoutinePersonSocialContext Social;

    UPROPERTY(BlueprintReadOnly)
    bool bHasRelationship = false;

    UPROPERTY(BlueprintReadOnly)
    FRBRoutineRelationshipState Relationship;

    UPROPERTY(BlueprintReadOnly)
    TArray<FRBRoutineMemoryRecord> RelevantMemories;
};

USTRUCT(BlueprintType)
struct RBROUTINE_API FRBRoutineDialogueOutcome
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName OtherAgentId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRBRoutineMemoryRecord Memory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double AffinityDelta = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double TrustDelta = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double FearDelta = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRBRoutineFactEffect> RoutineFactUpdates;
};
