#include "SoulSettlementAssetAuthoring.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialLayersFunctions.h"
#include "UObject/Package.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

bool USoulSettlementAssetAuthoring::RebindUnloadedOwnedSublevel(UWorld* World,
    FName SourcePackage, FName OwnedReplacement)
{
#if WITH_EDITOR
    const FString Prefix(TEXT("/Game/Soul/Maps/Settlements/"));
    if (!World || !World->GetOutermost()->GetName().StartsWith(Prefix)
        || !OwnedReplacement.ToString().StartsWith(Prefix)
        || !FPackageName::DoesPackageExist(OwnedReplacement.ToString())) return false;
    ULevelStreaming* Match = nullptr;
    for (auto* Level : World->GetStreamingLevels())
        if (Level && Level->GetWorldAssetPackageFName() == SourcePackage)
        {
            // Never unload a live world or resolve another soft UWorld merely
            // to edit metadata. The caller saves this owned package explicitly.
            if (Match || Level->GetLoadedLevel()) return false;
            Match = Level;
        }
    if (!Match) return false;
    World->Modify();
    Match->Modify();
    Match->SetWorldAssetByPackageName(OwnedReplacement);
    World->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

bool USoulSettlementAssetAuthoring::ReparentOwnedMaterialFunction(UMaterialFunctionInstance* Function,
    UMaterialFunctionInterface* Parent)
{
#if WITH_EDITOR
    const FString Prefix(TEXT("/Game/Soul/Materials/Settlements/"));
    if (!Function || !Parent || Function == Parent
        || !Function->GetOutermost()->GetName().StartsWith(Prefix)
        || !Parent->GetOutermost()->GetName().StartsWith(Prefix)) return false;
    Function->Modify();
    // Keep Parent and the cached Base consistent through UE's native setter.
    Function->SetParent(Parent);
    Function->PostEditChange();
    Function->MarkPackageDirty();
    return Function->GetBaseFunction() && Function->GetBaseFunction()->GetOutermost()->GetName().StartsWith(Prefix);
#else
    return false;
#endif
}

bool USoulSettlementAssetAuthoring::RemapOwnedMaterialLayers(UMaterialInstanceConstant* Material,
    const TMap<UMaterialFunctionInterface*, UMaterialFunctionInterface*>& Replacements)
{
#if WITH_EDITOR
    auto IsOwned = [](const UObject* Asset)
    {
        return Asset && Asset->GetOutermost()->GetName().StartsWith(TEXT("/Game/Soul/Materials/Settlements/"));
    };
    if (!IsOwned(Material) || Replacements.IsEmpty()) return false;
    for (const auto& Pair : Replacements)
        if (!Pair.Key || !IsOwned(Pair.Value)) return false;
    FMaterialLayersFunctions Layers;
    if (!Material->GetMaterialLayers(Layers)) return false;
    bool Changed = false;
    for (auto& Layer : Layers.Layers)
        if (auto* const* Replacement = Replacements.Find(Layer.Get()))
        { Layer = *Replacement; Changed = true; }
    if (!Changed) return false;
    Material->Modify();
    {
        FMaterialInstanceParameterUpdateContext Update(Material);
        Update.SetMaterialLayers(Layers);
        Update.SetForceStaticPermutationUpdate(true);
    }
    Material->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
