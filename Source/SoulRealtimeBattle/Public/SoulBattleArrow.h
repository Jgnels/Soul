#pragma once
#include "RBCombatProjectile.h"
#include "SoulBattleArrow.generated.h"

// Presentation only: trajectory, collision and accepted hits stay in RB Combat.
UCLASS()
class SOULREALTIMEBATTLE_API ASoulBattleArrow : public ARBCombatProjectile
{
    GENERATED_BODY()
public:
    ASoulBattleArrow();
    virtual void Tick(float DeltaSeconds) override;
private:
    bool bLaunchPresented = false;
};


class ASoulRealtimeArenaGameMode;
// Uses RB Combat's swept projectile flight. RB Magic owns the committed payload.
UCLASS()
class SOULREALTIMEBATTLE_API ASoulBattleSpellProjectile : public ARBCombatProjectile
{
    GENERATED_BODY()
public:
    ASoulBattleSpellProjectile();
    void SetMagicOwner(ASoulRealtimeArenaGameMode* Owner);
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
protected:
    virtual void OnPhysicalImpact(const FHitResult& Hit,const FVector& Velocity) override;
private:
    TWeakObjectPtr<ASoulRealtimeArenaGameMode> MagicOwner;
};
