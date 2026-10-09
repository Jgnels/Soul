#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulCampaignCamera.h"
#include "SoulCampaignWorldActor.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

// An explicitly requested no-AI qualification observer; it uses the same state,
// graph, actions and controller F5/F9 as play. No ownership overrides/teleports.
void ASoulFounderPlaytestGameMode::TickSixFactionQualification(float Seconds)
{
    if(!State||!Campaign)return;
    Elapsed+=Seconds;auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    auto Fail=[&](const TCHAR* Why){UE_LOG(LogTemp,Error,TEXT("SOUL_SIX_FACTION_STATE_FAIL %s"),Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    auto Capture=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name,true,false);};
    if(!State->bInitialized||!State->IsSixFactionProfile()){Fail(TEXT("wrong profile"));return;}
    if(Elapsed>180){Fail(TEXT("bounded timeout"));return;}
    if(Elapsed<20+VisualStep*3)return;
    const bool Controlled=FParse::Param(FCommandLine::Get(),TEXT("SoulControlledTurn"));
    const auto& Ids=FSoulCampaignRules::CanonicalFactions();
    static const TMap<FName,FName> Secondary={{TEXT("humans"),TEXT("crossroads")},{TEXT("dwarves"),TEXT("dwarf_forge_approach")},{TEXT("orcs"),TEXT("orc_war_camp")},{TEXT("vikings"),TEXT("viking_forest_track")},{TEXT("nature"),TEXT("nature_forest_clearing")},{TEXT("dark"),TEXT("dark_castle_approach")}};
    if(VisualStep==0)
    {
        int32 Neutral=0,Owned=0,Edges=0;for(const auto& P:State->World.Regions){Edges+=P.Value.Neighbors.Num();if(P.Value.OwnerFactionId.IsNone())++Neutral;else ++Owned;}
        if(State->World.Regions.Num()!=36||Edges!=102||Neutral!=24||Owned!=12||State->GetCampaignSaveSlotName()!=(Controlled?TEXT("Soul.Composition3500.ControlledTurn"):TEXT("Soul.Composition3500.SixFactionProof")))
        {Fail(TEXT("canonical starting ownership/topology/slot"));return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_SIX_FACTION_START regions=36 edges=51 owned=12 neutral=24 ai=OFF"));
        Key(EKeys::I);++VisualStep;return;
    }
    if(VisualStep>=1&&VisualStep<=12)
    {
        if(VisualStep%2==0)
        {if(VisualStep<12)Key(EKeys::I);else Key(EKeys::Home);++VisualStep;return;}
        const FName Id=Ids[(VisualStep-1)/2];FSoulFactionCampaignState F;
        if(!State->InspectFactionArmy(Id,F)||F.Army.FactionId!=Id||Campaign->ViewFaction()!=Id||Campaign->GetSelectedRegion()!=F.Army.RegionId)
        {Fail(TEXT("controller I inspection identity/location"));return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_SIX_FACTION_INSPECT faction=%s army=%s region=%s unit=%s troops=%d ap=%d gold=%d"),
            *Id.ToString(),*F.Army.ArmyId.ToString(),*F.Army.RegionId.ToString(),*F.Army.UnitId.ToString(),F.Army.TroopCount,F.Economy.ActionPoints,F.Economy.Resources.FindRef(TEXT("gold")));
        Capture(TEXT("SixFaction_")+Id.ToString()+TEXT(".png"));
        ++VisualStep;return;
    }
    if(VisualStep==13)
    {
        const TMap<FName,TArray<FName>> Captures={
            {TEXT("humans"),{TEXT("crossroads"),TEXT("old_quarry")}},
            {TEXT("dwarves"),{TEXT("dwarf_forge_approach"),TEXT("dwarf_high_quarry")}},
            {TEXT("orcs"),{TEXT("orc_badlands")}},
            {TEXT("vikings"),{TEXT("viking_fjord_ridge")}},
            {TEXT("nature"),{TEXT("nature_river_woodland")}},
            {TEXT("dark"),{TEXT("dark_ash_plain")}}};
        for(FName Id:Ids)
        {
            const TArray<FName> Path=Controlled?Captures[Id]:TArray<FName>{Secondary[Id]};
            if(Controlled && !State->World.Regions[Path.Last()].OwnerFactionId.IsNone())
            {Fail(TEXT("controlled neutral target already owned"));return;}
            for(FName Destination:Path)
            {
                FSoulFactionCampaignState Before,After;State->InspectFactionArmy(Id,Before);FString Error;
                if(Controlled)
                {
                    FSoulControlledCampaignAction Action;
                    if(!State->PrepareControlledAction(Id,Before.Army.ArmyId,Before.Army.RegionId,Destination,Action,Error)
                        || !State->ExecuteControlledAction(Action,Error)){Fail(*Error);return;}
                    FRBSaveDomainState Stable,Rejected;State->CaptureRBSaveDomain_Implementation(Stable,Error);
                    if(State->ExecuteControlledAction(Action,Error)) {Fail(TEXT("stale proposal replay accepted"));return;}
                    State->CaptureRBSaveDomain_Implementation(Rejected,Error);
                    if(Stable.Fields[0].StringValue!=Rejected.Fields[0].StringValue){Fail(TEXT("rejection mutated state"));return;}
                }
                else if(!State->MoveFactionArmy(Id,Destination,Error)){Fail(*Error);return;}
                State->InspectFactionArmy(Id,After);
                if(After.Army.ArmyId!=Before.Army.ArmyId||After.Army.FactionId!=Id||After.Army.RegionId!=Destination
                    ||After.Economy.ActionPoints!=Before.Economy.ActionPoints-1||State->World.Regions[Destination].OwnerFactionId!=Id)
                {Fail(TEXT("controlled movement/capture state"));return;}
                UE_LOG(LogTemp,Display,TEXT("SOUL_SIX_FACTION_MOVE faction=%s from=%s to=%s ap=%d controlled=%d"),*Id.ToString(),*Before.Army.RegionId.ToString(),*After.Army.RegionId.ToString(),After.Economy.ActionPoints,Controlled);
            }
        }
        if(Controlled)
        {
            int32 Owned=0;for(const auto& P:State->World.Regions)if(!P.Value.OwnerFactionId.IsNone())++Owned;
            if(Owned!=18 || State->HasPendingBattle()){Fail(TEXT("six neutral captures/topology"));return;}
        }
        Key(EKeys::Home);FRBSaveDomainState Saved;FString Error;
        if(!State->CaptureRBSaveDomain_Implementation(Saved,Error)){Fail(*Error);return;}
        ExpectedSnapshot=Saved.Fields[0].StringValue;
        if(!FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")))){Fail(TEXT("snapshot write"));return;}
        Key(EKeys::F5);++VisualStep;return;
    }
    if(VisualStep==14)
    {
        if(State->bPersistenceBusy)return;
        if(!State->bLastSaveSucceeded){Fail(TEXT("controller F5"));return;}
        Capture(TEXT("SixFaction_Saved.png"));++VisualStep;return;
    }
    if(VisualStep==15)
    {
        State->AdvanceDay(); // Real mutation after save; restore must undo it for all six.
        if(State->Economy.Day!=2){Fail(TEXT("post-save mutation"));return;}
        Key(EKeys::F9);++VisualStep;return;
    }
    if(VisualStep==16)
    {
        if(State->bPersistenceBusy)return;FRBSaveDomainState Loaded;FString Error;
        if(!State->bLastLoadSucceeded||!State->CaptureRBSaveDomain_Implementation(Loaded,Error)||Loaded.Fields[0].StringValue!=ExpectedSnapshot)
        {Fail(TEXT("F9 exact all-faction restoration"));return;}
        Capture(TEXT("SixFaction_Restored.png"));
        if(Controlled){UE_LOG(LogTemp,Display,TEXT("SOUL_CONTROLLED_TURN_PASS factions=6 moves=8 neutral_captures=6 replay_rejections=8 owned=18 neutral=18 F5=1 F9=1 exact_snapshot=1 ai=OFF"));}
        else {UE_LOG(LogTemp,Display,TEXT("SOUL_SIX_FACTION_STATE_PASS factions=6 controlled_moves=6 owned=12 neutral=24 controller_I=1 controller_F5=1 controller_F9=1 exact_snapshot=1 ai=OFF"));}
        ++VisualStep;return;
    }
    if(VisualStep==17&&Elapsed>=105){bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}
}
