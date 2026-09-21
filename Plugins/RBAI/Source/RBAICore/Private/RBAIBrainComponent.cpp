#include "RBAIBrainComponent.h"

#include "Core/RBAIUtilityCore.h"
#include "RBAIProfile.h"
#include "RBAINativeTags.h"
#include "RBAIWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
    RBAI::Core::CurveType ToCoreCurve(const ERBAIResponseCurve Curve)
    {
        using CoreCurve = RBAI::Core::CurveType;
        switch (Curve)
        {
        case ERBAIResponseCurve::InverseLinear: return CoreCurve::InverseLinear;
        case ERBAIResponseCurve::Quadratic: return CoreCurve::Quadratic;
        case ERBAIResponseCurve::InverseQuadratic: return CoreCurve::InverseQuadratic;
        case ERBAIResponseCurve::Step: return CoreCurve::Step;
        default: return CoreCurve::Linear;
        }
    }
}

URBAIBrainComponent::URBAIBrainComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URBAIBrainComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (URBAIWorldSubsystem* Scheduler = GetWorld()->GetSubsystem<URBAIWorldSubsystem>())
        {
            Scheduler->RegisterBrain(this);
        }
    }
}

void URBAIBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBAIWorldSubsystem* Scheduler = World->GetSubsystem<URBAIWorldSubsystem>())
        {
            Scheduler->UnregisterBrain(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void URBAIBrainComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(URBAIBrainComponent, Decision);
}


void URBAIBrainComponent::SetRuntimeActions(const TArray<FRBAIActionDefinition>& InActions)
{
    RuntimeActions = InActions;
    RequestEvaluation();
}

void URBAIBrainComponent::ClearRuntimeActions()
{
    RuntimeActions.Reset();
    RequestEvaluation();
}

const TArray<FRBAIActionDefinition>& URBAIBrainComponent::GetActionDefinitions() const
{
    static const TArray<FRBAIActionDefinition> Empty;
    if (!RuntimeActions.IsEmpty()) return RuntimeActions;
    return Profile ? Profile->Actions : Empty;
}

void URBAIBrainComponent::SetSignal(const FGameplayTag SignalTag, const float Value)
{
    if (SignalTag.IsValid())
    {
        Signals.FindOrAdd(SignalTag) = Value;
        RequestEvaluation();
    }
}

void URBAIBrainComponent::ClearSignal(const FGameplayTag SignalTag)
{
    Signals.Remove(SignalTag);
    RequestEvaluation();
}

void URBAIBrainComponent::AddStateTag(const FGameplayTag Tag)
{
    if (Tag.IsValid())
    {
        StateTags.AddTag(Tag);
        RequestEvaluation();
    }
}

void URBAIBrainComponent::RemoveStateTag(const FGameplayTag Tag)
{
    StateTags.RemoveTag(Tag);
    RequestEvaluation();
}

void URBAIBrainComponent::UpsertContext(const FRBAIContext& Context)
{
    if (Context.ContextId.IsNone())
    {
        return;
    }
    if (FRBAIContext* Existing = Contexts.FindByPredicate([&Context](const FRBAIContext& Item)
        { return Item.ContextId == Context.ContextId; }))
    {
        *Existing = Context;
    }
    else
    {
        Contexts.Add(Context);
    }
    RequestEvaluation();
}

void URBAIBrainComponent::RemoveContext(const FName ContextId)
{
    Contexts.RemoveAll([ContextId](const FRBAIContext& Item) { return Item.ContextId == ContextId; });
    RequestEvaluation();
}

void URBAIBrainComponent::ClearContexts()
{
    Contexts.Reset();
    RequestEvaluation();
}

void URBAIBrainComponent::RequestEvaluation()
{
    NextEvaluationWorldTime = 0.0;
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (URBAIWorldSubsystem* Scheduler = GetWorld()->GetSubsystem<URBAIWorldSubsystem>())
        {
            Scheduler->RequestImmediateEvaluation(this);
        }
    }
}

