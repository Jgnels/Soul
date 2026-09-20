#pragma once
#include "Components/ActorComponent.h"
#include "RBCombatRouter.h"
#include "RBCombatGuard.h"
#include "RBVariantCombatBindingComponent.generated.h"

class AController;
class UDamageType;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FRBAcceptedContactEvent, float, Damage,
    FVector, ImpactPoint, FName, WeaponCategory, bool, bExactImpact, FGuid, ContactId);

// Subclass in a host to supply lifetime-safe sink and equipment provider accessors.
UCLASS(Abstract, ClassGroup = (RefinedBadger))
class REFINEDBADGERCOMBATVARIANT_API URBVariantCombatBindingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBVariantCombatBindingComponent();
    // Cosmetic notification on the victim, after the host transaction succeeds.
    // Approximate native observations explicitly carry bExactImpact=false.
    UPROPERTY(BlueprintAssignable, Category = "RefinedBadger|Presentation")
    FRBAcceptedContactEvent OnAcceptedContact;
    bool BindCombatant(const FRBCombatantRef& Identity);
    FRBCombatantRef GetCombatant() const { return Combatant; }
    bool HasValidCombatant() const;
    bool IsAbleToAct() const { return CanParticipateInCombat(); }
    bool ResolveEquippedWeapon(FRBWeaponProfile& OutProfile) const;
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    void SetGuardIntent(bool bRequested) { bGuardRequested = bRequested; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    bool IsGuardRequested() const { return bGuardRequested; }
    bool ConfigureGuard(const FRBGuardPolicy& Policy, float EffortPerBlock = 0);
    FRBGuardResult GetLastGuardResult() const { return LastGuardResult; }
    bool TrySpendResources(FName WeaponProfileId, FName ConsumedItem, int32 Quantity, float Effort, FString& OutError);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    FGuid BeginNativeAttackContact();
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    void EndNativeAttackContact() { Router.EndContact(); }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FGuid GetActiveContactId() const { return Router.GetActiveContact(); }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FGuid GetLastObservedNativeContactId() const { return LastObservedContact; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FString GetLastNativeObservationError() const { return LastError; }
    bool PublishEvidence(URBVariantCombatBindingComponent* Victim, const FRBCombatHit& Hit, FString& OutError);
    // Optional physical producer path. Snapshotted source/weapon survive a
    // projectile's shooter representation or equipped weapon changing in flight.
    // Calls native PointDamage once; the subscribed AnyDamage callback supplies
    // the accepted amount to the host. No post-damage guard transformation.
    bool ReceiveProducedImpact(const FRBCombatHit& Evidence, const FHitResult& Impact,
        const FVector& Direction, AActor* Producer, FString& OutError);
protected:
    virtual IRBCombatConsequenceSink* GetConsequenceSink() const { return nullptr; }
    virtual const IRBCombatWeaponProvider* GetWeaponProvider() const { return nullptr; }
    virtual IRBCombatResourceAuthority* GetResourceAuthority() const { return nullptr; }
    // Hosts derive these facts from equipment and participation authority.
    virtual bool HasEligibleGuardEquipment() const { return false; }
    virtual bool CanParticipateInCombat() const { return HasValidCombatant(); }
    // A downed body may still receive damage although it cannot attack.
    virtual bool CanReceiveCombatDamage() const { return HasValidCombatant(); }
    virtual bool IsPerformingAttack() const;
    UFUNCTION(BlueprintImplementableEvent, Category = "RefinedBadger|Combat")
    void PresentGuardResult(bool bBlocked, float IncomingDamage, float EffectiveDamage);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION()
    void HandleOwnerAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);
    URBVariantCombatBindingComponent* ResolveAttacker(AActor* DamageCauser, AController* InstigatedBy) const;
    FRBCombatantRef Combatant;
    FRBCombatRouter Router;
    FGuid LastObservedContact;
    FString LastError;
    const FRBCombatHit* PendingImpact = nullptr;
    AActor* PendingProducer = nullptr;
    bool bPendingObserved = false;
    bool bPendingAccepted = false;
    TArray<FGuid> ProducedAttempts;
    FRBGuardPolicy GuardPolicy;
    FRBGuardResult LastGuardResult;
    float GuardEffort = 0;
    bool bGuardRequested = false;
    bool bApplyingProducedImpact = false;
};
