#include "RBAIWorldSubsystem.h"

#include "RBAIBrainComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

void URBAIWorldSubsystem::RegisterBrain(URBAIBrainComponent* Brain)
{
    if (IsValid(Brain))
    {
        Brains.AddUnique(Brain);
        Brain->NextEvaluationWorldTime = 0.0;
    }
}

void URBAIWorldSubsystem::UnregisterBrain(URBAIBrainComponent* Brain)
{
    Brains.RemoveAll([Brain](const TWeakObjectPtr<URBAIBrainComponent>& Item)
    {
        return !Item.IsValid() || Item.Get() == Brain;
    });
    Cursor = FMath::Clamp(Cursor, 0, FMath::Max(0, Brains.Num() - 1));
}

void URBAIWorldSubsystem::RequestImmediateEvaluation(URBAIBrainComponent* Brain)
{
    if (IsValid(Brain))
    {
        Brain->NextEvaluationWorldTime = 0.0;
    }
}

float URBAIWorldSubsystem::DistanceSquaredToNearestPlayer(const FVector& Location) const
{
    const UWorld* World = GetWorld();
    float Best = TNumericLimits<float>::Max();
    if (!World)
    {
        return Best;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (Pawn)
        {
            Best = FMath::Min(Best, FVector::DistSquared(Location, Pawn->GetActorLocation()));
        }
    }
    return Best;
}

void URBAIWorldSubsystem::Tick(float DeltaTime)
{
    (void)DeltaTime;
    LastVisitedCount = 0;
    LastEvaluatedCount = 0;
    LastDecisionTimeMs = 0.0;
    UWorld* World = GetWorld();
    if (!World || Brains.IsEmpty())
    {
        return;
    }

    Brains.RemoveAll([](const TWeakObjectPtr<URBAIBrainComponent>& Item)
    {
        return !Item.IsValid();
    });
    if (Brains.IsEmpty())
    {
        Cursor = 0;
        return;
    }

    const double WorldTime = World->GetTimeSeconds();
    const double Started = FPlatformTime::Seconds();
    const int32 Available = Brains.Num();
    int32 Visited = 0;
    int32 Evaluated = 0;

    while (Visited < Available && Evaluated < MaxEvaluationsPerFrame)
    {
        Cursor %= Brains.Num();
        URBAIBrainComponent* Brain = Brains[Cursor].Get();
        Cursor = (Cursor + 1) % Brains.Num();
        ++Visited;
        if (!IsValid(Brain) || Brain->NextEvaluationWorldTime > WorldTime)
        {
            continue;
        }

        const AActor* Owner = Brain->GetOwner();
        const float DistanceSquared = Owner
            ? DistanceSquaredToNearestPlayer(Owner->GetActorLocation())
            : TNumericLimits<float>::Max();

        Brain->EvaluateNow();
        Brain->NextEvaluationWorldTime = WorldTime + Brain->GetDesiredEvaluationInterval(DistanceSquared);
        ++Evaluated;

        const double ElapsedMs = (FPlatformTime::Seconds() - Started) * 1000.0;
        if (ElapsedMs >= DecisionBudgetMs)
        {
            break;
        }
    }

    LastVisitedCount = Visited;
    LastEvaluatedCount = Evaluated;
    LastDecisionTimeMs = (FPlatformTime::Seconds() - Started) * 1000.0;
}

TStatId URBAIWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URBAIWorldSubsystem, STATGROUP_Tickables);
}