const FRBAIActionDefinition* URBAIBrainComponent::FindActionDefinition(const FGameplayTag ActionTag) const
{
    if (!ActionTag.IsValid())
    {
        return nullptr;
    }
    return GetActionDefinitions().FindByPredicate([ActionTag](const FRBAIActionDefinition& Item)
        { return Item.ActionTag == ActionTag; });
}

bool URBAIBrainComponent::IsActionAllowed(const FRBAIActionDefinition& Action, const FRBAIContext& Context) const
{
    FGameplayTagContainer Combined = StateTags;
    Combined.AppendTags(Context.Tags);
    return Combined.HasAll(Action.RequiredTags) && !Combined.HasAny(Action.BlockedTags);
}

bool URBAIBrainComponent::EvaluateNow()
{
    AActor* Owner = GetOwner();
    const TArray<FRBAIActionDefinition>& ActionDefinitions = GetActionDefinitions();
    if (!Owner || !Owner->HasAuthority() || ActionDefinitions.IsEmpty())
    {
        return false;
    }

    if (bActionInProgress)
    {
        if (const FRBAIActionDefinition* Current = FindActionDefinition(Decision.ActionTag))
        {
            if (!Current->bInterruptible)
            {
                return false;
            }
        }
    }

    std::vector<RBAI::Core::Action> CoreActions;
    CoreActions.reserve(ActionDefinitions.Num());
    for (const FRBAIActionDefinition& Def : ActionDefinitions)
    {
        if (!Def.ActionTag.IsValid())
        {
            continue;
        }
        RBAI::Core::Action CoreAction;
        CoreAction.Id = TCHAR_TO_UTF8(*Def.ActionTag.ToString());
        CoreAction.PriorityGroup = Def.PriorityGroup;
        CoreAction.BaseScore = Def.BaseScore;
        CoreAction.InertiaBonus = Def.InertiaBonus;
        CoreAction.Interruptible = Def.bInterruptible;
        CoreAction.Considerations.reserve(Def.Considerations.Num());
        for (const FRBAIConsiderationDefinition& Item : Def.Considerations)
        {
            if (!Item.SignalTag.IsValid())
            {
                continue;
            }
            RBAI::Core::Consideration CoreItem;
            CoreItem.Signal = TCHAR_TO_UTF8(*Item.SignalTag.ToString());
            CoreItem.MinValue = Item.MinValue;
            CoreItem.MaxValue = Item.MaxValue;
            CoreItem.Curve = ToCoreCurve(Item.Curve);
            CoreItem.Exponent = Item.Exponent;
            CoreItem.Weight = Item.Weight;
            CoreAction.Considerations.push_back(std::move(CoreItem));
        }
        CoreActions.push_back(std::move(CoreAction));
    }

    TArray<FRBAIContext> WorkingContexts = Contexts;
    if (WorkingContexts.IsEmpty())
    {
        FRBAIContext Self;
        Self.ContextId = TEXT("Self");
        Self.ContextActor = Owner;
        Self.Tags.AddTag(RBAI::Tags::Context_Self);
        WorkingContexts.Add(Self);
    }

    std::vector<RBAI::Core::Context> CoreContexts;
    CoreContexts.reserve(WorkingContexts.Num());
    for (const FRBAIContext& Context : WorkingContexts)
    {
        RBAI::Core::Context CoreContext;
        CoreContext.Id = TCHAR_TO_UTF8(*Context.ContextId.ToString());
        for (const TPair<FGameplayTag, float>& Pair : Signals)
        {
            CoreContext.Signals[TCHAR_TO_UTF8(*Pair.Key.ToString())] = Pair.Value;
        }
        for (const TPair<FGameplayTag, float>& Pair : Context.Signals)
        {
            CoreContext.Signals[TCHAR_TO_UTF8(*Pair.Key.ToString())] = Pair.Value;
        }
        for (const FRBAIActionDefinition& Action : ActionDefinitions)
        {
            if (Action.ActionTag.IsValid() && !IsActionAllowed(Action, Context))
            {
                CoreContext.BlockedActionIds.insert(TCHAR_TO_UTF8(*Action.ActionTag.ToString()));
            }
        }
        CoreContexts.push_back(std::move(CoreContext));
    }

    RBAI::Core::SelectionOptions Options;
    Options.CurrentActionId = TCHAR_TO_UTF8(*Decision.ActionTag.ToString());
    Options.CurrentContextId = TCHAR_TO_UTF8(*Decision.ContextId.ToString());
    Options.MinimumScore = Profile ? Profile->MinimumActionScore : 0.000001;

    const RBAI::Core::SelectionResult Result =
        RBAI::Core::ChooseAction(CoreActions, CoreContexts, Options);
    if (!Result.Success)
    {
        return false;
    }

    const FGameplayTag SelectedTag = FGameplayTag::RequestGameplayTag(
        FName(UTF8_TO_TCHAR(Result.ActionId.c_str())), false);
    const FName SelectedContext(UTF8_TO_TCHAR(Result.ContextId.c_str()));
    if (!SelectedTag.IsValid())
    {
        return false;
    }

    if (bActionInProgress && Decision.ActionTag == SelectedTag && Decision.ContextId == SelectedContext)
    {
        return false;
    }
    AActor* SelectedActor = nullptr;
    if (const FRBAIContext* Selected = WorkingContexts.FindByPredicate(
        [SelectedContext](const FRBAIContext& Item) { return Item.ContextId == SelectedContext; }))
    {
        SelectedActor = Selected->ContextActor;
    }

    Decision.ActionTag = SelectedTag;
    Decision.ContextId = SelectedContext;
    Decision.ContextActor = SelectedActor;
    ++Decision.Revision;
    bActionInProgress = true;

    OnDecisionChanged.Broadcast(Decision.Revision);
    OnActionRequested.Broadcast(Decision.ActionTag, Decision.ContextActor, Decision.ContextId);
    Owner->ForceNetUpdate();
    return true;
}

