#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulFounderPlaytestCampaignActor.generated.h"

UCLASS()
class SOUL_API ASoulFounderPlaytestCampaignActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulFounderPlaytestCampaignActor();

    virtual void BeginPlay() override;
    virtual void Tick(float Seconds) override;

    void HandleRegionClicked(FName RegionId);
    void EndDay();
    void HandleNumberKey(int32 Index);
    void HireTavernHero();
    void ToggleTownPanel();
    void StartBattle();
    void CastTailwind();
    void CancelPanel();

    TArray<FString> BuildHudLines() const;
    bool IsSkillChoiceOpen() const;
    bool IsTownPanelOpen() const { return bTownPanelOpen; }
    bool IsBattleAvailable() const;
    FString LastMessage = TEXT("Click an explored adjacent region to move.");

private:
    UPROPERTY()
    TObjectPtr<class USoulFounderPlaytestStateSubsystem> State;
    UPROPERTY()
    TMap<FName, TObjectPtr<class ASoulPlaytestRegionActor>> RegionActors;

    FName SelectedBattleRegion;
    bool bTownPanelOpen = false;
    bool bBattlePromptOpen = false;

    void SpawnRegions();
    void RefreshRegionVisuals();
    void DrawConnections();
    FString DisplayName(FName RegionId) const;
    FLinearColor RegionColor(FName RegionId) const;
    static const TMap<FName, FVector>& RegionPositions();
};
