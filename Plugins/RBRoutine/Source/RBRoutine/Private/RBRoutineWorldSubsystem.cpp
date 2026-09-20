#include "RBRoutineWorldSubsystem.h"

#include "Core/RBRoutinePlannerCore.h"
#include "RBRoutineComponent.h"
#include "RBRoutineProfile.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

void URBRoutineWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Agents.Reset();
    NextEvaluationRealTime.Reset();
    LogicalPeople.Reset();
    PopulationOrder.Reset();
    EmbodiedAgents.Reset();
    PopulationProfiles.Reset();
    PendingEmbodimentRequests.Reset();
    PendingDemotionRequests.Reset();
    SocialReservations.Reset();
    AgentSocialReservations.Reset();
    PendingItemAuthorityRequests.Reset();
    RoundRobinCursor = 0;
    PopulationCursor = 0;
    PopulationMetrics = FRBRoutinePopulationMetrics();
}

void URBRoutineWorldSubsystem::Deinitialize()
{
    Agents.Reset();
    NextEvaluationRealTime.Reset();    LogicalPeople.Reset();
    PopulationOrder.Reset();
    EmbodiedAgents.Reset();
    PopulationProfiles.Reset();
    PendingEmbodimentRequests.Reset();
    PendingDemotionRequests.Reset();
    SocialReservations.Reset();
    AgentSocialReservations.Reset();
    PendingItemAuthorityRequests.Reset();
    Super::Deinitialize();
}

TStatId URBRoutineWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URBRoutineWorldSubsystem, STATGROUP_Tickables);
}

bool URBRoutineWorldSubsystem::IsPopulationAuthority() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

void URBRoutineWorldSubsystem::SetExternalTime(int64 DayIndex, int32 MinuteOfDay)
{
    ExternalDayIndex = DayIndex;
    ExternalMinuteOfDay = FMath::Clamp(MinuteOfDay, 0, 1439);
}

bool URBRoutineWorldSubsystem::CreateLogicalPerson(FName AgentId, URBRoutineProfile* Profile, const FRBRoutineCoarseLocation& InitialLocation, FString& OutError)
{
    OutError.Reset();    if (!IsPopulationAuthority())
    {
        OutError = TEXT("not_authoritative");
        return false;
    }
    if (AgentId.IsNone())
    {
        OutError = TEXT("missing_agent_id");
        return false;
    }
    if (LogicalPeople.Contains(AgentId))
    {
        OutError = TEXT("duplicate_agent_id");
        return false;
    }

    FRBRoutineLogicalPersonRecord Person;
    Person.AgentId = AgentId;
    Person.RoutineState.AgentId = AgentId;
    Person.RoutineState.CapturedWorldMinute = GetExternalWorldMinute();
    Person.LastSimulatedWorldMinute = GetExternalWorldMinute();
    Person.Location = InitialLocation;
    if (Profile)
    {
        Person.ProfilePath = FSoftObjectPath(Profile);
        PopulationProfiles.Add(AgentId, Profile);
    }
    LogicalPeople.Add(AgentId, Person);
    PopulationOrder.Add(AgentId);
    RefreshPopulationMetrics();
    return true;
}

bool URBRoutineWorldSubsystem::GetLogicalPerson(FName AgentId, FRBRoutineLogicalPersonRecord& OutPerson) const
{
    if (const FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId))
    {
        OutPerson = *Person;
        OutPerson.Location = ResolveLocationAt(*Person, GetExternalWorldMinute());
        return true;
    }
    return false;
}

bool URBRoutineWorldSubsystem::SetLogicalPersonProfile(FName AgentId, URBRoutineProfile* Profile, FString& OutError)
{
    OutError.Reset();
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    if (!Profile) { OutError = TEXT("missing_profile"); return false; }
    Person->ProfilePath = FSoftObjectPath(Profile);
    PopulationProfiles.Add(AgentId, Profile);
    return true;
}

bool URBRoutineWorldSubsystem::SetLogicalPersonSocialContext(FName AgentId, const FRBRoutinePersonSocialContext& Social, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    if (Social.ImportantAgentIds.Num() > 32) { OutError = TEXT("social_links_over_capacity"); return false; }
    TSet<FName> Seen;
    for (const FName OtherId : Social.ImportantAgentIds)
    {
        if (OtherId.IsNone() || OtherId == AgentId || Seen.Contains(OtherId))
        {
            OutError = TEXT("invalid_social_link");
            return false;
        }
        Seen.Add(OtherId);
    }
    Person->Social = Social;
    return true;
}


bool URBRoutineWorldSubsystem::BeginPairedSocialReservation(
    FName AgentA,
    FName AgentB,
    FGameplayTag ActivityTag,
    FName SiteId,
    FRBRoutineSocialReservation& OutReservation,
    FString& OutError)
{
    OutReservation = FRBRoutineSocialReservation();
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (AgentA.IsNone() || AgentB.IsNone() || AgentA == AgentB) { OutError = TEXT("invalid_social_pair"); return false; }
    if (!ActivityTag.IsValid()) { OutError = TEXT("missing_activity_tag"); return false; }
    if (!LogicalPeople.Contains(AgentA) || !LogicalPeople.Contains(AgentB)) { OutError = TEXT("unknown_agent_id"); return false; }
    if (AgentSocialReservations.Contains(AgentA) || AgentSocialReservations.Contains(AgentB))
    {
        OutError = TEXT("agent_already_reserved");
        return false;
    }

    FRBRoutineSocialReservation Reservation;
    Reservation.ReservationId = FGuid::NewGuid();
    Reservation.AgentA = AgentA;
    Reservation.AgentB = AgentB;
    Reservation.ActivityTag = ActivityTag;
    Reservation.SiteId = SiteId;
    Reservation.ReservedWorldMinute = GetExternalWorldMinute();    SocialReservations.Add(Reservation.ReservationId, Reservation);
    AgentSocialReservations.Add(AgentA, Reservation.ReservationId);
    AgentSocialReservations.Add(AgentB, Reservation.ReservationId);
    OutReservation = Reservation;
    return true;
}

bool URBRoutineWorldSubsystem::ReleaseSocialReservation(FGuid ReservationId, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!ReservationId.IsValid()) { OutError = TEXT("invalid_reservation_id"); return false; }
    const FRBRoutineSocialReservation* Found = SocialReservations.Find(ReservationId);
    if (!Found) { return true; }
    const FName AgentA = Found->AgentA;
    const FName AgentB = Found->AgentB;
    SocialReservations.Remove(ReservationId);
    AgentSocialReservations.Remove(AgentA);
    AgentSocialReservations.Remove(AgentB);
    return true;
}

