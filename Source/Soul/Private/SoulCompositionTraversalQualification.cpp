#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
#include "SoulCampaignCamera.h"
#include "SoulPlaytestRegionActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

// Opt-in acceptance observer. No teleports, ownership edits, replacement movement,
// AP overrides or fabricated battles. Every step uses existing campaign actions.
void ASoulFounderPlaytestGameMode::TickCompositionTraversal(float Seconds)
{
    if(!State||!Campaign)return;
    static const TArray<FName> Journey={
        TEXT("crossroads"),TEXT("river_ford"),TEXT("southern_crossing"),TEXT("orc_broken_bridge"),
        TEXT("dark_ruined_causeway"),TEXT("dark_castle_approach"),TEXT("dark_fortress"),TEXT("dark_ash_plain"),
        TEXT("dark_corrupted_valley"),TEXT("southern_crossing"),TEXT("river_ford"),TEXT("crossroads"),
        TEXT("old_quarry"),TEXT("ancient_shrine"),TEXT("mountain_shrine"),TEXT("viking_snow_pass"),
        TEXT("viking_forest_track"),TEXT("viking_harbour"),TEXT("viking_fjord_ridge"),TEXT("viking_snow_pass"),
        TEXT("mountain_shrine"),TEXT("dwarf_mountain_pass"),TEXT("dwarf_high_quarry"),TEXT("dwarf_forge_approach"),
        TEXT("dwarf_hold"),TEXT("dwarf_snow_basin"),TEXT("dwarf_high_quarry"),TEXT("dwarf_mountain_pass"),
        TEXT("mountain_shrine"),TEXT("ancient_shrine"),TEXT("old_quarry"),TEXT("crossroads"),
        TEXT("human_capital"),TEXT("coastal_ruins"),TEXT("nature_shrine"),TEXT("nature_forest_clearing"),
        TEXT("nature_treehold"),TEXT("nature_river_woodland"),TEXT("nature_grassland_edge"),TEXT("river_ford")};
    static int32 Index=0,Phase=0,FerryFrames=0,FerryLegs=0;
    static double Next=0,Start=0;
    static FName From;
    static int32 BeforeAP=0;
    const double Now=FPlatformTime::Seconds();if(Start==0){Start=Now;Next=Now+20;}
    auto Fail=[&](const TCHAR* Why){UE_LOG(LogTemp,Error,TEXT("SOUL_COMPOSITION_TRAVEL_FAIL index=%d reason=%s"),Index,Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
    if(!SoulCampaignTerrain::Composition()||Now-Start>1100){Fail(TEXT("profile or bounded deadline"));return;}
    auto* PC=Cast<ASoulFounderPlaytestPlayerController>(GetWorld()->GetFirstPlayerController());
    auto* Camera=PC?Cast<ASoulCampaignCamera>(PC->GetViewTarget()):nullptr;
    ASoulCampaignWorldActor* View=nullptr;for(TActorIterator<ASoulCampaignWorldActor> It(GetWorld());It;++It){View=*It;break;}
    if(!PC||!Camera||!View)return;
    USceneComponent* Party=nullptr;TArray<USceneComponent*> Components;View->GetComponents(Components);
    for(auto* C:Components)if(C->GetFName()==TEXT("PlayerParty")){Party=C;break;}
    if(!Party){Fail(TEXT("missing real party component"));return;}
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    if(Phase==3&&Party->bHiddenInGame)
    {
        ++FerryFrames;
        if(!SoulCampaignTerrain::RouteUsesFerry(From,Journey[Index])){Fail(TEXT("party hidden on dry route"));return;}
        if(FerryFrames==1)
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/FString::Printf(TEXT("traversal_%02d_ferry.png"),Index),true,false);
    }
    if(Now<Next)return;
    if(Index>=Journey.Num())
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_COMPOSITION_TRAVEL_PASS legal_moves=%d ferry_legs=%d teleports=0 ownership_overrides=0"),Index,FerryLegs);
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    const FName To=Journey[Index];
    if(Phase==0)
    {
        if(State->Economy.ActionPoints==0){Key(EKeys::SpaceBar);Next=Now+.3;return;}
        From=State->PlayerRegion;BeforeAP=State->Economy.ActionPoints;FerryFrames=0;
        if(!FSoulWorldRules::CanMove(State->World,From,To)||State->HasHostileGarrison(To)){Fail(TEXT("journey requires legal peaceful edge"));return;}
        Camera->Focus(ASoulCampaignWorldActor::Locations().FindRef(To));Next=Now+3;Phase=1;return;
    }
    if(Phase==1)
    {
        ASoulPlaytestRegionActor* Target=nullptr;for(TActorIterator<ASoulPlaytestRegionActor> It(GetWorld());It;++It)if(It->RegionId==To){Target=*It;break;}
        if(!Target||Target->IsHidden()){Fail(TEXT("destination not revealed through canonical vision"));return;}
        FVector2D Screen;FHitResult Hit;
        if(!PC->ProjectWorldLocationToScreen(Target->Marker->Bounds.Origin,Screen)
            ||!PC->GetHitResultAtScreenPosition(Screen,ECC_Visibility,false,Hit)||Hit.GetActor()!=Target)
        {Fail(TEXT("actual viewport cannot select destination"));return;}
        PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));Key(EKeys::LeftMouseButton);
        // Input bindings are processed by the next normal PlayerController tick.
        Next=Now+.3;Phase=2;return;
    }
    if(Phase==2)
    {
        if(State->PlayerRegion!=To||State->Economy.ActionPoints!=BeforeAP-1){Fail(TEXT("ordinary click did not commit exactly one legal move"));return;}
        Next=Now+13;Phase=3;return;
    }
    if(State->PlayerRegion!=To||Party->bHiddenInGame
        ||FVector::DistXY(View->PresentedPartyLocation(),ASoulCampaignWorldActor::Locations().FindRef(To))>5)
    {Fail(TEXT("actual party did not arrive and reappear"));return;}
    const bool Ferry=SoulCampaignTerrain::RouteUsesFerry(From,To);
    if(Ferry&&FerryFrames==0){Fail(TEXT("ferry leg never suppressed walking representation"));return;}
    FerryLegs+=Ferry;
    UE_LOG(LogTemp,Display,TEXT("SOUL_COMPOSITION_TRAVEL_LEG from=%s to=%s ap=%d ferry=%d hidden_frames=%d"),*From.ToString(),*To.ToString(),State->Economy.ActionPoints,Ferry,FerryFrames);
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/FString::Printf(TEXT("traversal_%02d_%s.png"),Index,*To.ToString()),true,false);
    ++Index;Phase=0;Next=Now+2;
}
