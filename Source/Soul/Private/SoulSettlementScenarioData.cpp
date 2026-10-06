#include "SoulSettlementScenarioData.h"

FSoulBuildingDefinition FSoulBuildingDevelopmentSpec::ToDefinition() const
{
    FSoulBuildingDefinition Definition;
    Definition.Id = BuildingId;
    Definition.Category = Category;
    Definition.MaxLevel = MaxLevel;
    Definition.BuildDays = BuildDays;
    Definition.BuildCost = BuildCost;
    Definition.Prerequisites = Prerequisites;
    Definition.UnlockIds = UnlockIds;
    return Definition;
}

const FSoulBuildingDevelopmentSpec* USoulSettlementScenarioData::FindDevelopmentDefinition(FName Id) const
{
    return DevelopmentDefinitions.FindByPredicate([Id](const auto& Definition) { return Definition.BuildingId == Id; });
}

const FSoulBuildingDevelopmentSpec* USoulSettlementScenarioData::FindUniqueServiceDefinition(FName UnlockId) const
{
    const FSoulBuildingDevelopmentSpec* Found = nullptr;
    for (const auto& Definition : DevelopmentDefinitions)
        if (Definition.UnlockIds.Contains(UnlockId))
        {
            if (Found) return nullptr; // A single-building proof must be unambiguous.
            Found = &Definition;
        }
    return Found;
}

bool USoulSettlementScenarioData::ValidateDefinition(FString& OutError) const
{
    auto Reject = [&OutError](const FString& Reason) { OutError = Reason; return false; };
    if (SettlementId.IsNone() || FactionId.IsNone() || RegionId.IsNone())
        return Reject(TEXT("Settlement, faction and region ids are required."));
    if (FortificationLevel < 0 || WallIntegrityPermille < 0 || WallIntegrityPermille > 1000)
        return Reject(TEXT("Invalid initial fortification or wall integrity."));
    TSet<FName> BuildingIds, DefinitionIds;
    for (const auto& Building : Buildings)
    {
        if (Building.BuildingId.IsNone() || BuildingIds.Contains(Building.BuildingId))
            return Reject(TEXT("Initial buildings require unique nonempty ids."));
        if (Building.Level < 0 || (Building.bBuilt && Building.Level == 0)
            || Building.IntegrityPermille < 0 || Building.IntegrityPermille > 1000)
            return Reject(TEXT("Invalid initial building level or integrity."));
        BuildingIds.Add(Building.BuildingId);
    }
    for (const auto& Definition : DevelopmentDefinitions)
    {
        if (Definition.BuildingId.IsNone() || !BuildingIds.Contains(Definition.BuildingId)
            || DefinitionIds.Contains(Definition.BuildingId))
            return Reject(TEXT("Development definitions require unique ids present in the initial buildings."));
        if (Definition.MaxLevel < 1 || Definition.BuildDays < 1)
            return Reject(TEXT("Development maximum level and construction days must be positive."));
        for (const auto& Cost : Definition.BuildCost)
            if (Cost.Key.IsNone() || Cost.Value < 0) return Reject(TEXT("Construction costs require resource ids and nonnegative values."));
        TSet<FName> Seen;
        for (FName Prerequisite : Definition.Prerequisites)
        {
            if (!BuildingIds.Contains(Prerequisite) || Prerequisite == Definition.BuildingId || Seen.Contains(Prerequisite))
                return Reject(TEXT("Development prerequisite is missing, self-referencing or repeated."));
            Seen.Add(Prerequisite);
        }
        Seen.Reset();
        for (FName Unlock : Definition.UnlockIds)
        {
            if (Unlock.IsNone() || Seen.Contains(Unlock)) return Reject(TEXT("Unlock bindings require unique nonempty ids."));
            Seen.Add(Unlock);
        }
        const auto* Initial = Buildings.FindByPredicate([&Definition](const auto& B) { return B.BuildingId == Definition.BuildingId; });
        if (Initial->bBuilt && Initial->Level > Definition.MaxLevel)
            return Reject(TEXT("Initial building exceeds its development maximum level."));
        DefinitionIds.Add(Definition.BuildingId);
    }
    TSet<FName> Visiting, Complete;
    TFunction<bool(FName)> Visit = [&](FName Id)
    {
        if (Complete.Contains(Id)) return true;
        if (Visiting.Contains(Id)) return false;
        Visiting.Add(Id);
        if (const auto* Definition = FindDevelopmentDefinition(Id))
            for (FName Prerequisite : Definition->Prerequisites) if (!Visit(Prerequisite)) return false;
        Visiting.Remove(Id);
        Complete.Add(Id);
        return true;
    };
    for (FName Id : DefinitionIds) if (!Visit(Id)) return Reject(TEXT("Development prerequisites contain a cycle."));
    OutError.Reset();
    return true;
}
