#include "SoulCampaign.h"

bool FSoulCampaignRules::SpendAction(FSoulCampaignEconomy& Campaign, int32 Cost)
{
    Cost = FMath::Max(0, Cost);
    if (Campaign.ActionPoints < Cost)
    {
        return false;
    }
    Campaign.ActionPoints -= Cost;
    return true;
}

void FSoulCampaignRules::AdvanceDay(FSoulCampaignEconomy& Campaign)
{
    ++Campaign.Day;
    Campaign.ActionPoints = FMath::Max(0, Campaign.MaxActionPoints);

    for (const TPair<FName, int32>& Income : Campaign.DailyIncome)
    {
        Campaign.Resources.FindOrAdd(Income.Key) += Income.Value;
    }

    if ((Campaign.Day - 1) % 7 == 0)
    {
        for (TPair<FName, FSoulRecruitmentPool>& Pair : Campaign.RecruitmentPools)
        {
            FSoulRecruitmentPool& Pool = Pair.Value;
            Pool.Available = FMath::Min(Pool.Capacity, Pool.Available + FMath::Max(0, Pool.WeeklyGrowth));
        }
    }
}

bool FSoulCampaignRules::CanAfford(const FSoulCampaignEconomy& Campaign, const TMap<FName, int32>& Cost, int32 Quantity)
{
    if (Quantity <= 0) return false;
    for (const TPair<FName, int32>& Pair : Cost)
    {
        const int32 Needed = FMath::Max(0, Pair.Value) * Quantity;
        if (Campaign.Resources.FindRef(Pair.Key) < Needed)
        {
            return false;
        }
    }
    return true;
}

bool FSoulCampaignRules::Recruit(FSoulCampaignEconomy& Campaign, FName UnitId, int32 Quantity)
{
    if (Quantity <= 0) return false;
    FSoulRecruitmentPool* Pool = Campaign.RecruitmentPools.Find(UnitId);
    if (!Pool || Pool->Available < Quantity || !CanAfford(Campaign, Pool->CostPerUnit, Quantity))
    {
        return false;
    }

    for (const TPair<FName, int32>& Pair : Pool->CostPerUnit)
    {
        Campaign.Resources.FindOrAdd(Pair.Key) -= FMath::Max(0, Pair.Value) * Quantity;
    }
    Pool->Available -= Quantity;
    return true;
}
