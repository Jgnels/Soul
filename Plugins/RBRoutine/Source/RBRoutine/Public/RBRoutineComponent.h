#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RBRoutineProfile.h"
#include "RBRoutineTypes.h"
#include "RBRoutineComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRBRoutineActionRequestedSignature, FName, ActionId, FGameplayTag, ActivityTag, int32, PlanSerial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBRoutineActionAbortedSignature, FName, ActionId, FGameplayTag, ReasonTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRBRoutinePlanChangedSignature, FName, GoalId, int32, ActionCount, int32, PlanSerial);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBRoutineStatusChangedSignature, ERBRoutineStatus, NewStatus);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RBROUTINE_API URBRoutineComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBRoutineComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    TObjectPtr<URBRoutineProfile> Profile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Identity")
    FName AgentId = NAME_None;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RoutineState, Category="Runtime")
    FName CurrentGoalId = NAME_None;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RoutineState, Category="Runtime")
    FName CurrentActionId = NAME_None;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RoutineState, Category="Runtime")
    FGameplayTag CurrentActivityTag;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_RoutineState, Category="Runtime")
    ERBRoutineStatus Status = ERBRoutineStatus::Idle;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Runtime")
    ERBRoutineSimulationLOD SimulationLOD = ERBRoutineSimulationLOD::Full;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Runtime")
    int32 PlanSerial = 0;

    UPROPERTY(BlueprintAssignable, Category="Routine")
    FRBRoutineActionRequestedSignature OnActionRequested;

    UPROPERTY(BlueprintAssignable, Category="Routine")
    FRBRoutineActionAbortedSignature OnActionAborted;

    UPROPERTY(BlueprintAssignable, Category="Routine")
    FRBRoutinePlanChangedSignature OnPlanChanged;

    UPROPERTY(BlueprintAssignable, Category="Routine")
    FRBRoutineStatusChangedSignature OnStatusChanged;

    UFUNCTION(BlueprintCallable, Category="Routine|Facts")
    void SetFactBool(FName Key, bool Value);

    UFUNCTION(BlueprintCallable, Category="Routine|Facts")
    void SetFactInteger(FName Key, int64 Value);

    UFUNCTION(BlueprintCallable, Category="Routine|Facts")
    void SetFactNumber(FName Key, double Value);

    UFUNCTION(BlueprintCallable, Category="Routine|Facts")
    void SetFactName(FName Key, FName Value);

    UFUNCTION(BlueprintPure, Category="Routine|Facts")
    bool GetFact(FName Key, FRBRoutineFactValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category="Routine|Planning")
    bool EvaluateNow(int64 DayIndex, int32 MinuteOfDay);

    UFUNCTION(BlueprintCallable, Category="Routine|Execution")
    void NotifyActionFinished(bool bSucceeded);

    UFUNCTION(BlueprintCallable, Category="Routine|Interrupts")
    bool RequestInterrupt(const FRBRoutineInterruptRequest& Request);

    UFUNCTION(BlueprintCallable, Category="Routine|Interrupts")
    void ClearInterrupt(FGameplayTag ReasonTag);

    UFUNCTION(BlueprintCallable, Category="Routine|Continuity")
    bool RecordMemoryEvent(const FRBRoutineMemoryRecord& Memory, double AffinityDelta, double TrustDelta, double FearDelta);

    UFUNCTION(BlueprintPure, Category="Routine|Continuity")
    bool GetRelationship(FName OtherAgentId, FRBRoutineRelationshipState& OutRelationship) const;

    UFUNCTION(BlueprintPure, Category="Routine|Persistence")
    FRBRoutineSnapshot CreateSnapshot(int64 WorldMinute) const;

    UFUNCTION(BlueprintCallable, Category="Routine|Persistence")
    bool RestoreSnapshot(const FRBRoutineSnapshot& Snapshot, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="Routine|Runtime")
    void SetSimulationLOD(ERBRoutineSimulationLOD NewLOD);

    UFUNCTION(BlueprintPure, Category="Routine|Runtime")
    double GetSuggestedDecisionIntervalSeconds() const;

    UFUNCTION(BlueprintPure, Category="Routine|Population")
    bool IsPopulationDemotionSafe(FString& OutReason) const;

    UFUNCTION(BlueprintCallable, Category="Routine|Population")
    void ConfirmPopulationHandoffReady();

    bool NeedsEvaluation() const { return bNeedsEvaluation; }
    bool IsAuthoritative() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnRep_RoutineState();

private:
    UPROPERTY()
    TMap<FName, FRBRoutineFactValue> Facts;

    UPROPERTY()
    TArray<FRBRoutineMemoryRecord> Memories;

    UPROPERTY()
    TArray<FRBRoutineRelationshipState> Relationships;

    UPROPERTY()
    FRBRoutineInterruptRequest ActiveInterrupt;

    UPROPERTY()
    bool bHasActiveInterrupt = false;

    TArray<FName> CurrentPlan;
    int32 CurrentPlanIndex = INDEX_NONE;
    bool bNeedsEvaluation = true;
    bool bPopulationHandoffReady = true;
    int64 LastDayIndex = 0;
    int32 LastMinuteOfDay = 0;

    void SetFactInternal(FName Key, const FRBRoutineFactValue& Value);
    bool IsCurrentActionInterruptible() const;
    bool AreCurrentActionPreconditionsSatisfied() const;
    const FRBRoutineActionDefinition* FindAction(FName ActionId) const;
    void StartCurrentPlanAction();
    void AbortCurrentAction(FGameplayTag ReasonTag);
    void SetStatus(ERBRoutineStatus NewStatus);
    double GetNumericFact(FName Key, bool& bFound) const;
    double ComputeGoalPriority(const FRBRoutineGoalDefinition& Goal, int64 DayIndex, int32 MinuteOfDay) const;
    void ExpireInterruptIfNeeded(int64 WorldMinute);
};