bool URBRoutineWorldSubsystem::ReleaseSocialReservationForAgent(FName AgentId)
{
    const FGuid* ReservationId = AgentSocialReservations.Find(AgentId);
    if (!ReservationId) { return true; }
    FString Error;
    return ReleaseSocialReservation(*ReservationId, Error);
}

bool URBRoutineWorldSubsystem::GetSocialReservationForAgent(FName AgentId, FRBRoutineSocialReservation& OutReservation) const
{
    OutReservation = FRBRoutineSocialReservation();
    const FGuid* ReservationId = AgentSocialReservations.Find(AgentId);
    if (!ReservationId) { return false; }
    const FRBRoutineSocialReservation* Reservation = SocialReservations.Find(*ReservationId);
    if (!Reservation) { return false; }
    OutReservation = *Reservation;
    return true;
}

void URBRoutineWorldSubsystem::NotifyActivityEnded(FName AgentId, FGameplayTag ActivityTag)
{
    const FGuid* ReservationId = AgentSocialReservations.Find(AgentId);
    if (!ReservationId) { return; }
    const FRBRoutineSocialReservation* Reservation = SocialReservations.Find(*ReservationId);
    if (!Reservation) { AgentSocialReservations.Remove(AgentId); return; }
    if (!ActivityTag.IsValid() || Reservation->ActivityTag == ActivityTag)
    {
        ReleaseSocialReservationForAgent(AgentId);
    }
}

bool URBRoutineWorldSubsystem::RequestItemAuthorityAction(
    FName AgentId,
    ERBRoutineItemIntent Intent,
    const FString& StableItemRef,
    FGameplayTag ActivityTag,
    FRBRoutineItemAuthorityRequest& OutRequest,    FString& OutError)
{
    OutRequest = FRBRoutineItemAuthorityRequest();
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!LogicalPeople.Contains(AgentId)) { OutError = TEXT("unknown_agent_id"); return false; }
    if (StableItemRef.TrimStartAndEnd().IsEmpty()) { OutError = TEXT("missing_item_ref"); return false; }
    if (PendingItemAuthorityRequests.Num() >= 256) { OutError = TEXT("item_request_capacity"); return false; }

    FRBRoutineItemAuthorityRequest Request;
    Request.RequestId = FGuid::NewGuid();
    Request.AgentId = AgentId;
    Request.Intent = Intent;
    Request.StableItemRef = StableItemRef;
    Request.ActivityTag = ActivityTag;
    PendingItemAuthorityRequests.Add(Request.RequestId, Request);
    OutRequest = Request;
    OnItemAuthorityRequested.Broadcast(Request);
    return true;
}

bool URBRoutineWorldSubsystem::ApplyRoutineFactUpdates(
    FName AgentId,
    const TArray<FRBRoutineFactEffect>& Updates,
    FString& OutError)
{
    OutError.Reset();
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }

    TSet<FName> Keys;    for (const FRBRoutineFactEffect& Update : Updates)
    {
        if (Update.Key.IsNone() || Keys.Contains(Update.Key)) { OutError = TEXT("invalid_fact_update"); return false; }
        if (Update.Value.Type == ERBRoutineFactType::Number && !FMath::IsFinite(Update.Value.NumberValue))
        {
            OutError = TEXT("invalid_numeric_fact");
            return false;
        }
        Keys.Add(Update.Key);
    }

    URBRoutineComponent* Agent = nullptr;
    if (const TWeakObjectPtr<URBRoutineComponent>* Binding = EmbodiedAgents.Find(AgentId)) { Agent = Binding->Get(); }
    for (const FRBRoutineFactEffect& Update : Updates)
    {
        if (Agent)
        {
            switch (Update.Value.Type)
            {
            case ERBRoutineFactType::Bool: Agent->SetFactBool(Update.Key, Update.Value.BoolValue); break;
            case ERBRoutineFactType::Integer: Agent->SetFactInteger(Update.Key, Update.Value.IntegerValue); break;
            case ERBRoutineFactType::Number: Agent->SetFactNumber(Update.Key, Update.Value.NumberValue); break;
            case ERBRoutineFactType::Name: Agent->SetFactName(Update.Key, Update.Value.NameValue); break;
            }
        }
        else
        {
            Person->RoutineState.Facts.Add(Update.Key, Update.Value);
        }
    }
    if (Agent)
    {
        Person->RoutineState = Agent->CreateSnapshot(GetExternalWorldMinute());
        Person->LastSimulatedWorldMinute = GetExternalWorldMinute();
    }
    return true;
}

bool URBRoutineWorldSubsystem::ResolveItemAuthorityAction(const FRBRoutineItemAuthorityResult& Result, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!Result.RequestId.IsValid()) { OutError = TEXT("invalid_request_id"); return false; }
    const FRBRoutineItemAuthorityRequest* Request = PendingItemAuthorityRequests.Find(Result.RequestId);
    if (!Request) { OutError = TEXT("unknown_item_request"); return false; }
    const FName AgentId = Request->AgentId;

    if (!ApplyRoutineFactUpdates(AgentId, Result.RoutineFactUpdates, OutError)) { return false; }
    PendingItemAuthorityRequests.Remove(Result.RequestId);
    if (FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId))
    {
        Person->CurrentCoarseActivityTag = Result.bSucceeded ? Request->ActivityTag : FGameplayTag();
    }
    return true;
}

