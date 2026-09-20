#pragma once
#include "Components/ActorComponent.h"
#include "RBCombatWeaponData.h"
#include "RBCombatMeleeComponent.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class URBVariantCombatBindingComponent;
class ARBCombatProjectile;

UCLASS(ClassGroup = (RefinedBadger), meta = (BlueprintSpawnableComponent))
class REFINEDBADGERCOMBATVARIANT_API URBCombatMeleeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBCombatMeleeComponent();
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool Configure(URBCombatWeaponData* Data, USkeletalMeshComponent* AnimationMesh, USceneComponent* WeaponGeometry);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool RequestAttack(ERBWeaponAttack Attack);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool RequestThrow(FVector AimDirection);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    void InterruptAttack();
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    bool IsAttacking() const { return Action.GetPhase() != ERBActionPhase::Ready; }
    FGuid GetActionId() const { return Action.GetActionId(); }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FString GetLastError() const { return LastError; }
    void OpenContact(int32 MontageInstanceId);
    void SweepContact(int32 MontageInstanceId);
    void CloseContact(int32 MontageInstanceId);
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat") TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
    // Optional visual subclass. Native swept flight remains in the base producer.
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat") TSubclassOf<ARBCombatProjectile> ThrownProjectileClass;
private:
    bool StartAttack(ERBWeaponAttack Attack, int32 ComboStage);
    bool ReleaseThrownWeapon();
protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "RefinedBadger|Combat")
    void PresentThrownWeapon(ARBCombatProjectile* Projectile);
private:
    URBVariantCombatBindingComponent* Binding() const;
    UPROPERTY(Transient) TObjectPtr<URBCombatWeaponData> Weapon;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Animator;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> Geometry;
    FQuat RestGripRotation = FQuat::Identity;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> PlayingMontage;
    FRBCombatAction Action;
    FRBActionDefinition ActiveDefinition;
    FRBWeaponProfile AttackWeapon;
    FVector PreviousStart = FVector::ZeroVector;
    FVector PreviousEnd = FVector::ZeroVector;
    TSet<TWeakObjectPtr<AActor>> ContactedActors;
    int32 ActiveMontageId = INDEX_NONE;
    bool bBuffered = false;
    ERBWeaponAttack BufferedAttack = ERBWeaponAttack::Light;
    float BufferRemaining = 0;
    FString LastError;
    ERBWeaponAttack ActiveAttack = ERBWeaponAttack::Light;
    int32 ActiveComboStage = 0;
    FVector ThrowAim = FVector::ForwardVector;
    FVector ActiveBladeStart = FVector::ZeroVector;
    FVector ActiveBladeEnd = FVector::ZeroVector;
    float ActiveSweepRadius = 1;
    float ActiveThrowSpeed = 1;
    float ActiveProjectileRadius = 1;
    UPROPERTY(Transient) TSubclassOf<ARBCombatProjectile> ActiveProjectileClass;
};
