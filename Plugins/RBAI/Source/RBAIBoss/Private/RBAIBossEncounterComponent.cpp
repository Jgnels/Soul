#include "RBAIBossEncounterComponent.h"

#include "RBAIBossProfile.h"
#include "RBAIBrainComponent.h"
#include "RBAINativeTags.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

URBAIBossEncounterComponent::URBAIBossEncounterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URBAIBossEncounterComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        Brain = Owner->FindComponentByClass<URBAIBrainComponent>();
        if (Owner->HasAuthority() && Brain)
        {
            Brain->AddStateTag(RBAI::Tags::State_Boss);
        }
    }
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        SetHealthNormalized(1.0f);
    }
}

void URBAIBossEncounterComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(URBAIBossEncounterComponent, State);
}

int32 URBAIBossEncounterComponent::FindCurrentPhaseIndex() const
{
    if (!Profile || !State.PhaseTag.IsValid())
    {
        return INDEX_NONE;
    }
    return Profile->Phases.IndexOfByPredicate([this](const FRBAIBossPhaseSpec& Phase)
        { return Phase.PhaseTag == State.PhaseTag; });
}

const FRBAIBossActionSpec* URBAIBossEncounterComponent::FindPendingActionSpec() const
{
    const int32 PhaseIndex = FindCurrentPhaseIndex();
    if (PhaseIndex == INDEX_NONE || !State.PendingActionTag.IsValid())
    {
        return nullptr;
    }
    return Profile->Phases[PhaseIndex].Actions.FindByPredicate([this](const FRBAIBossActionSpec& Action)
        { return Action.ActionTag == State.PendingActionTag; });
}

bool URBAIBossEncounterComponent::SetHealthNormalized(const float Health01)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Profile)
    {
        return false;
    }
    const int32 NewIndex = Profile->FindPhaseForHealth(Health01);
    if (!Profile->Phases.IsValidIndex(NewIndex))
    {
        return false;
    }
    const FGameplayTag NewTag = Profile->Phases[NewIndex].PhaseTag;
    if (State.PhaseTag == NewTag)
    {
        return false;
    }

    State.PhaseTag = NewTag;
    State.PendingActionTag = FGameplayTag();
    State.TargetActor = nullptr;
    State.NextActionIndex = 0;
    State.bTelegraphing = false;
    bActionInProgress = false;
    ++State.Revision;
    PublishPhaseChange();
    Owner->ForceNetUpdate();
    return true;
}

bool URBAIBossEncounterComponent::RequestNextAction(AActor* TargetActor)
{
    AActor* Owner = GetOwner();
    const int32 PhaseIndex = FindCurrentPhaseIndex();
    if (!Owner || !Owner->HasAuthority() || PhaseIndex == INDEX_NONE ||
        State.PendingActionTag.IsValid() || bActionInProgress)
    {
        return false;
    }

    const TArray<FRBAIBossActionSpec>& Actions = Profile->Phases[PhaseIndex].Actions;
    if (Actions.IsEmpty())
    {
        return false;
    }
    const int32 ActionIndex = FMath::Abs(State.NextActionIndex) % Actions.Num();
    const FRBAIBossActionSpec& Action = Actions[ActionIndex];
    if (!Action.ActionTag.IsValid())
    {
        return false;
    }

    State.PendingActionTag = Action.ActionTag;
    State.TargetActor = TargetActor;
    State.NextActionIndex = (ActionIndex + 1) % Actions.Num();
    State.bTelegraphing = true;
    ++State.Revision;
    OnTelegraphRequested.Broadcast(Action.ActionTag, TargetActor,
        FMath::Max(0.0f, Action.TelegraphSeconds), State.Revision);
    Owner->ForceNetUpdate();
    return true;
}

bool URBAIBossEncounterComponent::CommitTelegraphedAction()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !State.bTelegraphing ||
        !State.PendingActionTag.IsValid() || !FindPendingActionSpec())
    {
        return false;
    }
    State.bTelegraphing = false;
    bActionInProgress = true;
    ++State.Revision;
    OnBossActionRequested.Broadcast(State.PendingActionTag, State.TargetActor, State.Revision);
    Owner->ForceNetUpdate();
    return true;
}

bool URBAIBossEncounterComponent::NotifyActionFinished(const bool bSucceeded)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !bActionInProgress)
    {
        return false;
    }
    bActionInProgress = false;
    State.PendingActionTag = FGameplayTag();
    State.TargetActor = nullptr;
    State.bTelegraphing = false;
    ++State.Revision;
    Owner->ForceNetUpdate();
    return true;
}

FRBAIBossSnapshot URBAIBossEncounterComponent::CaptureSnapshot() const
{
    FRBAIBossSnapshot Snapshot;
    Snapshot.Version = 1;
    Snapshot.PhaseTag = State.PhaseTag;
    Snapshot.NextActionIndex = State.NextActionIndex;
    Snapshot.Revision = State.Revision;
    return Snapshot;
}

bool URBAIBossEncounterComponent::RestoreSnapshot(const FRBAIBossSnapshot& Snapshot)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Profile || Snapshot.Version != 1 ||
        Snapshot.Revision < 0 || Snapshot.NextActionIndex < 0)
    {
        return false;
    }
    const int32 PhaseIndex = Profile->Phases.IndexOfByPredicate([&Snapshot](const FRBAIBossPhaseSpec& Phase)
        { return Phase.PhaseTag == Snapshot.PhaseTag; });
    if (!Profile->Phases.IsValidIndex(PhaseIndex))
    {
        return false;
    }

    State = FRBAIBossRuntimeState{};
    State.PhaseTag = Snapshot.PhaseTag;
    State.NextActionIndex = Snapshot.NextActionIndex;
    State.Revision = Snapshot.Revision;
    bActionInProgress = false;
    PublishPhaseChange();
    Owner->ForceNetUpdate();
    return true;
}

void URBAIBossEncounterComponent::PublishPhaseChange()
{
    OnPhaseChanged.Broadcast(State.PhaseTag, State.Revision);
}

void URBAIBossEncounterComponent::OnRepState()
{
    PublishPhaseChange();
}