bool URBRoutineWorldSubsystem::ApplyContinuityOutcome(
    FName AgentId,
    const FRBRoutineMemoryRecord& Memory,
    double AffinityDelta,    double TrustDelta,
    double FearDelta,
    FString& OutError)
{
    OutError.Reset();
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    if (Memory.EventId.IsNone())
    {
        if (AffinityDelta != 0.0 || TrustDelta != 0.0 || FearDelta != 0.0)
        {
            OutError = TEXT("relationship_delta_requires_memory_event");
            return false;
        }
        return true;
    }
    if (!FMath::IsFinite(Memory.Salience) || !FMath::IsFinite(Memory.Valence))
    {
        OutError = TEXT("invalid_memory_record");
        return false;
    }

    URBRoutineComponent* Agent = nullptr;
    if (const TWeakObjectPtr<URBRoutineComponent>* Binding = EmbodiedAgents.Find(AgentId)) { Agent = Binding->Get(); }
    if (Agent)
    {
        if (!Agent->RecordMemoryEvent(Memory, AffinityDelta, TrustDelta, FearDelta))
        {
            OutError = TEXT("duplicate_or_invalid_memory_event");
            return false;
        }        Person->RoutineState = Agent->CreateSnapshot(GetExternalWorldMinute());
        Person->LastSimulatedWorldMinute = GetExternalWorldMinute();
        return true;
    }

    RBRoutine::Core::ContinuityState CoreState;
    CoreState.AgentId = TCHAR_TO_UTF8(*AgentId.ToString());
    if (URBRoutineProfile* Profile = ResolvePopulationProfile(AgentId, *Person))
    {
        CoreState.MemoryCapacity = static_cast<std::size_t>(FMath::Max(0, Profile->MemoryCapacity));
    }
    else
    {
        CoreState.MemoryCapacity = 128;
    }
    for (const FRBRoutineMemoryRecord& Existing : Person->RoutineState.Memories)
    {
        CoreState.Memories.push_back({
            TCHAR_TO_UTF8(*Existing.EventId.ToString()),
            Existing.EventTag.IsValid() ? TCHAR_TO_UTF8(*Existing.EventTag.ToString()) : std::string(),
            TCHAR_TO_UTF8(*Existing.OtherAgentId.ToString()),
            Existing.Salience,
            Existing.Valence,
            Existing.WorldMinute});
    }
    for (const FRBRoutineRelationshipState& Existing : Person->RoutineState.Relationships)
    {
        const std::string Other = TCHAR_TO_UTF8(*Existing.OtherAgentId.ToString());
        CoreState.Relationships.emplace(Other, RBRoutine::Core::RelationshipState{
            Other, Existing.Affinity, Existing.Trust, Existing.Fear, Existing.LastChangedWorldMinute});    }

    const RBRoutine::Core::MemoryRecord CoreMemory{
        TCHAR_TO_UTF8(*Memory.EventId.ToString()),
        Memory.EventTag.IsValid() ? TCHAR_TO_UTF8(*Memory.EventTag.ToString()) : std::string(),
        TCHAR_TO_UTF8(*Memory.OtherAgentId.ToString()),
        FMath::Clamp(Memory.Salience, 0.0, 1.0),
        FMath::Clamp(Memory.Valence, -1.0, 1.0),
        Memory.WorldMinute};
    if (!RBRoutine::Core::RecordMemory(CoreState, CoreMemory))
    {
        OutError = TEXT("duplicate_or_invalid_memory_event");
        return false;
    }
    if (!Memory.OtherAgentId.IsNone() && (AffinityDelta != 0.0 || TrustDelta != 0.0 || FearDelta != 0.0))
    {
        RBRoutine::Core::ApplyRelationshipDelta(
            CoreState,
            TCHAR_TO_UTF8(*Memory.OtherAgentId.ToString()),
            AffinityDelta,
            TrustDelta,
            FearDelta,
            Memory.WorldMinute);
    }

    Person->RoutineState.Memories.Reset();
    for (const RBRoutine::Core::MemoryRecord& Existing : CoreState.Memories)
    {
        FRBRoutineMemoryRecord Converted;        Converted.EventId = FName(UTF8_TO_TCHAR(Existing.EventId.c_str()));
        if (!Existing.EventType.empty())
        {
            Converted.EventTag = FGameplayTag::RequestGameplayTag(FName(UTF8_TO_TCHAR(Existing.EventType.c_str())), false);
        }
        Converted.OtherAgentId = FName(UTF8_TO_TCHAR(Existing.OtherAgentId.c_str()));
        Converted.Salience = Existing.Salience;
        Converted.Valence = Existing.Valence;
        Converted.WorldMinute = Existing.WorldMinute;
        Person->RoutineState.Memories.Add(Converted);
    }

    Person->RoutineState.Relationships.Reset();
    for (const auto& Pair : CoreState.Relationships)
    {
        const RBRoutine::Core::RelationshipState& Existing = Pair.second;
        FRBRoutineRelationshipState Converted;
        Converted.OtherAgentId = FName(UTF8_TO_TCHAR(Existing.OtherAgentId.c_str()));
        Converted.Affinity = Existing.Affinity;
        Converted.Trust = Existing.Trust;
        Converted.Fear = Existing.Fear;
        Converted.LastChangedWorldMinute = Existing.LastChangedWorldMinute;
        Person->RoutineState.Relationships.Add(Converted);
    }
    Person->RoutineState.Relationships.Sort([](const FRBRoutineRelationshipState& A, const FRBRoutineRelationshipState& B)
    {
        return A.OtherAgentId.LexicalLess(B.OtherAgentId);
    });
    Person->RoutineState.CapturedWorldMinute = FMath::Max(Person->RoutineState.CapturedWorldMinute, Memory.WorldMinute);
    return true;
}

bool URBRoutineWorldSubsystem::BuildDialogueContext(
    FName AgentId,
    FName OtherAgentId,
    FRBRoutineDialogueContext& OutContext,
    FString& OutError) const
{
    OutContext = FRBRoutineDialogueContext();
    OutError.Reset();
    const FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }

    FRBRoutineSnapshot Snapshot = Person->RoutineState;
    FName CurrentGoal = Person->CurrentIntentionGoalId;
    FGameplayTag CurrentActivity = Person->CurrentCoarseActivityTag;
    if (const TWeakObjectPtr<URBRoutineComponent>* Binding = EmbodiedAgents.Find(AgentId))
    {
        if (const URBRoutineComponent* Agent = Binding->Get())
        {
            Snapshot = Agent->CreateSnapshot(GetExternalWorldMinute());
            CurrentGoal = Agent->CurrentGoalId;
            CurrentActivity = Agent->CurrentActivityTag;
        }
    }

    OutContext.AgentId = AgentId;
    OutContext.OtherAgentId = OtherAgentId;
    OutContext.CurrentIntentionGoalId = CurrentGoal;
    OutContext.CurrentActivityTag = CurrentActivity;
    OutContext.Social = Person->Social;
    for (const FRBRoutineRelationshipState& Relationship : Snapshot.Relationships)
    {
        if (Relationship.OtherAgentId == OtherAgentId)
        {
            OutContext.bHasRelationship = true;
            OutContext.Relationship = Relationship;
            break;
        }
    }

    TArray<FRBRoutineMemoryRecord> Candidates = Snapshot.Memories;
    Candidates.Sort([OtherAgentId](const FRBRoutineMemoryRecord& A, const FRBRoutineMemoryRecord& B)
    {
        const bool bADirect = A.OtherAgentId == OtherAgentId;
        const bool bBDirect = B.OtherAgentId == OtherAgentId;
        if (bADirect != bBDirect) { return bADirect; }
        if (A.Salience != B.Salience) { return A.Salience > B.Salience; }
        if (A.WorldMinute != B.WorldMinute) { return A.WorldMinute > B.WorldMinute; }
        return A.EventId.LexicalLess(B.EventId);
    });
    const int32 Limit = FMath::Min(8, Candidates.Num());
    OutContext.RelevantMemories.Reserve(Limit);
    for (int32 Index = 0; Index < Limit; ++Index)
    {
        OutContext.RelevantMemories.Add(Candidates[Index]);
    }
    return true;
}

