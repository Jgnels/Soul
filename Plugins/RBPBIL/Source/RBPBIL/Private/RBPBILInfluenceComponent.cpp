#include "RBPBILInfluenceComponent.h"

void URBPBILInfluenceComponent::SetChannelInfluence(
    ERBPBILChannel Channel,
    float Radius,
    float Strength,
    float InfluenceZLimitOffset)
{
    const FName Tag = FRBPBILLayers::ToTag(Channel);
    FTCATInfluenceConfigEntry* Existing = InfluenceLayerMap.FindByPredicate(
        [Tag](const FTCATInfluenceConfigEntry& Entry)
        {
            return Entry.MapTag == Tag;
        });

    FTCATInfluenceConfigEntry& Entry = Existing
        ? *Existing
        : InfluenceLayerMap.AddDefaulted_GetRef();

    Entry.MapTag = Tag;
    Entry.SourceData.InfluenceRadius = FMath::Max(0.0f, Radius);
    Entry.SourceData.Strength = Strength;
    Entry.SourceData.InfluenceZLimitOffset = InfluenceZLimitOffset;
    RebuildSourceMap();
}

bool URBPBILInfluenceComponent::RemoveChannelInfluence(
    ERBPBILChannel Channel)
{
    const FName Tag = FRBPBILLayers::ToTag(Channel);
    const int32 Removed = InfluenceLayerMap.RemoveAll(
        [Tag](const FTCATInfluenceConfigEntry& Entry)
        {
            return Entry.MapTag == Tag;
        });

    if (Removed > 0)
    {
        RebuildSourceMap();
        return true;
    }
    return false;
}
