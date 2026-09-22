#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "SoulRealtimeBattleRules.h"
#include "Core/RBAIUtilityCore.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    struct FLabOutcome
    {
        FName Winner;
        int32 Steps = 0;
        int32 PlayerWaves = 0;
        int32 EnemyWaves = 0;
        int32 PeakActive = 0;
        int32 PlayerAttackChoices = 0;
        int32 EnemyAttackChoices = 0;
        bool bDeadlock = false;

        bool operator==(const FLabOutcome& Other) const
        {
            return Winner == Other.Winner && Steps == Other.Steps
                && PlayerWaves == Other.PlayerWaves
                && EnemyWaves == Other.EnemyWaves
                && PeakActive == Other.PeakActive
                && PlayerAttackChoices == Other.PlayerAttackChoices
                && EnemyAttackChoices == Other.EnemyAttackChoices
                && bDeadlock == Other.bDeadlock;
        }
    };

    FSoulRealtimeFormation MakeLabFormation(
        const TCHAR* Id, const TCHAR* Side, int32 Count,
        ESoulRealtimeFormationRole Role, int32 Power, int32 Priority = 0)
    {
        FSoulRealtimeFormation F;
        F.FormationId = Id;
        F.SideId = Side;
        F.UnitId = Id;
        F.StrategicCount = Count;
        F.Role = Role;
        F.PowerPerBody = Power;
        F.ReinforcementPriority = Priority;
        return F;
    }

    FSoulRealtimeBattleState MakeLabBattle(int32 ActiveCap, bool bWaves)
    {
        FSoulRealtimeBattleState Battle;
        Battle.MaxActivePerSide = ActiveCap;
        Battle.MaxWaveSize = 8;
        Battle.ReinforcementTriggerPermille = 700;
        Battle.bReinforcementsEnabled = bWaves;
        Battle.Formations = {
            MakeLabFormation(TEXT("P_Line"), TEXT("P"), 10, ESoulRealtimeFormationRole::Line, 100),
            MakeLabFormation(TEXT("P_Guard"), TEXT("P"), 8, ESoulRealtimeFormationRole::Guard, 105),
            MakeLabFormation(TEXT("P_Spear"), TEXT("P"), 16, ESoulRealtimeFormationRole::Breaker, 115),
            MakeLabFormation(TEXT("P_Heavy"), TEXT("P"), 10, ESoulRealtimeFormationRole::Shock, 150),
            MakeLabFormation(TEXT("P_Archers"), TEXT("P"), 14, ESoulRealtimeFormationRole::Ranged, 110),
            MakeLabFormation(TEXT("P_Support"), TEXT("P"), 5, ESoulRealtimeFormationRole::Support, 145, 50),
            MakeLabFormation(TEXT("P_Hero"), TEXT("P"), 1, ESoulRealtimeFormationRole::Hero, 700, 200),
            MakeLabFormation(TEXT("P_Griffon"), TEXT("P"), 1, ESoulRealtimeFormationRole::Apex, 850, 150),
            MakeLabFormation(TEXT("E_Grunts"), TEXT("E"), 10, ESoulRealtimeFormationRole::Line, 105),
            MakeLabFormation(TEXT("E_Shields"), TEXT("E"), 8, ESoulRealtimeFormationRole::Guard, 120),
            MakeLabFormation(TEXT("E_Brutes"), TEXT("E"), 16, ESoulRealtimeFormationRole::Breaker, 125),
            MakeLabFormation(TEXT("E_Berserkers"), TEXT("E"), 10, ESoulRealtimeFormationRole::Shock, 150),
            MakeLabFormation(TEXT("E_Hunters"), TEXT("E"), 14, ESoulRealtimeFormationRole::Ranged, 110),
            MakeLabFormation(TEXT("E_Shamans"), TEXT("E"), 5, ESoulRealtimeFormationRole::Support, 145, 50),
            MakeLabFormation(TEXT("E_Hero"), TEXT("E"), 1, ESoulRealtimeFormationRole::Hero, 700, 200),
            MakeLabFormation(TEXT("E_Elephant"), TEXT("E"), 1, ESoulRealtimeFormationRole::Apex, 950, 150)
        };
        FSoulRealtimeBattleRules::InitializeDeployment(Battle);
        return Battle;
    }

    std::string ChooseTactic(int32 OwnPower, int32 EnemyPower, int32 OwnReserve)
    {
        using namespace RBAI::Core;
        const double Ratio = static_cast<double>(OwnPower) / FMath::Max(1, EnemyPower);
        const double Reserve = FMath::Clamp(static_cast<double>(OwnReserve) / 40.0, 0.0, 1.0);

        Context ContextRow;
        ContextRow.Id = "battle";
        ContextRow.Signals["advantage"] = Ratio;
        ContextRow.Signals["reserve"] = Reserve;

        Action Attack;
        Attack.Id = "attack";
        Attack.BaseScore = 1.0;
        Attack.Considerations.push_back({"advantage", 0.45, 1.55, CurveType::Linear, 1.0, 1.0});

        Action Hold;
        Hold.Id = "hold";
        Hold.BaseScore = 0.92;
        Hold.Considerations.push_back({"advantage", 0.45, 1.25, CurveType::InverseLinear, 1.0, 1.0});
        Hold.Considerations.push_back({"reserve", 0.0, 1.0, CurveType::Linear, 1.0, 0.2});

        const SelectionResult Result = ChooseAction({Attack, Hold}, {ContextRow});
        return Result.Success ? Result.ActionId : "hold";
    }

    FName PickCasualtyFormation(
        const FSoulRealtimeBattleState& Battle,
        FName SideId,
        FRandomStream& Rng)
    {
        int32 Total = 0;
        for (const FSoulRealtimeFormation& F : Battle.Formations)
        {
            if (F.SideId == SideId)
            {
                Total += FMath::Max(0, F.ActiveCount);
            }
        }
        if (Total <= 0) return NAME_None;

        int32 Ticket = Rng.RandRange(1, Total);
        for (const FSoulRealtimeFormation& F : Battle.Formations)
        {
            if (F.SideId != SideId || F.ActiveCount <= 0) continue;
            Ticket -= F.ActiveCount;
            if (Ticket <= 0) return F.FormationId;
        }
        return NAME_None;
    }

    void ApplySyntheticLosses(
        FSoulRealtimeBattleState& Battle,
        FName SideId,
        int32 Count,
        FRandomStream& Rng)
    {
        for (int32 I = 0; I < Count; ++I)
        {
            const FName FormationId = PickCasualtyFormation(Battle, SideId, Rng);
            if (FormationId.IsNone()) return;
            FSoulRealtimeBattleRules::ApplyCasualties(Battle, FormationId, 1);
        }
    }

    int32 SyntheticCasualties(
        int32 AttackerPower,
        int32 DefenderPower,
        bool bAttackChoice,
        int32 MagicPermille,
        FRandomStream& Rng)
    {
        const int64 Boosted = static_cast<int64>(AttackerPower)
            * (bAttackChoice ? 1100 : 900)
            * FMath::Max(1, MagicPermille);
        const int64 Denom = static_cast<int64>(FMath::Max(1, DefenderPower)) * 1000000;
        const int32 RatioPermille = static_cast<int32>(
            FMath::Clamp<int64>(Boosted * 1000 / Denom, 450, 1800));

        int32 Losses = 1;
        if (RatioPermille >= 1050) ++Losses;
        if (RatioPermille >= 1400) ++Losses;
        if (Rng.RandRange(0, 999) < (RatioPermille / 3)) ++Losses;
        return FMath::Clamp(Losses, 1, 4);
    }

    FLabOutcome RunSyntheticBattle(
        int32 Seed,
        int32 ActiveCap,
        bool bWaves,
        bool bVolcanic)
    {
        FSoulRealtimeBattleState Battle = MakeLabBattle(ActiveCap, bWaves);
        FRandomStream Rng(Seed);
        FLabOutcome Out;
        const FSoulTerrainMagicProfile Volcanic =
            FSoulRealtimeBattleRules::MakeVolcanicMagicProfile();

        for (int32 Step = 1; Step <= 180; ++Step)
        {
            const bool bPlayerAlive = FSoulRealtimeBattleRules::HasLivingForce(Battle, TEXT("P"));
            const bool bEnemyAlive = FSoulRealtimeBattleRules::HasLivingForce(Battle, TEXT("E"));
            if (!bPlayerAlive || !bEnemyAlive)
            {
                Out.Winner = bPlayerAlive ? TEXT("P") : (bEnemyAlive ? TEXT("E") : TEXT("Draw"));
                Out.Steps = Step - 1;
                break;
            }

            const int32 PlayerPower = FSoulRealtimeBattleRules::ActivePower(Battle, TEXT("P"));
            const int32 EnemyPower = FSoulRealtimeBattleRules::ActivePower(Battle, TEXT("E"));
            const std::string PlayerChoice = ChooseTactic(
                PlayerPower, EnemyPower, FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("P")));
            const std::string EnemyChoice = ChooseTactic(
                EnemyPower, PlayerPower, FSoulRealtimeBattleRules::ReserveBodies(Battle, TEXT("E")));
            const bool bPlayerAttack = PlayerChoice == "attack";
            const bool bEnemyAttack = EnemyChoice == "attack";
            Out.PlayerAttackChoices += bPlayerAttack ? 1 : 0;
            Out.EnemyAttackChoices += bEnemyAttack ? 1 : 0;

            const int32 PlayerMagic = bVolcanic
                ? Volcanic.MultiplierFor(TEXT("Magic.School.Fire")) : 1000;
            const int32 EnemyMagic = bVolcanic
                ? Volcanic.MultiplierFor(TEXT("Magic.School.Ice")) : 1000;

            const int32 EnemyLosses = SyntheticCasualties(
                PlayerPower, EnemyPower, bPlayerAttack, PlayerMagic, Rng);
            const int32 PlayerLosses = SyntheticCasualties(
                EnemyPower, PlayerPower, bEnemyAttack, EnemyMagic, Rng);
            ApplySyntheticLosses(Battle, TEXT("E"), EnemyLosses, Rng);
            ApplySyntheticLosses(Battle, TEXT("P"), PlayerLosses, Rng);

            if (FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("P")))
            {
                const FSoulReinforcementWave Wave =
                    FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("P"));
                Out.PlayerWaves += Wave.TotalBodies() > 0 ? 1 : 0;
            }
            if (FSoulRealtimeBattleRules::ShouldReinforce(Battle, TEXT("E")))
            {
                const FSoulReinforcementWave Wave =
                    FSoulRealtimeBattleRules::BuildAndApplyWave(Battle, TEXT("E"));
                Out.EnemyWaves += Wave.TotalBodies() > 0 ? 1 : 0;
            }

            const int32 Active = FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("P"))
                + FSoulRealtimeBattleRules::ActiveBodies(Battle, TEXT("E"));
            Out.PeakActive = FMath::Max(Out.PeakActive, Active);

            if (Step == 180)
            {
                Out.Winner = TEXT("Timeout");
                Out.Steps = Step;
                Out.bDeadlock = true;
            }
        }
        return Out;
    }

    struct FScenarioSummary
    {
        FString Name;
        int32 Seeds = 0;
        int32 PlayerWins = 0;
        int32 EnemyWins = 0;
        int32 Deadlocks = 0;
        double MeanSteps = 0.0;
        double MeanWaves = 0.0;
        int32 MaxPeakActive = 0;
        double PlayerAttackShare = 0.0;
    };

    FScenarioSummary RunScenario(
        const FString& Name,
        int32 SeedBase,
        int32 SeedCount,
        int32 Cap,
        bool bWaves,
        bool bVolcanic)
    {
        FScenarioSummary Summary;
        Summary.Name = Name;
        Summary.Seeds = SeedCount;
        int64 Steps = 0;
        int64 Waves = 0;
        int64 AttackChoices = 0;
        int64 TotalChoices = 0;

        for (int32 I = 0; I < SeedCount; ++I)
        {
            const FLabOutcome O = RunSyntheticBattle(
                SeedBase + I, Cap, bWaves, bVolcanic);
            Summary.PlayerWins += O.Winner == TEXT("P") ? 1 : 0;
            Summary.EnemyWins += O.Winner == TEXT("E") ? 1 : 0;
            Summary.Deadlocks += O.bDeadlock ? 1 : 0;
            Steps += O.Steps;
            Waves += O.PlayerWaves + O.EnemyWaves;
            Summary.MaxPeakActive = FMath::Max(Summary.MaxPeakActive, O.PeakActive);
            AttackChoices += O.PlayerAttackChoices;
            TotalChoices += O.Steps;
        }
        Summary.MeanSteps = SeedCount > 0
            ? static_cast<double>(Steps) / SeedCount : 0.0;
        Summary.MeanWaves = SeedCount > 0
            ? static_cast<double>(Waves) / SeedCount : 0.0;
        Summary.PlayerAttackShare = TotalChoices > 0
            ? static_cast<double>(AttackChoices) / TotalChoices : 0.0;
        return Summary;
    }

    FString JsonEscape(FString Value)
    {
        Value.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
        Value.ReplaceInline(TEXT("\""), TEXT("\\\""));
        return Value;
    }

    bool WriteLabEvidence(const TArray<FScenarioSummary>& Summaries)
    {
        const FString Dir = FPaths::ProjectDir() / TEXT("Evidence/RealtimeBattleLab");
        IFileManager::Get().MakeDirectory(*Dir, true);

        FString Json = TEXT("{\n");
        Json += TEXT("  \"schema\": 1,\n");
        Json += TEXT("  \"calibration_status\": \"UNCALIBRATED_SYNTHETIC\",\n");
        Json += TEXT("  \"purpose\": \"architecture and sensitivity testing, not predicted shipping win rates\",\n");
        Json += TEXT("  \"uses\": [\"SoulRealtimeBattleRules\", \"RBAI utility selection\", \"RB Combat group cap\"],\n");
        Json += TEXT("  \"scenarios\": [\n");
        for (int32 I = 0; I < Summaries.Num(); ++I)
        {
            const FScenarioSummary& S = Summaries[I];
            Json += FString::Printf(
                TEXT("    {\"name\":\"%s\",\"seeds\":%d,\"player_wins\":%d,\"enemy_wins\":%d,")
                TEXT("\"deadlocks\":%d,\"mean_steps\":%.3f,\"mean_waves\":%.3f,")
                TEXT("\"max_peak_active\":%d,\"player_attack_share\":%.5f}%s\n"),
                *JsonEscape(S.Name), S.Seeds, S.PlayerWins, S.EnemyWins,
                S.Deadlocks, S.MeanSteps, S.MeanWaves,
                S.MaxPeakActive, S.PlayerAttackShare,
                I + 1 < Summaries.Num() ? TEXT(",") : TEXT(""));
        }
        Json += TEXT("  ]\n}\n");

        FString Markdown = TEXT("# Soul Real-Time Battle Lab - Synthetic Evidence\n\n");
        Markdown += TEXT("**CALIBRATION: UNCALIBRATED SYNTHETIC.** These results compare architecture and sensitivity only; they are not predicted player win rates or shipping performance.\n\n");
        Markdown += TEXT("The lab exercises the current Soul reinforcement rules, the released RB AI deterministic utility selector, and RB Combat's 20-member group ceiling. Physical navigation, animation, collision, Niagara cost, TCAT, and rendered frame time remain separate qualification gates.\n\n");
        Markdown += TEXT("| Scenario | Seeds | P wins | E wins | Deadlocks | Mean steps | Mean waves | Peak active | P attack share |\n");
        Markdown += TEXT("|---|---:|---:|---:|---:|---:|---:|---:|---:|\n");
        for (const FScenarioSummary& S : Summaries)
        {
            Markdown += FString::Printf(
                TEXT("| %s | %d | %d | %d | %d | %.2f | %.2f | %d | %.1f%% |\n"),
                *S.Name, S.Seeds, S.PlayerWins, S.EnemyWins, S.Deadlocks,
                S.MeanSteps, S.MeanWaves, S.MaxPeakActive,
                S.PlayerAttackShare * 100.0);
        }

        return FFileHelper::SaveStringToFile(
                Json, *(Dir / TEXT("battle_lab_report.json")))
            && FFileHelper::SaveStringToFile(
                Markdown, *(Dir / TEXT("battle_lab_report.md")));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeBattleLabDeterminismTest,
    "Soul.RealtimeBattle.Lab.DeterministicReplication",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeBattleLabDeterminismTest::RunTest(const FString&)
{
    const FLabOutcome A = RunSyntheticBattle(20260921, 32, true, false);
    const FLabOutcome B = RunSyntheticBattle(20260921, 32, true, false);
    TestTrue(TEXT("same seed produces identical synthetic battle outcome"), A == B);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeBattleLabMatrixTest,
    "Soul.RealtimeBattle.Lab.SeededMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeBattleLabMatrixTest::RunTest(const FString&)
{
    constexpr int32 Seeds = 400;
    const int32 SeedBase = 202609210;
    TArray<FScenarioSummary> Summaries;
    Summaries.Add(RunScenario(TEXT("waves_24_active"), SeedBase, Seeds, 24, true, false));
    Summaries.Add(RunScenario(TEXT("waves_32_active"), SeedBase, Seeds, 32, true, false));
    Summaries.Add(RunScenario(TEXT("waves_48_active"), SeedBase, Seeds, 48, true, false));
    Summaries.Add(RunScenario(TEXT("all_at_once"), SeedBase, Seeds, 32, false, false));
    Summaries.Add(RunScenario(TEXT("volcanic_fire_vs_ice_32"), SeedBase, Seeds, 32, true, true));

    for (const FScenarioSummary& S : Summaries)
    {
        TestEqual(*FString::Printf(TEXT("%s has no synthetic deadlocks"), *S.Name), S.Deadlocks, 0);
        TestEqual(*FString::Printf(TEXT("%s accounts for all seeds"), *S.Name),
            S.PlayerWins + S.EnemyWins + S.Deadlocks, S.Seeds);
    }
    TestTrue(TEXT("lab evidence written"), WriteLabEvidence(Summaries));
    return true;
}

#endif
