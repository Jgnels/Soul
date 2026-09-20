#if WITH_DEV_AUTOMATION_TESTS

#include "RBRoutineComponent.h"
#include "RBRoutineProfile.h"
#include "RBRoutineWorldSubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

namespace
{
UWorld* RBCreateQualificationWorld(const TCHAR* Label)
{
    return UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("RBRoutineQual_%s_%llu"), Label, FPlatformTime::Cycles64())),
        GetTransientPackage());
}

AActor* RBSpawnLocatedActor(UWorld* World, const FVector& Location)
{
    AActor* Actor = World->SpawnActor<AActor>();
    USceneComponent* Root = NewObject<USceneComponent>(Actor);
    Root->RegisterComponent();
    Actor->SetRootComponent(Root);
    Actor->SetActorLocation(Location);
    return Actor;
}
ADefaultPawn* RBCreateViewer(UWorld* World, const FVector& Location)
{
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    World->AddController(Controller);
    ADefaultPawn* Pawn = World->SpawnActor<ADefaultPawn>(Location, FRotator::ZeroRotator);
    Controller->Possess(Pawn);
    return Pawn;
}

FRBRoutineFactCondition RBEqual(FName Key, const FRBRoutineFactValue& Value)
{
    FRBRoutineFactCondition Result;
    Result.Key = Key;
    Result.Operator = ERBRoutineCompareOp::Equal;
    Result.Value = Value;
    return Result;
}

FRBRoutineFactEffect RBSet(FName Key, const FRBRoutineFactValue& Value)
{
    FRBRoutineFactEffect Result;
    Result.Key = Key;
    Result.Value = Value;
    return Result;
}

