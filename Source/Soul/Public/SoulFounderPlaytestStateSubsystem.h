#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBSaveDomainProvider.h"
#include "SoulCampaign.h"
#include "SoulCampaignBattleBridge.h"
#include "SoulHero.h"
#include "SoulMemory.h"
#include "SoulWorld.h"
#include "SoulFounderPlaytestStateSubsystem.generated.h"

UCLASS()
class SOUL_API USoulFounderPlaytestStateSubsystem : public UGameInstanceSubsystem, public IRBSaveDomainProvider
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void InitializeScenario();
    bool MovePlayerTo(FName TargetRegion);
    bool IsHostile(FName Region) const;
    bool HasHostileGarrison(FName Region) const;
    bool BuildBattleDescriptor(FName TargetRegion, FSoulCampaignBattleDescriptor& Out, FString& Error) const;
    bool BeginBattle(FName TargetRegion, int32 QualificationActiveCap = 0);
    bool ApplyBattleResult(const FSoulCampaignBattleResult& Result);
    bool HasPendingBattle() const { return !PendingBattle.EncounterId.IsNone(); }
    void AdvanceDay();
    void AdvanceEnemyAI();
    bool Recruit(FName UnitId);
    bool ChooseSkill(FName SkillId);
    bool HireTavernHero();
    void SaveCampaign();
    void LoadCampaign();
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& Out, FString& Error) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& In, FString& Error) override;

    bool bInitialized = false, bSecondHeroHired = false, bBattleWon = false;
    bool bPersistenceBusy = false, bLastSaveSucceeded = false, bLastLoadSucceeded = false;
    uint32 CampaignLoadRevision = 0; // Presentation notification; never saved as game truth.
    FSoulWorldState World;
    FSoulCommanderState EnemyCommander;
    FString LastAIReport, LastPersistenceReport;
    FSoulCampaignEconomy Economy;
    FSoulHeroState Hero;
    FName PlayerRegion, EnemyRegion;
    FName PlayerFaction = TEXT("humans"), EnemyFaction = TEXT("dwarves");
    FName PlayerUnitId = TEXT("human_knight"), EnemyUnitId = TEXT("dwarf_warrior");
    FName CampaignMap, BattleMap;
    FVector BattleOrigin = FVector::ZeroVector;
    TArray<FSoulBattlefieldTemplate> BattlefieldTemplates;
    int32 ActiveCapPerSide = 5, EncounterOrdinal = 0;
    TMap<FName, int32> PlayerArmy, EnemyArmies;
    TMap<FName, FString> RegionDisplayNames;
    TSet<FName> RewardedRegions, ResolvedEncounters;
    FSoulCampaignBattleDescriptor PendingBattle;
    FSoulCampaignBattleResult LastBattleResult;
    static const TArray<FName>& HumanPlaytestRoster();
    FString BuildSummary() const;
private:
    void HandleBattleResolved(const FSoulCampaignBattleResult& Result);
    UFUNCTION() void OnCampaignSaved(const FRBSaveOperationResult& Result);
    UFUNCTION() void OnCampaignLoaded(const FRBSaveOperationResult& Result);
    TWeakObjectPtr<class URBSaveSubsystem> SaveSubsystem;
    TWeakObjectPtr<USoulCampaignBattleBridge> BattleBridge;
};
