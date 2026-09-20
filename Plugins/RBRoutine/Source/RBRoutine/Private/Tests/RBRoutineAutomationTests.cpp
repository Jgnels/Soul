#if WITH_DEV_AUTOMATION_TESTS

#include "Core/RBRoutinePlannerCore.h"
#include "RBRoutineComponent.h"
#include "RBRoutineProfile.h"
#include "RBRoutineNativeTags.h"
#include "RBRoutinePerceptionAdapterComponent.h"
#include "Perception/AISense_Sight.h"
#include "RBRoutineWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

using namespace RBRoutine::Core;

namespace
{
    Condition Eq(const char* Key, FactValue Value)
    {
        return {Key, CompareOp::Equal, std::move(Value)};
    }

    Effect Set(const char* Key, FactValue Value)
    {
        return {Key, std::move(Value)};
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutinePlannerAutomation, "RBRoutine.Core.Planner.CheapestDeterministicPlan", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePlannerAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    WorldState State{{"hungry", true}, {"hasFood", false}};
    GoalSpec Goal{"Fed", {}, {Eq("hungry", false)}, 10.0};
    std::vector<ActionSpec> Actions{
        {"BuyFood", {Eq("hasFood", false)}, {Set("hasFood", true)}, 2.0, true},
        {"Eat", {Eq("hasFood", true)}, {Set("hungry", false)}, 1.0, true},
        {"BegAndEat", {Eq("hungry", true)}, {Set("hungry", false)}, 8.0, true}
    };
    const PlanResult Plan = BuildPlan(State, Goal, Actions);
    TestTrue(TEXT("Plan succeeds"), Plan.Success);
    TestEqual(TEXT("Two actions"), static_cast<int32>(Plan.ActionIds.size()), 2);
    TestEqual(TEXT("First action"), FString(UTF8_TO_TCHAR(Plan.ActionIds[0].c_str())), FString(TEXT("BuyFood")));
    TestEqual(TEXT("Second action"), FString(UTF8_TO_TCHAR(Plan.ActionIds[1].c_str())), FString(TEXT("Eat")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutineGoalAutomation, "RBRoutine.Core.Goals.SatisfiedAndIneligibleGoalsAreSkipped", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutineGoalAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    WorldState State{{"hungry", false}, {"tired", true}, {"hasAxe", false}};
    std::vector<GoalSpec> Goals{
        {"AlreadyFed", {}, {Eq("hungry", false)}, 1000.0},
        {"Chop", {Eq("hasAxe", true)}, {Eq("tired", false)}, 500.0},
        {"Rest", {}, {Eq("tired", false)}, 10.0}
    };
    std::vector<ActionSpec> Actions{{"Sleep", {Eq("tired", true)}, {Set("tired", false)}, 1.0, true}};
    const PlanResult Plan = ChooseGoalAndPlan(State, Goals, Actions);
    TestTrue(TEXT("Fallback goal plans"), Plan.Success);
    TestEqual(TEXT("Rest selected"), FString(UTF8_TO_TCHAR(Plan.GoalId.c_str())), FString(TEXT("Rest")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutineScheduleAutomation, "RBRoutine.Core.Schedule.CrossMidnight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutineScheduleAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    ScheduleEntry Sleep{"Sleep", 0x7F, 1320, 360, "SleepGoal", 20.0};
    TestTrue(TEXT("23:00 active"), IsScheduleEntryActive(Sleep, 1, 1380));
    TestTrue(TEXT("00:30 active"), IsScheduleEntryActive(Sleep, 2, 30));
    TestFalse(TEXT("12:00 inactive"), IsScheduleEntryActive(Sleep, 2, 720));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutineContinuityAutomation, "RBRoutine.Core.Continuity.BoundedMemoryAndRelationshipClamp", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutineContinuityAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    ContinuityState State;
    State.AgentId = "Erik";
    State.MemoryCapacity = 2;
    TestTrue(TEXT("Memory 1"), RecordMemory(State, {"e1", "Met", "Bjorn", 0.1, 0.0, 10}));
    TestTrue(TEXT("Memory 2"), RecordMemory(State, {"e2", "Raid", "Bjorn", 0.9, -0.8, 20}));
    TestTrue(TEXT("Memory 3"), RecordMemory(State, {"e3", "Meal", "", 0.5, 0.2, 30}));
    TestEqual(TEXT("Bounded count"), static_cast<int32>(State.Memories.size()), 2);
    ApplyRelationshipDelta(State, "Bjorn", 2.0, -2.0, 2.0, 40);
    const RelationshipState& Relationship = State.Relationships.at("Bjorn");
    TestEqual(TEXT("Affinity clamped"), Relationship.Affinity, 1.0);
    TestEqual(TEXT("Trust clamped"), Relationship.Trust, -1.0);
    TestEqual(TEXT("Fear clamped"), Relationship.Fear, 1.0);
    return true;
}

namespace
{
    FRBRoutineFactCondition UEEq(FName Key, const FRBRoutineFactValue& Value)
    {
        FRBRoutineFactCondition Result;
        Result.Key = Key;
        Result.Operator = ERBRoutineCompareOp::Equal;
        Result.Value = Value;
        return Result;
    }

    FRBRoutineFactEffect UESet(FName Key, const FRBRoutineFactValue& Value)
    {
        FRBRoutineFactEffect Result;
        Result.Key = Key;
        Result.Value = Value;
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutineInterruptRuntimeAutomation, "RBRoutine.Runtime.Component.NonInterruptibleDefersForcedGoal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutineInterruptRuntimeAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AActor* Owner = NewObject<AActor>(GetTransientPackage());
    URBRoutineComponent* Component = NewObject<URBRoutineComponent>(Owner);
    URBRoutineProfile* Profile = NewObject<URBRoutineProfile>(Component);
    Component->AgentId = TEXT("Erik");
    Component->Profile = Profile;

    TestTrue(TEXT("Transient owner is authoritative for isolated test"), Owner->HasAuthority());

    FRBRoutineGoalDefinition WorkGoal;
    WorkGoal.GoalId = TEXT("Work");
    WorkGoal.BasePriority = 10.0;
    WorkGoal.Desired.Add(UEEq(TEXT("workDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(WorkGoal);

    FRBRoutineGoalDefinition FleeGoal;
    FleeGoal.GoalId = TEXT("Flee");
    FleeGoal.BasePriority = 0.0;
    FleeGoal.Desired.Add(UEEq(TEXT("safe"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(FleeGoal);

    FRBRoutineActionDefinition WorkAction;
    WorkAction.ActionId = TEXT("WorkAction");
    WorkAction.PredictedEffects.Add(UESet(TEXT("workDone"), FRBRoutineFactValue::FromBool(true)));
    WorkAction.bInterruptible = false;
    Profile->Actions.Add(WorkAction);

    FRBRoutineActionDefinition FleeAction;
    FleeAction.ActionId = TEXT("FleeAction");
    FleeAction.PredictedEffects.Add(UESet(TEXT("safe"), FRBRoutineFactValue::FromBool(true)));
    Profile->Actions.Add(FleeAction);

    Component->SetFactBool(TEXT("workDone"), false);
    Component->SetFactBool(TEXT("safe"), false);
    TestTrue(TEXT("Initial work plan succeeds"), Component->EvaluateNow(0, 600));
    TestEqual(TEXT("Work action starts"), Component->CurrentActionId, FName(TEXT("WorkAction")));

    FRBRoutineInterruptRequest Interrupt;
    Interrupt.Priority = 100;
    Interrupt.ForcedGoalId = TEXT("Flee");
    TestTrue(TEXT("Interrupt accepted"), Component->RequestInterrupt(Interrupt));
    TestEqual(TEXT("Noninterruptible action remains active"), Component->CurrentActionId, FName(TEXT("WorkAction")));

    Component->NotifyActionFinished(true);
    TestEqual(TEXT("Forced goal selected after action finishes"), Component->CurrentGoalId, FName(TEXT("Flee")));
    TestEqual(TEXT("Flee action starts after deferred interrupt"), Component->CurrentActionId, FName(TEXT("FleeAction")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRBRoutineSnapshotRuntimeAutomation, "RBRoutine.Runtime.Snapshot.FailClosedRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutineSnapshotRuntimeAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AActor* Owner = NewObject<AActor>(GetTransientPackage());
    URBRoutineComponent* Component = NewObject<URBRoutineComponent>(Owner);
    Component->AgentId = TEXT("Erik");
    Component->SetFactNumber(TEXT("hunger"), 0.5);

    const FRBRoutineSnapshot Good = Component->CreateSnapshot(1234);
    FRBRoutineSnapshot BadSchema = Good;
    BadSchema.SchemaVersion = 99;
    FString Error;
    TestFalse(TEXT("Unsupported schema rejected"), Component->RestoreSnapshot(BadSchema, Error));
    TestEqual(TEXT("Schema error reason"), Error, FString(TEXT("unsupported_schema")));

    FRBRoutineSnapshot WrongAgent = Good;
    WrongAgent.AgentId = TEXT("Bjorn");
    TestFalse(TEXT("Identity mismatch rejected"), Component->RestoreSnapshot(WrongAgent, Error));
    TestEqual(TEXT("Identity error reason"), Error, FString(TEXT("agent_identity_mismatch")));

    Component->SetFactNumber(TEXT("hunger"), 0.9);
    TestTrue(TEXT("Valid snapshot restores"), Component->RestoreSnapshot(Good, Error));
    FRBRoutineFactValue Restored;
    TestTrue(TEXT("Restored fact exists"), Component->GetFact(TEXT("hunger"), Restored));
    TestEqual(TEXT("Restored fact type"), Restored.Type, ERBRoutineFactType::Number);
    TestEqual(TEXT("Restored numeric value"), Restored.NumberValue, 0.5);
    return true;
}


namespace
{
    UWorld* CreatePopulationTestWorld(const TCHAR* Label)
    {
        const FName WorldName(*FString::Printf(TEXT("RBRoutine_%s_%llu"), Label, FPlatformTime::Cycles64()));
        return UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationScaleAutomation,
    "RBRoutine.Population.Scale.TenThousandLogicalNoActors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationScaleAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("Scale"));
    TestNotNull(TEXT("Isolated population world created"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    Population->MaxPopulationRecordsPerTick = 128;
    Population->MaxCatchUpRecordsPerTick = 64;    Population->MaxEmbodiedPopulation = 100;
    Population->SetExternalTime(0, 720);

    const double CreateStart = FPlatformTime::Seconds();
    for (int32 Index = 0; Index < 10000; ++Index)
    {
        const FName AgentId(*FString::Printf(TEXT("Logical_%05d"), Index));
        FRBRoutineCoarseLocation Location;
        FString Error;
        if (!Population->CreateLogicalPerson(AgentId, nullptr, Location, Error))
        {
            AddError(FString::Printf(TEXT("Create failed at %d: %s"), Index, *Error));
            return false;
        }
    }
    const double CreateMilliseconds = (FPlatformTime::Seconds() - CreateStart) * 1000.0;

    TestEqual(TEXT("Ten thousand logical people exist"), Population->GetLogicalPopulationCount(), 10000);
    TestEqual(TEXT("No logical person requires an actor"), Population->GetEmbodiedPopulationCount(), 0);

    Population->SetExternalTime(1, 0);
    const double TickStart = FPlatformTime::Seconds();
    Population->Tick(0.016f);
    const double TickMilliseconds = (FPlatformTime::Seconds() - TickStart) * 1000.0;
    const FRBRoutinePopulationMetrics Metrics = Population->GetPopulationMetrics();

    TestEqual(TEXT("Scheduler visit budget is exact"), Metrics.RecordsVisitedLastTick, 128);
    TestEqual(TEXT("Catch-up budget is exact"), Metrics.CatchUpsLastTick, 64);    TestEqual(TEXT("No automatic embodiment without a viewer"), Metrics.EmbodiedPeople, 0);
    TestEqual(TEXT("No promotion requests without a viewer"), Metrics.PromotionRequestsLastTick, 0);
    TestTrue(TEXT("Population memory estimate is recorded"), Metrics.EstimatedAllocatedBytes > 0);
    AddInfo(FString::Printf(
        TEXT("RB_POP_SCALE logical=10000 embodied=%d create_ms=%.3f tick_ms=%.3f scheduler_ms=%.3f memory_bytes=%lld visited=%d catchups=%d"),
        Metrics.EmbodiedPeople, CreateMilliseconds, TickMilliseconds, Metrics.SchedulerMillisecondsLastTick,
        Metrics.EstimatedAllocatedBytes, Metrics.RecordsVisitedLastTick, Metrics.CatchUpsLastTick));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationCatchUpAutomation,
    "RBRoutine.Population.CatchUp.IdempotentScheduleAndTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationCatchUpAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("CatchUp"));
    TestNotNull(TEXT("Isolated catch-up world created"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    URBRoutineProfile* Profile = NewObject<URBRoutineProfile>(World);    FRBRoutineScheduleEntry Work;
    Work.EntryId = TEXT("Work");
    Work.DayMask = 127;
    Work.StartMinuteOfDay = 480;
    Work.EndMinuteOfDay = 1020;
    Work.PreferredGoalId = TEXT("WorkGoal");
    Work.PriorityBoost = 20.0;
    Profile->Schedule.Add(Work);

    FRBRoutineScheduleEntry Sleep;
    Sleep.EntryId = TEXT("Sleep");
    Sleep.DayMask = 127;
    Sleep.StartMinuteOfDay = 1320;
    Sleep.EndMinuteOfDay = 360;
    Sleep.PreferredGoalId = TEXT("SleepGoal");
    Sleep.PriorityBoost = 30.0;
    Profile->Schedule.Add(Sleep);

    Population->SetExternalTime(0, 600);
    FRBRoutineCoarseLocation Origin;
    Origin.AreaId = TEXT("Village");
    Origin.AnchorId = TEXT("Home");
    Origin.WorldLocation = FVector(0.0, 0.0, 0.0);
    Origin.bHasWorldLocation = true;
    FString Error;
    TestTrue(TEXT("Logical traveler created"), Population->CreateLogicalPerson(TEXT("Traveler"), Profile, Origin, Error));

    FRBRoutineCoarseLocation Destination;
    Destination.AreaId = TEXT("Fields");    Destination.AnchorId = TEXT("NorthField");
    Destination.WorldLocation = FVector(1200.0, 0.0, 0.0);
    Destination.bHasWorldLocation = true;
    TestTrue(TEXT("Coarse travel starts"), Population->StartCoarseTravel(TEXT("Traveler"), Destination, 120, Error));

    Population->SetExternalTime(0, 660);
    TestTrue(TEXT("Halfway catch-up succeeds"), Population->CatchUpLogicalPerson(TEXT("Traveler"), Error));
    FRBRoutineLogicalPersonRecord Halfway;
    TestTrue(TEXT("Halfway person readable"), Population->GetLogicalPerson(TEXT("Traveler"), Halfway));
    TestEqual(TEXT("Work schedule selected at 11:00"), Halfway.CurrentIntentionGoalId, FName(TEXT("WorkGoal")));
    TestTrue(TEXT("Travel remains active halfway"), Halfway.Travel.bActive);
    TestTrue(TEXT("Halfway location interpolates"), FMath::IsNearlyEqual(Halfway.Location.WorldLocation.X, 600.0));
    const int64 HalfwaySimulatedMinute = Halfway.LastSimulatedWorldMinute;

    TestTrue(TEXT("Repeated same-time catch-up succeeds"), Population->CatchUpLogicalPerson(TEXT("Traveler"), Error));
    FRBRoutineLogicalPersonRecord SameTime;
    Population->GetLogicalPerson(TEXT("Traveler"), SameTime);
    TestEqual(TEXT("Same interval is not applied twice"), SameTime.LastSimulatedWorldMinute, HalfwaySimulatedMinute);
    TestTrue(TEXT("Same-time location is unchanged"), FMath::IsNearlyEqual(SameTime.Location.WorldLocation.X, 600.0));

    Population->SetExternalTime(0, 1380);
    TestTrue(TEXT("Night catch-up succeeds"), Population->CatchUpLogicalPerson(TEXT("Traveler"), Error));
    FRBRoutineLogicalPersonRecord Night;
    Population->GetLogicalPerson(TEXT("Traveler"), Night);
    TestFalse(TEXT("Travel completes analytically"), Night.Travel.bActive);
    TestEqual(TEXT("Destination area committed"), Night.Location.AreaId, FName(TEXT("Fields")));
    TestTrue(TEXT("Destination position committed"), FMath::IsNearlyEqual(Night.Location.WorldLocation.X, 1200.0));    TestEqual(TEXT("Cross-midnight sleep schedule selected"), Night.CurrentIntentionGoalId, FName(TEXT("SleepGoal")));

    Population->SetExternalTime(2, 600);
    const FRBRoutinePopulationSnapshot PendingSnapshot = Population->CreatePopulationSnapshot();
    TestEqual(TEXT("Snapshot has one person"), PendingSnapshot.People.Num(), 1);
    TestTrue(TEXT("Snapshot preserves pending catch-up"), PendingSnapshot.People[0].LastSimulatedWorldMinute < PendingSnapshot.CapturedWorldMinute);
    const int64 PreservedLastMinute = PendingSnapshot.People[0].LastSimulatedWorldMinute;

    TestTrue(TEXT("Population snapshot restore succeeds"), Population->RestorePopulationSnapshot(PendingSnapshot, Error));
    FRBRoutineLogicalPersonRecord RestoredBeforeCatchUp;
    Population->GetLogicalPerson(TEXT("Traveler"), RestoredBeforeCatchUp);
    TestEqual(TEXT("Restore keeps last simulated minute"), RestoredBeforeCatchUp.LastSimulatedWorldMinute, PreservedLastMinute);
    TestTrue(TEXT("Post-restore catch-up succeeds"), Population->CatchUpLogicalPerson(TEXT("Traveler"), Error));
    FRBRoutineLogicalPersonRecord RestoredAfterCatchUp;
    Population->GetLogicalPerson(TEXT("Traveler"), RestoredAfterCatchUp);
    TestEqual(TEXT("Catch-up reaches restored world time"), RestoredAfterCatchUp.LastSimulatedWorldMinute, PendingSnapshot.CapturedWorldMinute);
    TestEqual(TEXT("Restored current schedule is work"), RestoredAfterCatchUp.CurrentIntentionGoalId, FName(TEXT("WorkGoal")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationIdentityAutomation,
    "RBRoutine.Population.Identity.RepeatedBindUnbind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationIdentityAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("Identity"));
    TestNotNull(TEXT("Isolated identity world created"), World);    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    Population->SetExternalTime(0, 600);
    FRBRoutineCoarseLocation Home;
    Home.AreaId = TEXT("Village");
    FString Error;
    TestTrue(TEXT("Stable logical person created"), Population->CreateLogicalPerson(TEXT("Erik"), nullptr, Home, Error));

    AActor* OwnerA = World->SpawnActor<AActor>();
    URBRoutineComponent* AgentA = NewObject<URBRoutineComponent>(OwnerA);
    AgentA->AgentId = TEXT("Erik");
    TestTrue(TEXT("First embodiment binds"), Population->BindEmbodiedAgent(TEXT("Erik"), AgentA, Error));
    TestEqual(TEXT("Exactly one embodiment is bound"), Population->GetEmbodiedPopulationCount(), 1);

    AgentA->SetFactInteger(TEXT("continuityCounter"), 7);
    FRBRoutineMemoryRecord Memory;
    Memory.EventId = TEXT("met_bjorn");
    Memory.OtherAgentId = TEXT("Bjorn");
    Memory.Salience = 0.8;
    Memory.WorldMinute = Population->GetExternalWorldMinute();
    TestTrue(TEXT("Bounded memory records while embodied"), AgentA->RecordMemoryEvent(Memory, 0.25, 0.1, 0.0));

    AActor* DuplicateOwner = World->SpawnActor<AActor>();
    URBRoutineComponent* DuplicateAgent = NewObject<URBRoutineComponent>(DuplicateOwner);
    DuplicateAgent->AgentId = TEXT("Erik");    TestFalse(TEXT("Duplicate live identity is rejected"), Population->BindEmbodiedAgent(TEXT("Erik"), DuplicateAgent, Error));
    TestEqual(TEXT("Duplicate identity error is explicit"), Error, FString(TEXT("duplicate_embodied_identity")));

    TestTrue(TEXT("Idle embodiment can safely dehydrate"), Population->UnbindEmbodiedAgent(AgentA, false, Error));
    TestEqual(TEXT("Person remains logical after demotion"), Population->GetLogicalPopulationCount(), 1);
    TestEqual(TEXT("No embodiment remains after demotion"), Population->GetEmbodiedPopulationCount(), 0);

    AActor* OwnerB = World->SpawnActor<AActor>();
    URBRoutineComponent* AgentB = NewObject<URBRoutineComponent>(OwnerB);
    AgentB->AgentId = TEXT("Erik");
    TestTrue(TEXT("Same stable person re-embodies"), Population->BindEmbodiedAgent(TEXT("Erik"), AgentB, Error));
    FRBRoutineFactValue Counter;
    TestTrue(TEXT("Routine fact survives re-embodiment"), AgentB->GetFact(TEXT("continuityCounter"), Counter));
    TestEqual(TEXT("Routine fact value survives re-embodiment"), Counter.IntegerValue, static_cast<int64>(7));
    FRBRoutineRelationshipState Relationship;
    TestTrue(TEXT("Relationship survives re-embodiment"), AgentB->GetRelationship(TEXT("Bjorn"), Relationship));
    TestTrue(TEXT("Affinity survives re-embodiment"), FMath::IsNearlyEqual(Relationship.Affinity, 0.25));
    TestTrue(TEXT("Second demotion succeeds"), Population->UnbindEmbodiedAgent(AgentB, false, Error));

    TestTrue(TEXT("Previously rejected component can bind after release"), Population->BindEmbodiedAgent(TEXT("Erik"), DuplicateAgent, Error));
    TestEqual(TEXT("Identity is still singular"), Population->GetEmbodiedPopulationCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationSnapshotAutomation,
    "RBRoutine.Population.Snapshot.FailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)bool FRBRoutinePopulationSnapshotAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("Snapshot"));
    TestNotNull(TEXT("Isolated snapshot world created"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    Population->SetExternalTime(3, 300);
    FRBRoutineCoarseLocation Location;
    Location.AreaId = TEXT("Harbor");
    FString Error;
    TestTrue(TEXT("Snapshot person created"), Population->CreateLogicalPerson(TEXT("Astrid"), nullptr, Location, Error));
    const FRBRoutinePopulationSnapshot Good = Population->CreatePopulationSnapshot();
    TestEqual(TEXT("Good snapshot has one person"), Good.People.Num(), 1);

    FRBRoutinePopulationSnapshot Duplicate = Good;
    Duplicate.People.Add(Good.People[0]);
    TestFalse(TEXT("Duplicate population identity is rejected"), Population->RestorePopulationSnapshot(Duplicate, Error));
    TestEqual(TEXT("Duplicate error is explicit"), Error, FString(TEXT("duplicate_agent_id")));
    TestEqual(TEXT("Failed restore leaves live state untouched"), Population->GetLogicalPopulationCount(), 1);

    FRBRoutinePopulationSnapshot BadSchema = Good;
    BadSchema.SchemaVersion = 99;
    TestFalse(TEXT("Unknown population schema is rejected"), Population->RestorePopulationSnapshot(BadSchema, Error));
    TestEqual(TEXT("Schema error is explicit"), Error, FString(TEXT("unsupported_population_schema")));
    TestEqual(TEXT("Second failed restore still leaves state untouched"), Population->GetLogicalPopulationCount(), 1);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationContractsAutomation,
    "RBRoutine.Population.Contracts.SocialItemDialogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationContractsAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("Contracts"));
    TestNotNull(TEXT("Isolated contracts world created"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    Population->SetExternalTime(0, 720);
    FRBRoutineCoarseLocation Village;
    Village.AreaId = TEXT("Village");
    FString Error;
    TestTrue(TEXT("Astrid exists"), Population->CreateLogicalPerson(TEXT("Astrid"), nullptr, Village, Error));
    TestTrue(TEXT("Bjorn exists"), Population->CreateLogicalPerson(TEXT("Bjorn"), nullptr, Village, Error));
    TestTrue(TEXT("Erik exists"), Population->CreateLogicalPerson(TEXT("Erik"), nullptr, Village, Error));

    FRBRoutineSocialReservation Reservation;
    TestTrue(TEXT("Pair can reserve social activity"), Population->BeginPairedSocialReservation(
        TEXT("Astrid"), TEXT("Bjorn"), TAG_RBRoutine_Activity_Social, TEXT("LonghouseTable"), Reservation, Error));
    TestTrue(TEXT("Reservation id is valid"), Reservation.ReservationId.IsValid());    FRBRoutineSocialReservation DuplicateReservation;
    TestFalse(TEXT("Reserved participant cannot double book"), Population->BeginPairedSocialReservation(
        TEXT("Bjorn"), TEXT("Erik"), TAG_RBRoutine_Activity_Social, TEXT("Dock"), DuplicateReservation, Error));
    TestEqual(TEXT("Double booking failure is explicit"), Error, FString(TEXT("agent_already_reserved")));

    FRBRoutineSocialReservation AstridReservation;
    TestTrue(TEXT("Astrid reservation is queryable"), Population->GetSocialReservationForAgent(TEXT("Astrid"), AstridReservation));
    TestEqual(TEXT("Both participants share reservation"), AstridReservation.ReservationId, Reservation.ReservationId);

    AActor* AstridOwner = World->SpawnActor<AActor>();
    URBRoutineComponent* AstridAgent = NewObject<URBRoutineComponent>(AstridOwner);
    AstridAgent->AgentId = TEXT("Astrid");
    TestTrue(TEXT("Astrid can embody while socially reserved"), Population->BindEmbodiedAgent(TEXT("Astrid"), AstridAgent, Error));
    TestTrue(TEXT("Unexpected/unload-safe demotion releases paired reservation"), Population->UnbindEmbodiedAgent(AstridAgent, false, Error));
    TestFalse(TEXT("Astrid reservation cleared"), Population->GetSocialReservationForAgent(TEXT("Astrid"), AstridReservation));
    FRBRoutineSocialReservation BjornReservation;
    TestFalse(TEXT("Bjorn reservation cleared with partner"), Population->GetSocialReservationForAgent(TEXT("Bjorn"), BjornReservation));

    FRBRoutineItemAuthorityRequest ItemRequest;
    TestTrue(TEXT("Routine emits opaque item-authority request"), Population->RequestItemAuthorityAction(
        TEXT("Astrid"), ERBRoutineItemIntent::Equip, TEXT("42"), TAG_RBRoutine_Activity_Social, ItemRequest, Error));
    TestTrue(TEXT("Item request has stable request id"), ItemRequest.RequestId.IsValid());
    TestEqual(TEXT("Opaque stable item ref is preserved"), ItemRequest.StableItemRef, FString(TEXT("42")));
    FRBRoutineItemAuthorityResult ItemResult;
    ItemResult.RequestId = ItemRequest.RequestId;
    ItemResult.bSucceeded = true;
    ItemResult.ResultCode = TEXT("equipped");
    FRBRoutineFactEffect EquippedFact;
    EquippedFact.Key = TEXT("hasEquippedTool");
    EquippedFact.Value = FRBRoutineFactValue::FromBool(true);
    ItemResult.RoutineFactUpdates.Add(EquippedFact);
    TestTrue(TEXT("Authoritative item result resolves"), Population->ResolveItemAuthorityAction(ItemResult, Error));
    FRBRoutineLogicalPersonRecord Astrid;
    Population->GetLogicalPerson(TEXT("Astrid"), Astrid);
    const FRBRoutineFactValue* Equipped = Astrid.RoutineState.Facts.Find(TEXT("hasEquippedTool"));
    TestNotNull(TEXT("Authoritative result can update Routine-owned fact"), Equipped);
    if (Equipped) { TestTrue(TEXT("Routine-owned equipped fact is true"), Equipped->BoolValue); }

    FRBRoutineDialogueOutcome Outcome;
    Outcome.AgentId = TEXT("Astrid");
    Outcome.OtherAgentId = TEXT("Bjorn");
    Outcome.Memory.EventId = TEXT("social_bjorn_001");
    Outcome.Memory.OtherAgentId = TEXT("Bjorn");
    Outcome.Memory.Salience = 0.9;
    Outcome.Memory.Valence = 0.6;
    Outcome.Memory.WorldMinute = Population->GetExternalWorldMinute();
    Outcome.AffinityDelta = 0.25;
    Outcome.TrustDelta = 0.1;
    TestTrue(TEXT("External dialogue outcome applies without generating dialogue"), Population->ApplyDialogueOutcome(Outcome, Error));
    FRBRoutineDialogueContext Context;
    TestTrue(TEXT("Dialogue context can be built"), Population->BuildDialogueContext(TEXT("Astrid"), TEXT("Bjorn"), Context, Error));
    TestTrue(TEXT("Dialogue context exposes relationship"), Context.bHasRelationship);
    if (Context.bHasRelationship)
    {
        TestTrue(TEXT("Dialogue relationship affinity is authoritative Routine continuity"), FMath::IsNearlyEqual(Context.Relationship.Affinity, 0.25));
        TestTrue(TEXT("Dialogue relationship trust is authoritative Routine continuity"), FMath::IsNearlyEqual(Context.Relationship.Trust, 0.1));
    }
    TestEqual(TEXT("Dialogue context returns one relevant bounded memory"), Context.RelevantMemories.Num(), 1);
    if (!Context.RelevantMemories.IsEmpty())
    {
        TestEqual(TEXT("Dialogue context contains the social memory"), Context.RelevantMemories[0].EventId, FName(TEXT("social_bjorn_001")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePerceptionAdapterAutomation,
    "RBRoutine.Population.Perception.AIPerceptionInterrupt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePerceptionAdapterAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = CreatePopulationTestWorld(TEXT("Perception"));
    TestNotNull(TEXT("Perception world created"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }
    Population->SetExternalTime(0, 720);

    FRBRoutineCoarseLocation Location;
    Location.AreaId = TEXT("Village");
    FString Error;
    TestTrue(TEXT("Guard logical person exists"), Population->CreateLogicalPerson(TEXT("Guard"), nullptr, Location, Error));

    AActor* GuardOwner = World->SpawnActor<AActor>();
    URBRoutineComponent* Guard = NewObject<URBRoutineComponent>(GuardOwner);
    URBRoutineProfile* Profile = NewObject<URBRoutineProfile>(Guard);
    Guard->AgentId = TEXT("Guard");
    Guard->Profile = Profile;

    FRBRoutineGoalDefinition FleeGoal;
    FleeGoal.GoalId = TEXT("Flee");
    FleeGoal.BasePriority = 1.0;
    FleeGoal.Desired.Add(UEEq(TEXT("safe"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(FleeGoal);
    FRBRoutineActionDefinition FleeAction;
    FleeAction.ActionId = TEXT("FleeAction");
    FleeAction.PredictedEffects.Add(UESet(TEXT("safe"), FRBRoutineFactValue::FromBool(true)));
    Profile->Actions.Add(FleeAction);
    Guard->SetFactBool(TEXT("safe"), false);
    TestTrue(TEXT("Guard embodiment binds"), Population->BindEmbodiedAgent(TEXT("Guard"), Guard, Error));

    URBRoutinePerceptionAdapterComponent* Adapter = NewObject<URBRoutinePerceptionAdapterComponent>(GuardOwner);
    Adapter->ThreatForcedGoalId = TEXT("Flee");
    Adapter->Configure(nullptr, Guard);

    AActor* Raider = World->SpawnActor<AActor>();
    Raider->Tags.Add(TEXT("Threat"));
    const UAISense_Sight* Sight = GetDefault<UAISense_Sight>();
    FAIStimulus Stimulus(*Sight, 1.0f, Raider->GetActorLocation(), GuardOwner->GetActorLocation(), FAIStimulus::SensingSucceeded);
    TestTrue(TEXT("Sight threat forwards into Routine"), Adapter->ForwardStimulus(Raider, Stimulus, Error));
    TestEqual(TEXT("Perception interrupt selects flee goal"), Guard->CurrentGoalId, FName(TEXT("Flee")));
    TestEqual(TEXT("Perception interrupt starts flee action"), Guard->CurrentActionId, FName(TEXT("FleeAction")));

    const FRBRoutineSnapshot Snapshot = Guard->CreateSnapshot(Population->GetExternalWorldMinute());
    TestEqual(TEXT("Perception records one bounded memory"), Snapshot.Memories.Num(), 1);
    if (!Snapshot.Memories.IsEmpty())
    {
        TestTrue(TEXT("Threat memory uses perception threat tag"), Snapshot.Memories[0].EventTag == FGameplayTag(TAG_RBRoutine_Event_Perception_Threat));
    }
    return true;
}
#endif
