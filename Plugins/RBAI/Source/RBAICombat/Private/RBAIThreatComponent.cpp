#include "RBAIThreatComponent.h"

#include "GameFramework/Actor.h"

URBAIThreatComponent::URBAIThreatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBAIThreatComponent::AddThreat(AActor* Target, const float Delta)
{
    if (!IsValid(Target) || Delta == 0.0f)
    {
        return;
    }
    SetThreat(Target, GetThreat(Target) + Delta);
}

void URBAIThreatComponent::SetThreat(AActor* Target, const float Value)
{
    if (!IsValid(Target))
    {
        return;
    }
    const float Clamped = FMath::Clamp(Value, 0.0f, MaximumThreat);
    if (Clamped <= KINDA_SMALL_NUMBER)
    {
        ThreatByTarget.Remove(Target);
        return;
    }
    ThreatByTarget.FindOrAdd(Target) = Clamped;
}

void URBAIThreatComponent::RemoveTarget(AActor* Target)
{
    ThreatByTarget.Remove(Target);
}

void URBAIThreatComponent::ClearThreat()
{
    ThreatByTarget.Reset();
}

void URBAIThreatComponent::ApplyDecay(const float Multiplier)
{
    const float SafeMultiplier = FMath::Clamp(Multiplier, 0.0f, 1.0f);
    for (auto It = ThreatByTarget.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()))
        {
            It.RemoveCurrent();
            continue;
        }
        It.Value() *= SafeMultiplier;
        if (It.Value() <= KINDA_SMALL_NUMBER)
        {
            It.RemoveCurrent();
        }
    }
}

float URBAIThreatComponent::GetThreat(AActor* Target) const
{
    if (const float* Value = ThreatByTarget.Find(Target))
    {
        return *Value;
    }
    return 0.0f;
}

AActor* URBAIThreatComponent::GetHighestThreatTarget() const
{
    AActor* BestTarget = nullptr;
    float BestThreat = -1.0f;
    for (const TPair<TObjectPtr<AActor>, float>& Pair : ThreatByTarget)
    {
        if (!IsValid(Pair.Key))
        {
            continue;
        }
        if (Pair.Value > BestThreat ||
            (FMath::IsNearlyEqual(Pair.Value, BestThreat) &&
             Pair.Key->GetPathName() < BestTarget->GetPathName()))
        {
            BestTarget = Pair.Key;
            BestThreat = Pair.Value;
        }
    }
    return BestTarget;
}
