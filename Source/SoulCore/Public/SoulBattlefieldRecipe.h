#pragma once

#include "CoreMinimal.h"

struct FSoulBattleContext
{
    FName Biome;
    FName Landform;
    FName Feature;
    FName Weather;
    FName TimeOfDay;
    FName AttackerApproach;
    FName DefenderApproach;
    bool bRoadPresent = false;
    bool bSettlementNearby = false;
    bool bSiege = false;
};

struct FSoulBattlefieldTemplate
{
    FName Id;
    TSet<FName> Biomes;
    TSet<FName> Landforms;
    TSet<FName> Features;
    int32 BaseScore = 0;
    bool bSupportsSiege = false;
};

class SOULCORE_API FSoulBattlefieldRecipeRules
{
public:
    static int32 Score(const FSoulBattleContext& Context, const FSoulBattlefieldTemplate& Template);
    static FName SelectBest(const FSoulBattleContext& Context, const TArray<FSoulBattlefieldTemplate>& Templates);
};
