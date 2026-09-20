#include "RBRoutinePerceptionAdapterComponent.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "RBRoutineComponent.h"
#include "RBRoutineNativeTags.h"
#include "RBRoutineWorldSubsystem.h"

URBRoutinePerceptionAdapterComponent::URBRoutinePerceptionAdapterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBRoutinePerceptionAdapterComponent::Configure(
    UAIPerceptionComponent* InPerceptionComponent,
    URBRoutineComponent* InRoutineComponent)
{
    UnbindPerception();
    PerceptionComponent = InPerceptionComponent;
    RoutineComponent = InRoutineComponent;
    BindPerception();
}

void URBRoutinePerceptionAdapterComponent::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    if (!RoutineComponent && Owner)
    {
        RoutineComponent = Owner->FindComponentByClass<URBRoutineComponent>();
    }
    if (!PerceptionComponent && Owner)
    {
        PerceptionComponent = Owner->FindComponentByClass<UAIPerceptionComponent>();
    }
    BindPerception();
}

void URBRoutinePerceptionAdapterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindPerception();
    Super::EndPlay(EndPlayReason);
}

void URBRoutinePerceptionAdapterComponent::BindPerception()
{
    if (PerceptionComponent)
    {
        PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &URBRoutinePerceptionAdapterComponent::HandleTargetPerceptionUpdated);
        PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &URBRoutinePerceptionAdapterComponent::HandleTargetPerceptionUpdated);
    }
}

void URBRoutinePerceptionAdapterComponent::UnbindPerception()
{
    if (PerceptionComponent)
    {
        PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &URBRoutinePerceptionAdapterComponent::HandleTargetPerceptionUpdated);
    }
}

void URBRoutinePerceptionAdapterComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    FString Error;
    ForwardStimulus(Actor, Stimulus, Error);
}

bool URBRoutinePerceptionAdapterComponent::ForwardStimulus(AActor* SourceActor, const FAIStimulus& Stimulus, FString& OutError)
{
    OutError.Reset();
    if (!RoutineComponent || !RoutineComponent->IsAuthoritative())
    {
        OutError = TEXT("missing_authoritative_routine");
        return false;
    }
    UWorld* World = GetWorld();
    URBRoutineWorldSubsystem* Population = World ? World->GetSubsystem<URBRoutineWorldSubsystem>() : nullptr;
    if (!Population)
    {
        OutError = TEXT("missing_population_subsystem");
        return false;
    }

    const bool bSight = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
    const bool bHearing = Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>();
    const bool bSensed = Stimulus.WasSuccessfullySensed();
    const bool bThreat = bSensed && SourceActor && !ThreatActorTag.IsNone() && SourceActor->ActorHasTag(ThreatActorTag);

    FRBRoutinePerceptionEvent Event;
    Event.SourceAgentId = SourceActor ? SourceActor->GetFName() : NAME_None;
    if (SourceActor)
    {
        if (const URBRoutineComponent* SourceRoutine = SourceActor->FindComponentByClass<URBRoutineComponent>())
        {
            if (!SourceRoutine->AgentId.IsNone()) { Event.SourceAgentId = SourceRoutine->AgentId; }
        }
    }
    Event.EventTag = bThreat ? TAG_RBRoutine_Event_Perception_Threat :
        (!bSensed ? TAG_RBRoutine_Event_Perception_Lost :
        (bHearing ? TAG_RBRoutine_Event_Perception_Heard : TAG_RBRoutine_Event_Perception_Saw));
    Event.bRecordMemory = bRecordPerceptionMemories;
    Event.Salience = bThreat ? 1.0 : (bSensed ? 0.6 : 0.3);
    Event.Valence = bThreat ? -1.0 : 0.0;
    Event.bRequestInterrupt = bThreat;
    Event.InterruptPriority = bThreat ? ThreatInterruptPriority : 0;
    Event.ForcedGoalId = bThreat ? ThreatForcedGoalId : NAME_None;
    Event.InterruptReasonTag = bThreat ? TAG_RBRoutine_Event_Perception_Threat : Event.EventTag;

    const int64 WorldMinute = Population->GetExternalWorldMinute();
    const FString SourceKey = Event.SourceAgentId.IsNone() ? TEXT("Unknown") : Event.SourceAgentId.ToString();
    const FString SenseKey = bSight ? TEXT("Sight") : (bHearing ? TEXT("Hearing") : TEXT("Sense"));
    Event.EventId = FName(*FString::Printf(TEXT("Perception_%s_%s_%lld_%s"),
        *SourceKey, *SenseKey, WorldMinute, bSensed ? TEXT("On") : TEXT("Off")));

    TSet<FName> FactKeys;
    for (const FRBRoutineFactEffect& Update : Event.RoutineFactUpdates)
    {
        if (Update.Key.IsNone() || FactKeys.Contains(Update.Key) ||
            (Update.Value.Type == ERBRoutineFactType::Number && !FMath::IsFinite(Update.Value.NumberValue)))
        {
            OutError = TEXT("invalid_perception_fact_update");
            return false;
        }
        FactKeys.Add(Update.Key);
    }

    if (Event.bRecordMemory)
    {
        FRBRoutineSnapshot Existing = RoutineComponent->CreateSnapshot(WorldMinute);
        if (Event.EventId.IsNone() || Existing.Memories.ContainsByPredicate([&Event](const FRBRoutineMemoryRecord& Memory)
            { return Memory.EventId == Event.EventId; }))
        {
            OutError = TEXT("duplicate_or_invalid_perception_event");
            return false;
        }
    }
    if (Event.bRequestInterrupt)
    {
        FRBRoutineInterruptRequest Interrupt;
        Interrupt.ReasonTag = Event.InterruptReasonTag.IsValid() ? Event.InterruptReasonTag : Event.EventTag;
        Interrupt.Priority = Event.InterruptPriority;
        Interrupt.ForcedGoalId = Event.ForcedGoalId;
        if (!RoutineComponent->RequestInterrupt(Interrupt))
        {
            OutError = TEXT("perception_interrupt_rejected");
            return false;
        }
    }

    for (const FRBRoutineFactEffect& Update : Event.RoutineFactUpdates)
    {
        switch (Update.Value.Type)
        {
        case ERBRoutineFactType::Bool: RoutineComponent->SetFactBool(Update.Key, Update.Value.BoolValue); break;
        case ERBRoutineFactType::Integer: RoutineComponent->SetFactInteger(Update.Key, Update.Value.IntegerValue); break;
        case ERBRoutineFactType::Number: RoutineComponent->SetFactNumber(Update.Key, Update.Value.NumberValue); break;
        case ERBRoutineFactType::Name: RoutineComponent->SetFactName(Update.Key, Update.Value.NameValue); break;
        }
    }
    if (Event.bRecordMemory)
    {
        FRBRoutineMemoryRecord Memory;
        Memory.EventId = Event.EventId;
        Memory.EventTag = Event.EventTag;
        Memory.OtherAgentId = Event.SourceAgentId;
        Memory.Salience = FMath::Clamp(Event.Salience, 0.0, 1.0);
        Memory.Valence = FMath::Clamp(Event.Valence, -1.0, 1.0);
        Memory.WorldMinute = WorldMinute;
        if (!RoutineComponent->RecordMemoryEvent(Memory, 0.0, 0.0, 0.0))
        {
            OutError = TEXT("perception_memory_rejected");
            return false;
        }
    }

    return true;
}