bool URBRoutineWorldSubsystem::ApplyDialogueOutcome(
    const FRBRoutineDialogueOutcome& Outcome,
    FString& OutError)
{    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (Outcome.AgentId.IsNone() || !LogicalPeople.Contains(Outcome.AgentId)) { OutError = TEXT("unknown_agent_id"); return false; }
    if (!Outcome.Memory.OtherAgentId.IsNone() && !Outcome.OtherAgentId.IsNone() && Outcome.Memory.OtherAgentId != Outcome.OtherAgentId)
    {
        OutError = TEXT("dialogue_other_agent_mismatch");
        return false;
    }
    TSet<FName> FactKeys;
    for (const FRBRoutineFactEffect& Update : Outcome.RoutineFactUpdates)
    {
        if (Update.Key.IsNone() || FactKeys.Contains(Update.Key)) { OutError = TEXT("invalid_fact_update"); return false; }
        if (Update.Value.Type == ERBRoutineFactType::Number && !FMath::IsFinite(Update.Value.NumberValue))
        {
            OutError = TEXT("invalid_numeric_fact");
            return false;
        }
        FactKeys.Add(Update.Key);
    }

    FRBRoutineMemoryRecord Memory = Outcome.Memory;
    if (Memory.OtherAgentId.IsNone()) { Memory.OtherAgentId = Outcome.OtherAgentId; }
    if (!ApplyContinuityOutcome(Outcome.AgentId, Memory, Outcome.AffinityDelta, Outcome.TrustDelta, Outcome.FearDelta, OutError))
    {
        return false;
    }
    return ApplyRoutineFactUpdates(Outcome.AgentId, Outcome.RoutineFactUpdates, OutError);
}

FRBRoutineCoarseLocation URBRoutineWorldSubsystem::ResolveLocationAt(const FRBRoutineLogicalPersonRecord& Person, int64 WorldMinute) const
{
    if (!Person.Travel.bActive)
    {
        return Person.Location;
    }

    FRBRoutineCoarseLocation Result = Person.Location;
    if (WorldMinute >= Person.Travel.ArrivalWorldMinute)
    {
        Result.AreaId = Person.Travel.DestinationAreaId;
        Result.AnchorId = Person.Travel.DestinationAnchorId;
        Result.WorldLocation = Person.Travel.DestinationWorldLocation;
        Result.bHasWorldLocation = Person.Travel.bHasDestinationWorldLocation;
        return Result;
    }

    Result.AreaId = Person.Travel.OriginAreaId;
    Result.AnchorId = NAME_None;
    if (Person.Travel.bHasOriginWorldLocation && Person.Travel.bHasDestinationWorldLocation && Person.Travel.ArrivalWorldMinute > Person.Travel.StartedWorldMinute)
    {
        const double Alpha = FMath::Clamp(
            static_cast<double>(WorldMinute - Person.Travel.StartedWorldMinute) /
            static_cast<double>(Person.Travel.ArrivalWorldMinute - Person.Travel.StartedWorldMinute), 0.0, 1.0);
        Result.WorldLocation = FMath::Lerp(Person.Travel.OriginWorldLocation, Person.Travel.DestinationWorldLocation, Alpha);
        Result.bHasWorldLocation = true;
    }
    else
    {
        Result.WorldLocation = Person.Travel.OriginWorldLocation;
        Result.bHasWorldLocation = Person.Travel.bHasOriginWorldLocation;
    }
    return Result;
}

bool URBRoutineWorldSubsystem::StartCoarseTravel(FName AgentId, const FRBRoutineCoarseLocation& Destination, int64 TravelDurationWorldMinutes, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (TravelDurationWorldMinutes < 0) { OutError = TEXT("invalid_travel_duration"); return false; }
    if (Destination.AreaId.IsNone() && Destination.AnchorId.IsNone() && !Destination.bHasWorldLocation)
    {
        OutError = TEXT("missing_destination");
        return false;
    }
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    if (!CatchUpRecord(*Person, GetExternalWorldMinute(), OutError)) { return false; }

    const FRBRoutineCoarseLocation Origin = ResolveLocationAt(*Person, GetExternalWorldMinute());
    if (TravelDurationWorldMinutes == 0)
    {
        Person->Location = Destination;
        Person->Travel = FRBRoutineCoarseTravelState();
        return true;
    }

    Person->Travel = FRBRoutineCoarseTravelState();
    Person->Travel.bActive = true;
    Person->Travel.OriginAreaId = Origin.AreaId;
    Person->Travel.OriginAnchorId = Origin.AnchorId;
    Person->Travel.OriginWorldLocation = Origin.WorldLocation;
    Person->Travel.bHasOriginWorldLocation = Origin.bHasWorldLocation;
    Person->Travel.DestinationAreaId = Destination.AreaId;
    Person->Travel.DestinationAnchorId = Destination.AnchorId;
    Person->Travel.DestinationWorldLocation = Destination.WorldLocation;
    Person->Travel.bHasDestinationWorldLocation = Destination.bHasWorldLocation;
    Person->Travel.StartedWorldMinute = GetExternalWorldMinute();
    Person->Travel.ArrivalWorldMinute = GetExternalWorldMinute() + TravelDurationWorldMinutes;
    Person->Location = Origin;
    return true;
}

URBRoutineProfile* URBRoutineWorldSubsystem::ResolvePopulationProfile(FName AgentId, const FRBRoutineLogicalPersonRecord& Person) const
{
    if (const TWeakObjectPtr<URBRoutineProfile>* Found = PopulationProfiles.Find(AgentId))
    {
        if (Found->IsValid()) { return Found->Get(); }
    }    return Cast<URBRoutineProfile>(Person.ProfilePath.ResolveObject());
}

