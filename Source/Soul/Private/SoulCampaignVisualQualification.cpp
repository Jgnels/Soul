#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
#include "SoulPlaytestRegionActor.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"

// Explicit command-line qualification only. Uses the normal input and action paths.
void ASoulFounderPlaytestGameMode::TickVisualQualification(float Seconds)
{
    if(!State||!Campaign)return;
    VisualElapsed+=Seconds;
    if(VisualStep==6&&VisualElapsed-Seconds<25.9f&&VisualElapsed>=25.9f)
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(CapturePrefix+TEXT("_travel.png")),true,false);
    if(VisualElapsed>=10.f) { ++VisualFrames;VisualFrameSeconds+=Seconds;VisualWorstFrame=FMath::Max(VisualWorstFrame,Seconds); }
    auto* PC=Cast<ASoulFounderPlaytestPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
    if(!PC||!Camera)return;
    if(VisualStep==29) PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX,IE_Axis,8.f,1));
    // Analog samples accumulate within a frame and retain their last value when
    // absent. Keep sending neutral samples after release, like a real device.
    if(VisualStep>=35)PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,VisualStep==35?.7f:0.f,1));
    if(VisualStep>=37)PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX,IE_Axis,VisualStep==37?.6f:0.f,1));
    if(VisualElapsed<10.f+VisualStep*3.f)return;
    auto Capture=[&](const TCHAR* Label){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(CapturePrefix+TEXT("_")+Label+TEXT(".png")),true,false);};
    auto Require=[&](bool Condition,const TCHAR* Label)
    {
        if(Condition){UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_CHECK_PASS %s"),Label);return true;}
        UE_LOG(LogTemp,Error,TEXT("SOUL_WORLD_CHECK_FAIL %s step=%d"),Label,VisualStep);
        Capture(TEXT("failure"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return false;
    };
    auto CheckKnowledge=[&](bool RequireMemory)
    {
        int32 Unknown=0,Memory=0,Count=0;bool Matches=true;
        for(TActorIterator<ASoulPlaytestRegionActor> It(GetWorld());It;++It)
        {
            const bool Explored=FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,It->RegionId);
            const bool Visible=FSoulWorldRules::IsVisible(State->World,State->PlayerFaction,It->RegionId);
            Matches=Matches && It->IsHidden()==!Explored && It->GetActorEnableCollision()==Explored;
            Unknown+=!Explored;Memory+=Explored&&!Visible;++Count;
        }
        return Require(Matches && Count==State->World.Regions.Num() && Unknown>0 && (!RequireMemory||Memory>0),
            RequireMemory?TEXT("explored memory stays present while unknown locations remain hidden and unselectable"):TEXT("initial unknown locations are hidden and unselectable"));
    };
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    auto ClickHUD=[&](FName Name,FKey Button)
    {
        auto* HUD=PC->GetHUD();
        int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
        // Find a point in the rendered hitbox: no copied layout/resolution constants.
        for(int32 Y=4;HUD && Y<Height;Y+=8)
            for(int32 X=4;X<Width;X+=8)
                if(const auto* Hit=HUD->GetHitBoxAtCoordinates(FVector2D(X,Y),true))
                    if(Hit->GetName()==Name)
                    {PC->SetMouseLocation(X,Y);Key(Button);return true;}
        return Require(false,TEXT("requested HUD action has a rendered clickable hitbox"));
    };
    auto Click=[&](FName Region)
    {
        FVector2D Screen;
        if(!Require(PC->ProjectWorldLocationToScreen(ASoulCampaignWorldActor::Locations().FindRef(Region)+FVector(0,0,100*SoulCampaignTerrain::RegionScale()),Screen),TEXT("project selectable location")))return;
        FHitResult Hit;
        if(!Require(PC->GetHitResultAtScreenPosition(Screen,ECC_Visibility,false,Hit)&&Cast<ASoulPlaytestRegionActor>(Hit.GetActor())&&Cast<ASoulPlaytestRegionActor>(Hit.GetActor())->RegionId==Region,TEXT("viewport ray hits correct location")))return;
        PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));Key(EKeys::LeftMouseButton);
    };
    switch(VisualStep)
    {
    case 0: if(!CheckKnowledge(false))return;Capture(TEXT("initial"));break;
    case 1: Key(EKeys::One);break;
    case 2:
        if(!ClickHUD(TEXT("Town"),EKeys::LeftMouseButton))return;break;
    case 3: if(!Require(Campaign->IsTownPanelOpen(),TEXT("town HUD button opens recruitment panel")))return;Capture(TEXT("town"));Key(EKeys::One);break;
    case 4: if(!Require(State->PlayerArmy.FindRef(State->PlayerUnitId)==46,TEXT("number key recruits once")))return;Key(EKeys::T);break;
    case 5: Click(TEXT("crossroads"));break;
    case 6: if(!Require(State->PlayerRegion==TEXT("crossroads")&&State->Economy.ActionPoints==2,TEXT("mouse selection moves once and spends one action")))return;Capture(TEXT("crossroads"));break;
    case 7: Key(EKeys::MouseScrollUp);Key(EKeys::MouseScrollUp);PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Pressed,1));break;
    case 8:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Released,0));
        if(!Require(Camera->GetDistance()<7500&&FMath::Abs(Camera->GetFocus().X)>200,TEXT("wheel zoom and keyboard pan move perspective camera")))return;
        Capture(TEXT("camera"));Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("river_ford")));break;
    case 9: Click(TEXT("river_ford"));break;
    case 10: if(!Require(State->PlayerRegion==TEXT("river_ford"),TEXT("mouse movement reaches ford")))return;Capture(TEXT("frontier"));Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_watch")));break;
    case 11: Click(TEXT("orc_watch"));break;
    case 12:
        if(!Require(Campaign->IsBattleAvailable()&&State->PlayerRegion==TEXT("river_ford"),TEXT("hostile selection offers battle without preoccupation")))return;
        Capture(TEXT("battle_prompt"));break;
    case 13: Key(EKeys::Escape);Key(EKeys::SpaceBar);break;
    case 14:
        if(!Require(State->Economy.ActionPoints==State->Economy.MaxActionPoints,TEXT("end day restores actions")))return;
        // Check rejection through the actual campaign UI path, not a second simulator.
        Campaign->HandleRegionClicked(TEXT("human_capital"));
        if(!Require(State->PlayerRegion==TEXT("river_ford"),TEXT("non-adjacent movement rejected")))return;
        {FRBSaveDomainState Snapshot;FString Error;
         if(!Require(State->CaptureRBSaveDomain_Implementation(Snapshot,Error),TEXT("capture pre-F5 campaign snapshot")))return;
         ExpectedSnapshot=Snapshot.Fields[0].StringValue;
         FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));}
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("crossroads")));
        Key(EKeys::F5);break;
    case 15:
        if(State->bPersistenceBusy)return;
        if(!Require(State->bLastSaveSucceeded,TEXT("F5 saves through RBSave")))return;
        Click(TEXT("crossroads"));break;
    case 16:
        if(!Require(State->PlayerRegion==TEXT("crossroads"),TEXT("mouse changes live region before F9")))return;
        Key(EKeys::F9);break;
    case 17:
        if(State->bPersistenceBusy)return;
        if(!Require(State->bLastLoadSucceeded&&State->PlayerRegion==TEXT("river_ford"),TEXT("F9 restores saved campaign location")))return;
        {FRBSaveDomainState Restored;FString Error;
         if(!Require(State->CaptureRBSaveDomain_Implementation(Restored,Error)&&Restored.Fields[0].StringValue==ExpectedSnapshot,
             TEXT("F9 restores exact campaign snapshot including actions army economy exploration and encounter state")))return;}
        if(!Require(Campaign->GetSelectedRegion()==State->PlayerRegion&&!Campaign->IsBattleAvailable()&&!Campaign->IsTownPanelOpen()
            &&FVector::DistXY(Camera->GetFocus(),ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion))<3,
            TEXT("F9 synchronizes selection panels company camera")))return;
        for(TActorIterator<ASoulCampaignWorldActor> It(GetWorld());It;++It)
            if(!Require(It->PresentedPartyLocation().Equals(ASoulCampaignWorldActor::PartyAnchor(State->PlayerRegion),1.f),TEXT("F9 places physical company at restored terrain anchor")))return;
        Capture(TEXT("restored"));break;
    case 18:
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
        {Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("orc_watch")));break;}
        Campaign->HandleRegionClicked(TEXT("crossroads"));
        Campaign->HandleRegionClicked(TEXT("old_quarry"));
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("old_quarry")));break;
    case 19:
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
        {Click(TEXT("orc_watch"));break;}
        if(!Require(State->PlayerRegion==TEXT("old_quarry"),TEXT("legal quarry exploration")))return;
        Capture(TEXT("quarry_shrine"));Campaign->EndDay();Campaign->HandleRegionClicked(TEXT("ancient_shrine"));
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("ancient_shrine")));break;
    case 20:
        if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignMouseRoundtrip")))
        {
            if(!Require(Campaign->IsBattleAvailable(),TEXT("mouse-selected hostile ready for physical battle")))return;
            // Bounded defeat fixture, applied to the canonical force pools before
            // the ordinary battle action. Combat still decides the outcome.
            if(FParse::Param(FCommandLine::Get(),TEXT("SoulCampaignDefeatProof")))
            {
                State->PlayerArmy[State->PlayerUnitId]=12;
                State->EnemyArmies[TEXT("orc_watch")]=70;
            }
            UE_LOG(LogTemp,Display,TEXT("SOUL_CAMPAIGN_MOUSE_BATTLE_COMMIT source=%s target=orc_watch"),*State->PlayerRegion.ToString());
            bVisualQualification=false;bQualification=true;bStarted=true;Elapsed=0;Key(EKeys::B);return;
        }
        {
            if(!CheckKnowledge(true))return;
            FName FriendlyMemory,HostileMemory;
            for(const auto& Pair:State->World.Regions)
                if(FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,Pair.Key)
                    && !FSoulWorldRules::IsVisible(State->World,State->PlayerFaction,Pair.Key))
                { if(State->IsHostile(Pair.Key))HostileMemory=Pair.Key;else FriendlyMemory=Pair.Key; }
            if(!Require(!FriendlyMemory.IsNone()&&!HostileMemory.IsNone(),TEXT("route exercises friendly and hostile remembered places")))return;
            const FString PreviousMessage=Campaign->LastMessage;
            const FName BeforeRegion=State->PlayerRegion;const int32 BeforeActions=State->Economy.ActionPoints;
            Campaign->HandleRegionClicked(FriendlyMemory);const FString FriendlyReply=Campaign->LastMessage;
            Campaign->HandleRegionClicked(HostileMemory);
            if(!Require(FriendlyReply==Campaign->LastMessage && State->PlayerRegion==BeforeRegion && State->Economy.ActionPoints==BeforeActions,
                TEXT("distant selection reveals no current garrison information and spends no action")))return;
            Campaign->HandleRegionClicked(BeforeRegion);Campaign->LastMessage=PreviousMessage;
            Capture(TEXT("shrine"));
        }break;
    case 21:
        Campaign->HandleRegionClicked(TEXT("old_quarry"));Campaign->HandleRegionClicked(TEXT("crossroads"));Campaign->EndDay();
        Campaign->HandleRegionClicked(TEXT("forest_edge"));Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("forest_edge")));break;
    case 22:
        if(!Require(State->PlayerRegion==TEXT("forest_edge")&&FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,TEXT("north_pass")),TEXT("forest route reveals the pass through strategic vision")))return;
        Capture(TEXT("forest_pass"));break;
    case 23:
        for(int32 I=0;I<30;++I)Key(EKeys::MouseScrollUp);
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Q,IE_Pressed,1));break;
    case 24:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Q,IE_Released,0));
        if(!Require(FMath::IsNearlyEqual(Camera->GetDistance(),Camera->GetMinimumDistance(),2.f)
            && Camera->GetActorRotation().Yaw<-110.f
            && Camera->GetActorLocation().Z>=ASoulCampaignWorldActor::HeightAt(Camera->GetActorLocation().X,Camera->GetActorLocation().Y)+449.f*SoulCampaignTerrain::Scale(),
            TEXT("minimum zoom and orbit retain terrain clearance")))return;
        Capture(TEXT("close_orbit"));break;
    case 25:
        for(int32 I=0;I<40;++I)Key(EKeys::MouseScrollDown);
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Pressed,1));break;
    case 26:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Released,0));
        if(!Require(FMath::IsNearlyEqual(Camera->GetDistance(),Camera->GetMaximumDistance(),2.f)
            && FMath::Abs(Camera->GetFocus().X)<=SoulCampaignTerrain::FocusBounds().X+1.f
            && FMath::Abs(Camera->GetFocus().Y)<=SoulCampaignTerrain::FocusBounds().Y+1.f,
            TEXT("maximum zoom and sustained pan stay bounded")))return;
        if(!Require(Camera->ViewFitsTerrain(),TEXT("maximum zoom camera footprint stays inside integrated terrain")))return;
        Capture(TEXT("wide_bounds"));break;
    case 27:
        // Select the army through its ordinary HUD hitbox, not a state edit.
        if(!ClickHUD(TEXT("Company"),EKeys::LeftMouseButton))return;
        break;
    case 28:
        if(!Require(Campaign->GetSelectedRegion()==State->PlayerRegion&&!Campaign->IsBattleAvailable()&&!Campaign->IsTownPanelOpen(),TEXT("army HUD click selects company without moving or opening town")))return;
        if(!Require(FVector::DistXY(Camera->GetFocus(),ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion))<3.f,TEXT("army selection focuses camera on company")))return;
        VisualDragStart=Camera->GetFocus();
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MiddleMouseButton,IE_Pressed,1));break;
    case 29:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MiddleMouseButton,IE_Released,0));
        if(!Require(FVector::DistXY(Camera->GetFocus(),VisualDragStart)>100.f,TEXT("middle-mouse drag pans through normal mouse-axis input")))return;
        Capture(TEXT("drag"));break;
    case 30:
        if(SoulCampaignTerrain::Enabled())
        {
            Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("north_pass")));
            Camera->Zoom(8);break;
        }
        [[fallthrough]];
    case 34:
        VisualDragStart=Camera->GetFocus();
        {FRBSaveDomainState Snapshot;FString Error;
         if(!Require(State->CaptureRBSaveDomain_Implementation(Snapshot,Error),TEXT("capture state before controller navigation")))return;
         ExpectedSnapshot=Snapshot.Fields[0].StringValue;}
        break;
    case 35:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX,IE_Axis,0.f,1));
        if(!Require(FVector::DistXY(Camera->GetFocus(),VisualDragStart)>100.f,TEXT("controller left stick pans campaign camera")))return;
        Key(EKeys::Gamepad_LeftTrigger);Key(EKeys::Gamepad_LeftTrigger);
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightShoulder,IE_Pressed,1));break;
    case 36:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightShoulder,IE_Released,0));
        if(!Require(Camera->GetDistance()>Camera->GetMinimumDistance()+100&&Camera->GetActorRotation().Yaw>-70,TEXT("controller trigger zoom and shoulder orbit")))return;
        Campaign->SelectedRegion=TEXT("human_capital"); // Distinct presentation-only fixture.
        Key(EKeys::Gamepad_Special_Left);PC->SetMouseLocation(400,400);break;
    case 37:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX,IE_Axis,0.f,1));
        {float X=0,Y=0;if(!Require(PC->GetMousePosition(X,Y)&&X>500,TEXT("controller right stick moves bounded selection cursor")))return;}
        if(!Require(Campaign->GetSelectedRegion()==State->PlayerRegion&&FVector::DistXY(Camera->GetFocus(),ASoulCampaignWorldActor::Locations().FindRef(State->PlayerRegion))<3.f,TEXT("controller Back selects and focuses the company")))return;
        Campaign->SelectedRegion=TEXT("human_capital"); // Bottom-button test must change selection.
        if(!ClickHUD(TEXT("Company"),EKeys::Gamepad_FaceButton_Bottom))return;
        break;
    case 38:
        {FRBSaveDomainState Snapshot;FString Error;
         if(!Require(State->CaptureRBSaveDomain_Implementation(Snapshot,Error)&&Snapshot.Fields[0].StringValue==ExpectedSnapshot
             &&Campaign->GetSelectedRegion()==State->PlayerRegion&&!Campaign->IsTownPanelOpen()&&!Campaign->IsBattleAvailable(),TEXT("controller company selection preserves canonical campaign snapshot")))return;}
        {FVector2D Screen;int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
         if(!Require(PC->ProjectWorldLocationToScreen(ASoulCampaignWorldActor::PartyAnchor(State->PlayerRegion),Screen)
             &&Screen.X>0&&Screen.X<Width&&Screen.Y>0&&Screen.Y<Height,TEXT("selected company stays inside the viewport")))return;}
        Capture(TEXT("controller_company"));break;
    case 39:
        UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_FRAME_SAMPLE frames=%d mean_fps=%.2f worst_ms=%.2f includes_screenshot_capture=1"),VisualFrames,VisualFrames/FMath::Max(VisualFrameSeconds,.001),VisualWorstFrame*1000.f);
        UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_VISUAL_INPUT_PASS movement=recruitment=pan=zoom=selection=save_load=controller=1"));
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
    case 31: Capture(TEXT("north_pass"));break;
    case 32: Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("human_capital")));Camera->Zoom(3);break;
    case 33: Capture(TEXT("capital_ground"));break;
    }
    ++VisualStep;
}