double RBPercentile(TArray<double> Values, double Quantile)
{
    if (Values.IsEmpty()) { return 0.0; }
    Values.Sort();
    const int32 Index = FMath::Clamp(FMath::RoundToInt((Values.Num() - 1) * Quantile), 0, Values.Num() - 1);
    return Values[Index];
}
URBRoutineProfile* RBCreateBenchmarkProfile(UObject* Outer)
{
    URBRoutineProfile* Profile = NewObject<URBRoutineProfile>(Outer);
    FRBRoutineGoalDefinition Goal;
    Goal.GoalId = TEXT("BenchmarkGoal");
    Goal.BasePriority = 10.0;
    Goal.Desired.Add(RBEqual(TEXT("BenchmarkDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(Goal);

    FRBRoutineActionDefinition Action;
    Action.ActionId = TEXT("BenchmarkAction");
    Action.PredictedEffects.Add(RBSet(TEXT("BenchmarkDone"), FRBRoutineFactValue::FromBool(true)));
    Action.bInterruptible = true;
    Profile->Actions.Add(Action);
    return Profile;
}

void RBLogDistribution(FAutomationTestBase& Test, const TCHAR* Prefix, const TArray<double>& Samples)
{
    double Sum = 0.0;
    double Worst = 0.0;
    for (double Value : Samples) { Sum += Value; Worst = FMath::Max(Worst, Value); }
    const double Mean = Samples.IsEmpty() ? 0.0 : Sum / Samples.Num();
    Test.AddInfo(FString::Printf(TEXT("%s mean=%.4f median=%.4f p95=%.4f p99=%.4f worst=%.4f n=%d"),
        Prefix, Mean, RBPercentile(Samples, 0.50), RBPercentile(Samples, 0.95),
        RBPercentile(Samples, 0.99), Worst, Samples.Num()));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationPromotionAutomation,
    "RBRoutine.Population.Lifecycle.ViewerPromotionDemotionContinuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationPromotionAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = RBCreateQualificationWorld(TEXT("Lifecycle"));
    TestNotNull(TEXT("Lifecycle world exists"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }
    Population->MaxPopulationRecordsPerTick = 16;
    Population->MaxCatchUpRecordsPerTick = 16;
    Population->MaxEmbodimentRequestsPerTick = 4;
    Population->MaxDemotionRequestsPerTick = 4;
    Population->SetExternalTime(0, 600);

    ADefaultPawn* Viewer = RBCreateViewer(World, FVector::ZeroVector);
    FRBRoutineCoarseLocation Location;
    Location.AreaId = TEXT("Village");
    Location.WorldLocation = FVector(250.0, 0.0, 0.0);
    Location.bHasWorldLocation = true;
    FString Error;
    TestTrue(TEXT("Logical person created"), Population->CreateLogicalPerson(TEXT("Erik"), nullptr, Location, Error));
    Population->Tick(0.016f);
    TestEqual(TEXT("Nearby logical person requests embodiment"), Population->GetPopulationMetrics().PromotionRequestsLastTick, 1);
    AActor* FirstOwner = RBSpawnLocatedActor(World, Location.WorldLocation);
    URBRoutineComponent* First = NewObject<URBRoutineComponent>(FirstOwner);
    First->AgentId = TEXT("Erik");
    Population->RegisterAgent(First);
    TestEqual(TEXT("One embodiment bound"), Population->GetEmbodiedPopulationCount(), 1);
    First->SetFactInteger(TEXT("ContinuityMarker"), 77);

    Viewer->SetActorLocation(FVector(100000.0, 0.0, 0.0));
    Population->Tick(0.016f);
    Population->Tick(0.016f);
    TestTrue(TEXT("Far embodied person requests demotion"), Population->GetPopulationMetrics().DemotionRequestsLastTick >= 1);
    Population->UnregisterAgent(First);
    TestEqual(TEXT("Demotion leaves logical person"), Population->GetLogicalPopulationCount(), 1);
    TestEqual(TEXT("Demotion releases embodiment"), Population->GetEmbodiedPopulationCount(), 0);

    Viewer->SetActorLocation(FVector::ZeroVector);
    Population->Tick(0.016f);
    TestTrue(TEXT("Returning viewer requests promotion"), Population->GetPopulationMetrics().PromotionRequestsLastTick >= 1);

    AActor* SecondOwner = RBSpawnLocatedActor(World, Location.WorldLocation);
    URBRoutineComponent* Second = NewObject<URBRoutineComponent>(SecondOwner);
    Second->AgentId = TEXT("Erik");
    Population->RegisterAgent(Second);
    FRBRoutineFactValue Marker;
    TestTrue(TEXT("Continuity marker survives promotion cycle"), Second->GetFact(TEXT("ContinuityMarker"), Marker));
    if (Marker.Type == ERBRoutineFactType::Integer) { TestEqual(TEXT("Continuity marker value preserved"), Marker.IntegerValue, static_cast<int64>(77)); }
    TestEqual(TEXT("Stable identity remains singular"), Population->GetLogicalPopulationCount(), 1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutineScheduleTransitionAutomation,
    "RBRoutine.Population.Schedule.EmbodiedDayNightTransition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBRoutineScheduleTransitionAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    AActor* Owner = NewObject<AActor>(GetTransientPackage());
    URBRoutineComponent* Agent = NewObject<URBRoutineComponent>(Owner);
    URBRoutineProfile* Profile = NewObject<URBRoutineProfile>(Agent);
    Agent->AgentId = TEXT("SchedulePerson");
    Agent->Profile = Profile;

    FRBRoutineGoalDefinition WorkGoal;
    WorkGoal.GoalId = TEXT("Work");
    WorkGoal.BasePriority = 1.0;
    WorkGoal.Desired.Add(RBEqual(TEXT("WorkDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(WorkGoal);
    FRBRoutineGoalDefinition SleepGoal;
    SleepGoal.GoalId = TEXT("Sleep");
    SleepGoal.BasePriority = 1.0;
    SleepGoal.Desired.Add(RBEqual(TEXT("SleepDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Goals.Add(SleepGoal);
    FRBRoutineActionDefinition WorkAction;
    WorkAction.ActionId = TEXT("WorkAction");
    WorkAction.PredictedEffects.Add(RBSet(TEXT("WorkDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Actions.Add(WorkAction);
    FRBRoutineActionDefinition SleepAction;
    SleepAction.ActionId = TEXT("SleepAction");
    SleepAction.PredictedEffects.Add(RBSet(TEXT("SleepDone"), FRBRoutineFactValue::FromBool(true)));
    Profile->Actions.Add(SleepAction);

    FRBRoutineScheduleEntry WorkSchedule;
    WorkSchedule.EntryId = TEXT("DayWork");
    WorkSchedule.DayMask = 127;
    WorkSchedule.StartMinuteOfDay = 480;
    WorkSchedule.EndMinuteOfDay = 1020;
    WorkSchedule.PreferredGoalId = TEXT("Work");
    WorkSchedule.PriorityBoost = 20.0;
    Profile->Schedule.Add(WorkSchedule);
    FRBRoutineScheduleEntry SleepSchedule;
    SleepSchedule.EntryId = TEXT("NightSleep");
    SleepSchedule.DayMask = 127;
    SleepSchedule.StartMinuteOfDay = 1320;
    SleepSchedule.EndMinuteOfDay = 360;
    SleepSchedule.PreferredGoalId = TEXT("Sleep");
    SleepSchedule.PriorityBoost = 30.0;
    Profile->Schedule.Add(SleepSchedule);
    Agent->SetFactBool(TEXT("WorkDone"), false);
    Agent->SetFactBool(TEXT("SleepDone"), false);
    TestTrue(TEXT("Day schedule plans"), Agent->EvaluateNow(0, 600));
    TestEqual(TEXT("Day schedule selects work"), Agent->CurrentGoalId, FName(TEXT("Work")));
    TestEqual(TEXT("Day schedule starts work action"), Agent->CurrentActionId, FName(TEXT("WorkAction")));

    Agent->SetFactInteger(TEXT("TimePulse"), 1);
    TestTrue(TEXT("Night schedule replans"), Agent->EvaluateNow(0, 1380));
    TestEqual(TEXT("Night schedule selects sleep"), Agent->CurrentGoalId, FName(TEXT("Sleep")));
    TestEqual(TEXT("Night schedule starts sleep action"), Agent->CurrentActionId, FName(TEXT("SleepAction")));

    Agent->SetFactInteger(TEXT("TimePulse"), 2);
    TestTrue(TEXT("Cross-midnight schedule remains active"), Agent->EvaluateNow(1, 120));
    TestEqual(TEXT("02:00 remains sleep"), Agent->CurrentGoalId, FName(TEXT("Sleep")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutinePopulationCatchUpBenchmarkAutomation,
    "RBRoutine.Population.Scale.TenThousandCatchUpDistribution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRBRoutinePopulationCatchUpBenchmarkAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UWorld* World = RBCreateQualificationWorld(TEXT("CatchUpBench"));
    TestNotNull(TEXT("Catch-up benchmark world exists"), World);
    if (!World) { return false; }
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
    TestNotNull(TEXT("Population subsystem exists"), Population);
    if (!Population) { return false; }

    Population->MaxPopulationRecordsPerTick = 512;
    Population->MaxCatchUpRecordsPerTick = 512;
    Population->SetExternalTime(0, 0);
    FString Error;
    FRBRoutineCoarseLocation Location;
    for (int32 Index = 0; Index < 10000; ++Index)
    {
        TestTrue(TEXT("Logical person creation remains valid"), Population->CreateLogicalPerson(
            FName(*FString::Printf(TEXT("Catch_%05d"), Index)), nullptr, Location, Error));
    }
    Population->SetExternalTime(30, 0);
    TArray<double> SchedulerSamples;
    TArray<double> CatchUpSamples;
    int32 GuardTicks = 0;
    while (GuardTicks++ < 32)
    {
        Population->Tick(0.016f);
        const FRBRoutinePopulationMetrics Metrics = Population->GetPopulationMetrics();
        SchedulerSamples.Add(Metrics.SchedulerMillisecondsLastTick);
        CatchUpSamples.Add(Metrics.CatchUpMillisecondsLastTick);
        if (Metrics.CatchUpsLastTick == 0 && GuardTicks > 20) { break; }
    }

    const FRBRoutinePopulationSnapshot Snapshot = Population->CreatePopulationSnapshot();
    int32 Stale = 0;
    for (const FRBRoutineLogicalPersonRecord& Person : Snapshot.People)
    {
        if (Person.LastSimulatedWorldMinute != Population->GetExternalWorldMinute()) { ++Stale; }
    }
    TestEqual(TEXT("All 10k people catch up under bounded sweeps"), Stale, 0);
    TestEqual(TEXT("No catch-up creates physical actors"), Population->GetEmbodiedPopulationCount(), 0);
    TestTrue(TEXT("Catch-up finished in bounded tick count"), GuardTicks <= 32);
    RBLogDistribution(*this, TEXT("RB_POP_10K_SCHEDULER_MS"), SchedulerSamples);
    RBLogDistribution(*this, TEXT("RB_POP_10K_CATCHUP_MS"), CatchUpSamples);
    AddInfo(FString::Printf(TEXT("RB_POP_10K_CATCHUP logical=%d memory_bytes=%lld ticks=%d world_minutes=%lld"),
        Population->GetLogicalPopulationCount(), Population->GetPopulationMetrics().EstimatedAllocatedBytes,
        GuardTicks, Population->GetExternalWorldMinute()));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBRoutineEmbodiedScaleBenchmarkAutomation,
    "RBRoutine.Population.Scale.Embodied10_50_100_250",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBRoutineEmbodiedScaleBenchmarkAutomation::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const TArray<int32> Counts = {10, 50, 100, 250};
    for (const int32 Count : Counts)
    {
        UWorld* World = RBCreateQualificationWorld(*FString::Printf(TEXT("Embodied%d"), Count));
        TestNotNull(TEXT("Embodied benchmark world exists"), World);
        if (!World) { return false; }
        URBRoutineWorldSubsystem* Population = World->GetSubsystem<URBRoutineWorldSubsystem>();
        TestNotNull(TEXT("Population subsystem exists"), Population);
        if (!Population) { World->DestroyWorld(false); return false; }
        Population->MaxEmbodiedPopulation = 300;
        Population->MaxPopulationRecordsPerTick = 512;
        Population->MaxCatchUpRecordsPerTick = 512;
        Population->MaxAgentUpdatesPerTick = FMath::Min(100, Count);
        Population->SetExternalTime(0, 720);
        ADefaultPawn* Viewer = RBCreateViewer(World, FVector::ZeroVector);
        (void)Viewer;
        URBRoutineProfile* Profile = RBCreateBenchmarkProfile(World);
        TArray<URBRoutineComponent*> Agents;
        Agents.Reserve(Count);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            const bool bNear = Index < FMath::Min(100, Count);
            const FVector Location = bNear
                ? FVector(500.0 + Index * 10.0, 0.0, 0.0)
                : FVector(30000.0 + Index * 10.0, 0.0, 0.0);
            AActor* Owner = RBSpawnLocatedActor(World, Location);
            URBRoutineComponent* Agent = NewObject<URBRoutineComponent>(Owner);
            Agent->AgentId = FName(*FString::Printf(TEXT("Embodied_%03d_%03d"), Count, Index));
            Agent->Profile = Profile;
            Agent->SetFactBool(TEXT("BenchmarkDone"), false);
            Agent->SetFactInteger(TEXT("Pulse"), 0);
            Population->RegisterAgent(Agent);
            Agents.Add(Agent);
        }
        TestEqual(TEXT("Requested embodied population is bound"), Population->GetEmbodiedPopulationCount(), Count);
        Population->Tick(0.016f);
        Population->Tick(0.016f);

        int32 Full = 0, Reduced = 0, Dormant = 0;
        for (const URBRoutineComponent* Agent : Agents)
        {
            if (Agent->SimulationLOD == ERBRoutineSimulationLOD::Full) { ++Full; }
            else if (Agent->SimulationLOD == ERBRoutineSimulationLOD::Reduced) { ++Reduced; }
            else { ++Dormant; }
        }
        TestTrue(TEXT("Full-fidelity population remains bounded at 100"), Full <= 100);
        TestEqual(TEXT("All embodied people receive an explicit LOD"), Full + Reduced + Dormant, Count);
        TArray<double> TickSamples;
        TArray<double> SchedulerSamples;
        TArray<double> DecisionSamples;
        int32 MaxDecisions = 0;
        for (int32 Sample = 1; Sample <= 20; ++Sample)
        {
            for (URBRoutineComponent* Agent : Agents) { Agent->SetFactInteger(TEXT("Pulse"), Sample); }
            const double Start = FPlatformTime::Seconds();
            Population->Tick(0.016f);
            TickSamples.Add((FPlatformTime::Seconds() - Start) * 1000.0);
            const FRBRoutinePopulationMetrics Metrics = Population->GetPopulationMetrics();
            SchedulerSamples.Add(Metrics.SchedulerMillisecondsLastTick);
            DecisionSamples.Add(Metrics.DecisionMillisecondsLastTick);
            MaxDecisions = FMath::Max(MaxDecisions, Metrics.DecisionsExecutedLastTick);
        }
        TestTrue(TEXT("Expensive decision work is capped at 100 agents per tick"), MaxDecisions <= 100);
        RBLogDistribution(*this, *FString::Printf(TEXT("RB_POP_EMBODIED_%d_TOTAL_MS"), Count), TickSamples);
        RBLogDistribution(*this, *FString::Printf(TEXT("RB_POP_EMBODIED_%d_SCHEDULER_MS"), Count), SchedulerSamples);
        RBLogDistribution(*this, *FString::Printf(TEXT("RB_POP_EMBODIED_%d_DECISION_MS"), Count), DecisionSamples);
        AddInfo(FString::Printf(TEXT("RB_POP_EMBODIED count=%d full=%d reduced=%d dormant=%d max_decisions=%d memory_bytes=%lld"),
            Count, Full, Reduced, Dormant, MaxDecisions, Population->GetPopulationMetrics().EstimatedAllocatedBytes));
        World->DestroyWorld(false);
    }
    return true;
}

#endif