void URBAIBrainComponent::NotifyActionFinished(const bool bSucceeded)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }
    bActionInProgress = false;
    RequestEvaluation();
}

FRBAIBrainSnapshot URBAIBrainComponent::CaptureSnapshot() const
{
    FRBAIBrainSnapshot Snapshot;
    Snapshot.Version = 1;
    Snapshot.CurrentActionTag = Decision.ActionTag;
    Snapshot.CurrentContextId = Decision.ContextId;
    Snapshot.DecisionRevision = Decision.Revision;
    return Snapshot;
}

bool URBAIBrainComponent::RestoreSnapshot(const FRBAIBrainSnapshot& Snapshot)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || Snapshot.Version != 1 || Snapshot.DecisionRevision < 0)
    {
        return false;
    }
    if (Snapshot.CurrentActionTag.IsValid() && !FindActionDefinition(Snapshot.CurrentActionTag))
    {
        return false;
    }

    Decision.ActionTag = Snapshot.CurrentActionTag;
    Decision.ContextId = Snapshot.CurrentContextId;
    Decision.ContextActor = nullptr;
    Decision.Revision = Snapshot.DecisionRevision;
    bActionInProgress = Decision.ActionTag.IsValid();
    OnDecisionChanged.Broadcast(Decision.Revision);
    Owner->ForceNetUpdate();
    return true;
}

float URBAIBrainComponent::GetDesiredEvaluationInterval(const float DistanceSquared) const
{
    if (!Profile)
    {
        return 1.0f;
    }
    if (DistanceSquared <= FMath::Square(Profile->NearDistance))
    {
        return Profile->NearIntervalSeconds;
    }
    if (DistanceSquared <= FMath::Square(Profile->MediumDistance))
    {
        return Profile->MediumIntervalSeconds;
    }
    if (DistanceSquared <= FMath::Square(Profile->FarDistance))
    {
        return Profile->FarIntervalSeconds;
    }
    return Profile->OutOfBoundsIntervalSeconds;
}

void URBAIBrainComponent::OnRepDecision()
{
    OnDecisionChanged.Broadcast(Decision.Revision);
}
