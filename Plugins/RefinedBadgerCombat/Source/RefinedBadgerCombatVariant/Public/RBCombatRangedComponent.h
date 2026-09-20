#pragma once

#include "Components/ActorComponent.h"
#include "RBCombatActions.h"
#include "RBCombatRangedComponent.generated.h"

class URBVariantCombatBindingComponent;
class ARBCombatProjectile;

// Configuration only. Ammunition quantities and effort remain host-owned.
USTRUCT(BlueprintType)
struct REFINEDBADGERCOMBATVARIANT_API FRBCombatBowSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") FName Ammunition = TEXT("Arrow");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float MinimumDrawSeconds = .18f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float FullDrawSeconds = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float MinimumSpeed = 1450;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float MaximumSpeed = 4100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float GravityScale = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float LifetimeSeconds = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bow") float Radius = 2;
    FRBRangedProfile Core() const;
};

UCLASS(ClassGroup = (RefinedBadger), meta = (BlueprintSpawnableComponent))
class REFINEDBADGERCOMBATVARIANT_API URBCombatRangedComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBCombatRangedComponent();
    bool Configure(const FRBRangedProfile& Profile);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool ConfigureBow(const FRBCombatBowSettings& Settings);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool BeginDraw();
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    bool ReleaseShot(FVector MuzzleLocation, FVector AimDirection);
    UFUNCTION(BlueprintCallable, Category = "RefinedBadger|Combat")
    void CancelDraw();
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    bool IsDrawing() const { return Draw.IsDrawing(); }
    // Transient control ownership, distinct from the eventual projectile contact.
    FGuid GetDrawSessionId() const { return DrawSessionId; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    bool IsDrawReady() const { return Draw.IsDrawing() && Draw.GetDrawSeconds() >= RangedProfile.FullDrawSeconds; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    float GetDrawProgress() const { return Draw.IsDrawing() ? FMath::Clamp(Draw.GetDrawSeconds() / RangedProfile.FullDrawSeconds, 0.f, 1.f) : 0.f; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FName GetAmmunitionId() const { return RangedProfile.Ammunition; }
    UFUNCTION(BlueprintPure, Category = "RefinedBadger|Combat")
    FString GetLastError() const { return LastError; }
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat", meta = (ClampMin = "0"))
    float DrawEffortPerSecond = 2.8f;
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat", meta = (ClampMin = "0"))
    float ReleaseEffort = 2;
    UPROPERTY(EditAnywhere, Category = "RefinedBadger|Combat")
    TSubclassOf<ARBCombatProjectile> ProjectileClass;
private:
    URBVariantCombatBindingComponent* Binding() const;
    FRBRangedProfile RangedProfile;
    FRBRangedDraw Draw;
    FGuid DrawSessionId;
    FRBWeaponProfile DrawWeapon;
    FString LastError;
};
