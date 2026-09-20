#pragma once

#include "GameFramework/Actor.h"
#include "RBCombatActions.h"
#include "RBCombatProjectile.generated.h"

// Physical producer shared by arrows and thrown weapons. Hosts provide the
// impact receiver, which connects native accepted damage to their authority.
// It contains no health, ammunition or inventory state.
UCLASS(Blueprintable)
class REFINEDBADGERCOMBATVARIANT_API ARBCombatProjectile : public AActor
{
    GENERATED_BODY()
public:
    ARBCombatProjectile();
    bool Launch(const FRBProjectileLaunch& Evidence, AActor* SourceRepresentation);
    const FRBProjectileLaunch& GetLaunchEvidence() const { return LaunchEvidence; }
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditDefaultsOnly, Category = "RefinedBadger|Combat")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
protected:
    // Invoked once for the first physical blocking surface, including neutral
    // people and scenery. Resolve identity in the host, not by hostility filter.
    virtual void OnPhysicalImpact(const FHitResult& Hit, const FVector& IncomingVelocity);
    UFUNCTION(BlueprintImplementableEvent, Category = "RefinedBadger|Combat")
    void PresentImpact(const FHitResult& Hit);
    UFUNCTION(BlueprintImplementableEvent, Category = "RefinedBadger|Combat")
    void PresentExpiry();
private:
    FRBProjectileLaunch LaunchEvidence;
    FRBProjectileFlight Flight;
    TWeakObjectPtr<AActor> IgnoredSource;
    bool bLaunched = false;
};
