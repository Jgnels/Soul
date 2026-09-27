#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

class SOULCORE_API FSoulVeterancy
{
public:
    static ESoulRegimentRank RankForExperience(int32 Experience);
    static int32 CombatBonusPermille(ESoulRegimentRank Rank);
    static int32 MoraleBonus(ESoulRegimentRank Rank);
    static FName RankId(ESoulRegimentRank Rank);
    // Unit-card text from the existing five-rank XP authority. Hero level and
    // core roster role (including apex) do not alter regiment rank.
    static FString ProgressSummary(int32 Experience);
    static void AddExperience(FSoulRegimentState& Regiment, int32 Amount);
};
