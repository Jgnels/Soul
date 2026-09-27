#include "SoulFounderPlaytestCampaignActor.h"

#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulPlaytestRegionActor.h"

ASoulFounderPlaytestCampaignActor::ASoulFounderPlaytestCampaignActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f;
}

const TMap<FName, FVector>& ASoulFounderPlaytestCampaignActor::RegionPositions()
{
    static const TMap<FName, FVector> Positions = {
        {TEXT("human_capital"), FVector(-3000, 0, 50)},
        {TEXT("crossroads"), FVector(-1600, 0, 50)},
        {TEXT("old_quarry"), FVector(-700, -1500, 50)},
        {TEXT("river_ford"), FVector(-200, 1200, 50)},
        {TEXT("forest_edge"), FVector(200, -500, 50)},
        {TEXT("ancient_shrine"), FVector(700, -2100, 50)},
        {TEXT("orc_watch"), FVector(1600, 800, 50)},
        {TEXT("north_pass"), FVector(1700, -1300, 50)},
        {TEXT("orc_camp"), FVector(3100, 0, 50)}
    };
    return Positions;
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
    SpawnRegions();
    DrawConnections();
    RefreshRegionVisuals();
}

FString ASoulFounderPlaytestCampaignActor::DisplayName(FName RegionId) const
{
    if (State) if (const FString* Name = State->RegionDisplayNames.Find(RegionId)) return *Name;
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
            State->World, TEXT("humans"), Pair.Key);
        Pair.Value->SetVisualState(
            RegionColor(Pair.Key),
            bVisible,
            Pair.Key == State->PlayerRegion);
    }
}

void ASoulFounderPlaytestCampaignActor::DrawConnections()
{
    if (!State || !GetWorld()) return;
    TSet<FString> Seen;
    for (const TPair<FName, FSoulRegionState>& Pair : State->World.Regions)
    {
        const FVector* A = RegionPositions().Find(Pair.Key);
        if (!A) continue;
        for (FName Neighbor : Pair.Value.Neighbors)
        {
            const FVector* B = RegionPositions().Find(Neighbor);
            if (!B) continue;
            const FString KA = Pair.Key.ToString();
            const FString KB = Neighbor.ToString();
            const FString Key = KA < KB ? KA + TEXT("|") + KB : KB + TEXT("|") + KA;
            if (Seen.Contains(Key)) continue;
            Seen.Add(Key);
            DrawDebugLine(
                GetWorld(), *A + FVector(0,0,20), *B + FVector(0,0,20),
                FColor(130,130,130), true, -1.0f, 0, 14.0f);
        }
    }
}

void ASoulFounderPlaytestCampaignActor::HandleRegionClicked(FName RegionId)
{
    if (!State || bTownPanelOpen) return;
    if (RegionId == State->PlayerRegion)
    {
        if (RegionId == TEXT("human_capital"))
        {
            bTownPanelOpen = true;
            LastMessage = TEXT("Capital panel opened. [1] recruits a Knight; [H] hires a tavern hero.");
        }
        return;
    }

    const FSoulRegionState* Target = State->World.Regions.Find(RegionId);
    if (!Target)
    {
        LastMessage = TEXT("Unknown region.");
        return;
    }

    if (State->HasHostileGarrison(RegionId))
    {
        if (!FSoulWorldRules::CanMove(State->World, State->PlayerRegion, RegionId))
        {
            LastMessage = TEXT("Enemy stronghold is not adjacent.");
            return;
        }
        SelectedBattleRegion = RegionId;
        bBattlePromptOpen = true;
        LastMessage = FString::Printf(TEXT("%s: press B to commit to battle."), *DisplayName(RegionId));
        return;
    }

    const int32 BeforeLevel = State->Hero.Level;
    if (!State->MovePlayerTo(RegionId))
    {
        LastMessage = State->Economy.ActionPoints <= 0
            ? TEXT("No action points. Press Space to end the day.")
            : TEXT("That region is not an adjacent legal move.");
        return;
    }

    LastMessage = FString::Printf(TEXT("Moved to %s."), *DisplayName(RegionId));
    if (State->Hero.Level > BeforeLevel)
    {
        LastMessage += TEXT(" Level up: choose a skill with 1 Command, 2 Adventure, or 3 Magic.");
    }
    bBattlePromptOpen = false;
    RefreshRegionVisuals();
}

void ASoulFounderPlaytestCampaignActor::EndDay()
{
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
    return State && !bTownPanelOpen && State->Hero.UnspentSkillPoints > 0;
}

