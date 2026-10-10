#pragma once

#include "CoreMinimal.h"
#include "SoulBattlefieldRecipe.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SoulCampaignBattleBridge.generated.h"

// Transport only: campaign owns commitment/consequences; realtime battle owns combat.
struct SOULCORE_API FSoulCampaignBattleDescriptor
{
    FName EncounterId;
    FName SourceRegion;
    FName TargetRegion;
    FName PlayerFaction;
    FName EnemyFaction;
    FName PlayerUnitId;
    FName EnemyUnitId;
    TMap<FName,int32> PlayerCompanies, EnemyCompanies; // Optional exact roster transport; no new authority.
    static bool ValidCompanies(FName Faction,const TMap<FName,int32>& Companies,int32 Total);
    FName BattlefieldId;
    FSoulBattleContext BattleContext;
    FName MapPackage;
    FName ReturnMapPackage;
    FVector ArenaOrigin = FVector::ZeroVector;
    int32 PlayerStrategicCount = 0;
    int32 EnemyStrategicCount = 0;
    int32 ActiveCapPerSide = 5;
    int32 EncounterOrdinal = 0;
    // Combat accounting remains attacker=0 / defender=1. Input ownership is separate.
    int32 TacticalPlayerSide = 0;
    bool bAutoResolve = false;
    int32 PlayerMana = 80;
    bool bPlayerHeroAvailable=true;
    FName NonPlayerHeroId; // Exact admitted commander; absent means no commander substitution.
    FName NonPlayerHeroFaction;
    bool bRestrictPlayerSpells=false;
    TSet<FName> AllowedPlayerSpells; // Derived immutable hero/development admission, not a second spell authority.
    static bool SupportsExactPair(FName Attacker, FName AttackerUnit, FName Defender, FName DefenderUnit);
    bool IsValid() const;
};

struct SOULCORE_API FSoulCampaignBattleResult
{
    FName EncounterId;
    FName TargetRegion;
    bool bPlayerWon = false;
    // Includes undeployed reserves: these are the surviving strategic pools.
    TMap<FName,int32> PlayerCompanies, EnemyCompanies; // Surviving bodies plus undeployed reserves, by exact unit.
    int32 PlayerSurvivors = 0;
    int32 EnemySurvivors = 0;
    int32 PlayerReinforcements = 0;
    int32 EnemyReinforcements = 0;
    int32 MagicCasts = 0;
    int32 PlayerManaRemaining = 0;
    bool bNonPlayerHeroWounded=false;
    bool bTacticalHeroWounded=false; // Reported from the actual RBCombat-controlled hero, never inferred from troop loss.
    bool IsValidFor(const FSoulCampaignBattleDescriptor& Encounter) const;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FSoulCampaignBattleResolved, const FSoulCampaignBattleResult&);

UCLASS()
class SOULCORE_API USoulCampaignBattleBridge : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    bool BeginEncounter(const FSoulCampaignBattleDescriptor& Descriptor);
    bool ResolveEncounter(const FSoulCampaignBattleResult& Result);
    const FSoulCampaignBattleDescriptor* GetPendingEncounter() const;
    const FSoulCampaignBattleResult* GetLastResult() const;
    FSoulCampaignBattleResolved OnBattleResolved;
private:
    FSoulCampaignBattleDescriptor Pending;
    FSoulCampaignBattleResult Last;
    bool bPending = false;
    bool bHasResult = false;
};
