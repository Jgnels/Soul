#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SoulFounderPlaytestPlayerController.generated.h"

UCLASS()
class SOUL_API ASoulFounderPlaytestPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASoulFounderPlaytestPlayerController();
    virtual void PlayerTick(float DeltaSeconds) override;
    UFUNCTION(Exec) void SoulFocusCompany();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    class ASoulFounderPlaytestCampaignActor* GetCampaign() const;
    void PrimaryClick();
    void GamepadSelect();
    void ZoomIn();
    void ZoomOut();
    void Number1();
    void Number2();
    void Number3();
    void Number4();
    void Number5();
    void Number6();
    void Number7();
    void EndDay();
    void ToggleTown();
    void HireHero();
    void StartBattle();
    void Defend();
    void Wait();
    void Spell1();
    void Spell2();
    void Spell3();
    void Spell4();
    void Spell5();
    void Spell6();
    void CancelPanel();
};
