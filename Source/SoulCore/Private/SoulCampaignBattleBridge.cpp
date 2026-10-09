#include "SoulCampaignBattleBridge.h"

bool FSoulCampaignBattleDescriptor::IsValid() const
{
    return !EncounterId.IsNone() && !SourceRegion.IsNone() && !TargetRegion.IsNone()
        && SourceRegion != TargetRegion && !PlayerFaction.IsNone() && !EnemyFaction.IsNone()
        && PlayerFaction != EnemyFaction && !PlayerUnitId.IsNone() && !EnemyUnitId.IsNone()
        && !MapPackage.IsNone() && !ReturnMapPackage.IsNone()
        && PlayerStrategicCount > 0 && EnemyStrategicCount > 0
        && ActiveCapPerSide > 0 && ActiveCapPerSide <= 35
        && PlayerMana >= 0
        // Each admitted pair has an exact owned physical roster. A faction name
        // alone cannot select a visually unrelated side-based fallback.
        && PlayerFaction == TEXT("humans") && PlayerUnitId == TEXT("human_knight")
        && ((EnemyFaction == TEXT("dwarves") && EnemyUnitId == TEXT("dwarf_warrior"))
            || (EnemyFaction == TEXT("orcs") && EnemyUnitId == TEXT("orc_hammer_warrior"))
            || (EnemyFaction == TEXT("vikings") && EnemyUnitId == TEXT("viking_axe_warrior")));
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
