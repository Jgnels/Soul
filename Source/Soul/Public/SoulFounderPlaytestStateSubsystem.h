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

// Ephemeral proposal only: never serialized and never campaign authority.
struct FSoulControlledCampaignAction
{
    FName FactionId, ArmyId, SourceRegion, TargetRegion;
    FString ExpectedProfile, ExpectedCampaignState;
    int32 RecruitQuantity = 0; // Zero is travel; positive uses the same finite recruitment authority.
    uint32 ExpectedLoadRevision = 0;
};

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
    // The explicit proof binds an existing settlement authority; definitions are immutable inputs.
    bool InitializeSettlementDevelopment(class USoulSettlementScenarioData* Scenario,
        class USoulSettlementStateSubsystem* Authority, FString& OutError);
    bool IsSettlementDevelopmentEnabled() const { return bSettlementDevelopmentRequested; }
    bool IsSettlementDevelopmentReady() const;
    const class USoulSettlementScenarioData* GetSettlementScenario() const { return SettlementScenario.Get(); }
    const FString& GetSettlementDevelopmentError() const { return SettlementDevelopmentError; }
    bool BeginSettlementConstruction(FName BuildingId, FString& OutError);
    bool IsTavernOperational() const;
    FName GetTavernBuildingId() const;
    FName GetDevelopmentRegion() const;
    FString GetDevelopmentBuildingName() const;
    uint32 SettlementDevelopmentRevision = 0; // Presentation invalidation only, not saved game truth.
    FString GetCampaignSaveSlotName() const;
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
    bool IsFourFactionAlpha() const { return bFourFactionAlpha; }
    bool IsAlphaTurnActive() const { return bFourFactionAlpha && AlphaNextFaction<3; }
    bool IsAlphaActiveFaction(FName Id) const;
    FName FindOwnedRecruitmentDestination(FName Faction) const;
    FString HumanRecoveryGuidance() const;
    bool CanOpenHumanSettlementServices(FString& Reason) const;
    bool bPlaytestContinueAttempted = false, bPlaytestContinuePending = false; // Process-only; never rewound by F9.
    void RunNextAlphaAction();
    bool PrepareControlledRecruitment(FName Id,int32 Quantity,FSoulControlledCampaignAction& Out,FString& Error) const;
    int32 GetAlphaSeed() const { return AlphaSeed; }
    bool IsSixFactionProfile() const { return bSixFactionProfile; }
    bool InspectFactionArmy(FName FactionId, FSoulFactionCampaignState& Out) const;
    bool MoveFactionArmy(FName FactionId, FName TargetRegion, FString& OutError);
    bool PrepareControlledAction(FName FactionId, FName ArmyId, FName ExpectedSource, FName TargetRegion,
        FSoulControlledCampaignAction& Out, FString& Error) const;
    bool ExecuteControlledAction(const FSoulControlledCampaignAction& Action, FString& Error);
    bool BuildFactionBattleDescriptor(FName FactionId, FName TargetRegion, FSoulCampaignBattleDescriptor& Out, FString& Error) const;


    int32 ArmyCountAtRegion(FName RegionId) const;
    FName ArmyUnitAtRegion(FName RegionId) const;
    FString ArmyInspectionAtRegion(FName RegionId) const;
private:
    // Human state stays in the existing founder fields, never mirrored here.
    // All factions use this existing Soul.Campaign RBSave provider.
    bool bFourFactionAlpha = false;
    int32 AlphaNextFaction = 3, AlphaSeed = 1701, AlphaTurnDay = 0;
    void InitializeFourFactionAlpha();
    void AppendAlphaBattleRecap(const FSoulCampaignBattleResult& Result);
    bool ValidateControlledRecruitment(FName Id,FName ArmyId,FName Region,int32 Quantity,FString& Error) const;
    bool ExecuteControlledRecruitment(const FSoulControlledCampaignAction& A,FString& Error);
    bool bSixFactionProfile = false;
    FString SixFactionSaveSlot = TEXT("Soul.Composition3500.SixFactionProof");
    TMap<FName, FSoulFactionCampaignState> OtherFactionStates;
    bool ValidateControlledAction(FName FactionId, FName ArmyId, FName ExpectedSource, FName TargetRegion,
        FSoulCampaignBattleDescriptor& Encounter, FString& Error) const;
    bool InitializeSixFactionState(const class FJsonObject& Starts, const class FJsonObject& Config, FString& Error);
    void CaptureSixFactionState(class FJsonObject& Root) const;
    bool ValidateSixFactionRestore(const class FJsonObject& Root,
        TMap<FName, FSoulFactionCampaignState>& OutStates, FSoulWorldState& OutWorld, FString& Error) const;
    void HandleBattleResolved(const FSoulCampaignBattleResult& Result);
    UFUNCTION() void OnCampaignSaved(const FRBSaveOperationResult& Result);
    UFUNCTION() void OnCampaignLoaded(const FRBSaveOperationResult& Result);
    TWeakObjectPtr<class URBSaveSubsystem> SaveSubsystem;
    TWeakObjectPtr<USoulCampaignBattleBridge> BattleBridge;
    bool bSettlementDevelopmentRequested = false;
    FString SettlementDevelopmentError;
    UPROPERTY(Transient) TObjectPtr<class USoulSettlementScenarioData> SettlementScenario;
    TWeakObjectPtr<class USoulSettlementStateSubsystem> SettlementAuthority;
    bool ValidateSettlementDevelopmentBinding(FString& OutError) const;
};
