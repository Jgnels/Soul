#include "SoulVeterancy.h"

ESoulRegimentRank FSoulVeterancy::RankForExperience(int32 Experience)
{
    if (Experience >= 1500) return ESoulRegimentRank::Legendary;
    if (Experience >= 700) return ESoulRegimentRank::Elite;
    if (Experience >= 300) return ESoulRegimentRank::Veteran;
    if (Experience >= 100) return ESoulRegimentRank::Seasoned;
    return ESoulRegimentRank::Recruit;
}

int32 FSoulVeterancy::CombatBonusPermille(ESoulRegimentRank Rank)
{
    switch (Rank)
    {
        case ESoulRegimentRank::Seasoned: return 25;
        case ESoulRegimentRank::Veteran: return 50;
        case ESoulRegimentRank::Elite: return 75;
        case ESoulRegimentRank::Legendary: return 100;
        default: return 0;
    }
}

int32 FSoulVeterancy::MoraleBonus(ESoulRegimentRank Rank)
{
    switch (Rank)
    {
        case ESoulRegimentRank::Veteran: return 1;
        case ESoulRegimentRank::Elite: return 1;
        case ESoulRegimentRank::Legendary: return 2;
        default: return 0;
    }
}

FName FSoulVeterancy::RankId(ESoulRegimentRank Rank)
{
    switch (Rank)
    {
        case ESoulRegimentRank::Seasoned: return TEXT("Seasoned");
        case ESoulRegimentRank::Veteran: return TEXT("Veteran");
        case ESoulRegimentRank::Elite: return TEXT("Elite");
        case ESoulRegimentRank::Legendary: return TEXT("Legendary");
        default: return TEXT("Recruit");
    }
}

void FSoulVeterancy::AddExperience(FSoulRegimentState& Regiment, int32 Amount)
{
    Regiment.Experience = FMath::Max(0, Regiment.Experience + FMath::Max(0, Amount));
    Regiment.Rank = RankForExperience(Regiment.Experience);
}
