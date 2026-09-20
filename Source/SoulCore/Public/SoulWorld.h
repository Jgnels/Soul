#pragma once

#include "CoreMinimal.h"
#include "SoulBattlefieldRecipe.h"

struct FSoulRegionState
{
    FName Id;
    FName OwnerFactionId;
    FName Biome;
    FName Landform;
    FName Feature;
    TArray<FName> Neighbors;
    TSet<FName> RoadNeighbors;
    TMap<FName, FName> ApproachFromNeighbor;
    bool bSettlement = false;
};

struct FSoulFactionKnowledge
{
    TSet<FName> ExploredRegions;
    TSet<FName> VisibleRegions;
};

struct FSoulWorldState
{
    TMap<FName, FSoulRegionState> Regions;
    TMap<FName, FSoulFactionKnowledge> KnowledgeByFaction;
};

class SOULCORE_API FSoulWorldRules
{
public:
    static bool CanMove(const FSoulWorldState& World, FName FromRegion, FName ToRegion);
    static void RefreshVision(FSoulWorldState& World, FName FactionId, FName CenterRegion);
    static bool IsExplored(const FSoulWorldState& World, FName FactionId, FName RegionId);
    static bool IsVisible(const FSoulWorldState& World, FName FactionId, FName RegionId);
    static FName Capture(FSoulWorldState& World, FName RegionId, FName NewOwner);
    static FSoulBattleContext BuildBattleContext(const FSoulWorldState& World, FName AttackerOrigin, FName DefenderRegion, FName Weather, FName TimeOfDay, bool bSiege);
};
