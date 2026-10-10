#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
void ASoulFounderPlaytestGameMode::TickHeartlandQualification(float Seconds)
{
    if(!State||!Campaign||!State->bInitialized||State->bPersistenceBusy||State->IsAlphaTurnActive())return;
#if WITH_EDITOR
    if(FAssetCompilingManager::Get().GetNumRemainingAssets()>0)return;
#endif
    Elapsed+=Seconds;
    auto Fail=[&](const TCHAR* Why){UE_LOG(LogTemp,Error,TEXT("SOUL_HEARTLAND_FAIL %s"),Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
    const FString Root=FPaths::ProjectSavedDir();
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandVisitLoadQualification")))
    {
        if(Elapsed<8||bLoading)return;
        auto* PC=GetWorld()->GetFirstPlayerController();
        const FString Entered=Root/TEXT("HeartlandVisitRestoreEntered.txt");
        if(!IFileManager::Get().FileExists(*Entered))
        {
            if(VisualStep==0)
            {
                Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("crossroads"));
                FRBSaveDomainState Snapshot;FString Error;
                if(State->PlayerRegion!=TEXT("crossroads")||!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)){Fail(TEXT("visit-load remote checkpoint"));return;}
                FFileHelper::SaveStringToFile(Snapshot.Fields[0].StringValue,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));
                PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5,IE_Released,0));VisualStep=1;return;
            }
            if(!State->bLastSaveSucceeded){Fail(TEXT("visit-load F5"));return;}
            Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("human_capital"));
            if(State->PlayerRegion!=TEXT("human_capital")){Fail(TEXT("visit-load paid return to city"));return;}
            FFileHelper::SaveStringToFile(TEXT("city entry requested"),*Entered);bLoading=true;Campaign->VisitSettlement();return;
        }
        FRBSaveDomainState Snapshot;FString Error,Expected;
        if(!State->bLastLoadSucceeded||State->PlayerRegion!=TEXT("crossroads")
            ||!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)
            ||!FFileHelper::LoadFileToString(Expected,*(Root/TEXT("CampaignInputExpectedSnapshot.json")))
            ||Snapshot.Fields[0].StringValue!=Expected){Fail(TEXT("visit-load restored state or return mismatch"));return;}
        if(VisualStep==0){FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Heartland_Visit_Remote_Restore.png"),true,false);VisualStep=1;}
        if(Elapsed>=75){UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_VISIT_RESTORE_PASS region=crossroads exact_snapshot=1 input_F9=1 campaign_return=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandBattleQualification")))
    {
        if(Elapsed<8)return;
        const FName Attacker=FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandCommanderQualification"))?FName(TEXT("dwarves")):FName(TEXT("orcs"));
        if(State->LastBattleResult.EncounterId.IsNone())
        {
            if(bStarted)return;bStarted=true;FString Error;FSoulControlledCampaignAction Action;
            if(!State->PrepareControlledAction(Attacker,FName(*(Attacker.ToString()+TEXT(".primary"))),TEXT("crossroads"),TEXT("human_capital"),Action,Error)
                ||!State->ExecuteControlledAction(Action,Error)){UE_LOG(LogTemp,Error,TEXT("SOUL_HEARTLAND_ADMISSION %s"),*Error);Fail(TEXT("capital defense admission"));return;}
            const auto& D=State->PendingBattle;
            if(D.MapPackage!=TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored")||D.TacticalPlayerSide!=1){Fail(TEXT("wrong city or player side"));return;}
            UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_CITY_BATTLE_BEGIN attacker=%s defender=humans map=%s origin=%s"),*Attacker.ToString(),*D.MapPackage.ToString(),*D.ArenaOrigin.ToString());
            UGameplayStatics::OpenLevel(this,D.MapPackage,true,TEXT("game=/Script/SoulRealtimeBattle.SoulRealtimeArenaGameMode"));return;
        }
        const auto& Result=State->LastBattleResult;
        if(Result.TargetRegion!=TEXT("human_capital")||State->PlayerRegion!=TEXT("human_capital")||State->HasPendingBattle()
           ||State->World.Regions.FindChecked(TEXT("human_capital")).OwnerFactionId!=(Result.bPlayerWon?Attacker:FName(TEXT("humans")))){Fail(TEXT("capital battle result/return"));return;}
        auto* PC=GetWorld()->GetFirstPlayerController();
        auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
        FRBSaveDomainState Snapshot;FString Error;
        if(!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)){Fail(TEXT("post-city-battle save capture"));return;}
        if(VisualStep==0){ExpectedSnapshot=Snapshot.Fields[0].StringValue;FFileHelper::SaveStringToFile(ExpectedSnapshot,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));Key(EKeys::F5);VisualStep=1;return;}
        if(VisualStep==1){if(!State->bLastSaveSucceeded){Fail(TEXT("city battle F5"));return;}Key(EKeys::F9);VisualStep=2;return;}
        if(VisualStep==2){if(!State->bLastLoadSucceeded||Snapshot.Fields[0].StringValue!=ExpectedSnapshot){Fail(TEXT("city battle F9"));return;}FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Heartland_City_Battle_Return.png"),true,false);VisualStep=3;}
        if(Elapsed>=75){UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_CITY_BATTLE_PASS natural_result=1 attacker_won=%d survivors=%d/%d return=1 F5=1 F9=1"),Result.bPlayerWon,Result.PlayerSurvivors,Result.EnemySurvivors);bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}return;
    }
    if(IFileManager::Get().FileExists(*(Root/TEXT("HeartlandVisitDone.txt"))))
    {
        if(!bStarted)
        {
            bStarted=true;if(State->PlayerRegion!=TEXT("human_capital")||!State->Hero.KnownSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard"))){Fail(TEXT("visit return lost campaign state"));return;}
            Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("crossroads"));
            FString Message;if(State->PlayerRegion!=TEXT("crossroads")||!State->InteractHeartlandSite(Message)){Fail(TEXT("normal travel/site interaction"));return;}
            Campaign->LastMessage=Message;
            FRBSaveDomainState Snapshot;FString Error;if(!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)){Fail(TEXT("snapshot"));return;}
            FFileHelper::SaveStringToFile(Snapshot.Fields[0].StringValue,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));
            auto* PC=GetWorld()->GetFirstPlayerController();PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::F5,IE_Released,0));
            FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Heartland_Site_Save.png"),true,false);return;
        }
        if(Elapsed>=75){if(!State->bLastSaveSucceeded){Fail(TEXT("remote F5"));return;}UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_PASS city_walk_return=1 frost_learned=1 site=1 remote_F5=1 region=%s"),*State->PlayerRegion.ToString());bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}return;
    }
    if(!bStarted&&Elapsed>4)
    {
        bStarted=true;FString Error;if(!State->IsHeartlandEnabled()||!State->BeginSettlementConstruction(TEXT("human.arcane_hall"),Error)){Fail(TEXT("paid Arcane Hall construction"));return;}State->AdvanceDay();return;
    }
    if(bStarted&&Elapsed>10&&!bLoading)
    {
        if(!State->Hero.KnownSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard"))){Fail(TEXT("one-day guild spell learning"));return;}
        if(auto* Camera=Cast<ASoulCampaignCamera>(GetWorld()->GetFirstPlayerController()->GetViewTarget())){Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("human_capital")));Camera->Orbit(1,2);Camera->Zoom(8);}
        Campaign->ToggleTownPanel();bLoading=true;return;
    }
    if(bLoading&&Elapsed>13&&VisualStep==0){FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Heartland_Guild_Orbit.png"),true,false);VisualStep=1;return;}
    if(bLoading&&Elapsed>17&&VisualStep==1){Campaign->CancelPanel();VisualStep=2;return;}
    if(bLoading&&Elapsed>20&&VisualStep==2){FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Heartland_Capital_Orbit.png"),true,false);VisualStep=3;return;}
    if(bLoading&&Elapsed>24)Campaign->VisitSettlement();
}
