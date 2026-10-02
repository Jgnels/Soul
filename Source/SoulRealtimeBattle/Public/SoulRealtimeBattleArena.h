#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "RBCombatBlueprintBinding.h"
#include "RBCombatGroupDriver.h"
#include "RBMagicAuthority.h"
#include "SoulRealtimeBattleRules.h"
#include "SoulRealtimeBattleTactics.h"
#include "SoulRealtimeBattleArena.generated.h"

class AActor;
class ASoulBattleSpellProjectile;
class ACameraActor;
class ACharacter;
class USoulRealtimeBattlePBIL;
class UAnimationAsset;
class USkeletalMesh;
class USpringArmComponent;
class URBCombatRangedComponent;
class URBMagicPresentationProfile;
class URBMagicSpellDefinition;

enum class ESoulRealtimeMovementArchetype : uint8
{
    Infantry,
    LowProfile,
    Aerial
};

struct FSoulRealtimeArenaMagicArea
{
    FVector Center = FVector::ZeroVector;
    float Radius = 0.0f;
    float RemainingSeconds = 0.0f;
    float TickAccumulator = 0.0f;
    float DamagePerTick = 0.0f;
    float SlowFraction = 0.0f;
    int32 SourceSide = 0;
};

struct FSoulRealtimeArenaCombatant
{
    FGuid Id;
    int32 Side = 0;
    ESoulRealtimeFormationRole Role =
        ESoulRealtimeFormationRole::Line;
    float Health = 100.0f;
    float MaxHealth = 100.0f;
    float BaseWalkSpeed = 420.0f;
    int32 Arrows = 0;
    int32 GroupIndex = INDEX_NONE;
    FName FormationId;
    bool bRanged = false;
    bool bPlayerHero = false;
    float MeleeCooldown = 0.0f;
    float PendingMeleeSeconds = 0.0f;
    FRBHostIdentity PendingMeleeTarget;
    bool bVisualAttackPlaying = false;
    ESoulRealtimeMovementArchetype Movement =
        ESoulRealtimeMovementArchetype::Infantry;
    float WardPoints = 0.0f;
    float WardSeconds = 0.0f;
};
UCLASS()
class SOULREALTIMEBATTLE_API USoulRealtimeArenaBinding
    : public URBCombatBlueprintBinding
{
    GENERATED_BODY()
public:
    virtual bool HostIdentityExists_Implementation(
        FRBHostIdentity Identity) const override;
    virtual bool HostReadEquippedWeapon_Implementation(
        FRBHostIdentity Identity, FRBHostWeapon& Out) const override;
    virtual bool HostCommitAcceptedHit_Implementation(
        const FRBHostHit& Hit, FString& Error) override;
    virtual bool HostSpendResources_Implementation(
        FRBHostIdentity Identity, FName WeaponId,
        FName Item, int32 Quantity, float Effort,
        FString& Error) override;
    virtual bool HostCanAct_Implementation(
        FRBHostIdentity Identity) const override;
    virtual bool HostCanReceiveDamage_Implementation(
        FRBHostIdentity Identity) const override;
    virtual bool HostHasEligibleGuardEquipment_Implementation(
        FRBHostIdentity Identity) const override;
};
UCLASS()
class SOULREALTIMEBATTLE_API USoulRealtimeArenaGroupDriver
    : public URBCombatBlueprintGroupDriver
{
    GENERATED_BODY()
public:
    int32 GroupIndex = INDEX_NONE;
    int32 AlliedTargets = 0;

    virtual bool HostReadGroup_Implementation(
        FRBHostGroup& Out) const override;
    virtual bool HostReadParticipation_Implementation(
        FRBHostIdentity Identity,
        FRBHostParticipation& Out) const override;
    virtual bool HostAreOpponents_Implementation(
        FRBHostIdentity A, FRBHostIdentity B) const override;
    virtual bool HostHasAmmunition_Implementation(
        FRBHostIdentity Identity, FName Item) const override;
    virtual bool HostRequestMeleeFallback_Implementation(
        URBVariantCombatBindingComponent* Binding) override;
    virtual void HostObserveDecision_Implementation(
        FRBHostIdentity Identity,
        const FRBHostDecision& Decision) override;
    virtual bool HostCommitGroupOrder_Implementation(
        const FRBHostGroup& Previous,
        const FRBHostGroup& Replacement,
        FString& Error) override;
};
// The battlefield remains inspectable while the simulation is paused.
UCLASS()
class SOULREALTIMEBATTLE_API ASoulRealtimeArenaPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ASoulRealtimeArenaPlayerController();
    virtual void SetupInputComponent() override;
    void PauseBattle();
    virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const override;
};

