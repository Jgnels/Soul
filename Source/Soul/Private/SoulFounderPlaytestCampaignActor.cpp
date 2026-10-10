#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulCampaignTerrain.h"
#include "Engine/World.h"

#include "SoulCampaignWorldActor.h"
#include "SoulCampaignCamera.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementVisitGameMode.h"
#include "SoulPlaytestRegionActor.h"

ASoulFounderPlaytestCampaignActor::ASoulFounderPlaytestCampaignActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;
}

const TMap<FName, FVector>& ASoulFounderPlaytestCampaignActor::RegionPositions()
{
    return ASoulCampaignWorldActor::Locations();
}

void ASoulFounderPlaytestCampaignActor::BeginPlay()
{
    Super::BeginPlay();

    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        State = GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>();
    }
    if (!State)
    {
        LastMessage = TEXT("Playtest state subsystem unavailable.");
        return;
    }

    State->InitializeScenario();
    ObservedLoadRevision=State->CampaignLoadRevision;
    if(!State->LastBattleResult.EncounterId.IsNone())
    {
        const auto& Result=State->LastBattleResult;
        if(State->IsSixFactionProfile())
            LastMessage=FString::Printf(TEXT("%s at %s. Survivors: %d attackers / %d defenders. Your company: %d."),
                Result.bPlayerWon?TEXT("Attackers won"):TEXT("Defenders won"),*DisplayName(Result.TargetRegion),
                Result.PlayerSurvivors,Result.EnemySurvivors,State->GetPlayerTroopCount());
        else
            LastMessage=FString::Printf(TEXT("%s at %s. %d soldiers remain. [Space] restores travel actions."),
                Result.bPlayerWon?TEXT("Victory"):TEXT("Defeat"),*DisplayName(Result.TargetRegion),Result.PlayerSurvivors);
    }
    WorldPresentation = GetWorld()->SpawnActor<ASoulCampaignWorldActor>();
    WorldPresentation->Build(State);
    SpawnRegions();
    RefreshRegionVisuals();
}

FString ASoulFounderPlaytestCampaignActor::DisplayName(FName RegionId) const
{
    if (State) if (const FString* Name = State->RegionDisplayNames.Find(RegionId)) return SoulCampaignTerrain::LocationName(RegionId,*Name);
    static const TMap<FName, FString> Names = {
        {TEXT("human_capital"), TEXT("Human Capital")},
        {TEXT("crossroads"), TEXT("Crossroads")},
        {TEXT("old_quarry"), TEXT("Old Quarry")},
        {TEXT("river_ford"), TEXT("River Ford")},
        {TEXT("forest_edge"), TEXT("Forest Edge")},
        {TEXT("ancient_shrine"), TEXT("Ancient Shrine")},
        {TEXT("orc_watch"), TEXT("Orc Watch")},
        {TEXT("north_pass"), TEXT("North Pass")},
        {TEXT("orc_camp"), TEXT("Orc Stronghold")}
    };
    if (const FString* Name = Names.Find(RegionId)) return *Name;
    return RegionId.ToString();
}
void ASoulFounderPlaytestCampaignActor::SpawnRegions()
{
    if (!GetWorld() || !State) return;

    for (const TPair<FName, FVector>& Pair : RegionPositions())
    {
        if (!State->World.Regions.Contains(Pair.Key)) continue;
        ASoulPlaytestRegionActor* Node = GetWorld()->SpawnActor<ASoulPlaytestRegionActor>(
            ASoulPlaytestRegionActor::StaticClass(), Pair.Value, FRotator::ZeroRotator);
        if (!Node) continue;
        Node->Configure(Pair.Key, DisplayName(Pair.Key), Pair.Value);
        RegionActors.Add(Pair.Key, Node);
    }
}

