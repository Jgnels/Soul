#include "SoulCampaignBattleBridge.h"

bool FSoulCampaignBattleDescriptor::IsValid() const
{
    return !EncounterId.IsNone() && !SourceRegion.IsNone() && !TargetRegion.IsNone()
        && SourceRegion != TargetRegion && !PlayerFaction.IsNone() && !EnemyFaction.IsNone()
        && PlayerFaction != EnemyFaction && !PlayerUnitId.IsNone() && !EnemyUnitId.IsNone()
        && !MapPackage.IsNone() && !ReturnMapPackage.IsNone()
        && PlayerStrategicCount > 0 && EnemyStrategicCount > 0
        && ActiveCapPerSide > 0 && ActiveCapPerSide <= 35
        // The installed physical runtime supports this one ordered matchup.
        // Reject unsupported identities instead of silently substituting side-based assets.
        && PlayerFaction == TEXT("humans") && EnemyFaction == TEXT("dwarves")
        && PlayerUnitId == TEXT("human_knight") && EnemyUnitId == TEXT("dwarf_warrior");
}

bool USoulCampaignBattleBridge::BeginEncounter(const FSoulCampaignBattleDescriptor& Descriptor)
{
    if (bPending || !Descriptor.IsValid()) return false;
    Pending = Descriptor;
    bPending = true;
    return true;
}

bool USoulCampaignBattleBridge::ResolveEncounter(const FSoulCampaignBattleResult& Result)
{
    if (!bPending || Result.EncounterId != Pending.EncounterId
        || Result.TargetRegion != Pending.TargetRegion
        || Result.PlayerSurvivors < 0 || Result.PlayerSurvivors > Pending.PlayerStrategicCount
        || Result.EnemySurvivors < 0 || Result.EnemySurvivors > Pending.EnemyStrategicCount
        || (Result.bPlayerWon && (Result.EnemySurvivors != 0 || Result.PlayerSurvivors == 0))
        || (!Result.bPlayerWon && Result.PlayerSurvivors != 0)) return false;
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