UCLASS()
class SOULREALTIMEBATTLE_API ASoulRealtimeArenaHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName Name) override;
};

UCLASS()
class SOULREALTIMEBATTLE_API ASoulRealtimeArenaGameMode
    : public AGameModeBase, public IRBMagicAuthority
{
    GENERATED_BODY()
public:
    ASoulRealtimeArenaGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Seconds) override;

    int32 Index(FRBHostIdentity Identity) const;
    FRBHostIdentity IdentityAt(int32 Index) const;
    FRBWeaponProfile Profile(int32 Index) const;
    static bool IsDirectGroundRouteClear(UWorld* World, const FVector& From, const FVector& To, float Radius = 65.0f);
    static float MeleeBodyReach(ACharacter* Attacker, ACharacter* Target, float BaseReach);
    static bool TraceMeleeContact(ACharacter* Attacker, ACharacter* Intended,
        float Reach, FHitResult& OutHit);
    bool CommitHit(const FRBHostHit& Hit, FString& Error);
    bool SpendResources(FRBHostIdentity Identity,
        FName WeaponId, FName Item, int32 Quantity,
        float Effort, FString& Error);
    bool CanAct(FRBHostIdentity Identity) const;
    bool HasAmmunition(
        FRBHostIdentity Identity, FName Item) const;
    bool ReadGroup(int32 GroupIndex, FRBHostGroup& Out) const;
    bool ReadParticipation(
        FRBHostIdentity Identity, FRBHostParticipation& Out) const;
    bool AreOpponents(FRBHostIdentity A, FRBHostIdentity B) const;
    bool CommitGroupOrder(
        int32 GroupIndex,
        const FRBHostGroup& Previous,
        const FRBHostGroup& Replacement,
        FString& Error);
    bool RequestMeleeFallback(
        URBVariantCombatBindingComponent* Binding);
    void ObserveDecision(
        int32 GroupIndex,
        FRBHostIdentity Identity,
        const FRBHostDecision& Decision);

    virtual bool CanCastMagic(
        const URBMagicSpellDefinition& Spell,
        const FRBMagicCastRequest& Request,
        FString& OutError) const override;
    virtual bool TryCommitMagicCast(
        const URBMagicSpellDefinition& Spell,
        const FRBMagicCastRequest& Request,
        TConstArrayView<FRBMagicEffectIntent> Effects,
        FString& OutError) override;

    int32 AliveForSide(int32 Side) const;
    int32 CasualtiesForSide(int32 Side) const;
    int32 TotalAlliedTargets() const;
    int32 AcceptedContactCount() const;
    float PlayerHealth() const;
    int32 ReserveBodiesForSide(int32 Side) const;
    int32 ReinforcementWavesForSide(int32 Side) const;
    FString ReinforcementSummary(int32 Side) const;
    FString AlliedOrderSummary() const;
    FString TacticalSummary() const;
    FString SpellSummary() const;
    FString SelectedFormationSummary() const;
    FString AlliedFormationSummary(int32 Slot) const;
    int32 AlliedFormationCount() const;
    FVector SelectedFormationLocation() const;
    bool IsBattlePaused() const { return bBattlePaused; }
    bool IsTacticalCameraActive() const { return bTacticalCameraActive; }
    bool IsFirstPersonCamera() const { return bFirstPersonCamera; }
    float PlayerManaValue() const { return PlayerMana; }
    int32 MagicCastCount() const { return MagicCasts; }
    bool AreAlliedFormationsCharging() const { return bAlliedCharge; }
    FString Status;
    UPROPERTY() TArray<TObjectPtr<ACharacter>> Actors;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaBinding>> Bindings;
    UPROPERTY() TArray<TObjectPtr<URBCombatRangedComponent>> Ranged;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaGroupDriver>> Drivers;
    UPROPERTY() TArray<TObjectPtr<AActor>> BattlefieldBounds;
    UPROPERTY() TObjectPtr<ACharacter> PlayerHero;

