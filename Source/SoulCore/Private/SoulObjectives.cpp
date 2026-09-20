#include "SoulObjectives.h"

namespace
{
    bool SideHasLiving(const TArray<FSoulRegimentState>& Regiments, FName Side)
    {
        return Regiments.ContainsByPredicate([Side](const FSoulRegimentState& Regiment)
        {
            return Regiment.SideId == Side && Regiment.bAlive && Regiment.Count > 0;
        });
    }

    bool EnemyHasLiving(const TArray<FSoulRegimentState>& Regiments, FName Side)
    {
        return Regiments.ContainsByPredicate([Side](const FSoulRegimentState& Regiment)
        {
            return Regiment.SideId != Side && Regiment.bAlive && Regiment.Count > 0;
        });
    }

    bool SideOccupiesAny(const TArray<FSoulRegimentState>& Regiments, FName Side, const TSet<FSoulHex>& Cells)
    {
        return Regiments.ContainsByPredicate([Side, &Cells](const FSoulRegimentState& Regiment)
        {
            return Regiment.SideId == Side && Regiment.bAlive && Cells.Contains(Regiment.Anchor);
        });
    }

    bool EnemyOccupiesAny(const TArray<FSoulRegimentState>& Regiments, FName Side, const TSet<FSoulHex>& Cells)
    {
        return Regiments.ContainsByPredicate([Side, &Cells](const FSoulRegimentState& Regiment)
        {
            return Regiment.SideId != Side && Regiment.bAlive && Cells.Contains(Regiment.Anchor);
        });
    }
}

FSoulObjectiveEvaluation FSoulObjectiveRules::Evaluate(const FSoulBattleObjective& Objective, const TArray<FSoulRegimentState>& Regiments, int32 CurrentRound)
{
    FSoulObjectiveEvaluation Result;
    Result.Progress = Objective.Progress;

    switch (Objective.Kind)
    {
        case ESoulObjectiveKind::EliminateEnemy:
            Result.bComplete = SideHasLiving(Regiments, Objective.OwnerSide) && !EnemyHasLiving(Regiments, Objective.OwnerSide);
            Result.Reason = Result.bComplete ? TEXT("enemy_eliminated") : TEXT("enemy_still_active");
            break;

        case ESoulObjectiveKind::HoldHexes:
            Result.bComplete = Objective.Progress >= FMath::Max(1, Objective.RequiredRounds);
            Result.Reason = Result.bComplete ? TEXT("control_held") : TEXT("control_incomplete");
            break;

        case ESoulObjectiveKind::CaptureHex:
        case ESoulObjectiveKind::CaptureKeep:
            Result.bComplete = SideOccupiesAny(Regiments, Objective.OwnerSide, Objective.Cells)
                && !EnemyOccupiesAny(Regiments, Objective.OwnerSide, Objective.Cells);
            Result.Reason = Result.bComplete ? TEXT("objective_secured") : TEXT("objective_contested");
            break;

        case ESoulObjectiveKind::ReachExit:
            Result.bComplete = SideOccupiesAny(Regiments, Objective.OwnerSide, Objective.Cells);
            Result.Reason = Result.bComplete ? TEXT("exit_reached") : TEXT("exit_not_reached");
            break;

        case ESoulObjectiveKind::SurviveRounds:
            Result.bComplete = SideHasLiving(Regiments, Objective.OwnerSide)
                && CurrentRound >= FMath::Max(1, Objective.RequiredRounds);
            Result.Reason = Result.bComplete ? TEXT("survival_complete") : TEXT("survival_incomplete");
            break;
    }
    return Result;
}

void FSoulObjectiveRules::EndRound(FSoulBattleObjective& Objective, const TArray<FSoulRegimentState>& Regiments)
{
    if (Objective.Kind != ESoulObjectiveKind::HoldHexes)
    {
        return;
    }

    const bool bOwnerPresent = SideOccupiesAny(Regiments, Objective.OwnerSide, Objective.Cells);
    const bool bEnemyPresent = EnemyOccupiesAny(Regiments, Objective.OwnerSide, Objective.Cells);
    if (bOwnerPresent && !bEnemyPresent)
    {
        ++Objective.Progress;
    }
    else
    {
        Objective.Progress = 0;
    }
}
