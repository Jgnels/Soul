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
    static void AddExperience(FSoulRegimentState& Regiment, int32 Amount);
};
