#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RBAICombatExecutionBridgeComponent.generated.h"

class URBAIBrainComponent;

UENUM(BlueprintType)
enum class ERBAICombatRequestType : uint8
{
    None,
    Attack,
    Pursue,
    Flee,
    Guard,
    Flank,
    TakeCover,
    Investigate
};

USTRUCT(BlueprintType)
struct RBAICOMBAT_API FRBAICombatRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    FGuid RequestId;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    ERBAICombatRequestType Type = ERBAICombatRequestType::None;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    FGameplayTag ActionTag;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    FName ContextId;

    UPROPERTY(BlueprintReadOnly, Category="RB AI|Combat")
    int64 DecisionRevision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBAICombatRequestIssued,
    const FRBAICombatRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBAICombatRequestResolved,
    FGuid, RequestId, bool, bSucceeded);

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAICOMBAT_API URBAICombatExecutionBridgeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAICombatExecutionBridgeComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Combat")
    FRBAICombatRequestIssued OnCombatRequest;

    UPROPERTY(BlueprintAssignable, Category="RB AI|Combat")
    FRBAICombatRequestResolved OnCombatRequestResolved;

    UFUNCTION(BlueprintCallable, Category="RB AI|Combat")
    bool ResolveRequest(FGuid RequestId, bool bSucceeded);

    UFUNCTION(BlueprintPure, Category="RB AI|Combat")
    bool HasPendingRequest() const { return PendingRequest.RequestId.IsValid(); }

    UFUNCTION(BlueprintPure, Category="RB AI|Combat")
    FRBAICombatRequest GetPendingRequest() const { return PendingRequest; }

private:
    UFUNCTION()
    void HandleActionRequested(FGameplayTag ActionTag, AActor* ContextActor, FName ContextId);

    static ERBAICombatRequestType MapAction(FGameplayTag ActionTag);

    UPROPERTY(Transient)
    TObjectPtr<URBAIBrainComponent> Brain;

    UPROPERTY(Transient)
    FRBAICombatRequest PendingRequest;
};
