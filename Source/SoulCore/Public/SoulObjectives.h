#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

enum class ESoulObjectiveKind : uint8
{
    EliminateEnemy,
    HoldHexes,
    CaptureHex,
    ReachExit,
    SurviveRounds,
    CaptureKeep
};

struct FSoulBattleObjective
{
    FName Id;
    ESoulObjectiveKind Kind = ESoulObjectiveKind::EliminateEnemy;
    FName OwnerSide;
    TSet<FSoulHex> Cells;
    int32 RequiredRounds = 0;
    int32 Progress = 0;
};

struct FSoulObjectiveEvaluation
{
    bool bComplete = false;
    int32 Progress = 0;
    FName Reason;
};

class SOULCORE_API FSoulObjectiveRules
{
public:
    static FSoulObjectiveEvaluation Evaluate(const FSoulBattleObjective& Objective, const TArray<FSoulRegimentState>& Regiments, int32 CurrentRound);
    static void EndRound(FSoulBattleObjective& Objective, const TArray<FSoulRegimentState>& Regiments);
};
