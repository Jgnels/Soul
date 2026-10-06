#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulCampaignTerrain.h"
#include "RBSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "InputKeyEventArgs.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Opt-in functional proof only. All actions enter through normal controller input;
// snapshots and subsystem reads are assertions, never replacement game commands.
void ASoulFounderPlaytestGameMode::TickSettlementDevelopmentQualification(float Seconds)
{
    const double Now=FPlatformTime::Seconds();
    if(DevelopmentQualificationStart==0)DevelopmentQualificationStart=Now;
    auto Fail=[&](const FString& Reason)
    {
        UE_LOG(LogTemp,Error,TEXT("SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_FAIL step=%d reason=%s authored_environment=0 miniature=0 battle_environment=0"),VisualStep,*Reason);
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);
    };
    auto Require=[&](bool Condition,const TCHAR* Label)
    {
        if(!Condition){Fail(Label);return false;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_CHECK_PASS step=%d %s"),VisualStep,Label);return true;
    };
    if(Now-DevelopmentQualificationStart>180){Fail(TEXT("180-second controls qualification timeout"));return;}
    if(!State||!Campaign||!GetGameInstance()){Fail(TEXT("campaign or GameInstance missing"));return;}
    auto* PC=Cast<ASoulFounderPlaytestPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* Authority=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
    auto* Save=GetGameInstance()->GetSubsystem<URBSaveSubsystem>();
    if(!PC||!Authority||!Save){Fail(TEXT("controller, settlement authority or RBSave subsystem missing"));return;}
    const FString SavePath=Save->GetDomainSlotPath(TEXT("Soul.VerticalCampaign"));
    const FString CaptureRoot=FPaths::ProjectSavedDir()/TEXT("Screenshots");
    auto Path=[&](const TCHAR* Label){return CaptureRoot/(CapturePrefix+TEXT("_")+Label+TEXT(".png"));};
    const TCHAR* Shots[]={TEXT("start"),TEXT("mid"),TEXT("completed"),TEXT("loaded"),TEXT("hired"),TEXT("loaded_completed")};
    auto Capture=[&](const TCHAR* Label){FScreenshotRequest::RequestScreenshot(Path(Label),true,false);};
    auto Key=[&](FKey K)
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_INPUT step=%d key=%s"),VisualStep,*K.ToString());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));
    };
    auto FindHUD=[&](FName Name,FVector2D& Point)
    {
        auto* HUD=PC->GetHUD();int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
        for(int32 Y=4;HUD&&Y<Height;Y+=8)for(int32 X=4;X<Width;X+=8)
            if(const auto* Hit=HUD->GetHitBoxAtCoordinates(FVector2D(X,Y),true))
                if(Hit->GetName()==Name){Point=FVector2D(X,Y);return true;}
        return false;
    };
    auto ClickHUD=[&](FName Name)
    {
        FVector2D Point;
        if(!Require(FindHUD(Name,Point),TEXT("requested action has an actual rendered HUD hitbox")))return false;
        PC->SetMouseLocation(FMath::RoundToInt(Point.X),FMath::RoundToInt(Point.Y));Key(EKeys::LeftMouseButton);return true;
    };
    auto Snapshots=[&](FString& CampaignJson,FString& SettlementJson)
    {
        FRBSaveDomainState C,S;FString Error;
        if(!State->CaptureRBSaveDomain_Implementation(C,Error)||!Authority->CaptureRBSaveDomain_Implementation(S,Error))
        {Fail(TEXT("domain capture: ")+Error);return false;}
        if(!Require(C.DomainId==TEXT("Soul.Campaign")&&S.DomainId==TEXT("Soul.Settlements")
            &&C.Fields.Num()==1&&S.Fields.Num()==1,TEXT("both actual RBSave domain snapshots captured")))return false;
        CampaignJson=C.Fields[0].StringValue;SettlementJson=S.Fields[0].StringValue;return true;
    };
    auto Remember=[&](){return Snapshots(ExpectedSnapshot,DevelopmentSettlementSnapshot);};
    auto Matches=[&](const TCHAR* Label)
    {
        FString C,S;if(!Snapshots(C,S))return false;
        return Require(C==ExpectedSnapshot&&S==DevelopmentSettlementSnapshot,Label);
    };
    auto PersistExpected=[&](const TCHAR* Label)
    {
        const FString Base=FPaths::ProjectSavedDir()/(CapturePrefix+TEXT("_")+Label);
        return Require(FFileHelper::SaveStringToFile(ExpectedSnapshot,*(Base+TEXT("_Soul.Campaign.json")))
            &&FFileHelper::SaveStringToFile(DevelopmentSettlementSnapshot,*(Base+TEXT("_Soul.Settlements.json"))),TEXT("both expected snapshots written to isolated run evidence"));
    };
    auto StartSave=[&]()
    {
        DevelopmentSaveBefore.Reset();
        if(IFileManager::Get().FileExists(*SavePath)&&!FFileHelper::LoadFileToArray(DevelopmentSaveBefore,*SavePath))
        {Fail(TEXT("cannot read existing qualification checkpoint before F5"));return false;}
        bDevelopmentObservedPersistenceBusy=false;Key(EKeys::F5);return true;
    };
    auto Saved=[&]()
    {
        TArray<uint8> After;
        return Require(State->bLastSaveSucceeded&&!State->bPersistenceBusy&&FFileHelper::LoadFileToArray(After,*SavePath)
            &&After.Num()>0&&After!=DevelopmentSaveBefore,TEXT("new F5 operation completed and changed actual RBSave checkpoint bytes"));
    };
    auto StartLoad=[&]()
    {
        DevelopmentExpectedLoadRevision=State->CampaignLoadRevision+1;
        DevelopmentExpectedStateRevision=State->SettlementDevelopmentRevision+1;
        bDevelopmentObservedPersistenceBusy=false;Key(EKeys::F9);
    };
    auto Loaded=[&]()
    {
        return Require(State->bLastLoadSucceeded&&!State->bPersistenceBusy
            &&State->CampaignLoadRevision==DevelopmentExpectedLoadRevision
            &&State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision
            &&State->IsSettlementDevelopmentReady(),TEXT("new F9 callback completed with exact load and settlement revisions"));
    };

    if(!bStarted)
    {
        const TCHAR* Cmd=FCommandLine::Get();FString RawPrefix,UserDir;
        // Match current terrain flag semantics without depending on unreleased review APIs.
        const bool WorldTerrain=FParse::Param(Cmd,TEXT("SoulWorldTerrain"));
        const bool WorldReview=WorldTerrain&&(FParse::Param(Cmd,TEXT("SoulWorldCapture"))
            ||FParse::Param(Cmd,TEXT("SoulCampaignWorldCapture")));
        const bool RetainedReview=!WorldTerrain&&FParse::Param(Cmd,TEXT("SoulEvilCorridor"))
            &&FParse::Param(Cmd,TEXT("SoulRetainedCapture"));
        if(!Require(FParse::Param(Cmd,TEXT("SoulSettlementDevelopmentProof"))
            &&!bVisualQualification&&!bQualification&&!FParse::Param(Cmd,TEXT("SoulCampaignLoadProof"))
            &&!FParse::Param(Cmd,TEXT("SoulTerrainBenchmark"))&&!WorldReview&&!RetainedReview,TEXT("explicit development proof enabled without competing qualification modes")))return;
        if(!Require(SoulCampaignTerrain::EvilCorridor()&&!WorldTerrain,TEXT("controls proof uses unchanged retained EvilCorridor terrain")))return;
        if(!Require(FParse::Value(Cmd,TEXT("SoulCampaignCapturePrefix="),RawPrefix)&&!RawPrefix.IsEmpty()
            &&RawPrefix==FPaths::MakeValidFileName(RawPrefix)&&RawPrefix!=TEXT("World"),TEXT("explicit valid unique capture prefix required")))return;
        if(!Require(FParse::Value(Cmd,TEXT("UserDir="),UserDir)&&!FPaths::IsRelative(UserDir)
            &&FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),FPaths::ConvertRelativePathToFull(UserDir)),TEXT("isolated absolute UserDir owns this run's Saved directory")))return;
        TArray<FString> Existing;IFileManager::Get().FindFiles(Existing,*(CaptureRoot/(CapturePrefix+TEXT("_*.png"))),true,false);
        const FString StartedPath=FPaths::ProjectSavedDir()/(CapturePrefix+TEXT("_controls_started.txt"));
        if(!Require(Existing.IsEmpty()&&!IFileManager::Get().FileExists(*StartedPath)
            &&!IFileManager::Get().FileExists(*SavePath),TEXT("fresh prefix and absent campaign save prevent reused evidence or player-save overwrite")))return;
        if(!Require(FFileHelper::SaveStringToFile(TEXT("controls-only qualification; authored_environment=0 miniature=0 battle_environment=0\n"),*StartedPath),TEXT("fresh-run marker reserved")))return;
        auto* MaxFPS=IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
        if(!Require(MaxFPS!=nullptr,TEXT("functional frame cap available")))return;
        MaxFPS->Set(20.f,ECVF_SetByCode);
        if(!Require(FMath::IsNearlyEqual(MaxFPS->GetFloat(),20.f),TEXT("20 FPS functional cap applied; not a performance benchmark")))return;
        bStarted=true;DevelopmentQualificationNextStep=Now+20.;
        UE_LOG(LogTemp,Display,TEXT("SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_BEGIN warmup_seconds=20 fps_cap=20 prefix=%s save=%s authored_environment=0 miniature=0 battle_environment=0"),*CapturePrefix,*SavePath);
        return;
    }
    if(State->bPersistenceBusy)bDevelopmentObservedPersistenceBusy=true;
    if(Now<DevelopmentQualificationNextStep)return;
    if(State->bPersistenceBusy)return; // RBSave completion may arrive on a later tick.
    int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
    if(!Require(Width==1920&&Height==1080,TEXT("actual viewport is 1920x1080")))return;
    if(!State->IsSettlementDevelopmentReady()){Fail(TEXT("proof fixture unavailable: ")+State->GetSettlementDevelopmentError());return;}
    const auto* Scenario=State->GetSettlementScenario();
    const auto* Town=Authority->FindSettlement(Scenario->SettlementId);
    const auto* Tavern=Town?Town->Buildings.Find(TEXT("human.tavern")):nullptr;
    if(!Require(Tavern!=nullptr,TEXT("actual tavern state exists")))return;
    const int32 Gold=State->Economy.Resources.FindRef(TEXT("gold"));
    const int32 PaidGold=DevelopmentInitialGold-200;
    FVector2D HUDPoint;
    switch(VisualStep)
    {
    case 0:
    {
        const auto* D=Scenario->FindDevelopmentDefinition(TEXT("human.tavern"));
        if(!Require(Scenario->SettlementId==TEXT("human_capital")&&Scenario->RegionId==TEXT("human_capital")
            &&Scenario->FactionId==TEXT("humans")&&D&&D->BuildDays==2&&D->MaxLevel==1
            &&D->BuildCost.Num()==1&&D->BuildCost.FindRef(TEXT("gold"))==200
            &&D->UnlockIds.Contains(TEXT("service.tavern_hero")),TEXT("loaded data fixture is human.tavern:200 gold,2 days,level1,tavern hero service")))return;
        if(!Require(State->PlayerRegion==TEXT("human_capital")&&!State->HasPendingBattle()
            &&State->CampaignLoadRevision==0&&!State->bSecondHeroHired&&!Campaign->IsTownPanelOpen()
            &&State->Economy.Day==1&&Tavern->Condition==ESoulBuildingCondition::Unbuilt
            &&Tavern->Level==0&&Tavern->ConstructionDaysRemaining==0&&!State->IsTavernOperational()
            &&Gold>=1400,TEXT("fresh initial state has an unbuilt tavern and no hired companion")))return;
        DevelopmentInitialGold=Gold;DevelopmentInitialDay=State->Economy.Day;
        DevelopmentGoldIncome=State->Economy.DailyIncome.FindRef(TEXT("gold"));
        if(!Remember()||!PersistExpected(TEXT("start")))return;Key(EKeys::T);break;
    }
    case 1:
        if(!Require(Campaign->IsTownPanelOpen()&&FindHUD(TEXT("BuildTavern"),HUDPoint)
            &&!FindHUD(TEXT("Hire"),HUDPoint),TEXT("T opens real town UI with construction visible and hiring absent")))return;
        Capture(TEXT("start"));Key(EKeys::H);break;
    case 2:
        if(!Require(!State->bSecondHeroHired&&Gold==DevelopmentInitialGold
            &&Campaign->LastMessage.Contains(TEXT("Complete the tavern")),TEXT("H is blocked before construction without spending gold"))
            ||!Matches(TEXT("blocked hiring preserves both domains")))return;
        DevelopmentExpectedStateRevision=State->SettlementDevelopmentRevision+1;Key(EKeys::U);break;
    case 3:
        if(!Require(Tavern->Condition==ESoulBuildingCondition::Building&&Tavern->ConstructionDaysRemaining==2
            &&Tavern->Level==0&&Gold==PaidGold&&State->Economy.Day==DevelopmentInitialDay
            &&State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision&&!State->IsTavernOperational()
            &&Campaign->LastMessage==TEXT("Tavern construction started. Advance the day to make progress."),TEXT("U starts construction once and deducts exactly200 gold without advancing day")))return;
        if(!Remember()||!ClickHUD(TEXT("BuildTavern")))return;break;
    case 4:
        if(!Require(Campaign->LastMessage==TEXT("Construction prerequisites, resources or building level do not permit this upgrade."),
            TEXT("rendered BuildTavern duplicate reached its handler and reported rejection"))
            ||!Matches(TEXT("duplicate rendered BuildTavern click preserves exact cost and both domains"))
            ||!Require(State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision,TEXT("rejected duplicate has no development revision")))return;
        ++DevelopmentExpectedStateRevision;Key(EKeys::SpaceBar);break;
    case 5:
        if(!Require(State->Economy.Day==DevelopmentInitialDay+1&&Gold==PaidGold+DevelopmentGoldIncome
            &&Tavern->Condition==ESoulBuildingCondition::Building&&Tavern->ConstructionDaysRemaining==1
            &&State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision&&!State->IsTavernOperational(),TEXT("one Space advances one campaign/construction day; hiring remains locked")))return;
        if(!Remember()||!PersistExpected(TEXT("mid")))return;Key(EKeys::T);break;
    case 6:
        if(!Require(Campaign->IsTownPanelOpen()&&FindHUD(TEXT("BuildTavern"),HUDPoint)
            &&!FindHUD(TEXT("Hire"),HUDPoint),TEXT("mid-construction town UI is reopened and rendered")))return;
        Capture(TEXT("mid"));if(!StartSave())return;break;
    case 7:
        if(!Saved()||!Matches(TEXT("F5 does not mutate either domain")))return;
        ++DevelopmentExpectedStateRevision;Key(EKeys::SpaceBar);break;
    case 8:
        if(!Require(State->Economy.Day==DevelopmentInitialDay+2&&Gold==PaidGold+2*DevelopmentGoldIncome
            &&Tavern->Condition==ESoulBuildingCondition::Intact&&Tavern->Level==1&&Tavern->ConstructionDaysRemaining==0
            &&State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision&&State->IsTavernOperational(),TEXT("second Space completes tavern and unlocks existing service")))return;
        Key(EKeys::T);break;
    case 9:
        if(!Require(Campaign->IsTownPanelOpen()&&FindHUD(TEXT("Hire"),HUDPoint),TEXT("completed town UI is reopened and rendered before capture")))return;
        Capture(TEXT("completed"));break;
    case 10:
        if(IFileManager::Get().FileSize(*Path(TEXT("completed")))<=0)
        {DevelopmentQualificationNextStep=Now+.25;return;}
        StartLoad();break;
    case 11:
        if(!Loaded()||!Matches(TEXT("F9 restores exact mid-construction campaign and settlement snapshots")))return;
        if(!Require(State->Economy.Day==DevelopmentInitialDay+1&&Gold==PaidGold+DevelopmentGoldIncome
            &&Tavern->Condition==ESoulBuildingCondition::Building&&Tavern->ConstructionDaysRemaining==1
            &&!State->IsTavernOperational()&&!State->bSecondHeroHired&&!Campaign->IsTownPanelOpen(),TEXT("loaded checkpoint restores one remaining day and synchronizes closed town UI")))return;
        Key(EKeys::T);break;
    case 12:
        if(!Require(Campaign->IsTownPanelOpen()&&!FindHUD(TEXT("Hire"),HUDPoint),TEXT("reopened loaded town still hides locked hiring")))return;
        Capture(TEXT("loaded"));Key(EKeys::H);break;
    case 13:
        if(!Require(Campaign->LastMessage.Contains(TEXT("Complete the tavern")),TEXT("H after rollback reaches the locked-service handler"))
            ||!Matches(TEXT("H after rollback stays blocked and preserves both restored domains")))return;
        DevelopmentExpectedStateRevision=State->SettlementDevelopmentRevision+1;Key(EKeys::SpaceBar);break;
    case 14:
        if(!Require(State->Economy.Day==DevelopmentInitialDay+2&&Tavern->Level==1&&State->IsTavernOperational()
            &&State->SettlementDevelopmentRevision==DevelopmentExpectedStateRevision&&!Campaign->IsTownPanelOpen(),TEXT("restored remaining day completes once and EndDay closes town")))return;
        break;
    case 15:
        if(!Require(!Campaign->IsTownPanelOpen(),TEXT("completed town remains closed after EndDay")))return;
        Key(EKeys::T);break;
    case 16:
        if(!Require(Campaign->IsTownPanelOpen()&&FindHUD(TEXT("Hire"),HUDPoint),TEXT("T reopens completed town with rendered hiring hitbox")))return;
        Key(EKeys::H);break;
    case 17:
        if(!Require(State->bSecondHeroHired&&Gold==PaidGold+2*DevelopmentGoldIncome-1200
            &&Tavern->Level==1&&State->IsTavernOperational()
            &&Campaign->LastMessage==TEXT("Tavern hero hired for 1200 gold."),TEXT("H hires existing companion once for exactly1200 gold")))return;
        if(!Remember()||!PersistExpected(TEXT("completed_hired")))return;
        Capture(TEXT("hired"));if(!ClickHUD(TEXT("Hire")))return;break;
    case 18:
        if(!Require(Campaign->LastMessage==TEXT("The tavern hero is already in your service."),
            TEXT("rendered Hire duplicate reached its handler and reported already hired"))
            ||!Matches(TEXT("duplicate rendered Hire click preserves completed town, hired hero and gold")))return;
        if(!StartSave())return;break;
    case 19:
        if(!Saved()||!Matches(TEXT("completed-plus-hired F5 preserves both live domains")))return;
        Key(EKeys::SpaceBar);break;
    case 20:
        if(!Require(State->Economy.Day==DevelopmentInitialDay+3&&State->bSecondHeroHired
            &&Gold==PaidGold+3*DevelopmentGoldIncome-1200&&State->IsTavernOperational(),TEXT("ordinary day input changes live checkpoint before final F9")))return;
        StartLoad();break;
    case 21:
        if(!Loaded()||!Matches(TEXT("final F9 restores exact completed-plus-hired campaign and settlement snapshots")))return;
        if(!Require(Tavern->Condition==ESoulBuildingCondition::Intact&&Tavern->Level==1&&State->IsTavernOperational()
            &&State->bSecondHeroHired&&Gold==PaidGold+2*DevelopmentGoldIncome-1200
            &&State->Economy.Day==DevelopmentInitialDay+2&&!Campaign->IsTownPanelOpen(),TEXT("completed building, companion, gold and day persist together")))return;
        Key(EKeys::T);break;
    case 22:
        if(!Require(Campaign->IsTownPanelOpen(),TEXT("loaded completed state is visible in real town panel")))return;
        Capture(TEXT("loaded_completed"));break;
    case 23:
    {
        if(!Matches(TEXT("final rendered town still matches both saved domains")))return;
        for(const TCHAR* Shot:Shots)if(!Require(IFileManager::Get().FileSize(*Path(Shot))>0,TEXT("requested screenshot was actually written")))return;
        const FString Receipt=FString::Printf(TEXT("{\"status\":\"CONTROLS_PASS\",\"authored_environment\":0,\"miniature\":0,\"battle_environment\":0,\"viewport\":[1920,1080],\"fps_cap\":20,\"warmup_seconds\":20,\"screenshot_count\":6,\"initial_gold\":%d,\"final_gold\":%d,\"final_day\":%d,\"load_revision\":%u,\"both_domain_snapshots_exact\":true,\"last_operation_busy_observed\":%s}\n"),DevelopmentInitialGold,Gold,State->Economy.Day,State->CampaignLoadRevision,bDevelopmentObservedPersistenceBusy?TEXT("true"):TEXT("false"));
        if(!Require(FFileHelper::SaveStringToFile(Receipt,*(FPaths::ProjectSavedDir()/(CapturePrefix+TEXT("_controls.json")))),TEXT("controls receipt written")))return;
        UE_LOG(LogTemp,Display,TEXT("SOUL_SETTLEMENT_DEVELOPMENT_CONTROLS_PASS authored_environment=0 miniature=0 battle_environment=0 two_domain_roundtrips=2 screenshots=6 gold=%d day=%d"),Gold,State->Economy.Day);
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    default:Fail(TEXT("unexpected controls qualification step"));return;
    }
    ++VisualStep;DevelopmentQualificationNextStep=Now+2.;
}