bool URBRoutineWorldSubsystem::CatchUpRecord(FRBRoutineLogicalPersonRecord& Person, int64 TargetWorldMinute, FString& OutError)
{
    OutError.Reset();
    if (TargetWorldMinute <= Person.LastSimulatedWorldMinute)
    {
        return true;
    }

    if (Person.Travel.bActive)
    {
        Person.Location = ResolveLocationAt(Person, TargetWorldMinute);
        if (TargetWorldMinute >= Person.Travel.ArrivalWorldMinute)
        {
            Person.Travel = FRBRoutineCoarseTravelState();
        }
    }

    if (URBRoutineProfile* Profile = ResolvePopulationProfile(Person.AgentId, Person))
    {
        const int64 DayIndex = FMath::FloorToInt64(static_cast<double>(TargetWorldMinute) / 1440.0);
        const int32 MinuteOfDay = static_cast<int32>((TargetWorldMinute % 1440 + 1440) % 1440);
        const FRBRoutineScheduleEntry* Best = nullptr;
        for (const FRBRoutineScheduleEntry& Entry : Profile->Schedule)
        {
            RBRoutine::Core::ScheduleEntry CoreEntry;
            CoreEntry.Id = TCHAR_TO_UTF8(*Entry.EntryId.ToString());
            CoreEntry.DayMask = static_cast<std::uint8_t>(FMath::Clamp(Entry.DayMask, 0, 127));
            CoreEntry.StartMinuteOfDay = Entry.StartMinuteOfDay;
            CoreEntry.EndMinuteOfDay = Entry.EndMinuteOfDay;            CoreEntry.PreferredGoalId = TCHAR_TO_UTF8(*Entry.PreferredGoalId.ToString());
            CoreEntry.PriorityBoost = Entry.PriorityBoost;
            if (!Entry.PreferredGoalId.IsNone() && RBRoutine::Core::IsScheduleEntryActive(CoreEntry, DayIndex, MinuteOfDay))
            {
                if (!Best || Entry.PriorityBoost > Best->PriorityBoost ||
                    (Entry.PriorityBoost == Best->PriorityBoost && Entry.EntryId.LexicalLess(Best->EntryId)))
                {
                    Best = &Entry;
                }
            }
        }
        if (Best)
        {
            Person.CurrentIntentionGoalId = Best->PreferredGoalId;
        }
    }

    Person.LastSimulatedWorldMinute = TargetWorldMinute;
    Person.RoutineState.CapturedWorldMinute = TargetWorldMinute;
    return true;
}

bool URBRoutineWorldSubsystem::CatchUpLogicalPerson(FName AgentId, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    return CatchUpRecord(*Person, GetExternalWorldMinute(), OutError);
}

bool URBRoutineWorldSubsystem::RequestEmbodiment(FName AgentId, FString& OutError)
{    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person) { OutError = TEXT("unknown_agent_id"); return false; }
    if (Person->RepresentationState == ERBRoutineRepresentationState::Embodied) { return true; }
    if (PendingEmbodimentRequests.Contains(AgentId)) { return true; }
    if (EmbodiedAgents.Num() + PendingEmbodimentRequests.Num() >= MaxEmbodiedPopulation)
    {
        OutError = TEXT("embodiment_budget_reached");
        return false;
    }
    if (!CatchUpRecord(*Person, GetExternalWorldMinute(), OutError)) { return false; }
    PendingDemotionRequests.Remove(AgentId);
    PendingEmbodimentRequests.Add(AgentId);
    OnEmbodimentRequested.Broadcast(AgentId, ResolveLocationAt(*Person, GetExternalWorldMinute()));
    return true;
}

bool URBRoutineWorldSubsystem::RequestDemotion(FName AgentId, FGameplayTag ReasonTag, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    const TWeakObjectPtr<URBRoutineComponent>* Found = EmbodiedAgents.Find(AgentId);
    URBRoutineComponent* Agent = Found ? Found->Get() : nullptr;
    if (!Agent) { PendingDemotionRequests.Remove(AgentId); return true; }
    if (PendingDemotionRequests.Contains(AgentId)) { return true; }
    if (!Agent->IsPopulationDemotionSafe(OutError)) { return false; }
    PendingEmbodimentRequests.Remove(AgentId);    PendingDemotionRequests.Add(AgentId);
    OnDemotionRequested.Broadcast(AgentId, ReasonTag);
    return true;
}

void URBRoutineWorldSubsystem::CaptureEmbodiedState(FRBRoutineLogicalPersonRecord& Person, const URBRoutineComponent* Agent, bool bUnexpected) const
{
    if (!Agent) { return; }
    Person.RoutineState = Agent->CreateSnapshot(GetExternalWorldMinute());
    Person.CurrentIntentionGoalId = Agent->CurrentGoalId;
    Person.CurrentCoarseActivityTag = Agent->CurrentActivityTag;
    Person.SimulationLOD = Agent->SimulationLOD;
    Person.LastSimulatedWorldMinute = GetExternalWorldMinute();
    Person.RepresentationState = ERBRoutineRepresentationState::LogicalOnly;
    if (const AActor* Owner = Agent->GetOwner())
    {
        Person.Location.WorldLocation = Owner->GetActorLocation();
        Person.Location.bHasWorldLocation = true;
    }
    FString HandoffError;
    Person.bNeedsAuthoritativeReconciliation = bUnexpected && !Agent->IsPopulationDemotionSafe(HandoffError);
}

bool URBRoutineWorldSubsystem::BindEmbodiedAgent(FName AgentId, URBRoutineComponent* Agent, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!IsValid(Agent) || !Agent->IsAuthoritative()) { OutError = TEXT("invalid_embodied_agent"); return false; }
    if (AgentId.IsNone()) { OutError = TEXT("missing_agent_id"); return false; }
    if (!Agent->AgentId.IsNone() && Agent->AgentId != AgentId) { OutError = TEXT("agent_identity_mismatch"); return false; }
    if (const TWeakObjectPtr<URBRoutineComponent>* Existing = EmbodiedAgents.Find(AgentId))
    {
        if (Existing->IsValid() && Existing->Get() != Agent)
        {
            OutError = TEXT("duplicate_embodied_identity");
            return false;
        }
    }

    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
    if (!Person)
    {
        FRBRoutineCoarseLocation InitialLocation;
        if (const AActor* Owner = Agent->GetOwner())
        {
            InitialLocation.WorldLocation = Owner->GetActorLocation();
            InitialLocation.bHasWorldLocation = true;
        }
        if (!CreateLogicalPerson(AgentId, Agent->Profile, InitialLocation, OutError)) { return false; }
        Person = LogicalPeople.Find(AgentId);
        Person->RoutineState = Agent->CreateSnapshot(GetExternalWorldMinute());
    }
    else if (!CatchUpRecord(*Person, GetExternalWorldMinute(), OutError))
    {
        return false;
    }

    Agent->AgentId = AgentId;
    if (!Agent->Profile)
    {
        Agent->Profile = ResolvePopulationProfile(AgentId, *Person);
    }    if (Agent->Profile)
    {
        Person->ProfilePath = FSoftObjectPath(Agent->Profile);
        PopulationProfiles.Add(AgentId, Agent->Profile);
    }

    if (!Agent->RestoreSnapshot(Person->RoutineState, OutError))
    {
        return false;
    }
    Agent->CurrentGoalId = Person->CurrentIntentionGoalId;
    Agent->SetSimulationLOD(Person->SimulationLOD);
    Person->RepresentationState = ERBRoutineRepresentationState::Embodied;
    Person->bNeedsAuthoritativeReconciliation = false;
    EmbodiedAgents.Add(AgentId, Agent);
    PendingEmbodimentRequests.Remove(AgentId);
    PendingDemotionRequests.Remove(AgentId);
    RefreshPopulationMetrics();
    return true;
}

