#pragma once
#include "RBVariantCombatBindingComponent.h"
#include "RBCombatBlueprintBinding.generated.h"

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostIdentity
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FName Domain;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FGuid Id;
    FRBCombatantRef Core() const { return {Domain, Id}; }
    static FRBHostIdentity From(const FRBCombatantRef& Value)
    { FRBHostIdentity Result; Result.Domain = Value.Domain; Result.Id = Value.Id; return Result; }
};

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostWeapon
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FName Category;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") FName DamageType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float BaseDamage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float Reach = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float BleedingTendency = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat") float StaggerSeconds = 0;
    FRBWeaponProfile Core() const;
    static FRBHostWeapon From(const FRBWeaponProfile& Value);
};

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBHostHit
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FGuid ContactId;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FRBHostIdentity Attacker;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FRBHostIdentity Victim;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FRBHostWeapon Weapon;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") float AcceptedDamage = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") float Bleeding = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") float StaggerSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") FVector ImpactPoint = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") bool bHasExactImpact = false;
    UPROPERTY(BlueprintReadOnly, Category = "Combat") TArray<FRBHostIdentity> Witnesses;
    static FRBHostHit From(const FRBCombatHit& Value);
};

// Blueprint host facade. No health, inventory, faction or identity registry is
// stored here. Every host decision defaults to rejection until implemented.
UCLASS(Blueprintable, ClassGroup = (RefinedBadger), meta = (BlueprintSpawnableComponent))
class REFINEDBADGERCOMBATVARIANT_API URBCombatBlueprintBinding : public URBVariantCombatBindingComponent,
    public IRBCombatConsequenceSink, public IRBCombatWeaponProvider, public IRBCombatResourceAuthority
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool BindExistingIdentity(FRBHostIdentity Identity) { return BindCombatant(Identity.Core()); }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FRBHostIdentity GetHostIdentity() const { return FRBHostIdentity::From(GetCombatant()); }
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool SetGuardConfiguration(float BlockedFraction = .8f, float MinimumFacingDot = .5f, float EffortCost = 0);

    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostIdentityExists(FRBHostIdentity Identity) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostReadEquippedWeapon(FRBHostIdentity Identity, FRBHostWeapon& Weapon) const;
    // Validate every field first; return false without changing host state.
    // This runs synchronously after native acceptance. Never use a latent event.
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostCommitAcceptedHit(const FRBHostHit& Hit, FString& Error);
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostSpendResources(FRBHostIdentity Identity, FName WeaponId, FName ConsumedItem, int32 Quantity, float Effort, FString& Error);
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostCanAct(FRBHostIdentity Identity) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostCanReceiveDamage(FRBHostIdentity Identity) const;
    UFUNCTION(BlueprintNativeEvent, Category = "RefinedBadger|Host")
    bool HostHasEligibleGuardEquipment(FRBHostIdentity Identity) const;

    virtual bool IsCombatantValid(const FRBCombatantRef& Identity) const override;
    virtual bool ResolveWeapon(const FRBCombatantRef& Identity, FRBWeaponProfile& Out) const override;
    virtual bool TryApplyCombatHit(const FRBCombatHit& Hit, FString& Error) override;
    virtual bool TrySpendCombatResources(const FRBCombatantRef& Identity, FName WeaponId, FName Item,
        int32 Quantity, float Effort, FString& Error) override;
protected:
    virtual IRBCombatConsequenceSink* GetConsequenceSink() const override { return const_cast<URBCombatBlueprintBinding*>(this); }
    virtual const IRBCombatWeaponProvider* GetWeaponProvider() const override { return this; }
    virtual IRBCombatResourceAuthority* GetResourceAuthority() const override { return const_cast<URBCombatBlueprintBinding*>(this); }
    virtual bool CanParticipateInCombat() const override;
    virtual bool CanReceiveCombatDamage() const override;
    virtual bool HasEligibleGuardEquipment() const override;
};
