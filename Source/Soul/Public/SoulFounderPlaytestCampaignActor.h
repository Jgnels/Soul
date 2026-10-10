#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulFounderPlaytestCampaignActor.generated.h"

class USoulFounderPlaytestStateSubsystem;

UCLASS()
class SOUL_API ASoulFounderPlaytestCampaignActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulFounderPlaytestCampaignActor();

    virtual void BeginPlay() override;
    virtual void Tick(float Seconds) override;

    void HandleRegionClicked(FName RegionId);
    void SelectCompany();
    void CycleFactionInspection();
    FName ViewFaction() const;
    bool IsFactionInspection() const { return !InspectionFaction.IsNone(); }
    void EndDay();
    void HandleNumberKey(int32 Index);
    void HireTavernHero();
    void BuildTavern();
    void VisitSettlement();
    void ToggleTownPanel();
    void ToggleDiplomacy();
    bool bDiplomacyPanel=false;
    FName DiplomaticFaction=TEXT("dwarves");
    void StartBattle();
    void CastTailwind();
    void CancelPanel();

    TArray<FString> BuildHudLines() const;
    bool IsSkillChoiceOpen() const;
    bool IsTownPanelOpen() const { return bTownPanelOpen; }
    bool IsCompanySelected() const { return bCompanySelected; }
    bool IsBattleAvailable() const;
    FName SelectedRegion, HoveredRegion;
    USoulFounderPlaytestStateSubsystem* GetState() const { return State; }
    FName GetSelectedRegion() const { return SelectedRegion.IsNone() && State ? CurrentRegion() : SelectedRegion; }
    FName CurrentRegion() const;
    FString DisplayName(FName RegionId) const;
    FString LastMessage = TEXT("Click an explored adjacent region to move.");

private:
    UPROPERTY()
    TObjectPtr<class USoulFounderPlaytestStateSubsystem> State;
    UPROPERTY()
    TMap<FName, TObjectPtr<class ASoulPlaytestRegionActor>> RegionActors;

    UPROPERTY() TObjectPtr<class ASoulCampaignWorldActor> WorldPresentation;
    FName SelectedBattleRegion;
    FName InspectionFaction; // Read-only qualification UI, not campaign authority or saved state.
    uint32 ObservedLoadRevision = 0;
    bool bTownPanelOpen = false;
    bool bBattlePromptOpen = false;
    bool bCompanySelected = false;

    void SpawnRegions();
    void RefreshRegionVisuals();

    FLinearColor RegionColor(FName RegionId) const;
    static const TMap<FName, FVector>& RegionPositions();
};
