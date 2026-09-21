#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RBAIBossEncounterComponent.generated.h"

class URBAIBossProfile;
class URBAIBrainComponent;

USTRUCT(BlueprintType)
struct RBAIBOSS_API FRBAIBossRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    FGameplayTag PhaseTag;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    FGameplayTag PendingActionTag;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    int32 NextActionIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    int64 Revision = 0;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Boss")
    bool bTelegraphing = false;
};

USTRUCT(BlueprintType)
struct RBAIBOSS_API FRBAIBossSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    int32 Version = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    FGameplayTag PhaseTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    int32 NextActionIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Boss")
    int64 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBAIBossPhaseChanged,
    FGameplayTag, PhaseTag, int64, Revision);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRBAIBossTelegraphRequested,
    FGameplayTag, ActionTag, AActor*, TargetActor, float, TelegraphSeconds, int64, Revision);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRBAIBossActionRequested,
    FGameplayTag, ActionTag, AActor*, TargetActor, int64, Revision);

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAIBOSS_API URBAIBossEncounterComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAIBossEncounterComponent();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RB AI|Boss")
    TObjectPtr<URBAIBossProfile> Profile;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRepState, Category="RB AI|Boss")
    FRBAIBossRuntimeState State;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Boss")
    FRBAIBossPhaseChanged OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Boss")
    FRBAIBossTelegraphRequested OnTelegraphRequested;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Boss")
    FRBAIBossActionRequested OnBossActionRequested;

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    bool SetHealthNormalized(float Health01);

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    bool RequestNextAction(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    bool CommitTelegraphedAction();

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    bool NotifyActionFinished(bool bSucceeded = true);

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    FRBAIBossSnapshot CaptureSnapshot() const;

    UFUNCTION(BlueprintCallable, Category="RB AI|Boss")
    bool RestoreSnapshot(const FRBAIBossSnapshot& Snapshot);

protected:
    UFUNCTION()
    void OnRepState();

private:
    int32 FindCurrentPhaseIndex() const;
    const struct FRBAIBossActionSpec* FindPendingActionSpec() const;
    void PublishPhaseChange();

    UPROPERTY(Transient)
    TObjectPtr<URBAIBrainComponent> Brain;

    bool bActionInProgress = false;
};
