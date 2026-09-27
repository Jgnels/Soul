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
    FName BattlefieldId;
    FSoulBattleContext BattleContext;
    FName MapPackage;
    FName ReturnMapPackage;
    FVector ArenaOrigin = FVector::ZeroVector;
    int32 PlayerStrategicCount = 0;
    int32 EnemyStrategicCount = 0;
    int32 ActiveCapPerSide = 5;
    int32 EncounterOrdinal = 0;
    int32 PlayerMana = 80;
    bool IsValid() const;
};

struct SOULCORE_API FSoulCampaignBattleResult
{
    FName EncounterId;
    FName TargetRegion;
    bool bPlayerWon = false;
    // Includes undeployed reserves: these are the surviving strategic pools.
    int32 PlayerSurvivors = 0;
    int32 EnemySurvivors = 0;
    int32 PlayerReinforcements = 0;
    int32 EnemyReinforcements = 0;
    int32 MagicCasts = 0;
    int32 PlayerManaRemaining = 0;
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
