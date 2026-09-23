#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "RBCombatBlueprintBinding.h"
#include "RBCombatGroupDriver.h"
#include "RBMagicAuthority.h"
#include "SoulRealtimeBattleRules.h"
#include "SoulRealtimeBattleArena.generated.h"

class AActor;
class ACharacter;
class UAnimationAsset;
class USkeletalMesh;
class URBCombatRangedComponent;
class URBMagicPresentationProfile;
class URBMagicSpellDefinition;

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
    int32 Arrows = 0;
    int32 GroupIndex = INDEX_NONE;
    bool bRanged = false;
    bool bPlayerHero = false;
    float MeleeCooldown = 0.0f;
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
UCLASS()
class SOULREALTIMEBATTLE_API ASoulRealtimeArenaHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
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
    float PlayerManaValue() const { return PlayerMana; }
    int32 MagicCastCount() const { return MagicCasts; }
    FString Status;
    UPROPERTY() TArray<TObjectPtr<ACharacter>> Actors;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaBinding>> Bindings;
    UPROPERTY() TArray<TObjectPtr<URBCombatRangedComponent>> Ranged;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaGroupDriver>> Drivers;
    UPROPERTY() TArray<TObjectPtr<AActor>> BattlefieldBounds;
    UPROPERTY() TObjectPtr<ACharacter> PlayerHero;

private:
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
    bool SetupBattlefieldBounds();
    void TrackBattlefieldExtent();
    FVector ResolveSpawnLocation(const FVector& Desired);
    USkeletalMesh* ResolveVisualMesh(
        int32 Side, ESoulRealtimeFormationRole FormationRole) const;
    UAnimationAsset* ResolveVisualAnimation(
        int32 Side, bool bRunning) const;
    void UpdateVisualAnimations();
    void PlayerTick(float Seconds);
    void TickMagic(float Seconds);
    bool CastPlayerSpell(
        const TCHAR* SpellPath,
        const TCHAR* PresentationPath);
    int32 FindPlayerSpellTarget(float Range) const;
    bool ApplyMagicDamage(int32 TargetIndex, float Damage);
    void SpawnSpellPresentation(
        const URBMagicPresentationProfile* Profile,
        const FVector& Location);
    bool PerformMelee(
        int32 AttackerIndex, FRBHostIdentity IntendedTarget);
    void ToggleAlliedOrders();
    void UpdateDefeatedRepresentations();
    void FinishProof(bool bPassed, const FString& Detail);
    static FString RoleLabel(ESoulRealtimeFormationRole Role);
    static float RoleDamage(ESoulRealtimeFormationRole Role);
    static float RoleHealth(ESoulRealtimeFormationRole Role);
    static float RoleWalkSpeed(ESoulRealtimeFormationRole Role);

    TArray<FSoulRealtimeArenaCombatant> Combatants;
    TArray<FSoulRealtimeArenaMagicArea> ActiveMagicAreas;
    TArray<FRBHostGroup> Groups;
    TSet<FGuid> AcceptedContacts;
    TSet<int32> DefeatedRepresentations;
    TArray<bool> VisualRunning;
    FVector ArenaOrigin = FVector::ZeroVector;
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
};

