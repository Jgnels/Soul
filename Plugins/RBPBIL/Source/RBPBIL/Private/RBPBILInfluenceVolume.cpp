#include "RBPBILInfluenceVolume.h"

#include "RBPBILTypes.h"

ARBPBILInfluenceVolume::ARBPBILInfluenceVolume()
{
    ApplyPBILPreset();
}

void ARBPBILInfluenceVolume::ApplyPBILPreset()
{
    CellSize = 150.0f;
    BaseLayerConfigs.Reset();

    TArray<FName> ChannelTags;
    FRBPBILLayers::GetAll(ChannelTags);
    BaseLayerConfigs.Reserve(ChannelTags.Num());

    for (const FName Tag : ChannelTags)
    {
        FTCATBaseLayerConfig& Config = BaseLayerConfigs.AddDefaulted_GetRef();
        Config.BaseLayerTag = Tag;
        Config.ProjectionMask = 0;
    }
    bAdaptivelySwitchRefreshMode = true;
    bRefreshWithGPU = true;
    bEnablePositionPrediction = true;
}
