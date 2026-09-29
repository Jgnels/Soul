#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "SoulPlaytestRegionActor.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

// Explicit command-line qualification only. Uses the normal input and action paths.
void ASoulFounderPlaytestGameMode::TickVisualQualification(float Seconds)
{
    if(!State||!Campaign)return;
    VisualElapsed+=Seconds;
    if(VisualElapsed<10.f+VisualStep*3.f)return;
    auto* PC=Cast<ASoulFounderPlaytestPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
    if(!PC||!Camera)return;
    auto Capture=[&](const TCHAR* Label){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(CapturePrefix+TEXT("_")+Label+TEXT(".png")),true,false);};
    auto Require=[&](bool Condition,const TCHAR* Label)
    {
        if(Condition){UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_CHECK_PASS %s"),Label);return true;}
        UE_LOG(LogTemp,Error,TEXT("SOUL_WORLD_CHECK_FAIL %s step=%d"),Label,VisualStep);
        Capture(TEXT("failure"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return false;
    };
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    auto Click=[&](FName Region)
    {
        FVector2D Screen;
        if(!Require(PC->ProjectWorldLocationToScreen(ASoulCampaignWorldActor::Locations().FindRef(Region)+FVector(0,0,100),Screen),TEXT("project selectable location")))return;
        FHitResult Hit;
        if(!Require(PC->GetHitResultAtScreenPosition(Screen,ECC_Visibility,false,Hit)&&Cast<ASoulPlaytestRegionActor>(Hit.GetActor())&&Cast<ASoulPlaytestRegionActor>(Hit.GetActor())->RegionId==Region,TEXT("viewport ray hits correct location")))return;
        PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));Key(EKeys::LeftMouseButton);
    };
    switch(VisualStep)
    {
    case 0: Capture(TEXT("initial"));break;
    case 1: Key(EKeys::One);break;
    case 2: Key(EKeys::T);break;
    case 3: if(!Require(Campaign->IsTownPanelOpen(),TEXT("T opens recruitment panel")))return;Capture(TEXT("town"));Key(EKeys::One);break;
    case 4: if(!Require(State->PlayerArmy.FindRef(State->PlayerUnitId)==46,TEXT("number key recruits once")))return;Key(EKeys::T);break;
    case 5: Click(TEXT("crossroads"));break;
    case 6: if(!Require(State->PlayerRegion==TEXT("crossroads")&&State->Economy.ActionPoints==2,TEXT("mouse selection moves once and spends one action")))return;Capture(TEXT("crossroads"));break;
    case 7: Key(EKeys::MouseScrollUp);Key(EKeys::MouseScrollUp);PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Pressed,1));break;
    case 8:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Released,0));
        if(!Require(Camera->GetDistance()<7500&&FMath::Abs(Camera->GetFocus().X)>200,TEXT("wheel zoom and keyboard pan move perspective camera")))return;
        Capture(TEXT("camera"));Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("river_ford")));break;
    case 9: Click(TEXT("river_ford"));break;
    case 10: if(!Require(State->PlayerRegion==TEXT("river_ford"),TEXT("mouse movement reaches ford")))return;Capture(TEXT("frontier"));break;
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
        Key(EKeys::F5);break;
    case 15:
        if(State->bPersistenceBusy)return;
        if(!Require(State->bLastSaveSucceeded,TEXT("F5 saves through RBSave")))return;
        Campaign->HandleRegionClicked(TEXT("crossroads"));Key(EKeys::F9);break;
    case 16:
        if(State->bPersistenceBusy)return;
        if(!Require(State->bLastLoadSucceeded&&State->PlayerRegion==TEXT("river_ford"),TEXT("F9 restores saved campaign location")))return;
        Capture(TEXT("restored"));break;
    case 17:
        Campaign->HandleRegionClicked(TEXT("crossroads"));
        Campaign->HandleRegionClicked(TEXT("old_quarry"));
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("old_quarry")));break;
    case 18:
        if(!Require(State->PlayerRegion==TEXT("old_quarry"),TEXT("legal quarry exploration")))return;
        Capture(TEXT("quarry_shrine"));Campaign->EndDay();Campaign->HandleRegionClicked(TEXT("ancient_shrine"));
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("ancient_shrine")));break;
    case 19: Capture(TEXT("shrine"));break;
    case 20:
        Campaign->HandleRegionClicked(TEXT("old_quarry"));Campaign->HandleRegionClicked(TEXT("crossroads"));Campaign->EndDay();
        Campaign->HandleRegionClicked(TEXT("forest_edge"));Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("forest_edge")));break;
    case 21:
        if(!Require(State->PlayerRegion==TEXT("forest_edge")&&FSoulWorldRules::IsExplored(State->World,State->PlayerFaction,TEXT("north_pass")),TEXT("forest route reveals the pass through strategic vision")))return;
        Capture(TEXT("forest_pass"));break;
    case 22:
        UE_LOG(LogTemp,Display,TEXT("SOUL_WORLD_VISUAL_INPUT_PASS movement=recruitment=pan=zoom=selection=save_load=1"));
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    ++VisualStep;
}
