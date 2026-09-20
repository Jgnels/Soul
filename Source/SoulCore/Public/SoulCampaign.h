#pragma once

#include "CoreMinimal.h"

struct FSoulRecruitmentPool
{
    FName UnitId;
    int32 Available = 0;
    int32 WeeklyGrowth = 0;
    int32 Capacity = 0;
    TMap<FName, int32> CostPerUnit;
};

struct FSoulCampaignEconomy
{
    int32 Day = 1;
    int32 MaxActionPoints = 3;
    int32 ActionPoints = 3;
    TMap<FName, int32> Resources;
    TMap<FName, int32> DailyIncome;
    TMap<FName, FSoulRecruitmentPool> RecruitmentPools;
};

class SOULCORE_API FSoulCampaignRules
{
public:
    static bool SpendAction(FSoulCampaignEconomy& Campaign, int32 Cost = 1);
    static void AdvanceDay(FSoulCampaignEconomy& Campaign);
    static bool CanAfford(const FSoulCampaignEconomy& Campaign, const TMap<FName, int32>& Cost, int32 Quantity = 1);
    static bool Recruit(FSoulCampaignEconomy& Campaign, FName UnitId, int32 Quantity);
};