bool URBRoutineWorldSubsystem::UnbindEmbodiedAgent(URBRoutineComponent* Agent, bool bUnexpected, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (!IsValid(Agent) || Agent->AgentId.IsNone()) { OutError = TEXT("invalid_embodied_agent"); return false; }
    const TWeakObjectPtr<URBRoutineComponent>* Existing = EmbodiedAgents.Find(Agent->AgentId);
    if (!Existing || Existing->Get() != Agent) { OutError = TEXT("embodiment_binding_mismatch"); return false; }
    if (!bUnexpected && !Agent->IsPopulationDemotionSafe(OutError)) { return false; }
    FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(Agent->AgentId);    if (!Person) { OutError = TEXT("missing_logical_record"); return false; }
    ReleaseSocialReservationForAgent(Agent->AgentId);
    CaptureEmbodiedState(*Person, Agent, bUnexpected);
    EmbodiedAgents.Remove(Agent->AgentId);
    PendingEmbodimentRequests.Remove(Agent->AgentId);
    PendingDemotionRequests.Remove(Agent->AgentId);
    RefreshPopulationMetrics();
    return true;
}

void URBRoutineWorldSubsystem::RegisterAgent(URBRoutineComponent* Agent)
{
    if (!IsValid(Agent)) { return; }
    TWeakObjectPtr<URBRoutineComponent> WeakAgent(Agent);
    Agents.AddUnique(WeakAgent);
    NextEvaluationRealTime.FindOrAdd(WeakAgent) = 0.0;
    if (!Agent->IsAuthoritative() || Agent->AgentId.IsNone()) { return; }

    FString Error;
    if (!BindEmbodiedAgent(Agent->AgentId, Agent, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("RB Routine population bind failed for '%s': %s"), *Agent->AgentId.ToString(), *Error);
    }
}

void URBRoutineWorldSubsystem::UnregisterAgent(URBRoutineComponent* Agent)
{
    TWeakObjectPtr<URBRoutineComponent> WeakAgent(Agent);
    Agents.Remove(WeakAgent);
    NextEvaluationRealTime.Remove(WeakAgent);
    if (IsValid(Agent) && Agent->IsAuthoritative() && !Agent->AgentId.IsNone())
    {
        FString Error;        if (!UnbindEmbodiedAgent(Agent, true, Error))
        {
            UE_LOG(LogTemp, Warning, TEXT("RB Routine population unexpected unbind failed for '%s': %s"), *Agent->AgentId.ToString(), *Error);
        }
    }
    if (Agents.IsEmpty()) { RoundRobinCursor = 0; }
    else { RoundRobinCursor %= Agents.Num(); }
}

ERBRoutineSimulationLOD URBRoutineWorldSubsystem::DetermineLOD(const URBRoutineComponent* Agent) const
{
    const AActor* Owner = Agent ? Agent->GetOwner() : nullptr;
    const UWorld* World = GetWorld();
    if (!Owner || !World) { return ERBRoutineSimulationLOD::Dormant; }

    const FVector AgentLocation = Owner->GetActorLocation();
    double BestDistanceSquared = TNumericLimits<double>::Max();
    bool bFoundPlayer = false;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (!Pawn) { continue; }
        bFoundPlayer = true;
        BestDistanceSquared = FMath::Min(BestDistanceSquared, FVector::DistSquared(AgentLocation, Pawn->GetActorLocation()));
    }
    if (!bFoundPlayer) { return ERBRoutineSimulationLOD::Full; }
    if (BestDistanceSquared <= FMath::Square(FullSimulationDistance)) { return ERBRoutineSimulationLOD::Full; }
    if (BestDistanceSquared <= FMath::Square(ReducedSimulationDistance)) { return ERBRoutineSimulationLOD::Reduced; }
    return ERBRoutineSimulationLOD::Dormant;
}

ERBRoutineSimulationLOD URBRoutineWorldSubsystem::DetermineLogicalLOD(const FRBRoutineLogicalPersonRecord& Person) const
{
    const UWorld* World = GetWorld();
    const FRBRoutineCoarseLocation Location = ResolveLocationAt(Person, GetExternalWorldMinute());
    if (!World || !Location.bHasWorldLocation) { return ERBRoutineSimulationLOD::Dormant; }

    double BestDistanceSquared = TNumericLimits<double>::Max();
    bool bFoundPlayer = false;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (!Pawn) { continue; }
        bFoundPlayer = true;
        BestDistanceSquared = FMath::Min(BestDistanceSquared, FVector::DistSquared(Location.WorldLocation, Pawn->GetActorLocation()));
    }
    if (!bFoundPlayer) { return ERBRoutineSimulationLOD::Dormant; }
    if (BestDistanceSquared <= FMath::Square(FullSimulationDistance)) { return ERBRoutineSimulationLOD::Full; }
    if (BestDistanceSquared <= FMath::Square(ReducedSimulationDistance)) { return ERBRoutineSimulationLOD::Reduced; }
    return ERBRoutineSimulationLOD::Dormant;
}

