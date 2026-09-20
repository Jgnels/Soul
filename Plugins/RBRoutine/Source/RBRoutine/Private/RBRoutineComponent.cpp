#include "RBRoutineComponent.h"

#include "Core/RBRoutinePlannerCore.h"
#include "RBRoutineWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
    using namespace RBRoutine::Core;

    std::string ToStdString(FName Name)
    {
        return TCHAR_TO_UTF8(*Name.ToString());
    }

    FName ToFName(const std::string& Value)
    {
        return FName(UTF8_TO_TCHAR(Value.c_str()));
    }

    FactValue ToCoreValue(const FRBRoutineFactValue& Value)
    {
        switch (Value.Type)
        {
        case ERBRoutineFactType::Bool:
            return Value.BoolValue;
        case ERBRoutineFactType::Integer:
            return static_cast<std::int64_t>(Value.IntegerValue);
        case ERBRoutineFactType::Number:
            return Value.NumberValue;
        case ERBRoutineFactType::Name:
            return ToStdString(Value.NameValue);
        }
        return false;
    }

    CompareOp ToCoreOp(ERBRoutineCompareOp Op)
    {
        switch (Op)
        {
        case ERBRoutineCompareOp::Equal: return CompareOp::Equal;
        case ERBRoutineCompareOp::NotEqual: return CompareOp::NotEqual;
        case ERBRoutineCompareOp::Greater: return CompareOp::Greater;
        case ERBRoutineCompareOp::GreaterEqual: return CompareOp::GreaterEqual;
        case ERBRoutineCompareOp::Less: return CompareOp::Less;
        case ERBRoutineCompareOp::LessEqual: return CompareOp::LessEqual;
        }
        return CompareOp::Equal;
    }

    Condition ToCoreCondition(const FRBRoutineFactCondition& Item)
    {
        return {ToStdString(Item.Key), ToCoreOp(Item.Operator), ToCoreValue(Item.Value)};
    }

    Effect ToCoreEffect(const FRBRoutineFactEffect& Item)
    {
        return {ToStdString(Item.Key), ToCoreValue(Item.Value)};
    }

    WorldState ToCoreState(const TMap<FName, FRBRoutineFactValue>& Facts)
    {
        WorldState Result;
        Result.reserve(Facts.Num());
        for (const auto& Pair : Facts)
        {
            Result.emplace(ToStdString(Pair.Key), ToCoreValue(Pair.Value));
        }
        return Result;
    }

    bool AreConditionsSatisfiedUE(const TMap<FName, FRBRoutineFactValue>& Facts, const TArray<FRBRoutineFactCondition>& Conditions)
    {
        const WorldState State = ToCoreState(Facts);
        std::vector<Condition> CoreConditions;
        CoreConditions.reserve(Conditions.Num());
        for (const FRBRoutineFactCondition& Item : Conditions)
        {
            CoreConditions.push_back(ToCoreCondition(Item));
        }
        return AreConditionsSatisfied(State, CoreConditions);
    }
}

URBRoutineComponent::URBRoutineComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URBRoutineComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AgentId.IsNone() && GetOwner())
    {
        AgentId = GetOwner()->GetFName();
        UE_LOG(LogTemp, Warning, TEXT("RB Routine: %s has no authored AgentId; using owner name '%s' as a nonportable fallback."), *GetNameSafe(GetOwner()), *AgentId.ToString());
    }

    if (UWorld* World = GetWorld())
    {
        if (URBRoutineWorldSubsystem* Subsystem = World->GetSubsystem<URBRoutineWorldSubsystem>())
        {
            Subsystem->RegisterAgent(this);
        }
    }
}

void URBRoutineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (URBRoutineWorldSubsystem* Subsystem = World->GetSubsystem<URBRoutineWorldSubsystem>())
        {
            Subsystem->UnregisterAgent(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void URBRoutineComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(URBRoutineComponent, AgentId);
    DOREPLIFETIME(URBRoutineComponent, CurrentGoalId);
    DOREPLIFETIME(URBRoutineComponent, CurrentActionId);
    DOREPLIFETIME(URBRoutineComponent, CurrentActivityTag);
    DOREPLIFETIME(URBRoutineComponent, Status);
    DOREPLIFETIME(URBRoutineComponent, SimulationLOD);
    DOREPLIFETIME(URBRoutineComponent, PlanSerial);
}

bool URBRoutineComponent::IsAuthoritative() const
{
    const AActor* Owner = GetOwner();
    return Owner && Owner->HasAuthority();
}

void URBRoutineComponent::SetFactBool(FName Key, bool Value)
{
    SetFactInternal(Key, FRBRoutineFactValue::FromBool(Value));
}

void URBRoutineComponent::SetFactInteger(FName Key, int64 Value)
{
    SetFactInternal(Key, FRBRoutineFactValue::FromInteger(Value));
}

void URBRoutineComponent::SetFactNumber(FName Key, double Value)
{
    SetFactInternal(Key, FRBRoutineFactValue::FromNumber(Value));
}

void URBRoutineComponent::SetFactName(FName Key, FName Value)
{
    SetFactInternal(Key, FRBRoutineFactValue::FromName(Value));
}

void URBRoutineComponent::SetFactInternal(FName Key, const FRBRoutineFactValue& Value)
{
    if (!IsAuthoritative() || Key.IsNone())
    {
        return;
    }

    Facts.Add(Key, Value);
    bNeedsEvaluation = true;

    if (Profile && Profile->bAutoReplanOnFactChange && !CurrentActionId.IsNone() && !AreCurrentActionPreconditionsSatisfied())
    {
        if (IsCurrentActionInterruptible())
        {
            AbortCurrentAction(FGameplayTag());
        }
    }
}

bool URBRoutineComponent::GetFact(FName Key, FRBRoutineFactValue& OutValue) const
{
    if (const FRBRoutineFactValue* Found = Facts.Find(Key))
    {
        OutValue = *Found;
        return true;
    }
    return false;
}

double URBRoutineComponent::GetNumericFact(FName Key, bool& bFound) const
{
    bFound = false;
    const FRBRoutineFactValue* Value = Facts.Find(Key);
    if (!Value)
    {
        return 0.0;
    }

    bFound = true;
    switch (Value->Type)
    {
    case ERBRoutineFactType::Bool: return Value->BoolValue ? 1.0 : 0.0;
    case ERBRoutineFactType::Integer: return static_cast<double>(Value->IntegerValue);
    case ERBRoutineFactType::Number: return Value->NumberValue;
    case ERBRoutineFactType::Name:
        bFound = false;
        return 0.0;
    }
    bFound = false;
    return 0.0;
}

double URBRoutineComponent::ComputeGoalPriority(const FRBRoutineGoalDefinition& Goal, int64 DayIndex, int32 MinuteOfDay) const
{
    double Priority = Goal.BasePriority;

    for (const FRBRoutineUtilityTerm& Term : Goal.UtilityTerms)
    {
        bool bFound = false;
        double Value = GetNumericFact(Term.FactKey, bFound);
        if (!bFound)
        {
            continue;
        }
        if (Term.bClampInput)
        {
            const double MinValue = FMath::Min(Term.MinInput, Term.MaxInput);
            const double MaxValue = FMath::Max(Term.MinInput, Term.MaxInput);
            Value = FMath::Clamp(Value, MinValue, MaxValue);
        }
        Priority += (Value - Term.Baseline) * Term.Weight;
    }

    if (Profile)
    {
        for (const FRBRoutineScheduleEntry& Entry : Profile->Schedule)
        {
            RBRoutine::Core::ScheduleEntry CoreEntry;
            CoreEntry.Id = ToStdString(Entry.EntryId);
            CoreEntry.DayMask = static_cast<std::uint8_t>(FMath::Clamp(Entry.DayMask, 0, 127));
            CoreEntry.StartMinuteOfDay = Entry.StartMinuteOfDay;
            CoreEntry.EndMinuteOfDay = Entry.EndMinuteOfDay;
            CoreEntry.PreferredGoalId = ToStdString(Entry.PreferredGoalId);
            CoreEntry.PriorityBoost = Entry.PriorityBoost;
            if (CoreEntry.PreferredGoalId == ToStdString(Goal.GoalId) && RBRoutine::Core::IsScheduleEntryActive(CoreEntry, DayIndex, MinuteOfDay))
            {
                Priority += Entry.PriorityBoost;
            }
        }
    }

    if (bHasActiveInterrupt && !ActiveInterrupt.ForcedGoalId.IsNone() && ActiveInterrupt.ForcedGoalId == Goal.GoalId)
    {
        Priority += 1000000.0 + static_cast<double>(ActiveInterrupt.Priority);
    }

    return Priority;
}

void URBRoutineComponent::ExpireInterruptIfNeeded(int64 WorldMinute)
{
    if (bHasActiveInterrupt && ActiveInterrupt.ExpireAtWorldMinute >= 0 && WorldMinute >= ActiveInterrupt.ExpireAtWorldMinute)
    {
        bHasActiveInterrupt = false;
        ActiveInterrupt = FRBRoutineInterruptRequest();
        bNeedsEvaluation = true;
    }
}

bool URBRoutineComponent::EvaluateNow(int64 DayIndex, int32 MinuteOfDay)
{
    if (!IsAuthoritative())
    {
        return false;
    }

    LastDayIndex = DayIndex;
    LastMinuteOfDay = FMath::Clamp(MinuteOfDay, 0, 1439);
    ExpireInterruptIfNeeded(DayIndex * 1440 + LastMinuteOfDay);

    if (!Profile)
    {
        SetStatus(ERBRoutineStatus::Blocked);
        bNeedsEvaluation = false;
        return false;
    }

    if (!CurrentActionId.IsNone())
    {
        if (!bNeedsEvaluation && !bHasActiveInterrupt)
        {
            return true;
        }
        if (!IsCurrentActionInterruptible())
        {
            return true;
        }

        const FGameplayTag AbortReason = bHasActiveInterrupt ? ActiveInterrupt.ReasonTag : FGameplayTag();
        AbortCurrentAction(AbortReason);
    }

    SetStatus(ERBRoutineStatus::Planning);

    const RBRoutine::Core::WorldState CoreState = ToCoreState(Facts);
    std::vector<RBRoutine::Core::GoalSpec> Goals;
    Goals.reserve(Profile->Goals.Num());
    for (const FRBRoutineGoalDefinition& Goal : Profile->Goals)
    {
        if (Goal.GoalId.IsNone())
        {
            continue;
        }

        RBRoutine::Core::GoalSpec CoreGoal;
        CoreGoal.Id = ToStdString(Goal.GoalId);
        CoreGoal.Priority = ComputeGoalPriority(Goal, LastDayIndex, LastMinuteOfDay);
        for (const FRBRoutineFactCondition& Condition : Goal.Eligibility)
        {
            CoreGoal.Eligibility.push_back(ToCoreCondition(Condition));
        }
        for (const FRBRoutineFactCondition& Condition : Goal.Desired)
        {
            CoreGoal.Desired.push_back(ToCoreCondition(Condition));
        }
        Goals.push_back(std::move(CoreGoal));
    }

    std::vector<RBRoutine::Core::ActionSpec> Actions;
    Actions.reserve(Profile->Actions.Num());
    for (const FRBRoutineActionDefinition& Action : Profile->Actions)
    {
        if (Action.ActionId.IsNone())
        {
            continue;
        }

        RBRoutine::Core::ActionSpec CoreAction;
        CoreAction.Id = ToStdString(Action.ActionId);
        CoreAction.BaseCost = Action.BaseCost;
        CoreAction.Interruptible = Action.bInterruptible;
        for (const FRBRoutineFactCondition& Condition : Action.Preconditions)
        {
            CoreAction.Preconditions.push_back(ToCoreCondition(Condition));
        }
        for (const FRBRoutineFactEffect& Effect : Action.PredictedEffects)
        {
            CoreAction.Effects.push_back(ToCoreEffect(Effect));
        }
        Actions.push_back(std::move(CoreAction));
    }

    RBRoutine::Core::PlanOptions Options;
    Options.MaxDepth = static_cast<std::size_t>(FMath::Max(1, Profile->MaxPlanDepth));
    Options.MaxExpandedNodes = static_cast<std::size_t>(FMath::Max(32, Profile->MaxExpandedNodes));

    const RBRoutine::Core::PlanResult Result = RBRoutine::Core::ChooseGoalAndPlan(CoreState, Goals, Actions, Options);
    CurrentPlan.Reset();
    CurrentPlanIndex = INDEX_NONE;
    CurrentActionId = NAME_None;
    CurrentActivityTag = FGameplayTag();

    if (!Result.Success)
    {
        CurrentGoalId = NAME_None;
        SetStatus(Result.FailureReason == "no_eligible_goal" ? ERBRoutineStatus::Idle : ERBRoutineStatus::Blocked);
        bNeedsEvaluation = false;
        return false;
    }

    CurrentGoalId = ToFName(Result.GoalId);
    for (const std::string& ActionId : Result.ActionIds)
    {
        CurrentPlan.Add(ToFName(ActionId));
    }

    ++PlanSerial;
    OnPlanChanged.Broadcast(CurrentGoalId, CurrentPlan.Num(), PlanSerial);
    bNeedsEvaluation = false;

    if (CurrentPlan.IsEmpty())
    {
        SetStatus(ERBRoutineStatus::Idle);
        return true;
    }

    CurrentPlanIndex = 0;
    StartCurrentPlanAction();
    return true;
}

const FRBRoutineActionDefinition* URBRoutineComponent::FindAction(FName ActionId) const
{
    if (!Profile)
    {
        return nullptr;
    }
    return Profile->Actions.FindByPredicate([ActionId](const FRBRoutineActionDefinition& Item)
    {
        return Item.ActionId == ActionId;
    });
}

bool URBRoutineComponent::IsCurrentActionInterruptible() const
{
    const FRBRoutineActionDefinition* Action = FindAction(CurrentActionId);
    return !Action || Action->bInterruptible;
}

bool URBRoutineComponent::AreCurrentActionPreconditionsSatisfied() const
{
    const FRBRoutineActionDefinition* Action = FindAction(CurrentActionId);
    return Action && AreConditionsSatisfiedUE(Facts, Action->Preconditions);
}

void URBRoutineComponent::StartCurrentPlanAction()
{
    if (!CurrentPlan.IsValidIndex(CurrentPlanIndex))
    {
        CurrentActionId = NAME_None;
        CurrentActivityTag = FGameplayTag();
        SetStatus(ERBRoutineStatus::Idle);
        bNeedsEvaluation = true;
        return;
    }

    const FName NextActionId = CurrentPlan[CurrentPlanIndex];
    const FRBRoutineActionDefinition* Action = FindAction(NextActionId);
    if (!Action || !AreConditionsSatisfiedUE(Facts, Action->Preconditions))
    {
        CurrentActionId = NAME_None;
        CurrentActivityTag = FGameplayTag();
        bNeedsEvaluation = true;
        EvaluateNow(LastDayIndex, LastMinuteOfDay);
        return;
    }

    CurrentActionId = NextActionId;
    CurrentActivityTag = Action->ActivityTag;
    bPopulationHandoffReady = false;
    SetStatus(ERBRoutineStatus::Executing);
    OnActionRequested.Broadcast(CurrentActionId, CurrentActivityTag, PlanSerial);
}

void URBRoutineComponent::NotifyActionFinished(bool bSucceeded)
{
    if (!IsAuthoritative() || CurrentActionId.IsNone())
    {
        return;
    }

    const FGameplayTag CompletedActivityTag = CurrentActivityTag;
    CurrentActionId = NAME_None;
    CurrentActivityTag = FGameplayTag();
    bPopulationHandoffReady = true;

    if (UWorld* World = GetWorld())
    {
        if (URBRoutineWorldSubsystem* Subsystem = World->GetSubsystem<URBRoutineWorldSubsystem>())
        {
            Subsystem->NotifyActivityEnded(AgentId, CompletedActivityTag);
        }
    }

    if (!bSucceeded)

    {
        bNeedsEvaluation = true;
        EvaluateNow(LastDayIndex, LastMinuteOfDay);
        return;
    }

    if (bHasActiveInterrupt || bNeedsEvaluation)
    {
        bNeedsEvaluation = true;
        EvaluateNow(LastDayIndex, LastMinuteOfDay);
        return;
    }

    ++CurrentPlanIndex;
    if (CurrentPlan.IsValidIndex(CurrentPlanIndex))
    {
        StartCurrentPlanAction();
        return;
    }

    bNeedsEvaluation = true;
    EvaluateNow(LastDayIndex, LastMinuteOfDay);
}

void URBRoutineComponent::AbortCurrentAction(FGameplayTag ReasonTag)
{
    const FGameplayTag AbortedActivityTag = CurrentActivityTag;
    if (!CurrentActionId.IsNone())
    {
        bPopulationHandoffReady = false;
        OnActionAborted.Broadcast(CurrentActionId, ReasonTag);
    }
    if (UWorld* World = GetWorld())
    {
        if (URBRoutineWorldSubsystem* Subsystem = World->GetSubsystem<URBRoutineWorldSubsystem>())
        {
            Subsystem->NotifyActivityEnded(AgentId, AbortedActivityTag);
        }
    }
    CurrentActionId = NAME_None;
    CurrentActivityTag = FGameplayTag();

    CurrentPlan.Reset();
    CurrentPlanIndex = INDEX_NONE;
    SetStatus(ERBRoutineStatus::Interrupted);
    bNeedsEvaluation = true;
}

bool URBRoutineComponent::RequestInterrupt(const FRBRoutineInterruptRequest& Request)
{
    if (!IsAuthoritative())
    {
        return false;
    }

    if (bHasActiveInterrupt && Request.Priority <= ActiveInterrupt.Priority)
    {
        return false;
    }

    ActiveInterrupt = Request;
    bHasActiveInterrupt = true;
    bNeedsEvaluation = true;

    if (!CurrentActionId.IsNone() && !IsCurrentActionInterruptible())
    {
        return true;
    }

    AbortCurrentAction(Request.ReasonTag);
    EvaluateNow(LastDayIndex, LastMinuteOfDay);
    return true;
}

void URBRoutineComponent::ClearInterrupt(FGameplayTag ReasonTag)
{
    if (!IsAuthoritative() || !bHasActiveInterrupt)
    {
        return;
    }
    if (ReasonTag.IsValid() && ActiveInterrupt.ReasonTag != ReasonTag)
    {
        return;
    }

    bHasActiveInterrupt = false;
    ActiveInterrupt = FRBRoutineInterruptRequest();
    bNeedsEvaluation = true;

    if (!CurrentActionId.IsNone() && !IsCurrentActionInterruptible())
    {
        return;
    }

    AbortCurrentAction(ReasonTag);
    EvaluateNow(LastDayIndex, LastMinuteOfDay);
}

bool URBRoutineComponent::RecordMemoryEvent(const FRBRoutineMemoryRecord& Memory, double AffinityDelta, double TrustDelta, double FearDelta)
{
    if (!IsAuthoritative() || Memory.EventId.IsNone())
    {
        return false;
    }

    RBRoutine::Core::ContinuityState CoreState;
    CoreState.AgentId = ToStdString(AgentId);
    CoreState.MemoryCapacity = static_cast<std::size_t>(Profile ? FMath::Max(0, Profile->MemoryCapacity) : 128);

    for (const FRBRoutineMemoryRecord& Existing : Memories)
    {
        CoreState.Memories.push_back({
            ToStdString(Existing.EventId),
            Existing.EventTag.IsValid() ? TCHAR_TO_UTF8(*Existing.EventTag.ToString()) : std::string(),
            ToStdString(Existing.OtherAgentId),
            Existing.Salience,
            Existing.Valence,
            Existing.WorldMinute
        });
    }
    for (const FRBRoutineRelationshipState& Existing : Relationships)
    {
        CoreState.Relationships.emplace(ToStdString(Existing.OtherAgentId), RBRoutine::Core::RelationshipState{
            ToStdString(Existing.OtherAgentId), Existing.Affinity, Existing.Trust, Existing.Fear, Existing.LastChangedWorldMinute
        });
    }

    const RBRoutine::Core::MemoryRecord CoreMemory{
        ToStdString(Memory.EventId),
        Memory.EventTag.IsValid() ? TCHAR_TO_UTF8(*Memory.EventTag.ToString()) : std::string(),
        ToStdString(Memory.OtherAgentId),
        FMath::Clamp(Memory.Salience, 0.0, 1.0),
        FMath::Clamp(Memory.Valence, -1.0, 1.0),
        Memory.WorldMinute
    };

    if (!RBRoutine::Core::RecordMemory(CoreState, CoreMemory))
    {
        return false;
    }

    if (!Memory.OtherAgentId.IsNone() && (AffinityDelta != 0.0 || TrustDelta != 0.0 || FearDelta != 0.0))
    {
        RBRoutine::Core::ApplyRelationshipDelta(CoreState, ToStdString(Memory.OtherAgentId), AffinityDelta, TrustDelta, FearDelta, Memory.WorldMinute);
    }

    Memories.Reset();
    for (const auto& Existing : CoreState.Memories)
    {
        FRBRoutineMemoryRecord Converted;
        Converted.EventId = ToFName(Existing.EventId);
        if (!Existing.EventType.empty())
        {
            Converted.EventTag = FGameplayTag::RequestGameplayTag(ToFName(Existing.EventType), false);
        }
        Converted.OtherAgentId = ToFName(Existing.OtherAgentId);
        Converted.Salience = Existing.Salience;
        Converted.Valence = Existing.Valence;
        Converted.WorldMinute = Existing.WorldMinute;
        Memories.Add(Converted);
    }

    Relationships.Reset();
    for (const auto& Pair : CoreState.Relationships)
    {
        const auto& Existing = Pair.second;
        FRBRoutineRelationshipState Converted;
        Converted.OtherAgentId = ToFName(Existing.OtherAgentId);
        Converted.Affinity = Existing.Affinity;
        Converted.Trust = Existing.Trust;
        Converted.Fear = Existing.Fear;
        Converted.LastChangedWorldMinute = Existing.LastChangedWorldMinute;
        Relationships.Add(Converted);
    }

    bNeedsEvaluation = true;
    return true;
}

bool URBRoutineComponent::GetRelationship(FName OtherAgentId, FRBRoutineRelationshipState& OutRelationship) const
{
    if (const FRBRoutineRelationshipState* Found = Relationships.FindByPredicate([OtherAgentId](const FRBRoutineRelationshipState& Item)
    {
        return Item.OtherAgentId == OtherAgentId;
    }))
    {
        OutRelationship = *Found;
        return true;
    }
    return false;
}

FRBRoutineSnapshot URBRoutineComponent::CreateSnapshot(int64 WorldMinute) const
{
    FRBRoutineSnapshot Snapshot;
    Snapshot.SchemaVersion = 1;
    Snapshot.AgentId = AgentId;
    Snapshot.Facts = Facts;
    Snapshot.Memories = Memories;
    Snapshot.Relationships = Relationships;
    Snapshot.CapturedWorldMinute = WorldMinute;
    return Snapshot;
}

bool URBRoutineComponent::RestoreSnapshot(const FRBRoutineSnapshot& Snapshot, FString& OutError)
{
    OutError.Reset();
    if (!IsAuthoritative())
    {
        OutError = TEXT("not_authoritative");
        return false;
    }
    if (Snapshot.SchemaVersion != 1)
    {
        OutError = TEXT("unsupported_schema");
        return false;
    }
    if (Snapshot.AgentId.IsNone())
    {
        OutError = TEXT("missing_agent_id");
        return false;
    }
    if (!AgentId.IsNone() && AgentId != Snapshot.AgentId)
    {
        OutError = TEXT("agent_identity_mismatch");
        return false;
    }

    TSet<FName> EventIds;
    for (const FRBRoutineMemoryRecord& Memory : Snapshot.Memories)
    {
        if (Memory.EventId.IsNone() || EventIds.Contains(Memory.EventId) || !FMath::IsFinite(Memory.Salience) || !FMath::IsFinite(Memory.Valence))
        {
            OutError = TEXT("invalid_memory_record");
            return false;
        }
        EventIds.Add(Memory.EventId);
    }

    TSet<FName> RelationshipIds;
    for (const FRBRoutineRelationshipState& Relationship : Snapshot.Relationships)
    {
        if (Relationship.OtherAgentId.IsNone() || RelationshipIds.Contains(Relationship.OtherAgentId) ||
            !FMath::IsFinite(Relationship.Affinity) || !FMath::IsFinite(Relationship.Trust) || !FMath::IsFinite(Relationship.Fear) ||
            Relationship.Affinity < -1.0 || Relationship.Affinity > 1.0 || Relationship.Trust < -1.0 || Relationship.Trust > 1.0 || Relationship.Fear < 0.0 || Relationship.Fear > 1.0)
        {
            OutError = TEXT("invalid_relationship_record");
            return false;
        }
        RelationshipIds.Add(Relationship.OtherAgentId);
    }

    for (const auto& Pair : Snapshot.Facts)
    {
        if (Pair.Key.IsNone())
        {
            OutError = TEXT("invalid_fact_key");
            return false;
        }
        if (Pair.Value.Type == ERBRoutineFactType::Number && !FMath::IsFinite(Pair.Value.NumberValue))
        {
            OutError = TEXT("invalid_numeric_fact");
            return false;
        }
    }

    AgentId = Snapshot.AgentId;
    Facts = Snapshot.Facts;
    Memories = Snapshot.Memories;
    Relationships = Snapshot.Relationships;
    CurrentPlan.Reset();
    CurrentPlanIndex = INDEX_NONE;
    CurrentGoalId = NAME_None;
    CurrentActionId = NAME_None;
    CurrentActivityTag = FGameplayTag();
    bHasActiveInterrupt = false;
    ActiveInterrupt = FRBRoutineInterruptRequest();
    bNeedsEvaluation = true;
    bPopulationHandoffReady = true;
    SetStatus(ERBRoutineStatus::Idle);
    return true;
}

bool URBRoutineComponent::IsPopulationDemotionSafe(FString& OutReason) const
{
    OutReason.Reset();
    if (!IsAuthoritative())
    {
        OutReason = TEXT("not_authoritative");
        return false;
    }
    if (!CurrentActionId.IsNone())
    {
        OutReason = TEXT("physical_action_active");
        return false;
    }
    if (!bPopulationHandoffReady)
    {
        OutReason = TEXT("physical_handoff_pending");
        return false;
    }
    return true;
}

void URBRoutineComponent::ConfirmPopulationHandoffReady()
{
    if (IsAuthoritative() && CurrentActionId.IsNone())
    {
        bPopulationHandoffReady = true;
    }
}
void URBRoutineComponent::SetSimulationLOD(ERBRoutineSimulationLOD NewLOD)
{
    if (SimulationLOD != NewLOD)
    {
        SimulationLOD = NewLOD;
        bNeedsEvaluation = true;
    }
}

double URBRoutineComponent::GetSuggestedDecisionIntervalSeconds() const
{
    switch (SimulationLOD)
    {
    case ERBRoutineSimulationLOD::Full: return 0.5;
    case ERBRoutineSimulationLOD::Reduced: return 3.0;
    case ERBRoutineSimulationLOD::Dormant: return 15.0;
    }
    return 1.0;
}

void URBRoutineComponent::SetStatus(ERBRoutineStatus NewStatus)
{
    if (Status != NewStatus)
    {
        Status = NewStatus;
        OnStatusChanged.Broadcast(Status);
    }
}

void URBRoutineComponent::OnRep_RoutineState()
{
    OnStatusChanged.Broadcast(Status);
}
