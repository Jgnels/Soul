#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "RBCombatBlueprintBinding.h"
#include "RBCombatGroupDriver.h"
#include "SoulRealtimeBattleRules.h"
#include "SoulRealtimeBattleArena.generated.h"

class ACharacter;
class URBCombatRangedComponent;

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
    : public AGameModeBase
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

    int32 AliveForSide(int32 Side) const;
    int32 CasualtiesForSide(int32 Side) const;
    int32 TotalAlliedTargets() const;
    int32 AcceptedContactCount() const;
    float PlayerHealth() const;
    FString Status;
    UPROPERTY() TArray<TObjectPtr<ACharacter>> Actors;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaBinding>> Bindings;
    UPROPERTY() TArray<TObjectPtr<URBCombatRangedComponent>> Ranged;
    UPROPERTY() TArray<TObjectPtr<USoulRealtimeArenaGroupDriver>> Drivers;
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
    void PlayerTick(float Seconds);
    bool PerformMelee(
        int32 AttackerIndex, FRBHostIdentity IntendedTarget);
    void ToggleAlliedOrders();
    void UpdateDefeatedRepresentations();
    void FinishProof(bool bPassed, const FString& Detail);
    static FString RoleLabel(ESoulRealtimeFormationRole Role);
    static float RoleDamage(ESoulRealtimeFormationRole Role);
    static float RoleHealth(ESoulRealtimeFormationRole Role);

    TArray<FSoulRealtimeArenaCombatant> Combatants;
    TArray<FRBHostGroup> Groups;
    TSet<FGuid> AcceptedContacts;
    TSet<int32> DefeatedRepresentations;
    bool bProof = false;
    bool bFinished = false;
    bool bAttackHeld = false;
    bool bAlliedCharge = true;
    float ProofElapsed = 0.0f;
    int32 InitialAlive[2] = {0, 0};
};