bool ASoulFounderPlaytestCampaignActor::IsBattleAvailable() const
{
    return State && !SelectedBattleRegion.IsNone() && State->HasHostileGarrison(SelectedBattleRegion)
        && FSoulWorldRules::CanMove(State->World, State->PlayerRegion, SelectedBattleRegion);
}

void ASoulFounderPlaytestCampaignActor::HandleNumberKey(int32 Index)
{
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

    if (bTownPanelOpen && Index >= 1 && Index <= USoulFounderPlaytestStateSubsystem::HumanPlaytestRoster().Num())
    {
        const TArray<FName>& Roster = USoulFounderPlaytestStateSubsystem::HumanPlaytestRoster();
        const FName UnitId = Roster[Index - 1];
        if (State->Recruit(UnitId))
        {
            LastMessage = FString::Printf(TEXT("Recruited 1 %s."), *UnitId.ToString());
        }
        else
        {
            LastMessage = TEXT("Recruit failed: pool exhausted or not enough gold.");
        }
    }
}

void ASoulFounderPlaytestCampaignActor::HireTavernHero()
{
    if (!State || !bTownPanelOpen) return;
    if (State->HireTavernHero())
    {
        LastMessage = TEXT("Tavern hero hired for 1200 gold.");
    }
    else
    {
        LastMessage = State->bSecondHeroHired
            ? TEXT("The tavern hero is already in your service.")
            : TEXT("Need 1200 gold to hire the tavern hero.");
    }
}

void ASoulFounderPlaytestCampaignActor::ToggleTownPanel()
{
    if (!State || State->PlayerRegion != TEXT("human_capital"))
    {
        LastMessage = TEXT("Return to the Human Capital to recruit.");
        return;
    }
    bTownPanelOpen = !bTownPanelOpen;
    bBattlePromptOpen = false;
    LastMessage = bTownPanelOpen
        ? TEXT("Capital panel opened. [1] recruits a Knight; [H] hires a tavern hero.")
        : TEXT("Capital panel closed.");
}

void ASoulFounderPlaytestCampaignActor::CancelPanel()
{
    bTownPanelOpen = false;
    bBattlePromptOpen = false;
    LastMessage = TEXT("Back to adventure map.");
}

void ASoulFounderPlaytestCampaignActor::StartBattle()
{
    if (!State || !IsBattleAvailable() || !State->BeginBattle(SelectedBattleRegion))
    { LastMessage = TEXT("Battle needs an adjacent hostile force, an army and one action point."); return; }
    LastMessage = TEXT("Entering Dragon Graveyard...");
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
        Lines.Add(FString::Printf(TEXT("Last battle: %s at %s | Survivors: allied %d, enemy %d"),
            Result.bPlayerWon ? TEXT("VICTORY") : TEXT("DEFEAT"), *DisplayName(Result.TargetRegion),
            Result.PlayerSurvivors, Result.EnemySurvivors));
    }
    if (State->PlayerArmy.FindRef(State->PlayerUnitId) == 0)
        Lines.Add(TEXT("Army lost: return to Human Capital and press T, then 1 to recruit. Space restores actions."));
    if (bBattlePromptOpen && IsBattleAvailable())
        Lines.Add(FString::Printf(TEXT("Selected: %s | %d defenders including reserves | B commits 1 action"),
            *DisplayName(SelectedBattleRegion), State->EnemyArmies.FindRef(SelectedBattleRegion)));
    Lines.Add(LastMessage);
    Lines.Add(State->LastPersistenceReport);
    Lines.Add(TEXT("Controls: click region | T town | Space end day | B battle | F5 save | F9 load | Esc close panel"));

    if (IsSkillChoiceOpen())
    {
        Lines.Add(TEXT("SKILL POINT: [1] Command  [2] Adventure  [3] Magic"));
        Lines.Add(FString::Printf(
            TEXT("Ranks — Command %d/2 | Adventure %d/2 | Magic %d/2"),
            State->Hero.Skills.FindRef(TEXT("Command")),
            State->Hero.Skills.FindRef(TEXT("Adventure")),
            State->Hero.Skills.FindRef(TEXT("Magic"))));
    }

    if (bTownPanelOpen)
    {
        Lines.Add(TEXT("HUMAN CAPITAL — finite recruitment pools"));
        const TArray<FName>& Roster = USoulFounderPlaytestStateSubsystem::HumanPlaytestRoster();
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
            TEXT("[H] Tavern hero — %s"),
            State->bSecondHeroHired ? TEXT("HIRED") : TEXT("1200 gold")));
    }
    return Lines;
}

void ASoulFounderPlaytestCampaignActor::Tick(float Seconds)
{
    Super::Tick(Seconds);
    RefreshRegionVisuals();
}
