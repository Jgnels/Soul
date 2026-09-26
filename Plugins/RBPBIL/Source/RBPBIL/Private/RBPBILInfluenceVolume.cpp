#include "RBPBILInfluenceVolume.h"

#include "RBPBILTypes.h"
#include "Components/BoxComponent.h"

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

bool ARBPBILInfluenceVolume::SetRefreshPolicy(ERBPBILRefreshPolicy Policy)
{
    // An active field may have outstanding GPU readbacks. Select policy before registration.
    if (HasActorBegunPlay())
    {
        return false;
    }
    bAdaptivelySwitchRefreshMode = Policy == ERBPBILRefreshPolicy::Adaptive;
    bRefreshWithGPU = Policy != ERBPBILRefreshPolicy::CPU;
    bEnablePositionPrediction = bRefreshWithGPU;
    return true;
}

bool ARBPBILInfluenceVolume::ConfigureRuntimeBounds(FVector HalfExtents, float InCellSize)
{
    if (HasActorBegunPlay() || HalfExtents.ContainsNaN() ||
        HalfExtents.GetMin() <= 0.0f || !FMath::IsFinite(InCellSize) || InCellSize <= 0.0f)
    {
        return false;
    }
    if (!RuntimeBounds)
    {
        RuntimeBounds = NewObject<UBoxComponent>(this, TEXT("PBILRuntimeBounds"));
        AddInstanceComponent(RuntimeBounds);
        RuntimeBounds->SetupAttachment(GetRootComponent());
        RuntimeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RuntimeBounds->SetGenerateOverlapEvents(false);
        RuntimeBounds->SetHiddenInGame(true);
        RuntimeBounds->SetVisibility(false);
        RuntimeBounds->RegisterComponent();
    }
    RuntimeBounds->SetBoxExtent(HalfExtents, false);
    CellSize = InCellSize;
    DrawInfluence = ETCATDebugDrawMode::None;
    bShowInfluenceValues = false;
    UpdateGridSize();
    return true;
}
