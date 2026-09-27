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
    // Presentation destination associated with this recipe. Selection remains
    // deterministic and independent of loading/constructing the Unreal world.
    FName MapPackage;
    FVector ArenaOrigin = FVector::ZeroVector;
    TSet<FName> Biomes;
    TSet<FName> Landforms;
    TSet<FName> Features;
    int32 BaseScore = 0;
    bool bSupportsSiege = false;
    // Reference recipes may score, but only admitted bindings may load a world.
    bool bPlayable = false;
};

class SOULCORE_API FSoulBattlefieldRecipeRules
{
public:
    static int32 Score(const FSoulBattleContext& Context, const FSoulBattlefieldTemplate& Template);
    static const FSoulBattlefieldTemplate* SelectPlayable(const FSoulBattleContext& Context, const TArray<FSoulBattlefieldTemplate>& Templates);
    static FName SelectBest(const FSoulBattleContext& Context, const TArray<FSoulBattlefieldTemplate>& Templates);
};