bool URBRoutineWorldSubsystem::ValidateLogicalPerson(const FRBRoutineLogicalPersonRecord& Person, FString& OutError) const
{
    OutError.Reset();
    if (Person.SchemaVersion != 1) { OutError = TEXT("unsupported_person_schema"); return false; }
    if (Person.AgentId.IsNone()) { OutError = TEXT("missing_agent_id"); return false; }
    if (Person.RoutineState.SchemaVersion != 1) { OutError = TEXT("unsupported_routine_schema"); return false; }
    if (Person.RoutineState.AgentId != Person.AgentId) { OutError = TEXT("agent_identity_mismatch"); return false; }    if (Person.Travel.bActive && Person.Travel.ArrivalWorldMinute < Person.Travel.StartedWorldMinute)
    {
        OutError = TEXT("invalid_travel_interval");
        return false;
    }
    if (Person.Social.ImportantAgentIds.Num() > 32) { OutError = TEXT("social_links_over_capacity"); return false; }

    TSet<FName> EventIds;
    for (const FRBRoutineMemoryRecord& Memory : Person.RoutineState.Memories)
    {
        if (Memory.EventId.IsNone() || EventIds.Contains(Memory.EventId) || !FMath::IsFinite(Memory.Salience) || !FMath::IsFinite(Memory.Valence))
        {
            OutError = TEXT("invalid_memory_record");
            return false;
        }
        EventIds.Add(Memory.EventId);
    }

    TSet<FName> RelationshipIds;
    for (const FRBRoutineRelationshipState& Relationship : Person.RoutineState.Relationships)
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

    for (const auto& Pair : Person.RoutineState.Facts)
    {
        if (Pair.Key.IsNone()) { OutError = TEXT("invalid_fact_key"); return false; }
        if (Pair.Value.Type == ERBRoutineFactType::Number && !FMath::IsFinite(Pair.Value.NumberValue))
        {
            OutError = TEXT("invalid_numeric_fact");
            return false;
        }
    }

    TSet<FName> SocialIds;
    for (const FName OtherId : Person.Social.ImportantAgentIds)
    {
        if (OtherId.IsNone() || OtherId == Person.AgentId || SocialIds.Contains(OtherId))
        {
            OutError = TEXT("invalid_social_link");
            return false;
        }
        SocialIds.Add(OtherId);
    }
    return true;
}

FRBRoutinePopulationSnapshot URBRoutineWorldSubsystem::CreatePopulationSnapshot() const
{
    FRBRoutinePopulationSnapshot Snapshot;
    Snapshot.CapturedWorldMinute = GetExternalWorldMinute();
    Snapshot.People.Reserve(PopulationOrder.Num());
    for (const FName AgentId : PopulationOrder)
    {
        const FRBRoutineLogicalPersonRecord* Stored = LogicalPeople.Find(AgentId);        if (!Stored) { continue; }
        FRBRoutineLogicalPersonRecord Copy = *Stored;
        if (const TWeakObjectPtr<URBRoutineComponent>* Binding = EmbodiedAgents.Find(AgentId))
        {
            if (const URBRoutineComponent* Agent = Binding->Get())
            {
                CaptureEmbodiedState(Copy, Agent, false);
            }
        }
        Copy.RepresentationState = ERBRoutineRepresentationState::LogicalOnly;
        Snapshot.People.Add(MoveTemp(Copy));
    }
    return Snapshot;
}

bool URBRoutineWorldSubsystem::RestorePopulationSnapshot(const FRBRoutinePopulationSnapshot& Snapshot, FString& OutError)
{
    OutError.Reset();
    if (!IsPopulationAuthority()) { OutError = TEXT("not_authoritative"); return false; }
    if (Snapshot.SchemaVersion != 1) { OutError = TEXT("unsupported_population_schema"); return false; }
    if (!EmbodiedAgents.IsEmpty()) { OutError = TEXT("embodied_agents_present"); return false; }

    TMap<FName, FRBRoutineLogicalPersonRecord> Validated;
    TArray<FName> Order;
    Validated.Reserve(Snapshot.People.Num());
    Order.Reserve(Snapshot.People.Num());
    for (const FRBRoutineLogicalPersonRecord& SourcePerson : Snapshot.People)
    {        FString ValidationError;
        if (!ValidateLogicalPerson(SourcePerson, ValidationError))
        {
            OutError = FString::Printf(TEXT("invalid_person:%s:%s"), *SourcePerson.AgentId.ToString(), *ValidationError);
            return false;
        }
        if (Validated.Contains(SourcePerson.AgentId))
        {
            OutError = TEXT("duplicate_agent_id");
            return false;
        }
        if (SourcePerson.LastSimulatedWorldMinute > Snapshot.CapturedWorldMinute)
        {
            OutError = TEXT("person_time_after_snapshot");
            return false;
        }
        FRBRoutineLogicalPersonRecord Person = SourcePerson;
        Person.RepresentationState = ERBRoutineRepresentationState::LogicalOnly;
        Validated.Add(Person.AgentId, Person);
        Order.Add(Person.AgentId);
    }

    LogicalPeople = MoveTemp(Validated);
    PopulationOrder = MoveTemp(Order);
    PopulationProfiles.Reset();
    PendingEmbodimentRequests.Reset();
    PendingDemotionRequests.Reset();
    SocialReservations.Reset();
    AgentSocialReservations.Reset();
    PendingItemAuthorityRequests.Reset();
    PopulationCursor = 0;
    ExternalDayIndex = FMath::FloorToInt64(static_cast<double>(Snapshot.CapturedWorldMinute) / 1440.0);
    ExternalMinuteOfDay = static_cast<int32>((Snapshot.CapturedWorldMinute % 1440 + 1440) % 1440);
    RefreshPopulationMetrics();
    return true;
}

int64 URBRoutineWorldSubsystem::EstimatePopulationAllocatedBytes() const
{
    int64 Bytes = LogicalPeople.GetAllocatedSize() + PopulationOrder.GetAllocatedSize() +
        EmbodiedAgents.GetAllocatedSize() + PopulationProfiles.GetAllocatedSize() +
        PendingEmbodimentRequests.GetAllocatedSize() + PendingDemotionRequests.GetAllocatedSize();
    for (const auto& Pair : LogicalPeople)
    {
        const FRBRoutineLogicalPersonRecord& Person = Pair.Value;
        Bytes += Person.RoutineState.Facts.GetAllocatedSize();
        Bytes += Person.RoutineState.Memories.GetAllocatedSize();
        Bytes += Person.RoutineState.Relationships.GetAllocatedSize();
        Bytes += Person.Social.ImportantAgentIds.GetAllocatedSize();
    }
    return Bytes;
}

void URBRoutineWorldSubsystem::RefreshPopulationMetrics()
{
    for (auto It = EmbodiedAgents.CreateIterator(); It; ++It)
    {
        if (!It.Value().IsValid()) { It.RemoveCurrent(); }
    }
    PopulationMetrics.LogicalPeople = LogicalPeople.Num();
    PopulationMetrics.EmbodiedPeople = EmbodiedAgents.Num();
}

FRBRoutinePopulationMetrics URBRoutineWorldSubsystem::GetPopulationMetrics() const
{
    FRBRoutinePopulationMetrics Result = PopulationMetrics;
    Result.LogicalPeople = LogicalPeople.Num();
    Result.EmbodiedPeople = EmbodiedAgents.Num();
    Result.EstimatedAllocatedBytes = EstimatePopulationAllocatedBytes();
    return Result;
}

