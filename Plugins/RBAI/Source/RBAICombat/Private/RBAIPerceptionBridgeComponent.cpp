#include "RBAIPerceptionBridgeComponent.h"

#include "RBAIBrainComponent.h"
#include "RBAINativeTags.h"
#include "RBAIThreatComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

URBAIPerceptionBridgeComponent::URBAIPerceptionBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBAIPerceptionBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    BoundPerception = GetOwner() ? GetOwner()->FindComponentByClass<UAIPerceptionComponent>() : nullptr;
    if (BoundPerception)
    {
        BoundPerception->OnTargetPerceptionUpdated.AddDynamic(
            this, &ThisClass::HandleTargetPerceptionUpdated);
        BoundPerception->OnTargetPerceptionForgotten.AddDynamic(
            this, &ThisClass::HandleTargetPerceptionForgotten);
    }
}

void URBAIPerceptionBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (BoundPerception)
    {
        BoundPerception->OnTargetPerceptionUpdated.RemoveAll(this);
        BoundPerception->OnTargetPerceptionForgotten.RemoveAll(this);
    }
    ContextByActor.Reset();
    Super::EndPlay(EndPlayReason);
}

URBAIBrainComponent* URBAIPerceptionBridgeComponent::ResolveBrain() const
{
    if (AActor* Owner = GetOwner())
    {
        if (URBAIBrainComponent* Brain = Owner->FindComponentByClass<URBAIBrainComponent>())
        {
            return Brain;
        }
        if (const AAIController* AI = Cast<AAIController>(Owner))
        {
            if (APawn* Pawn = AI->GetPawn())
            {
                return Pawn->FindComponentByClass<URBAIBrainComponent>();
            }
        }
    }
    return nullptr;
}

URBAIThreatComponent* URBAIPerceptionBridgeComponent::ResolveThreat() const
{
    if (AActor* Owner = GetOwner())
    {
        if (URBAIThreatComponent* Threat = Owner->FindComponentByClass<URBAIThreatComponent>())
        {
            return Threat;
        }
        if (const AAIController* AI = Cast<AAIController>(Owner))
        {
            if (APawn* Pawn = AI->GetPawn())
            {
                return Pawn->FindComponentByClass<URBAIThreatComponent>();
            }
        }
    }
    return nullptr;
}

FName URBAIPerceptionBridgeComponent::MakeContextId(const AActor* Target) const
{
    return Target
        ? FName(*FString::Printf(TEXT("Perceived_%u"), Target->GetUniqueID()))
        : NAME_None;
}

