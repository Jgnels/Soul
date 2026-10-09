#include "SoulCampaignBattleBridge.h"

bool FSoulCampaignBattleDescriptor::SupportsExactPair(FName Attacker, FName AttackerUnit, FName Defender, FName DefenderUnit)
{
    // Explicit ordered admission, not a Cartesian product of available bodies.
    struct FPair { FName A, AU, D, DU; };
    static const FPair Pairs[] = {
        {TEXT("humans"),TEXT("human_knight"),TEXT("dwarves"),TEXT("dwarf_warrior")},
        {TEXT("humans"),TEXT("human_knight"),TEXT("orcs"),TEXT("orc_hammer_warrior")},
        {TEXT("humans"),TEXT("human_knight"),TEXT("vikings"),TEXT("viking_axe_warrior")},
        {TEXT("dwarves"),TEXT("dwarf_warrior"),TEXT("humans"),TEXT("human_knight")},
        {TEXT("orcs"),TEXT("orc_hammer_warrior"),TEXT("humans"),TEXT("human_knight")},
        {TEXT("vikings"),TEXT("viking_axe_warrior"),TEXT("humans"),TEXT("human_knight")},
        {TEXT("humans"),TEXT("human_knight"),TEXT("nature"),TEXT("nature_bear_warrior")},
        {TEXT("nature"),TEXT("nature_bear_warrior"),TEXT("humans"),TEXT("human_knight")},
        {TEXT("dwarves"),TEXT("dwarf_warrior"),TEXT("orcs"),TEXT("orc_hammer_warrior")},
        {TEXT("orcs"),TEXT("orc_hammer_warrior"),TEXT("dwarves"),TEXT("dwarf_warrior")},
        {TEXT("dwarves"),TEXT("dwarf_warrior"),TEXT("vikings"),TEXT("viking_axe_warrior")},
        {TEXT("vikings"),TEXT("viking_axe_warrior"),TEXT("dwarves"),TEXT("dwarf_warrior")},
        {TEXT("orcs"),TEXT("orc_hammer_warrior"),TEXT("vikings"),TEXT("viking_axe_warrior")},
        {TEXT("vikings"),TEXT("viking_axe_warrior"),TEXT("orcs"),TEXT("orc_hammer_warrior")}
    };
    for (const auto& P : Pairs)
        if (P.A==Attacker && P.AU==AttackerUnit && P.D==Defender && P.DU==DefenderUnit) return true;
    return false;
}

bool FSoulCampaignBattleDescriptor::IsValid() const
{
    return !EncounterId.IsNone() && !SourceRegion.IsNone() && !TargetRegion.IsNone()
        && SourceRegion != TargetRegion && !PlayerFaction.IsNone() && !EnemyFaction.IsNone()
        && PlayerFaction != EnemyFaction && !PlayerUnitId.IsNone() && !EnemyUnitId.IsNone()
        && !MapPackage.IsNone() && !ReturnMapPackage.IsNone()
        && PlayerStrategicCount > 0 && EnemyStrategicCount > 0
        && ActiveCapPerSide > 0 && ActiveCapPerSide <= 35
        && PlayerMana >= 0
        && SupportsExactPair(PlayerFaction, PlayerUnitId, EnemyFaction, EnemyUnitId);
}

bool USoulCampaignBattleBridge::BeginEncounter(const FSoulCampaignBattleDescriptor& Descriptor)
{
    if (bPending || !Descriptor.IsValid()) return false;
    Pending = Descriptor;
    bPending = true;
    return true;
}

bool FSoulCampaignBattleResult::IsValidFor(const FSoulCampaignBattleDescriptor& Encounter) const
{
    return Encounter.IsValid() && EncounterId == Encounter.EncounterId
        && TargetRegion == Encounter.TargetRegion
        && PlayerSurvivors >= 0 && PlayerSurvivors <= Encounter.PlayerStrategicCount
        && EnemySurvivors >= 0 && EnemySurvivors <= Encounter.EnemyStrategicCount
        && PlayerReinforcements >= 0 && EnemyReinforcements >= 0 && MagicCasts >= 0
        && PlayerManaRemaining >= 0 && PlayerManaRemaining <= Encounter.PlayerMana
        && (bPlayerWon ? (EnemySurvivors == 0 && PlayerSurvivors > 0) : PlayerSurvivors == 0);
}

bool USoulCampaignBattleBridge::ResolveEncounter(const FSoulCampaignBattleResult& Result)
{
    if (!bPending || !Result.IsValidFor(Pending)) return false;
    Last = Result;
    bHasResult = true;
    bPending = false;
    OnBattleResolved.Broadcast(Last);
    return true;
}

const FSoulCampaignBattleDescriptor* USoulCampaignBattleBridge::GetPendingEncounter() const
{
    return bPending ? &Pending : nullptr;
}

const FSoulCampaignBattleResult* USoulCampaignBattleBridge::GetLastResult() const
{
    return bHasResult ? &Last : nullptr;
}