private:
    friend class ASoulRealtimeArenaHUD;
    friend class ASoulRealtimeArenaPlayerController;
    friend class ASoulBattleSpellProjectile;
    friend class FSoulMagicProjectileAuthorityTest;
    friend class FSoulFormationSelectionTest;
    bool LaunchMagicProjectile(int32 Caster,int32 Target,const FGuid& CastId,float Damage);
    void ResolveMagicProjectile(ASoulBattleSpellProjectile* Projectile,const FHitResult& Hit);
    void ForgetMagicProjectile(ASoulBattleSpellProjectile* Projectile);
    TMap<FGuid,TWeakObjectPtr<ASoulBattleSpellProjectile>> MagicProjectiles;
    TSet<FGuid> CommittedMagicCasts;

    static constexpr float BattlefieldHalfX = 2600.0f;
    static constexpr float BattlefieldHalfY = 2200.0f;
    void HandleBattleAction(FName Action);
    FString SpellButtonLabel(int32 Slot) const;
    void SetupSpellBar();
    UPROPERTY() TArray<TObjectPtr<URBMagicSpellDefinition>> BattleSpells;
    UPROPERTY() TArray<TObjectPtr<UObject>> BattlePresentationAssets;
    void SelectPlayerSpell(int32 Slot);
    bool CastPlayerSpellSlot(int32 Slot, const FHitResult* AimHit = nullptr);
    bool ReadPointerHit(FHitResult& Hit) const;
    void MoveSelectedToPointer();
    bool bShowBattleHelp = false;
    int32 SelectedSpellSlot = INDEX_NONE;
    bool bPlaceFormationOrder = false;
    void TickBattleResolution(float Seconds);
    void TickSpatialOrders(float Seconds);
    void TickFormationTactics();
    void UpdateFormationMorale();
    void RefreshBattlePhase();
    void SetupBattleCamera();
    int32 AliveInGroup(int32 GroupIndex) const;
    FVector GroupCenter(int32 GroupIndex) const;
    int32 FindNearestEnemyToGroup(int32 GroupIndex, float& OutDistance) const;
    int32 FindFormationState(int32 GroupIndex) const;
    bool IssueFormationOrder(int32 GroupIndex, ERBHostGroupOrder Order,
        const FVector& Anchor, const FVector& Facing, bool bManual);
    bool ChooseReinforcementAnchor(int32 Side, FVector& OutAnchor) const;
    void SelectNextAlliedFormation();
    void SelectAlliedFormationSlot(int32 Slot);
    void CommandSelectedAllies(ERBHostGroupOrder Order);
    void ReturnSelectedAlliesToAI();
    void ToggleBattlePause();
    void ToggleBattleCamera();
    void ToggleFirstPersonCamera();
    void FinishBattle();
    bool bMapOnly = false;
    bool bAutobattle = false;
    bool bQualification = false;
    bool bTacticalMagic = false;
    bool bCampaignBattle = false;
    int32 ActiveCap = 5;
    int32 StrategicBodies[2] = {5, 5};
    float BattleElapsed = 0.0f;
    float ResultHoldSeconds = -1.0f;
    bool bFirstCapture = false;
    bool bSecondCapture = false;
    float SpatialElapsed = 0.0f;
    int32 SpatialOrders = 0;
    TArray<FSoulBattleFormationState> TacticalFormations;
    ESoulBattlePhase BattlePhase = ESoulBattlePhase::Deployment;
    int32 SelectedAlliedFormation = INDEX_NONE;
    bool bSelectAllAllies = true;
    bool bBattlePaused = false;
    bool bControlDiagnostics = false;
    bool bReadabilityProof = false;
    int32 ReadabilityStage = 0;
    double ReadabilityStarted = 0.0;
    double ReadabilityNextCapture = 0.0;
    double ReadabilityCaptureAt = 0.0;
    FString ReadabilityCaptureName;
    void TickReadabilityProof();
    double NextControlReceipt = 0.0;
    bool bTacticalCameraActive = false;
    bool bFirstPersonCamera = false;
    int32 RoutedSides[2] = {0, 0};
    bool bSideMoraleDefeated[2] = {false, false};
    UPROPERTY() TObjectPtr<USoulRealtimeBattlePBIL> Spatial;
    bool SetupArena();
    bool SpawnArmy(int32 Side);
    bool SpawnFormation(
        int32 Side,
        ESoulRealtimeFormationRole FormationRole,
        int32 Count,
        const FVector& Anchor);
    bool SpawnCombatant(
        int32 Side,
        ESoulRealtimeFormationRole FormationRole,
        int32 GroupIndex,
        const FVector& Location,
        bool bPlayerHero);
    bool SetupDrivers();
    bool RefreshDriverRepresentations();
    void SetupReinforcementState();
    void TickReinforcements();
    bool SpawnReinforcementWave(int32 Side, int32 Count, const FVector& ArrivalAnchor);
    void RollbackSpawnedFormation(int32 FirstCombatant, int32 FirstGroup, int32 FirstDriver);
    bool SetupBattlefieldBounds();
    void TrackBattlefieldExtent();
    FVector ResolveSpawnLocation(const FVector& Desired);
    USkeletalMesh* ResolveVisualMesh(
        int32 Side, ESoulRealtimeFormationRole FormationRole) const;
    UAnimationAsset* ResolveVisualAnimation(
        int32 Side, bool bRunning, ESoulRealtimeFormationRole FormationRole) const;
    UAnimationAsset* ResolveVisualAttack(
        int32 Side, ESoulRealtimeFormationRole FormationRole) const;
    UAnimationAsset* ResolveVisualDeath(
        int32 Side, ESoulRealtimeFormationRole FormationRole) const;
    bool UsesEvilVisualRoster() const;
    void UpdateVisualAnimations();
    void PlayerTick(float Seconds);
    void TickMagic(float Seconds);
    bool CastPlayerSpell(
        const TCHAR* SpellPath,
        const TCHAR* PresentationPath = nullptr,
        const FHitResult* AimHit = nullptr);
    int32 FindPlayerSpellTarget(float Range) const;
    bool ApplyMagicDamage(int32 TargetIndex, float Damage);
    void SpawnSpellPresentation(
        const URBMagicPresentationProfile* Profile,
        const FVector& Location);
    bool CommitMeleeImpact(int32 AttackerIndex, FRBHostIdentity IntendedTarget);
    bool PerformMelee(
        int32 AttackerIndex, FRBHostIdentity IntendedTarget);
    void ToggleAlliedOrders();
    void UpdateDefeatedRepresentations();
    void FinishProof(bool bPassed, const FString& Detail);
    static FString RoleLabel(ESoulRealtimeFormationRole Role);
    static float RoleDamage(ESoulRealtimeFormationRole Role);
    static float RoleHealth(ESoulRealtimeFormationRole Role);
    static float RoleWalkSpeed(ESoulRealtimeFormationRole Role);
    ESoulRealtimeMovementArchetype ResolveMovementArchetype(
        int32 Side, ESoulRealtimeFormationRole FormationRole) const;

    TArray<FSoulRealtimeArenaCombatant> Combatants;
    TArray<FSoulRealtimeArenaMagicArea> ActiveMagicAreas;
    TArray<FRBHostGroup> Groups;
    TSet<FGuid> AcceptedContacts;
    TSet<int32> DefeatedRepresentations;
    FSoulRealtimeBattleState ReinforcementBattle;
    int32 InitialStrategic[2] = {0, 0};
    int32 ReinforcementWaves[2] = {0, 0};
    int32 LastReinforcementBodies[2] = {0, 0};
    TArray<bool> VisualRunning;
    FVector ArenaOrigin = FVector::ZeroVector;
    FName EnemyVisualFaction = NAME_None;
    FName EnemyVisualRegion = NAME_None;
    bool bProof = false;
    bool bMagicProof = false;
    bool bExternalEnvironment = false;
    bool bVisualUnits = false;
    bool bFinished = false;
    bool bAttackHeld = false;
    bool bAlliedCharge = true;
    float ProofElapsed = 0.0f;
    float MaxObservedArenaOffsetX = 0.0f;
    float MaxObservedArenaOffsetY = 0.0f;
    float MagicProofElapsed = 0.0f;
    int32 MagicProofStage = 0;
    float PlayerMana = 80.0f;
    TMap<FName, float> SpellCooldowns;
    int32 MagicCasts = 0;
    int32 InitialAlive[2] = {0, 0};
    float SpeedBuffMultiplier[2] = {1.0f, 1.0f};
    float SpeedBuffSeconds[2] = {0.0f, 0.0f};
    FVector TacticalFocus = FVector::ZeroVector;
    float TacticalDistance = 2400.0f;
    FRotator TacticalRotation = FRotator(-38, -50, 0);
    UPROPERTY() TObjectPtr<ACameraActor> TacticalCamera;
    UPROPERTY() TObjectPtr<USpringArmComponent> PlayerCameraArm;
};