void URBAIPerceptionBridgeComponent::HandleTargetPerceptionUpdated(AActor* Target, FAIStimulus Stimulus)
{
    URBAIBrainComponent* Brain = ResolveBrain();
    if (!Brain || !IsValid(Target) || !Brain->GetOwner() || !Brain->GetOwner()->HasAuthority())
    {
        return;
    }

    FRBAIContext& Context = ContextByActor.FindOrAdd(Target);
    Context.ContextId = MakeContextId(Target);
    Context.ContextActor = Target;
    Context.Location = Target->GetActorLocation();
    Context.Tags.AddTag(RBAI::Tags::Context_Perceived);

    const float Distance = FVector::Distance(Brain->GetOwner()->GetActorLocation(), Context.Location);
    const float DistanceScore = 1.0f - FMath::Clamp(Distance / FMath::Max(1.0f, DistanceNormalizationCm), 0.0f, 1.0f);
    Context.Signals.FindOrAdd(RBAI::Tags::Signal_Distance) = DistanceScore;

    AAIController* AI = Cast<AAIController>(GetOwner());
    if (!AI)
    {
        if (const APawn* Pawn = Cast<APawn>(GetOwner()))
        {
            AI = Cast<AAIController>(Pawn->GetController());
        }
    }
    const ETeamAttitude::Type Attitude = AI
        ? AI->GetTeamAttitudeTowards(*Target)
        : ETeamAttitude::Neutral;
    Context.Tags.RemoveTag(RBAI::Tags::Context_Hostile);
    Context.Tags.RemoveTag(RBAI::Tags::Context_Friendly);
    if (Attitude == ETeamAttitude::Hostile)
    {
        Context.Tags.AddTag(RBAI::Tags::Context_Hostile);
    }
    else if (Attitude == ETeamAttitude::Friendly)
    {
        Context.Tags.AddTag(RBAI::Tags::Context_Friendly);
    }

    const bool bSensed = Stimulus.WasSuccessfullySensed();
    float ThreatDelta = 0.0f;
    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        Context.Signals.FindOrAdd(RBAI::Tags::Signal_Sight) = bSensed ? 1.0f : 0.0f;
        ThreatDelta = bSensed ? SightThreat * FMath::Max(0.1f, Stimulus.Strength) : 0.0f;
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
    {
        Context.Signals.FindOrAdd(RBAI::Tags::Signal_Hearing) = bSensed ? 1.0f : 0.0f;
        ThreatDelta = bSensed ? HearingThreat * FMath::Max(0.1f, Stimulus.Strength) : 0.0f;
    }

    if (Attitude == ETeamAttitude::Hostile)
    {
        if (URBAIThreatComponent* Threat = ResolveThreat())
        {
            if (ThreatDelta > 0.0f)
            {
                Threat->AddThreat(Target, ThreatDelta);
            }
            const float NormalizedThreat = Threat->MaximumThreat > KINDA_SMALL_NUMBER
                ? Threat->GetThreat(Target) / Threat->MaximumThreat
                : 0.0f;
            Context.Signals.FindOrAdd(RBAI::Tags::Signal_Threat) = FMath::Clamp(NormalizedThreat, 0.0f, 1.0f);
        }
        else
        {
            Context.Signals.FindOrAdd(RBAI::Tags::Signal_Threat) = bSensed ? 1.0f : 0.0f;
        }
    }
    else
    {
        Context.Signals.FindOrAdd(RBAI::Tags::Signal_Threat) = 0.0f;
    }

    Brain->UpsertContext(Context);
    RefreshThreatenedState();
}

void URBAIPerceptionBridgeComponent::HandleTargetPerceptionForgotten(AActor* Target)
{
    URBAIBrainComponent* Brain = ResolveBrain();
    if (!Brain || !IsValid(Target) || !Brain->GetOwner() || !Brain->GetOwner()->HasAuthority())
    {
        return;
    }

    if (const FRBAIContext* Context = ContextByActor.Find(Target))
    {
        Brain->RemoveContext(Context->ContextId);
    }
    ContextByActor.Remove(Target);
    if (URBAIThreatComponent* Threat = ResolveThreat())
    {
        Threat->RemoveTarget(Target);
    }
    RefreshThreatenedState();
}

void URBAIPerceptionBridgeComponent::RefreshThreatenedState()
{
    URBAIBrainComponent* Brain = ResolveBrain();
    if (!Brain)
    {
        return;
    }
    bool bThreatened = false;
    for (auto It = ContextByActor.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
            continue;
        }
        const FRBAIContext& Context = It.Value();
        const float Sight = Context.Signals.FindRef(RBAI::Tags::Signal_Sight);
        const float Hearing = Context.Signals.FindRef(RBAI::Tags::Signal_Hearing);
        if (Context.Tags.HasTag(RBAI::Tags::Context_Hostile) && (Sight > 0.0f || Hearing > 0.0f))
        {
            bThreatened = true;
            break;
        }
    }

    if (bThreatened)
    {
        Brain->AddStateTag(RBAI::Tags::State_Threatened);
        Brain->AddStateTag(RBAI::Tags::State_InCombat);
    }
    else
    {
        Brain->RemoveStateTag(RBAI::Tags::State_Threatened);
        Brain->RemoveStateTag(RBAI::Tags::State_InCombat);
    }
}