FLinearColor ASoulFounderPlaytestCampaignActor::RegionColor(FName RegionId) const
{
    if (!State) return FLinearColor::Gray;
    const FSoulRegionState* Region = State->World.Regions.Find(RegionId);
    if (!Region) return FLinearColor::Gray;
    if (Region->OwnerFactionId == TEXT("humans")) return FLinearColor(0.12f, 0.35f, 0.95f);
    if(State->IsSixFactionProfile())
    {
        if(Region->OwnerFactionId==TEXT("dwarves"))return FLinearColor(.75f,.52f,.20f);
        if(Region->OwnerFactionId==TEXT("orcs"))return FLinearColor(.75f,.22f,.10f);
        if(Region->OwnerFactionId==TEXT("vikings"))return FLinearColor(.20f,.70f,.85f);
        if(Region->OwnerFactionId==TEXT("nature"))return FLinearColor(.22f,.65f,.28f);
        if(Region->OwnerFactionId==TEXT("dark"))return FLinearColor(.60f,.27f,.70f);
    }
    if (Region->OwnerFactionId == State->EnemyFaction) return FLinearColor(0.85f, 0.15f, 0.10f);
    return FLinearColor(0.35f, 0.35f, 0.35f);
}

void ASoulFounderPlaytestCampaignActor::RefreshRegionVisuals()
{
    if (!State) return;
    for (const TPair<FName, TObjectPtr<ASoulPlaytestRegionActor>>& Pair : RegionActors)
    {
        if (!Pair.Value) continue;
        const bool bVisible = FSoulWorldRules::IsExplored(
            State->World, ViewFaction(), Pair.Key);
        const bool bLegalDestination = bCompanySelected &&
            Pair.Key != State->PlayerRegion &&
            FSoulWorldRules::CanMove(State->World, State->PlayerRegion, Pair.Key) && State->DiplomacyAllowsHostility(State->PlayerFaction,State->World.Regions.FindChecked(Pair.Key).OwnerFactionId);
        Pair.Value->SetVisualState(
            RegionColor(Pair.Key),
            bVisible,
            Pair.Key == State->PlayerRegion,
            FSoulWorldRules::IsVisible(State->World, ViewFaction(), Pair.Key),
            Pair.Key == SelectedRegion || Pair.Key == HoveredRegion || bLegalDestination,
            State->ArmyCountAtRegion(Pair.Key));
    }
}

FName ASoulFounderPlaytestCampaignActor::ViewFaction() const
{return State?(InspectionFaction.IsNone()?State->PlayerFaction:InspectionFaction):NAME_None;}

