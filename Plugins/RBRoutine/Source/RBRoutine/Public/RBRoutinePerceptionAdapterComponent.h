#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "RBRoutinePerceptionAdapterComponent.generated.h"

class UAIPerceptionComponent;
class URBRoutineComponent;

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class RBROUTINE_API URBRoutinePerceptionAdapterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBRoutinePerceptionAdapterComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Routine|Perception")
    FName ThreatActorTag = TEXT("Threat");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Routine|Perception")
    int32 ThreatInterruptPriority = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Routine|Perception")
    FName ThreatForcedGoalId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB Routine|Perception")
    bool bRecordPerceptionMemories = true;

    UFUNCTION(BlueprintCallable, Category="RB Routine|Perception")
    void Configure(UAIPerceptionComponent* InPerceptionComponent, URBRoutineComponent* InRoutineComponent);

    UFUNCTION(BlueprintCallable, Category="RB Routine|Perception")
    bool ForwardStimulus(AActor* SourceActor, const FAIStimulus& Stimulus, FString& OutError);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

    UPROPERTY(Transient)
    TObjectPtr<URBRoutineComponent> RoutineComponent;

    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    void BindPerception();
    void UnbindPerception();
};
