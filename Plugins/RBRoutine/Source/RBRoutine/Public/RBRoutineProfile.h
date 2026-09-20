#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RBRoutineTypes.h"
#include "RBRoutineProfile.generated.h"

UCLASS(BlueprintType)
class RBROUTINE_API URBRoutineActivityDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    FGameplayTag ActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    FGameplayTag SmartObjectActivityTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    FGameplayTag AnimationSemanticTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    bool bRequiresNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine", meta=(ClampMin="0.0"))
    double ExpectedDurationGameMinutes = 0.0;
};

UCLASS(BlueprintType)
class RBROUTINE_API URBRoutineProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planner")
    TArray<FRBRoutineGoalDefinition> Goals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planner")
    TArray<FRBRoutineActionDefinition> Actions;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    TArray<FRBRoutineScheduleEntry> Schedule;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Routine")
    TArray<TObjectPtr<URBRoutineActivityDefinition>> Activities;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Continuity", meta=(ClampMin="0", ClampMax="2048"))
    int32 MemoryCapacity = 128;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planner", meta=(ClampMin="1", ClampMax="32"))
    int32 MaxPlanDepth = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Planner", meta=(ClampMin="32", ClampMax="100000"))
    int32 MaxExpandedNodes = 4096;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Runtime")
    bool bAutoReplanOnFactChange = true;
};
