#pragma once
#include "Engine/DataAsset.h"
#include "RBCombatActions.h"
#include "RBCombatWeaponData.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class ERBWeaponAttack : uint8 { Light, Heavy, Thrust, Shoot, Throw };

USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBWeaponAnimation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") ERBWeaponAttack Attack = ERBWeaponAttack::Light;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation", meta = (ClampMin = "0", ClampMax = "7")) int32 ComboStage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") TObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") float DamageScale = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") float Effort = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") float WindupSeconds = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") float MaximumContactSeconds = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") float RecoverySeconds = .5f;
    // Local grip correction for this motion. Attach the visual below the same
    // unscaled geometry component so its pose and physical sweeps agree.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefinedBadger|Animation") FRotator GripRotation = FRotator::ZeroRotator;
    FRBActionDefinition Definition() const;
};

// Create a data asset per host weapon. Animation/meshes stay consumer-owned.
UCLASS(BlueprintType)
class REFINEDBADGERCOMBATVARIANT_API URBCombatWeaponData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") FName WeaponId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") FName Category = TEXT("Sword");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") float BaseDamage = 16;
    // Consequence requests consumed by the host; this asset owns no durable injury state.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon", meta = (ClampMin = "0", ClampMax = "1000")) float BleedingTendency = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon", meta = (ClampMin = "0", ClampMax = "30")) float StaggerSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") float Reach = 150;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") float SweepRadius = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") float ThrowSpeed = 2200;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") float ProjectileRadius = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") FVector LocalBladeStart = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") FVector LocalBladeEnd = FVector(100, 0, 0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefinedBadger|Weapon") TArray<FRBWeaponAnimation> Attacks;
    FRBWeaponProfile Profile() const;
    bool IsValidWeapon() const;
};