void ASoulFounderPlaytestCampaignActor::CycleFactionInspection()
{
    if(State && State->IsFourFactionAlpha()){LastMessage=TEXT("Army intelligence is limited to places currently in sight.");return;}
    if(!State||!State->IsSixFactionProfile()||State->HasPendingBattle()||State->bPersistenceBusy)return;
    const auto& Ids=FSoulCampaignRules::CanonicalFactions();
    const int32 Index=InspectionFaction.IsNone()?-1:Ids.IndexOfByKey(InspectionFaction);
    const FName Next=Ids[(Index+1)%Ids.Num()];FSoulFactionCampaignState F;
    if(!State->InspectFactionArmy(Next,F))return;
    CancelPanel();bCompanySelected=false;InspectionFaction=Next;SelectedRegion=F.Army.RegionId;
    LastMessage=State->ArmyInspectionAtRegion(SelectedRegion)+TEXT(" | Read-only inspection. I: next faction, Home: your army.");
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::SelectCompany()
{
    if(!State)return;
    InspectionFaction=NAME_None;CancelPanel();SelectedBattleRegion=NAME_None;SelectedRegion=State->PlayerRegion;bCompanySelected=true;
    int32 Army=0;for(const auto& Unit:State->PlayerArmy)Army+=Unit.Value;
    LastMessage=FString::Printf(TEXT("YOUR ARMY selected: %d troops at %s. Highlighted places cost 1 movement."),Army,*DisplayName(State->PlayerRegion));
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::HandleRegionClicked(FName RegionId)
{
    if(bDiplomacyPanel)return;
    if (!State || bTownPanelOpen) return;
    if(State->IsAlphaTurnActive()){LastMessage=TEXT("Wait for the other factions to finish their turns.");return;}
    if (!FSoulWorldRules::IsExplored(State->World, ViewFaction(), RegionId)) return;
    if(IsFactionInspection())
    {SelectedRegion=RegionId;LastMessage=State->ArmyInspectionAtRegion(RegionId);RefreshRegionVisuals();return;}
    if (RegionId == State->PlayerRegion && bCompanySelected)
    {
        SelectedRegion = RegionId;
        LastMessage = TEXT("YOUR ARMY is selected. Click a highlighted neighbouring place to spend 1 movement.");
        RefreshRegionVisuals();
        return;
    }
    SelectedRegion = RegionId;
    if(bCompanySelected && !State->DiplomacyAllowsHostility(State->PlayerFaction,State->World.Regions.FindChecked(RegionId).OwnerFactionId))
    {LastMessage=TEXT("Peace/treaty: no military access. Use Diplomacy to change relations before entering this territory.");RefreshRegionVisuals();return;}
    SelectedBattleRegion = NAME_None;
    bBattlePromptOpen = false;
    if (RegionId == State->PlayerRegion)
    {
        bCompanySelected = false;
        if (RegionId == State->GetDevelopmentRegion())
        {
            if(State->IsFourFactionAlpha() && !State->CanOpenHumanSettlementServices(LastMessage))return;
            bTownPanelOpen = true;
            LastMessage = TEXT("Settlement panel opened. Choose construction, services or a visit.");
        }
        RefreshRegionVisuals();
        return;
    }

    const FSoulRegionState* Target = State->World.Regions.Find(RegionId);
    if (!Target)
    {
        LastMessage = TEXT("Unknown region.");
        return;
    }

    // A remembered distant location must not disclose current forces through an
    // action rejection message. Adjacent locations are in authoritative sight.
    if (!FSoulWorldRules::CanMove(State->World, State->PlayerRegion, RegionId))
    {
        LastMessage = TEXT("Follow connected roads to reach this place.");
        return;
    }
    if(State->IsFourFactionAlpha() && !Target->OwnerFactionId.IsNone() && !State->IsAlphaActiveFaction(Target->OwnerFactionId))
    {LastMessage=TEXT("Nature and Dark are nonbelligerent in this alpha.");return;}
    if (State->HasHostileGarrison(RegionId))
    {
        if(State->IsFourFactionAlpha() && State->GetPlayerTroopCount()==0)
        {LastMessage=State->HumanRecoveryGuidance();return;}
        SelectedBattleRegion = RegionId;
        bBattlePromptOpen = true;
        LastMessage = FString::Printf(TEXT("%s: press B to commit to battle."), *DisplayName(RegionId));
        return;
    }

    const FName TravelOrigin = State->PlayerRegion;
    const int32 BeforeLevel = State->Hero.Level;
    if (!State->MovePlayerTo(RegionId))
    {
        if(State->IsFourFactionAlpha() && State->GetPlayerTroopCount()==0 && State->Economy.ActionPoints>0)
        {LastMessage=State->HumanRecoveryGuidance();return;}
        LastMessage = State->Economy.ActionPoints <= 0
            ? TEXT("No action points. Press Space to end the day.")
            : TEXT("That region is not an adjacent legal move.");
        return;
    }

    if (WorldPresentation) WorldPresentation->PresentPlayerLocation(RegionId, true);
    bCompanySelected = true;
    SelectedRegion = RegionId;
    LastMessage = FString::Printf(TEXT("Moved to %s. %d movement remaining."),
        *DisplayName(RegionId), State->Economy.ActionPoints);
    if (SoulCampaignTerrain::RouteUsesFerry(TravelOrigin,RegionId))
        LastMessage += TEXT(" Ferry passage included; your company disembarks at the far landing.");
    if (State->Hero.Level > BeforeLevel)
    {
        LastMessage += TEXT(" Level up: choose a skill with 1 Command, 2 Adventure, or 3 Magic.");
    }
    bBattlePromptOpen = false;
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::EndDay()
{
    bDiplomacyPanel=false;
    if (!State) return;
    const int32 PreviousDay = State->Economy.Day;
    State->AdvanceDay();
    if (State->Economy.Day == PreviousDay)
    {
        LastMessage = TEXT("Finish the current battle or save/load before ending the day.");
        return;
    }
    bTownPanelOpen = false;
    bBattlePromptOpen = false;
    LastMessage = TEXT("A new day begins. Income applied; mana recovered.");
    RefreshRegionVisuals();
}

bool ASoulFounderPlaytestCampaignActor::IsSkillChoiceOpen() const
{
    // Town number keys must perform the recruitment action displayed by the panel.
    // Unspent skill choices remain available when the panel closes.
    return State && !bTownPanelOpen && !bDiplomacyPanel && State->Hero.UnspentSkillPoints > 0;
}

bool ASoulFounderPlaytestCampaignActor::IsBattleAvailable() const
{
    return State && bBattlePromptOpen && !SelectedBattleRegion.IsNone() && State->HasHostileGarrison(SelectedBattleRegion)
        && FSoulWorldRules::CanMove(State->World, State->PlayerRegion, SelectedBattleRegion);
}

void ASoulFounderPlaytestCampaignActor::HandleNumberKey(int32 Index)
{
    if(bDiplomacyPanel)return;
    if (!State) return;

    if (IsSkillChoiceOpen() && Index >= 1 && Index <= 3)
    {
        const FName Skill = Index == 1 ? TEXT("Command")
            : Index == 2 ? TEXT("Adventure") : TEXT("Magic");
        if (State->ChooseSkill(Skill))
        {
            LastMessage = FString::Printf(TEXT("%s rank increased."), *Skill.ToString());
        }
        else
        {
            LastMessage = TEXT("That skill cannot be increased.");
        }
        return;
    }

    if (bTownPanelOpen && Index >= 1 && Index <= State->AvailableHumanRoster().Num())
    {
        const TArray<FName>& Roster = State->AvailableHumanRoster();
        const FName UnitId = Roster[Index - 1];
        if (State->Recruit(UnitId))
        {
            const FString UnitName=UnitId.ToString().Replace(TEXT("human_"),TEXT("")).Replace(TEXT("_"),TEXT(" "));
            LastMessage = FString::Printf(TEXT("Recruited 1 %s."), *UnitName);
        }
        else
        {
            LastMessage = TEXT("Recruit failed: pool exhausted or not enough gold.");
        }
    }
}

void ASoulFounderPlaytestCampaignActor::HireTavernHero()
{
    if(bDiplomacyPanel)return;
    if (!State || !bTownPanelOpen) return;
    if (State->HireTavernHero())
    {
        LastMessage = TEXT("Tavern hero hired for 1200 gold.");
    }
    else
    {
        LastMessage = State->bSecondHeroHired
            ? TEXT("The tavern hero is already in your service.")
            : !State->IsTavernOperational() ? TEXT("Complete the tavern before hiring a companion.")
            : TEXT("Need 1200 gold to hire the tavern hero.");
    }
}

void ASoulFounderPlaytestCampaignActor::BuildTavern()
{
    if(bDiplomacyPanel)return;
    if (!State || !bTownPanelOpen || !State->IsSettlementDevelopmentEnabled()) return;
    if (State->BeginSettlementConstruction(State->GetTavernBuildingId(), LastMessage))
        LastMessage = State->GetDevelopmentBuildingName() + TEXT(" construction started. Advance the day to make progress.");
}

void ASoulFounderPlaytestCampaignActor::VisitSettlement()
{
    if(bDiplomacyPanel)return;
    if (!State || !State->IsSettlementDevelopmentEnabled()) return;
    if (!ASoulSettlementVisitGameMode::CanVisit(State, LastMessage)) return;
    const FString Map = State->GetSettlementScenario()->OwnedEnvironmentMap.ToSoftObjectPath().GetLongPackageName();
    UGameplayStatics::OpenLevel(this, FName(*Map), true, TEXT("game=/Script/Soul.SoulSettlementVisitGameMode"));
}

void ASoulFounderPlaytestCampaignActor::ToggleTownPanel()
{
    bDiplomacyPanel=false;
    if(State && State->IsFourFactionAlpha() && !State->CanOpenHumanSettlementServices(LastMessage))
    {bTownPanelOpen=false;return;}
    if (!State || State->PlayerRegion != State->GetDevelopmentRegion())
    {
        LastMessage = TEXT("Return to your settlement to open its services.");
        return;
    }
    bTownPanelOpen = !bTownPanelOpen;
    bBattlePromptOpen = false;
    bCompanySelected = false;
    LastMessage = bTownPanelOpen
        ? TEXT("Settlement panel opened. Choose construction, services or a visit.")
        : TEXT("Capital panel closed.");
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::CancelPanel()
{
    bDiplomacyPanel=false;
    bTownPanelOpen = false;
    bBattlePromptOpen = false;
    bCompanySelected = false;
    LastMessage = TEXT("Back to adventure map.");
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::StartBattle()
{
    if(bDiplomacyPanel)return;
    if(State&&State->IsSixFactionProfile())
    {
        FSoulCampaignBattleDescriptor Descriptor;FString Error;
        if(!State->BuildBattleDescriptor(SelectedBattleRegion,Descriptor,Error)){LastMessage=Error;return;}
    }
    if (!State || !IsBattleAvailable() || !State->BeginBattle(SelectedBattleRegion))
    { LastMessage = TEXT("Battle needs an adjacent hostile force, an army and one action point."); return; }
    LastMessage = TEXT("Deploying to the battlefield...");
    UGameplayStatics::OpenLevel(this, State->PendingBattle.MapPackage, true,
        TEXT("game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"));
}

void ASoulFounderPlaytestCampaignActor::CastTailwind() {}

TArray<FString> ASoulFounderPlaytestCampaignActor::BuildHudLines() const
{
    TArray<FString> Lines;
    if (!State)
    {
        Lines.Add(TEXT("Soul playtest state unavailable."));
        return Lines;
    }
    Lines.Add(State->BuildSummary());
    Lines.Add(FString::Printf(TEXT("Region: %s"), *DisplayName(State->PlayerRegion)));
    int32 HostileRegions = 0;
    for (const auto& Region : State->World.Regions)
        if (State->IsHostile(Region.Key)) ++HostileRegions;
    Lines.Add(HostileRegions > 0
        ? FString::Printf(TEXT("Objective: secure hostile territory | %d regions remain"), HostileRegions)
        : TEXT("All hostile territory secured. Explore, recruit, or end the day to continue."));
    if (!State->LastBattleResult.EncounterId.IsNone())
    {
        const auto& Result = State->LastBattleResult;
        Lines.Add(FString::Printf(TEXT("Last battle: %s at %s | Survivors: %s %d, %s %d"),
            State->IsSixFactionProfile()?(Result.bPlayerWon?TEXT("ATTACKER VICTORY"):TEXT("ATTACKER DEFEAT")):(Result.bPlayerWon?TEXT("VICTORY"):TEXT("DEFEAT")), *DisplayName(Result.TargetRegion),
            State->IsSixFactionProfile()?TEXT("attackers"):TEXT("allied"),Result.PlayerSurvivors,
            State->IsSixFactionProfile()?TEXT("defenders"):TEXT("enemy"),Result.EnemySurvivors));
    }
    if (State->GetPlayerTroopCount() == 0)
        Lines.Add(TEXT("Army lost: return to Human Capital and press T, then 1 to recruit. Space restores actions."));
    if (bBattlePromptOpen && IsBattleAvailable())
        Lines.Add(FString::Printf(TEXT("Selected: %s | %d defenders including reserves | B commits 1 action"),
            *DisplayName(SelectedBattleRegion), State->ArmyCountAtRegion(SelectedBattleRegion)));
    Lines.Add(LastMessage);
    Lines.Add(State->LastPersistenceReport);
    Lines.Add(TEXT("Controls: click region | T town | Space end day | B battle | F5 save | F9 load | Esc close panel"));

    if (IsSkillChoiceOpen())
    {
        Lines.Add(TEXT("SKILL POINT: [1] Command  [2] Adventure  [3] Magic"));
        Lines.Add(FString::Printf(
            TEXT("Ranks â€” Command %d/2 | Adventure %d/2 | Magic %d/2"),
            State->Hero.Skills.FindRef(TEXT("Command")),
            State->Hero.Skills.FindRef(TEXT("Adventure")),
            State->Hero.Skills.FindRef(TEXT("Magic"))));
    }

    if (bTownPanelOpen)
    {
        Lines.Add(TEXT("HUMAN CAPITAL â€” finite recruitment pools"));
        const TArray<FName>& Roster = State->AvailableHumanRoster();
        for (int32 Index = 0; Index < Roster.Num(); ++Index)
        {
            const FSoulRecruitmentPool* Pool = State->Economy.RecruitmentPools.Find(Roster[Index]);
            const int32 ArmyCount = State->PlayerArmy.FindRef(Roster[Index]);
            Lines.Add(FString::Printf(
                TEXT("[%d] %s | pool %d | army %d"),
                Index + 1,
                *Roster[Index].ToString(),
                Pool ? Pool->Available : 0,
                ArmyCount));
        }
        Lines.Add(FString::Printf(
            TEXT("[H] Tavern hero â€” %s"),
            State->bSecondHeroHired ? TEXT("HIRED") : TEXT("1200 gold")));
    }
    return Lines;
}

void ASoulFounderPlaytestCampaignActor::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if (WorldPresentation && State)
    {
        if(ObservedLoadRevision!=State->CampaignLoadRevision)
        {
            ObservedLoadRevision=State->CampaignLoadRevision;
            InspectionFaction=NAME_None;CancelPanel();SelectedBattleRegion=NAME_None;HoveredRegion=NAME_None;
            SelectedRegion=State->PlayerRegion;
            LastMessage=TEXT("Campaign restored. Continue from the saved company location.");
            WorldPresentation->PresentPlayerLocation(State->PlayerRegion,false,true);
            if(auto* PC=GetWorld()->GetFirstPlayerController())
                if(auto* Camera=Cast<ASoulCampaignCamera>(PC->GetViewTarget()))Camera->Focus(RegionPositions().FindRef(State->PlayerRegion));
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_LOAD_PRESENTED region=%s revision=%u"),*State->PlayerRegion.ToString(),ObservedLoadRevision);
        }
        WorldPresentation->RefreshKnowledge();
        WorldPresentation->PresentPlayerLocation(State->PlayerRegion, false);
    }
    RefreshRegionVisuals();
}

FName ASoulFounderPlaytestCampaignActor::CurrentRegion() const { return State ? State->PlayerRegion : NAME_None; }

void ASoulFounderPlaytestCampaignActor::ToggleDiplomacy()
{
 if(!State||!State->IsHeartlandEnabled())return;
 const bool Open=!bDiplomacyPanel;CancelPanel();bDiplomacyPanel=Open;
 LastMessage=Open?TEXT("Diplomacy: accepted actions cost 1 movement. Peace gives no military access."):TEXT("Back to adventure map.");
}
