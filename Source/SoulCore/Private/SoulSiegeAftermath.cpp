#include "SoulSiegeAftermath.h"

void FSoulSiegeAftermathRules::Apply(FSoulSettlementState& Settlement, const FSoulSiegeAftermath& Aftermath)
{
    FName WallScar = NAME_None;
    if (!Aftermath.ScarIds.IsEmpty())
    {
        TArray<FName> Sorted = Aftermath.ScarIds.Array();
        Sorted.Sort(FNameLexicalLess());
        WallScar = Sorted[0];
    }

    if (Aftermath.WallDamagePermille > 0)
    {
        FSoulSettlementRules::DamageWalls(Settlement, Aftermath.WallDamagePermille, WallScar);
    }

    for (const TPair<FName, int32>& Damage : Aftermath.BuildingDamagePermille)
    {
        FSoulSettlementRules::DamageBuilding(
            Settlement,
            Damage.Key,
            Damage.Value,
            FName(*FString::Printf(TEXT("siege_%s"), *Damage.Key.ToString()))
        );
    }

    for (FName ScarId : Aftermath.ScarIds)
    {
        Settlement.PermanentScars.Add(ScarId);
    }
}
