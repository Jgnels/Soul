#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RBAITypes.h"
#include "RBAIBrainComponent.generated.h"

class URBAIProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRBAIActionRequested,
    FGameplayTag, ActionTag, AActor*, ContextActor, FName, ContextId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBAIDecisionChanged, int64, Revision);

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAICORE_API URBAIBrainComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAIBrainComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Profile")
    TObjectPtr<URBAIProfile> Profile;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Decision")
    FRBAIActionRequested OnActionRequested;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Decision")
    FRBAIDecisionChanged OnDecisionChanged;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRepDecision, Category="RB AI|Decision")
    FRBAIReplicatedDecision Decision;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|State")
    FGameplayTagContainer StateTags;

    UFUNCTION(BlueprintCallable, Category="RB AI|Actions")
    void SetRuntimeActions(const TArray<FRBAIActionDefinition>& InActions);

    UFUNCTION(BlueprintCallable, Category="RB AI|Actions")
    void ClearRuntimeActions();

    UFUNCTION(BlueprintPure, Category="RB AI|Actions")
    bool HasRuntimeActions() const { return !RuntimeActions.IsEmpty(); }

    UFUNCTION(BlueprintCallable, Category="RB AI|State")
    void SetSignal(FGameplayTag SignalTag, float Value);

    UFUNCTION(BlueprintCallable, Category="RB AI|State")
    void ClearSignal(FGameplayTag SignalTag);

    UFUNCTION(BlueprintCallable, Category="RB AI|State")
    void AddStateTag(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category="RB AI|State")
    void RemoveStateTag(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category="RB AI|Context")
    void UpsertContext(const FRBAIContext& Context);

    UFUNCTION(BlueprintCallable, Category="RB AI|Context")
    void RemoveContext(FName ContextId);

    UFUNCTION(BlueprintCallable, Category="RB AI|Context")
    void ClearContexts();

    UFUNCTION(BlueprintCallable, Category="RB AI|Decision")
    void RequestEvaluation();

    UFUNCTION(BlueprintCallable, Category="RB AI|Decision")
    bool EvaluateNow();

    UFUNCTION(BlueprintCallable, Category="RB AI|Decision")
    void NotifyActionFinished(bool bSucceeded = true);

    UFUNCTION(BlueprintPure, Category="RB AI|Decision")
    bool IsActionInProgress() const { return bActionInProgress; }

    UFUNCTION(BlueprintPure, Category="RB AI|Decision")
    FGameplayTag GetCurrentActionTag() const { return Decision.ActionTag; }

    UFUNCTION(BlueprintCallable, Category="RB AI|Snapshot")
    FRBAIBrainSnapshot CaptureSnapshot() const;

    UFUNCTION(BlueprintCallable, Category="RB AI|Snapshot")
    bool RestoreSnapshot(const FRBAIBrainSnapshot& Snapshot);

    float GetDesiredEvaluationInterval(float DistanceSquared) const;
    double NextEvaluationWorldTime = 0.0;

protected:
    UFUNCTION()
    void OnRepDecision();

private:
    const TArray<FRBAIActionDefinition>& GetActionDefinitions() const;
    const FRBAIActionDefinition* FindActionDefinition(FGameplayTag ActionTag) const;
    bool IsActionAllowed(const FRBAIActionDefinition& Action, const FRBAIContext& Context) const;

    UPROPERTY(Transient)
    TArray<FRBAIActionDefinition> RuntimeActions;

    UPROPERTY(Transient)
    TMap<FGameplayTag, float> Signals;

    UPROPERTY(Transient)
    TArray<FRBAIContext> Contexts;

    bool bActionInProgress = false;
};
