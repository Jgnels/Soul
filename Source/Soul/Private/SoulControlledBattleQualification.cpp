#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

// Opt-in observer only. Staging armies uses normal canonical moves, captures and days.
// Battle winner/damage remain entirely RBCombat-owned.
void ASoulFounderPlaytestGameMode::TickControlledBattleQualification(float Seconds)
{
    if(!State || !Campaign)return;
    Elapsed+=Seconds;auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    auto Fail=[&](const FString& Why){UE_LOG(LogTemp,Error,TEXT("SOUL_CONTROLLED_BATTLE_FAIL %s"),*Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    if(!State->bInitialized || !State->IsSixFactionProfile()){Fail(TEXT("requires isolated six-faction state"));return;}
    if(Elapsed>160){Fail(TEXT("campaign observer timeout"));return;}
    if(Elapsed<8 || State->bPersistenceBusy)return;
    FString Name;FParse::Value(FCommandLine::Get(),TEXT("SoulControlledBattle="),Name);FName Attacker=Name==TEXT("human_nature")?FName(TEXT("humans")):FName(*Name);
    FName Defender=Name==TEXT("human_nature")?FName(TEXT("nature")):FName(TEXT("humans"));
    if(Name==TEXT("dwarves_orcs")){Attacker=TEXT("dwarves");Defender=TEXT("orcs");}
    if(Name==TEXT("orcs_dwarves")){Attacker=TEXT("orcs");Defender=TEXT("dwarves");}
    if(Name==TEXT("dwarves_vikings")){Attacker=TEXT("dwarves");Defender=TEXT("vikings");}
    if(Name==TEXT("vikings_dwarves")){Attacker=TEXT("vikings");Defender=TEXT("dwarves");}
    if(Name==TEXT("orcs_vikings")){Attacker=TEXT("orcs");Defender=TEXT("vikings");}
    if(Name==TEXT("vikings_orcs")){Attacker=TEXT("vikings");Defender=TEXT("orcs");}

    const FName Recipe=(Attacker==TEXT("nature")||Defender==TEXT("nature"))?FName(TEXT("dragon_watch")):FName(TEXT("dragon_pass"));
    TArray<FName> DefenderPath,AttackerPath;FName Target;
    if(Name==TEXT("dwarves_orcs"))
    {DefenderPath={TEXT("orc_camp"),TEXT("north_pass")};AttackerPath={TEXT("dwarf_hold"),TEXT("dwarf_forge_approach")};Target=TEXT("north_pass");}
    else if(Name==TEXT("orcs_dwarves"))
    {DefenderPath={TEXT("dwarf_hold"),TEXT("dwarf_forge_approach"),TEXT("north_pass")};AttackerPath={TEXT("orc_camp")};Target=TEXT("north_pass");}
    else if(Name==TEXT("dwarves_vikings"))
    {DefenderPath={TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass")};AttackerPath={TEXT("dwarf_hold"),TEXT("dwarf_forge_approach"),TEXT("north_pass")};Target=TEXT("dwarf_mountain_pass");}
    else if(Name==TEXT("vikings_dwarves"))
    {DefenderPath={TEXT("dwarf_hold"),TEXT("dwarf_forge_approach"),TEXT("north_pass"),TEXT("dwarf_mountain_pass")};AttackerPath={TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine")};Target=TEXT("dwarf_mountain_pass");}
    else if(Name==TEXT("orcs_vikings"))
    {DefenderPath={TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass"),TEXT("north_pass")};AttackerPath={TEXT("orc_camp")};Target=TEXT("north_pass");}
    else if(Name==TEXT("vikings_orcs"))
    {DefenderPath={TEXT("orc_camp"),TEXT("north_pass")};AttackerPath={TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass")};Target=TEXT("north_pass");}
    else if(Attacker==TEXT("dwarves"))
    {DefenderPath={TEXT("human_capital"),TEXT("crossroads"),TEXT("forest_edge"),TEXT("north_pass"),TEXT("dwarf_mountain_pass")};AttackerPath={TEXT("dwarf_hold"),TEXT("dwarf_forge_approach"),TEXT("dwarf_high_quarry")};Target=TEXT("dwarf_mountain_pass");}
    else if(Attacker==TEXT("orcs"))
    {DefenderPath={TEXT("human_capital"),TEXT("crossroads"),TEXT("forest_edge"),TEXT("north_pass")};AttackerPath={TEXT("orc_camp")};Target=TEXT("north_pass");}
    else if(Attacker==TEXT("vikings"))
    {DefenderPath={TEXT("human_capital"),TEXT("crossroads"),TEXT("old_quarry"),TEXT("ancient_shrine"),TEXT("mountain_shrine"),TEXT("viking_snow_pass")};AttackerPath={TEXT("viking_harbour"),TEXT("viking_fjord_ridge")};Target=TEXT("viking_snow_pass");}
    else if(Attacker==TEXT("nature"))
    {DefenderPath={TEXT("human_capital"),TEXT("crossroads"),TEXT("river_ford"),TEXT("orc_watch")};AttackerPath={TEXT("nature_treehold"),TEXT("nature_river_woodland"),TEXT("nature_grassland_edge"),TEXT("southern_crossing"),TEXT("orc_broken_bridge"),TEXT("orc_ruined_field")};Target=TEXT("orc_watch");}
    else if(Defender==TEXT("nature"))
    {DefenderPath={TEXT("nature_treehold"),TEXT("nature_river_woodland"),TEXT("nature_grassland_edge"),TEXT("southern_crossing"),TEXT("orc_broken_bridge"),TEXT("orc_ruined_field"),TEXT("orc_watch")};AttackerPath={TEXT("human_capital"),TEXT("crossroads"),TEXT("river_ford")};Target=TEXT("orc_watch");}
    else {Fail(TEXT("unadmitted proof faction"));return;}
    FString Error;
    if(State->LastBattleResult.EncounterId.IsNone())
    {
        if(bStarted)return;bStarted=true;
        auto Travel=[&](FName Id,const TArray<FName>& Path)
        {
            FSoulFactionCampaignState F;State->InspectFactionArmy(Id,F);
            if(F.Army.RegionId!=Path[0])return false;
            for(int32 I=1;I<Path.Num();++I)
            {
                State->InspectFactionArmy(Id,F);
                if(F.Economy.ActionPoints<1){State->AdvanceDay();State->InspectFactionArmy(Id,F);}
                FSoulControlledCampaignAction A;
                if(!State->PrepareControlledAction(Id,F.Army.ArmyId,F.Army.RegionId,Path[I],A,Error)
                    || !State->ExecuteControlledAction(A,Error))return false;
                UE_LOG(LogTemp,Display,TEXT("SOUL_CONTROLLED_TRAVEL faction=%s from=%s to=%s"),*Id.ToString(),*F.Army.RegionId.ToString(),*Path[I].ToString());
            }
            return true;
        };
        if(!Travel(Defender,DefenderPath)||!Travel(Attacker,AttackerPath)){Fail(TEXT("legal staging: ")+Error);return;}
        FSoulFactionCampaignState F;State->InspectFactionArmy(Attacker,F);
        if(F.Economy.ActionPoints<1){State->AdvanceDay();State->InspectFactionArmy(Attacker,F);}
        FSoulControlledCampaignAction A;
        if(!State->PrepareControlledAction(Attacker,F.Army.ArmyId,F.Army.RegionId,Target,A,Error)
            || !State->ExecuteControlledAction(A,Error)){Fail(Error);return;}
        const auto& D=State->PendingBattle;
        if(D.PlayerFaction!=Attacker || D.EnemyFaction!=Defender || D.PlayerUnitId!=F.Army.UnitId
            || D.EnemyUnitId!=FSoulCampaignRules::AdmittedStrategicUnit(Defender) || D.BattlefieldId!=Recipe)
        {Fail(TEXT("exact directed battle descriptor"));return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_CONTROLLED_BATTLE_BEGIN attacker=%s defender=%s source=%s target=%s recipe=%s approach=%s"),
            *Attacker.ToString(),*Defender.ToString(),*D.SourceRegion.ToString(),*Target.ToString(),*D.BattlefieldId.ToString(),*D.BattleContext.AttackerApproach.ToString());
        UGameplayStatics::OpenLevel(this,D.MapPackage,true,TEXT("game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"));return;
    }
    const auto& R=State->LastBattleResult;
    if(VisualStep==0)
    {
        FSoulFactionCampaignState A,H;State->InspectFactionArmy(Attacker,A);State->InspectFactionArmy(Defender,H);
        const bool Valid=R.TargetRegion==Target && State->ResolvedEncounters.Contains(R.EncounterId) && !State->HasPendingBattle()
            && A.Army.RegionId==(R.bPlayerWon?Target:AttackerPath.Last()) && H.Army.RegionId==Target
            && A.Army.TroopCount==R.PlayerSurvivors && H.Army.TroopCount==R.EnemySurvivors
            && State->World.Regions[Target].OwnerFactionId==(R.bPlayerWon?Attacker:Defender)
            && (R.MagicCasts==0 || FParse::Param(FCommandLine::Get(),TEXT("SoulHumanDefenseProof"))) && State->bLastSaveSucceeded;
        if(!Valid){Fail(TEXT("directional result/ownership/army accounting"));return;}
        FRBSaveDomainState Snapshot;if(!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)){Fail(Error);return;}
        ExpectedSnapshot=Snapshot.Fields[0].StringValue;
        if(!FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")))){Fail(TEXT("snapshot evidence write"));return;}
        Key(EKeys::F5);VisualStep=1;return;
    }
    if(VisualStep==1)
    {
        if(!State->bLastSaveSucceeded){Fail(TEXT("F5"));return;}
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Controlled_Return.png"),true,false);
        State->AdvanceDay();Key(EKeys::F9);VisualStep=2;return;
    }
    if(VisualStep==2)
    {
        FRBSaveDomainState Loaded;
        if(!State->bLastLoadSucceeded || !State->CaptureRBSaveDomain_Implementation(Loaded,Error) || Loaded.Fields[0].StringValue!=ExpectedSnapshot)
        {Fail(TEXT("F9 exact six-faction restoration"));return;}
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Controlled_Restored.png"),true,false);
        UE_LOG(LogTemp,Display,TEXT("SOUL_CONTROLLED_BATTLE_PASS attacker=%s defender=%s target=%s victory=%d survivors=%d/%d F5=1 F9=1 exact_snapshot=1 AI=OFF"),
            *Attacker.ToString(),*Defender.ToString(),*Target.ToString(),R.bPlayerWon,R.PlayerSurvivors,R.EnemySurvivors);
        VisualStep=3;return;
    }
    if(VisualStep==3 && Elapsed>=75){bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}
}
