#pragma once
#include "CoreMinimal.h"
class ASoulCampaignWorldActor;
class AActor;
class USceneComponent;

// Baked presentation data only. Never consulted by campaign rules or saves.
namespace SoulCampaignTerrain
{
    bool Enabled();
    bool Mesa();
    float Scale();
    float RegionScale();
    FVector2D FocusBounds();
    float Height(float X,float Y);
    float RoadSurface(float X,float Y);
    const TMap<FName,FVector>& Locations();
    FVector Road(FName From,FName To,float Alpha);
    const TArray<TArray<FVector>>& WaterLines();
    void Build(ASoulCampaignWorldActor* Owner);
    bool DressRegion(AActor* Owner,FName Region);
    void DressRoad(AActor* Owner,USceneComponent* Road,FName From,FName To);
}
