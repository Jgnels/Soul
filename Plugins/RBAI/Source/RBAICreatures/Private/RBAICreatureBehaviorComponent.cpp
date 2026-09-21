#include "RBAICreatureBehaviorComponent.h"

#include "RBAIBrainComponent.h"
#include "RBAICreatureArchetype.h"
#include "RBAINativeTags.h"
#include "GameFramework/Actor.h"

URBAICreatureBehaviorComponent::URBAICreatureBehaviorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBAICreatureBehaviorComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        Brain = Owner->FindComponentByClass<URBAIBrainComponent>();
    }
    ApplyArchetype();
}

void URBAICreatureBehaviorComponent::ApplyArchetype()
{
    if (!Brain)
    {
        return;
    }
    Brain->AddStateTag(RBAI::Tags::State_Creature);
    if (Archetype && bInstallArchetypeActions)
    {
        Brain->SetRuntimeActions(Archetype->BuildStandardActions());
    }
}

void URBAICreatureBehaviorComponent::SetVitals(
    const float Health01, const float Hunger01, const float Fatigue01)
{
    if (!Brain)
    {
        return;
    }
    Brain->SetSignal(RBAI::Tags::Signal_Health, FMath::Clamp(Health01, 0.0f, 1.0f));
    Brain->SetSignal(RBAI::Tags::Signal_Hunger, FMath::Clamp(Hunger01, 0.0f, 1.0f));
    Brain->SetSignal(RBAI::Tags::Signal_Fatigue, FMath::Clamp(Fatigue01, 0.0f, 1.0f));
}

void URBAICreatureBehaviorComponent::SetPackSupport(const float PackSupport01)
{
    if (Brain)
    {
        Brain->SetSignal(RBAI::Tags::Signal_PackSupport,
            FMath::Clamp(PackSupport01, 0.0f, 1.0f));
    }
}

void URBAICreatureBehaviorComponent::SetTerritoryDistance(const float TerritoryDistance01)
{
    if (Brain)
    {
        Brain->SetSignal(RBAI::Tags::Signal_TerritoryDistance,
            FMath::Clamp(TerritoryDistance01, 0.0f, 1.0f));
    }
}