void URBRoutineWorldSubsystem::ProcessPopulation()
{
    const double SchedulerStart = FPlatformTime::Seconds();
    PopulationMetrics.RecordsVisitedLastTick = 0;
    PopulationMetrics.CatchUpsLastTick = 0;    PopulationMetrics.FullEvaluationsLastTick = 0;
    PopulationMetrics.ReducedEvaluationsLastTick = 0;
    PopulationMetrics.DormantEvaluationsLastTick = 0;
    PopulationMetrics.PromotionRequestsLastTick = 0;
    PopulationMetrics.DemotionRequestsLastTick = 0;
    PopulationMetrics.CatchUpWorldMinutesAppliedLastTick = 0;
    PopulationMetrics.CatchUpMillisecondsLastTick = 0.0;

    if (!IsPopulationAuthority() || PopulationOrder.IsEmpty())
    {
        RefreshPopulationMetrics();
        PopulationMetrics.SchedulerMillisecondsLastTick = (FPlatformTime::Seconds() - SchedulerStart) * 1000.0;
        return;
    }

    const int32 Visits = FMath::Min(MaxPopulationRecordsPerTick, PopulationOrder.Num());
    const int64 TargetWorldMinute = GetExternalWorldMinute();
    for (int32 Count = 0; Count < Visits; ++Count)
    {
        PopulationCursor %= PopulationOrder.Num();
        const FName AgentId = PopulationOrder[PopulationCursor++];
        FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(AgentId);
        if (!Person) { continue; }
        ++PopulationMetrics.RecordsVisitedLastTick;

        if (Person->LastSimulatedWorldMinute < TargetWorldMinute && PopulationMetrics.CatchUpsLastTick < MaxCatchUpRecordsPerTick)
        {
            const int64 Before = Person->LastSimulatedWorldMinute;
            const double CatchUpStart = FPlatformTime::Seconds();
            FString Error;
            if (CatchUpRecord(*Person, TargetWorldMinute, Error))
            {
                ++PopulationMetrics.CatchUpsLastTick;                PopulationMetrics.CatchUpWorldMinutesAppliedLastTick += TargetWorldMinute - Before;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("RB Routine catch-up failed for '%s': %s"), *AgentId.ToString(), *Error);
            }
            PopulationMetrics.CatchUpMillisecondsLastTick += (FPlatformTime::Seconds() - CatchUpStart) * 1000.0;
        }

        if (Person->RepresentationState == ERBRoutineRepresentationState::LogicalOnly)
        {
            Person->SimulationLOD = DetermineLogicalLOD(*Person);
        }

        switch (Person->SimulationLOD)
        {
        case ERBRoutineSimulationLOD::Full: ++PopulationMetrics.FullEvaluationsLastTick; break;
        case ERBRoutineSimulationLOD::Reduced: ++PopulationMetrics.ReducedEvaluationsLastTick; break;
        case ERBRoutineSimulationLOD::Dormant: ++PopulationMetrics.DormantEvaluationsLastTick; break;
        }

        if (Person->RepresentationState == ERBRoutineRepresentationState::LogicalOnly &&
            Person->SimulationLOD == ERBRoutineSimulationLOD::Full &&
            Person->LastSimulatedWorldMinute >= TargetWorldMinute &&
            PopulationMetrics.PromotionRequestsLastTick < MaxEmbodimentRequestsPerTick &&
            EmbodiedAgents.Num() + PendingEmbodimentRequests.Num() < MaxEmbodiedPopulation)
        {
            FString Error;
            if (RequestEmbodiment(AgentId, Error)) { ++PopulationMetrics.PromotionRequestsLastTick; }
        }        else if (Person->RepresentationState == ERBRoutineRepresentationState::Embodied &&
            Person->SimulationLOD != ERBRoutineSimulationLOD::Full &&
            PopulationMetrics.DemotionRequestsLastTick < MaxDemotionRequestsPerTick)
        {
            FString Error;
            if (RequestDemotion(AgentId, FGameplayTag(), Error)) { ++PopulationMetrics.DemotionRequestsLastTick; }
        }
    }

    RefreshPopulationMetrics();
    PopulationMetrics.SchedulerMillisecondsLastTick = (FPlatformTime::Seconds() - SchedulerStart) * 1000.0;
}

void URBRoutineWorldSubsystem::Tick(float DeltaTime)
{
    (void)DeltaTime;
    ProcessPopulation();
    PopulationMetrics.AgentUpdatesVisitedLastTick = 0;
    PopulationMetrics.DecisionsExecutedLastTick = 0;
    PopulationMetrics.DecisionMillisecondsLastTick = 0.0;

    Agents.RemoveAllSwap([](const TWeakObjectPtr<URBRoutineComponent>& Item)
    {
        return !Item.IsValid();
    }, EAllowShrinking::No);

    for (auto It = NextEvaluationRealTime.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) { It.RemoveCurrent(); }
    }

    if (Agents.IsEmpty())
    {
        RoundRobinCursor = 0;
        return;
    }
    const double Now = FPlatformTime::Seconds();
    const int32 UpdatesToVisit = FMath::Min(MaxAgentUpdatesPerTick, Agents.Num());
    for (int32 Count = 0; Count < UpdatesToVisit; ++Count)
    {
        RoundRobinCursor %= Agents.Num();
        TWeakObjectPtr<URBRoutineComponent> WeakAgent = Agents[RoundRobinCursor++];
        URBRoutineComponent* Agent = WeakAgent.Get();
        if (!Agent || !Agent->IsAuthoritative()) { continue; }
        ++PopulationMetrics.AgentUpdatesVisitedLastTick;

        const ERBRoutineSimulationLOD NewLOD = DetermineLOD(Agent);
        Agent->SetSimulationLOD(NewLOD);
        if (FRBRoutineLogicalPersonRecord* Person = LogicalPeople.Find(Agent->AgentId))
        {
            Person->SimulationLOD = NewLOD;
            Person->RepresentationState = ERBRoutineRepresentationState::Embodied;
            if (const AActor* Owner = Agent->GetOwner())
            {
                Person->Location.WorldLocation = Owner->GetActorLocation();
                Person->Location.bHasWorldLocation = true;
            }
        }

        double& NextTime = NextEvaluationRealTime.FindOrAdd(WeakAgent);
        if (Agent->NeedsEvaluation() || Now >= NextTime)
        {
            const double DecisionStart = FPlatformTime::Seconds();
            Agent->EvaluateNow(ExternalDayIndex, ExternalMinuteOfDay);
            PopulationMetrics.DecisionMillisecondsLastTick += (FPlatformTime::Seconds() - DecisionStart) * 1000.0;
            ++PopulationMetrics.DecisionsExecutedLastTick;
            NextTime = Now + Agent->GetSuggestedDecisionIntervalSeconds();
        }
    }
}
