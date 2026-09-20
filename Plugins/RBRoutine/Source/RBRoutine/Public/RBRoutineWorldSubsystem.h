#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "RBRoutinePopulationTypes.h"
#include "RBRoutineWorldSubsystem.generated.h"

class URBRoutineComponent;
class URBRoutineProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBRoutineEmbodimentRequestedSignature, FName, AgentId, FRBRoutineCoarseLocation, PreferredLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBRoutineDemotionRequestedSignature, FName, AgentId, FGameplayTag, ReasonTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBRoutineItemAuthorityRequestedSignature, FRBRoutineItemAuthorityRequest, Request);

UCLASS()
class RBROUTINE_API URBRoutineWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

    UFUNCTION(BlueprintCallable, Category="RB Routine|Time")
    void SetExternalTime(int64 DayIndex, int32 MinuteOfDay);

    UFUNCTION(BlueprintPure, Category="RB Routine|Time")
    int64 GetExternalDayIndex() const { return ExternalDayIndex; }

    UFUNCTION(BlueprintPure, Category="RB Routine|Time")
    int32 GetExternalMinuteOfDay() const { return ExternalMinuteOfDay; }

    UFUNCTION(BlueprintPure, Category="RB Routine|Time")
    int64 GetExternalWorldMinute() const { return ExternalDayIndex * 1440 + ExternalMinuteOfDay; }

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool CreateLogicalPerson(FName AgentId, URBRoutineProfile* Profile, const FRBRoutineCoarseLocation& InitialLocation, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Routine|Population")
    bool GetLogicalPerson(FName AgentId, FRBRoutineLogicalPersonRecord& OutPerson) const;

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool SetLogicalPersonProfile(FName AgentId, URBRoutineProfile* Profile, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool SetLogicalPersonSocialContext(FName AgentId, const FRBRoutinePersonSocialContext& Social, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Social")
    bool BeginPairedSocialReservation(FName AgentA, FName AgentB, FGameplayTag ActivityTag, FName SiteId, FRBRoutineSocialReservation& OutReservation, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Social")
    bool ReleaseSocialReservation(FGuid ReservationId, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Routine|Social")
    bool GetSocialReservationForAgent(FName AgentId, FRBRoutineSocialReservation& OutReservation) const;

    UFUNCTION(BlueprintCallable, Category="RB Routine|Items")
    bool RequestItemAuthorityAction(FName AgentId, ERBRoutineItemIntent Intent, const FString& StableItemRef, FGameplayTag ActivityTag, FRBRoutineItemAuthorityRequest& OutRequest, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Items")
    bool ResolveItemAuthorityAction(const FRBRoutineItemAuthorityResult& Result, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Routine|Dialogue")
    bool BuildDialogueContext(FName AgentId, FName OtherAgentId, FRBRoutineDialogueContext& OutContext, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category="RB Routine|Dialogue")
    bool ApplyDialogueOutcome(const FRBRoutineDialogueOutcome& Outcome, FString& OutError);
    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool StartCoarseTravel(FName AgentId, const FRBRoutineCoarseLocation& Destination, int64 TravelDurationWorldMinutes, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool CatchUpLogicalPerson(FName AgentId, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool RequestEmbodiment(FName AgentId, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool RequestDemotion(FName AgentId, FGameplayTag ReasonTag, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool BindEmbodiedAgent(FName AgentId, URBRoutineComponent* Agent, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool UnbindEmbodiedAgent(URBRoutineComponent* Agent, bool bUnexpected, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Routine|Population")
    FRBRoutinePopulationSnapshot CreatePopulationSnapshot() const;

    UFUNCTION(BlueprintCallable, Category="RB Routine|Population")
    bool RestorePopulationSnapshot(const FRBRoutinePopulationSnapshot& Snapshot, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB Routine|Population")
    FRBRoutinePopulationMetrics GetPopulationMetrics() const;

    UFUNCTION(BlueprintPure, Category="RB Routine|Population")
    int32 GetLogicalPopulationCount() const { return LogicalPeople.Num(); }

    UFUNCTION(BlueprintPure, Category="RB Routine|Population")
    int32 GetEmbodiedPopulationCount() const { return EmbodiedAgents.Num(); }

    UPROPERTY(BlueprintAssignable, Category="RB Routine|Population")
    FRBRoutineEmbodimentRequestedSignature OnEmbodimentRequested;

    UPROPERTY(BlueprintAssignable, Category="RB Routine|Population")
    FRBRoutineDemotionRequestedSignature OnDemotionRequested;

    UPROPERTY(BlueprintAssignable, Category="RB Routine|Items")
    FRBRoutineItemAuthorityRequestedSignature OnItemAuthorityRequested;

    void RegisterAgent(URBRoutineComponent* Agent);
    void UnregisterAgent(URBRoutineComponent* Agent);
    void NotifyActivityEnded(FName AgentId, FGameplayTag ActivityTag);

    UPROPERTY(EditAnywhere, Category="Budget", meta=(ClampMin="1", ClampMax="1024"))
    int32 MaxAgentUpdatesPerTick = 32;

    UPROPERTY(EditAnywhere, Category="Population|Budget", meta=(ClampMin="1", ClampMax="4096"))
    int32 MaxPopulationRecordsPerTick = 256;

    UPROPERTY(EditAnywhere, Category="Population|Budget", meta=(ClampMin="1", ClampMax="4096"))
    int32 MaxCatchUpRecordsPerTick = 128;

    UPROPERTY(EditAnywhere, Category="Population|Budget", meta=(ClampMin="1", ClampMax="1024"))
    int32 MaxEmbodiedPopulation = 100;

    UPROPERTY(EditAnywhere, Category="Population|Budget", meta=(ClampMin="1", ClampMax="128"))
    int32 MaxEmbodimentRequestsPerTick = 8;

    UPROPERTY(EditAnywhere, Category="Population|Budget", meta=(ClampMin="1", ClampMax="128"))
    int32 MaxDemotionRequestsPerTick = 8;

    UPROPERTY(EditAnywhere, Category="LOD", meta=(ClampMin="0.0"))
    double FullSimulationDistance = 5000.0;

    UPROPERTY(EditAnywhere, Category="LOD", meta=(ClampMin="0.0"))
    double ReducedSimulationDistance = 20000.0;

private:
    TArray<TWeakObjectPtr<URBRoutineComponent>> Agents;
    TMap<TWeakObjectPtr<URBRoutineComponent>, double> NextEvaluationRealTime;
    TMap<FName, FRBRoutineLogicalPersonRecord> LogicalPeople;
    TArray<FName> PopulationOrder;
    TMap<FName, TWeakObjectPtr<URBRoutineComponent>> EmbodiedAgents;
    TMap<FName, TWeakObjectPtr<URBRoutineProfile>> PopulationProfiles;
    TSet<FName> PendingEmbodimentRequests;
    TSet<FName> PendingDemotionRequests;
    TMap<FGuid, FRBRoutineSocialReservation> SocialReservations;
    TMap<FName, FGuid> AgentSocialReservations;
    TMap<FGuid, FRBRoutineItemAuthorityRequest> PendingItemAuthorityRequests;
    int32 RoundRobinCursor = 0;
    int32 PopulationCursor = 0;
    int64 ExternalDayIndex = 0;
    int32 ExternalMinuteOfDay = 720;
    FRBRoutinePopulationMetrics PopulationMetrics;

    ERBRoutineSimulationLOD DetermineLOD(const URBRoutineComponent* Agent) const;
    ERBRoutineSimulationLOD DetermineLogicalLOD(const FRBRoutineLogicalPersonRecord& Person) const;
    void ProcessPopulation();
    bool CatchUpRecord(FRBRoutineLogicalPersonRecord& Person, int64 TargetWorldMinute, FString& OutError);
    bool ValidateLogicalPerson(const FRBRoutineLogicalPersonRecord& Person, FString& OutError) const;
    URBRoutineProfile* ResolvePopulationProfile(FName AgentId, const FRBRoutineLogicalPersonRecord& Person) const;
    FRBRoutineCoarseLocation ResolveLocationAt(const FRBRoutineLogicalPersonRecord& Person, int64 WorldMinute) const;
    void CaptureEmbodiedState(FRBRoutineLogicalPersonRecord& Person, const URBRoutineComponent* Agent, bool bUnexpected) const;
    void RefreshPopulationMetrics();
    int64 EstimatePopulationAllocatedBytes() const;
    bool ReleaseSocialReservationForAgent(FName AgentId);
    bool ApplyRoutineFactUpdates(FName AgentId, const TArray<FRBRoutineFactEffect>& Updates, FString& OutError);
    bool ApplyContinuityOutcome(FName AgentId, const FRBRoutineMemoryRecord& Memory, double AffinityDelta, double TrustDelta, double FearDelta, FString& OutError);
    bool IsPopulationAuthority() const;
};
