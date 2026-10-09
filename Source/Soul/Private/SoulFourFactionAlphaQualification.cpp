#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "UnrealClient.h"

// Player automation is opt-in qualification only. AI turns and every battle use production authority.
// Progress below records test observation, never campaign state or outcomes.
void ASoulFounderPlaytestGameMode::TickFourFactionAlphaQualification(float Seconds)
{
    Elapsed+=Seconds;if(!State||!Campaign||!State->bInitialized||!State->IsFourFactionAlpha()||Elapsed<8)return;
    const double Now=FPlatformTime::Seconds();if(Now<AlphaProofNextTime)return;AlphaProofNextTime=Now+1;
    if(State->bPersistenceBusy||State->HasPendingBattle()||State->IsAlphaTurnActive())return;
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
    auto Fail=[&](const FString& Why){UE_LOG(LogTemp,Error,TEXT("SOUL_ALPHA_FAIL %s"),*Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
    auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
    const FString Root=FPaths::ProjectSavedDir(),ProgressPath=Root/TEXT("AlphaQualificationProgress.json");
    FString Text;TSharedPtr<FJsonObject> P=MakeShared<FJsonObject>();
    const bool Defense=FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaDefenseProof"));
    const bool Attack=FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaAttackProof"));
    const bool Cold=FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaColdContinue"));
    int32 CheckpointDay=10;FParse::Value(FCommandLine::Get(),TEXT("SoulAlphaCheckpointDay="),CheckpointDay);
    CheckpointDay=FMath::Clamp(CheckpointDay,5,10); // Short smoke can restore mid-run without changing gameplay.
    if(FFileHelper::LoadFileToString(Text,*ProgressPath))
    {if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),P)){Fail(TEXT("invalid observation progress"));return;}}
    else {P->SetNumberField(TEXT("phase"),Cold?10:0);P->SetNumberField(TEXT("last_day"),0);}
    auto Write=[&](){FString S;FJsonSerializer::Serialize(P.ToSharedRef(),TJsonWriterFactory<>::Create(&S));return FFileHelper::SaveStringToFile(S,*ProgressPath);};
    FRBSaveDomainState Current;FString Error;if(!State->CaptureRBSaveDomain_Implementation(Current,Error)){Fail(Error);return;}
    const int32 Phase=P->GetIntegerField(TEXT("phase"));
    if(Phase==7)
    {
        if(State->LastBattleResult.TargetRegion!=TEXT("orc_badlands") || State->ResolvedEncounters.IsEmpty())
        {Fail(TEXT("Human B input did not complete the requested battle"));return;}
        P->SetNumberField(TEXT("phase"),3);Write();return;
    }
    if(Phase==10)
    {P->SetNumberField(TEXT("phase"),11);Write();Key(EKeys::F9);return;}
    if(Phase==11)
    {
        FString Expected;FFileHelper::LoadFileToString(Expected,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));
        if(!State->bLastLoadSucceeded||Expected!=Current.Fields[0].StringValue){Fail(TEXT("separate-process alpha restore mismatch"));return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_COLD_RESTORE_PASS day=%d exact=1"),State->Economy.Day);
        P->SetNumberField(TEXT("phase"),3);Write();
        FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Alpha_Cold_Restored.png"),true,false);return;
    }
    if(Phase==2)
    {
        FString Expected;FFileHelper::LoadFileToString(Expected,*(Root/TEXT("AlphaMidExpected.json")));
        if(!State->bLastLoadSucceeded||Expected!=Current.Fields[0].StringValue){Fail(TEXT("midcampaign F9 mismatch"));return;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_MID_SAVE_PASS day=%d exact=1 all_six_armies=1 pools=1 cursor=1 seed=1"),State->Economy.Day);
        P->SetNumberField(TEXT("phase"),3);P->SetNumberField(TEXT("last_day"),0);Write();return;
    }
    if(Phase==6)
    {
        if(!Campaign->IsTownPanelOpen()){Fail(TEXT("recovery town input did not open services"));return;}
        const int32 Issued=P->GetIntegerField(TEXT("recovery_issued"));
        if(Issued<4){Key(EKeys::One);P->SetNumberField(TEXT("recovery_issued"),Issued+1);Write();return;}
        const int32 Gold=P->GetIntegerField(TEXT("recovery_gold"));
        if(State->PlayerArmy.FindRef(State->PlayerUnitId)!=4 || State->Economy.Resources.FindRef(TEXT("gold"))!=Gold-4*P->GetIntegerField(TEXT("recovery_unit_cost"))
            || State->Economy.RecruitmentPools.FindChecked(State->PlayerUnitId).Available!=P->GetIntegerField(TEXT("recovery_pool"))-4)
        {Fail(TEXT("paid Human defeat recovery"));return;}
        Campaign->CancelPanel();P->SetNumberField(TEXT("phase"),3);Write();
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_HUMAN_RECOVERY_PASS cold_restore=1 legal_withdrawal=3 recruited=4 paid_gold=%d pool_spent=4"),Gold-State->Economy.Resources.FindRef(TEXT("gold")));
        return;
    }
    if(Phase==1 && State->Economy.Day>=CheckpointDay+3)
    {P->SetNumberField(TEXT("phase"),2);Write();Key(EKeys::F9);return;}
    if(State->Economy.Day==CheckpointDay && Phase==0)
    {
        if(!FFileHelper::SaveStringToFile(Current.Fields[0].StringValue,*(Root/TEXT("AlphaMidExpected.json")))){Fail(TEXT("checkpoint evidence"));return;}
        Key(EKeys::F5);P->SetNumberField(TEXT("phase"),1);Write();return;
    }
    int32 Target=Defense&&!Cold?2:Attack&&!Cold?3:21;FParse::Value(FCommandLine::Get(),TEXT("SoulAlphaTargetDay="),Target);
    if(State->Economy.Day>=Target)
    {
        if(Phase==5)
        {
            FString Expected;FFileHelper::LoadFileToString(Expected,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));
            if(!State->bLastLoadSucceeded||Expected!=Current.Fields[0].StringValue){Fail(TEXT("autonomous defense F9 mismatch"));return;}
            UE_LOG(LogTemp,Display,TEXT("%s day=%d natural_result=1 F5=1 F9=1 cold_continued=%d"),Attack?TEXT("SOUL_ALPHA_ATTACK_PASS"):TEXT("SOUL_ALPHA_DEFENSE_PASS"),State->Economy.Day,Cold);
            bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
        }
        if(Phase!=4)
        {
            FFileHelper::SaveStringToFile(Current.Fields[0].StringValue,*(Root/TEXT("CampaignInputExpectedSnapshot.json")));
            Key(EKeys::F5);P->SetNumberField(TEXT("phase"),4);Write();
            FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Alpha_Final_Campaign.png"),true,false);return;
        }
        if(!State->bLastSaveSucceeded){Fail(TEXT("final F5"));return;}
        if(Defense||Attack)
        {
            const FName ExpectedTarget=Attack?TEXT("orc_badlands"):TEXT("north_pass");
            if(!Cold && (State->LastBattleResult.TargetRegion!=ExpectedTarget || State->ResolvedEncounters.IsEmpty())){Fail(TEXT("requested Human battle did not resolve"));return;}
            P->SetNumberField(TEXT("phase"),5);Write();State->AdvanceDay();Key(EKeys::F9);return;
        }
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_CAMPAIGN_PASS day=%d turns=%d seed=%d battles=%d cold_continued=%d"),
            State->Economy.Day,State->Economy.Day-1,State->GetAlphaSeed(),State->ResolvedEncounters.Num(),Cold);
        bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    if(P->GetIntegerField(TEXT("last_day"))!=State->Economy.Day)
    {
        P->SetNumberField(TEXT("last_day"),State->Economy.Day);Write();
        // Compact line-delimited snapshots are the one authoritative qualification timeline.
        TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Current.Fields[0].StringValue),O);
        FString Line;FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Line));
        FFileHelper::SaveStringToFile(Line+TEXT("\n"),*(Root/TEXT("AlphaTimeline.jsonl")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_DAY day=%d player=%s troops=%d gold=%d battles=%d"),State->Economy.Day,*State->PlayerRegion.ToString(),State->PlayerArmy.FindRef(State->PlayerUnitId),State->Economy.Resources.FindRef(TEXT("gold")),State->ResolvedEncounters.Num());
        // The scripted Human explores a different frontier from the mountain armies.
        // This is normal UI movement, not an ownership/army fixture.
        FName Destination;
        if((Defense||Attack) && !Cold && State->Economy.Day==1)
        {
            for(FName N:{FName(TEXT("crossroads")),FName(TEXT("forest_edge")),FName(TEXT("north_pass"))})
            {
                Campaign->SelectCompany();Campaign->HandleRegionClicked(N);
                if(State->PlayerRegion!=N){Fail(TEXT("legal defensive fixture staging"));return;}
            }
            UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_HUMAN_STAGED player=humans troops=%d region=north_pass normal_moves=3 AP=0"),State->PlayerArmy.FindRef(State->PlayerUnitId));
        }
        if(Attack && !Cold && State->Economy.Day==2)
        {
            Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("orc_camp"));
            if(State->PlayerRegion!=TEXT("orc_camp")){Fail(TEXT("Human attack approach failed"));return;}
            Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("orc_badlands"));
            P->SetNumberField(TEXT("phase"),7);Write();
            Key(EKeys::B);
            UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_HUMAN_ATTACK_INPUT target=orc_badlands normal_input=1"));return;
        }
        if(Defense && Cold && State->Economy.Day==2 && State->PlayerArmy.FindRef(State->PlayerUnitId)==0)
        {
            for(FName N:{FName(TEXT("forest_edge")),FName(TEXT("crossroads")),FName(TEXT("human_capital"))})
            {
                Campaign->SelectCompany();Campaign->HandleRegionClicked(N);
                if(State->PlayerRegion!=N){Fail(TEXT("defeated Human cannot withdraw through owned regions"));return;}
            }
            const int32 Gold=State->Economy.Resources.FindRef(TEXT("gold"));
            const auto Pool=State->Economy.RecruitmentPools.FindChecked(State->PlayerUnitId);
            P->SetNumberField(TEXT("recovery_gold"),Gold);P->SetNumberField(TEXT("recovery_pool"),Pool.Available);
            P->SetNumberField(TEXT("recovery_unit_cost"),Pool.CostPerUnit.FindRef(TEXT("gold")));P->SetNumberField(TEXT("recovery_issued"),0);
            P->SetNumberField(TEXT("phase"),6);Write();Key(EKeys::T);return;
        }
        if(!Defense && !Attack && State->Economy.Day==1)Destination=TEXT("crossroads");
        else if(!Defense && !Attack && State->Economy.Day==2)Destination=TEXT("river_ford");
        else if(!Defense && !Attack && State->Economy.Day==3)Destination=TEXT("southern_crossing");
        if(!Destination.IsNone() && FSoulWorldRules::CanMove(State->World,State->PlayerRegion,Destination))
        {
            Campaign->SelectCompany();Campaign->HandleRegionClicked(Destination);
            UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_HUMAN_ACTION target=%s actual=%s ap=%d"),*Destination.ToString(),*State->PlayerRegion.ToString(),State->Economy.ActionPoints);
        }
        if(State->Economy.Day==1)FScreenshotRequest::RequestScreenshot(Root/TEXT("Screenshots/Alpha_Player_Turn.png"),true,false);
        return;
    }
    Campaign->EndDay();
}
