#include "SoulVeterancy.h"

namespace
{
    constexpr int32 RankThresholds[] = {0, 100, 300, 700, 1500};
}

ESoulRegimentRank FSoulVeterancy::RankForExperience(int32 Experience)
{
    for (int32 Index = 4; Index > 0; --Index)
        if (Experience >= RankThresholds[Index]) return static_cast<ESoulRegimentRank>(Index);
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

FString FSoulVeterancy::ProgressSummary(int32 Experience)
{
    Experience = FMath::Max(0, Experience);
    const auto Rank = RankForExperience(Experience);
    const int32 Index = static_cast<int32>(Rank);
    if (Rank == ESoulRegimentRank::Legendary)
        return FString::Printf(TEXT("%s | %d XP | Maximum rank"), *RankId(Rank).ToString(), Experience);
    return FString::Printf(TEXT("%s | %d XP | %d to %s"), *RankId(Rank).ToString(), Experience,
        RankThresholds[Index + 1] - Experience,
        *RankId(static_cast<ESoulRegimentRank>(Index + 1)).ToString());
}

void FSoulVeterancy::AddExperience(FSoulRegimentState& Regiment, int32 Amount)
{
    const int64 Total = static_cast<int64>(FMath::Max(0, Regiment.Experience)) + FMath::Max(0, Amount);
    Regiment.Experience = static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));
    Regiment.Rank = RankForExperience(Regiment.Experience);
}
