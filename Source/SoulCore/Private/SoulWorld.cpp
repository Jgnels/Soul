#include "SoulWorld.h"

bool FSoulWorldRules::CanMove(const FSoulWorldState& World, FName FromRegion, FName ToRegion)
{
    const FSoulRegionState* Region = World.Regions.Find(FromRegion);
    return Region && Region->Neighbors.Contains(ToRegion) && World.Regions.Contains(ToRegion);
}

void FSoulWorldRules::RefreshVision(FSoulWorldState& World, FName FactionId, FName CenterRegion)
{
    FSoulFactionKnowledge& Knowledge = World.KnowledgeByFaction.FindOrAdd(FactionId);
    Knowledge.VisibleRegions.Reset();

    const FSoulRegionState* Center = World.Regions.Find(CenterRegion);
    if (!Center) return;

    Knowledge.VisibleRegions.Add(CenterRegion);
    Knowledge.ExploredRegions.Add(CenterRegion);
    for (FName Neighbor : Center->Neighbors)
    {
        if (World.Regions.Contains(Neighbor))
        {
            Knowledge.VisibleRegions.Add(Neighbor);
            Knowledge.ExploredRegions.Add(Neighbor);
        }
    }
}

bool FSoulWorldRules::IsExplored(const FSoulWorldState& World, FName FactionId, FName RegionId)
{
    const FSoulFactionKnowledge* Knowledge = World.KnowledgeByFaction.Find(FactionId);
    return Knowledge && Knowledge->ExploredRegions.Contains(RegionId);
}

bool FSoulWorldRules::IsVisible(const FSoulWorldState& World, FName FactionId, FName RegionId)
{
    const FSoulFactionKnowledge* Knowledge = World.KnowledgeByFaction.Find(FactionId);
    return Knowledge && Knowledge->VisibleRegions.Contains(RegionId);
}

FName FSoulWorldRules::Capture(FSoulWorldState& World, FName RegionId, FName NewOwner)
{
    FSoulRegionState* Region = World.Regions.Find(RegionId);
    if (!Region) return NAME_None;
    const FName Previous = Region->OwnerFactionId;
    Region->OwnerFactionId = NewOwner;
    return Previous;
}

FSoulBattleContext FSoulWorldRules::BuildBattleContext(const FSoulWorldState& World, FName AttackerOrigin, FName DefenderRegion, FName Weather, FName TimeOfDay, bool bSiege)
{
    FSoulBattleContext Context;
    const FSoulRegionState* Defender = World.Regions.Find(DefenderRegion);
    if (!Defender) return Context;

    Context.Biome = Defender->Biome;
    Context.Landform = Defender->Landform;
    Context.Feature = Defender->Feature;
    Context.Weather = Weather;
    Context.TimeOfDay = TimeOfDay;
    Context.AttackerApproach = Defender->ApproachFromNeighbor.FindRef(AttackerOrigin);
    Context.bRoadPresent = Defender->RoadNeighbors.Contains(AttackerOrigin);
    Context.bSettlementNearby = Defender->bSettlement;
    Context.bSiege = bSiege;
    return Context;
}
