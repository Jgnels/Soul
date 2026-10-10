#include "SoulFounderPlaytestGameMode.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulSettlementStateSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
void ASoulFounderPlaytestGameMode::TickSiegeQualification(float Seconds)
{
 if(!State||!Campaign||!State->bInitialized||State->bPersistenceBusy||State->HasPendingBattle())return;
 Elapsed+=Seconds;if(Elapsed<6)return;
 auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
 auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
 auto Fail=[&](const FString& Why){UE_LOG(LogTemp,Error,TEXT("SOUL_SIEGE_QUAL_FAIL %s"),*Why);bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);};
 FString Error,Phase;const FString PhasePath=FPaths::ProjectSavedDir()/TEXT("SiegePhase.txt");FFileHelper::LoadFileToString(Phase,*PhasePath);
 auto* Town=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
 auto Snapshot=[&](FString& C,FString& T){FRBSaveDomainState D,S;if(!State->CaptureRBSaveDomain_Implementation(D,Error)||!Town->CaptureRBSaveDomain_Implementation(S,Error))return false;C=D.Fields[0].StringValue;T=S.Fields[0].StringValue;return true;};
 const FString CPath=FPaths::ProjectSavedDir()/TEXT("SiegeExpectedCampaign.json"),TPath=FPaths::ProjectSavedDir()/TEXT("SiegeExpectedTown.json");
 if(FParse::Param(FCommandLine::Get(),TEXT("SoulSiegeColdRestore")))
 {
  if(VisualStep==0){Key(EKeys::F9);VisualStep=1;Elapsed=0;return;}
  FString C,T,EC,ET;FFileHelper::LoadFileToString(EC,*CPath);FFileHelper::LoadFileToString(ET,*TPath);
  if(!State->bLastLoadSucceeded||!Snapshot(C,T)||C!=EC||T!=ET){Fail(TEXT("cold restore differs"));return;}
  if(VisualStep==1){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Siege_Cold_Restore.png"),true,false);VisualStep=2;Elapsed=0;return;}
  if(Elapsed<65)return; // Keep the guarded cold-load observation alive long enough to qualify.
  UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_COLD_PASS exact_campaign=1 exact_settlement=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
 }
 if(Phase.IsEmpty())
 {
  FSoulCampaignBattleDescriptor D;
  if(!State->BuildBattleDescriptor(TEXT("human_capital"),D,Error)||!D.bSiege){Fail(TEXT("siege admission: ")+Error);return;}
  Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("human_capital"));
  if(!Campaign->IsBattleAvailable()){Fail(TEXT("normal attack unavailable"));return;}
  FFileHelper::SaveStringToFile(TEXT("battle"),*PhasePath);Campaign->StartBattle();return;
 }
 if(Phase==TEXT("battle"))
 {
  const auto& R=State->LastBattleResult;
  if(!R.bSiege||R.EncounterId.IsNone()){Fail(TEXT("missing real siege result"));return;}
  const FName ExpectedOwner=R.bPlayerWon?FName(TEXT("humans")):FName(TEXT("dwarves"));
  if(State->World.Regions.FindChecked(TEXT("human_capital")).OwnerFactionId!=ExpectedOwner){Fail(TEXT("wrong ownership"));return;}
  UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_CAMPAIGN_RESULT won=%d survivors=%d/%d owner=%s gate=%d hero=%d objective=%d"),R.bPlayerWon,R.PlayerSurvivors,R.EnemySurvivors,*ExpectedOwner.ToString(),R.SiegeGateRemaining,int32(State->Hero.Condition),R.bCourtyardCaptured);
  FString C,T;if(!Snapshot(C,T)){Fail(Error);return;}FFileHelper::SaveStringToFile(C,*CPath);FFileHelper::SaveStringToFile(T,*TPath);
  FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Siege_Campaign_Result.png"),true,false);
  Key(EKeys::F5);FFileHelper::SaveStringToFile(TEXT("saved"),*PhasePath);Elapsed=0;return;
 }
 if(Phase==TEXT("saved"))
 {
  if(!State->bLastSaveSucceeded){Fail(TEXT("F5 failed"));return;}Key(EKeys::F9);FFileHelper::SaveStringToFile(TEXT("restored"),*PhasePath);Elapsed=0;return;
 }
 if(Phase==TEXT("restored"))
 {
  FString C,T,EC,ET;FFileHelper::LoadFileToString(EC,*CPath);FFileHelper::LoadFileToString(ET,*TPath);
  if(!State->bLastLoadSucceeded||!Snapshot(C,T)||C!=EC||T!=ET){Fail(TEXT("F9 exact restoration"));return;}
  UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_QUAL_PASS F5=1 F9_exact=1 natural_result=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);
 }
}
