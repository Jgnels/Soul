#include "SoulFounderPlaytestGameMode.h"
#include "SoulSettlementVisitGameMode.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestHUD.h"
#include "SoulCampaignCamera.h"
#include "SoulPlaytestRegionActor.h"
#include "SoulCampaignWorldActor.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulSettlementBuildingActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif
namespace {
FString DepthPhase(){FString P;FFileHelper::LoadFileToString(P,*(FPaths::ProjectSavedDir()/TEXT("HeartlandDepthPhase.txt")));return P;}
void SetDepthPhase(const TCHAR* P){FFileHelper::SaveStringToFile(P,*(FPaths::ProjectSavedDir()/TEXT("HeartlandDepthPhase.txt")));}
void DepthFail(const FString& Why){UE_LOG(LogTemp,Error,TEXT("SOUL_DEPTH_FAIL %s"),*Why);FPlatformMisc::RequestExitWithStatus(false,1);}
void DepthShot(const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(TEXT("Depth_")+Name+TEXT(".png")),true,false);}
}
void ASoulFounderPlaytestGameMode::TickHeartlandDepthQualification(float Seconds)
{
 if(!State||!Campaign||!State->bInitialized||State->bPersistenceBusy||State->IsAlphaTurnActive()||State->HasPendingBattle())return;
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()>0)return;
#endif
 Elapsed+=Seconds;if(Elapsed<8)return;
 FString P=DepthPhase(),Error;auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
 auto Key=[&](FKey K){PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,IE_Released,0));};
 auto Build=[&](FName Id){if(!State->BeginSettlementConstruction(Id,Error)){DepthFail(Id.ToString()+TEXT(": ")+Error);return false;}return true;};
 auto Day=[&](const TCHAR* Next){State->AdvanceDay();SetDepthPhase(Next);Elapsed=0;};
 auto* TownAuthority=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
 const auto* Town=TownAuthority->FindSettlement(TEXT("human_capital"));
 auto Level=[&](FName Id){const auto* B=Town?Town->Buildings.Find(Id):nullptr;return B?B->Level:0;};
 if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandDiplomacyProof")))
 {
  auto* HUD=Cast<ASoulFounderPlaytestHUD>(PC->GetHUD());if(!HUD){DepthFail(TEXT("diplomacy HUD missing"));return;}
  auto Act=[&](int32 A){HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("DiplomacyAction:%d"),A)));};
  if(P.IsEmpty())
  {
   if(VisualStep==0){Key(EKeys::F9);VisualStep=1;Elapsed=0;return;}
   if(VisualStep==1){if(!State->bLastLoadSucceeded){DepthFail(TEXT("diplomacy legacy paid checkpoint"));return;}Key(EKeys::L);VisualStep=2;Elapsed=0;return;}
   if(VisualStep==2){DepthShot(TEXT("Diplomacy_Initial_Reasons"));VisualStep=3;Elapsed=0;return;}
   if(VisualStep==3){Act(3);VisualStep=4;Elapsed=0;return;}
   if(VisualStep==4){Act(3);VisualStep=5;Elapsed=0;return;}
   if(VisualStep==5){Act(1);if(State->DiplomaticRelation(TEXT("dwarves")).Stance!=ESoulDiplomaticStance::Peace||State->Economy.ActionPoints!=0){DepthFail(TEXT("paid diplomatic peace"));return;}VisualStep=6;Elapsed=0;return;}
   if(VisualStep==6){DepthShot(TEXT("Diplomacy_Peace"));VisualStep=7;Elapsed=0;return;}
   if(VisualStep==7){Campaign->CancelPanel();VisualStep=0;SetDepthPhase(TEXT("diplomacy_day"));State->AdvanceDay();Elapsed=0;return;}
  }
  if(P==TEXT("diplomacy_day"))
  {
   if(VisualStep==0){if(!Campaign->bDiplomacyPanel)Key(EKeys::L);VisualStep=1;Elapsed=0;return;}
   if(VisualStep==1){if(!Campaign->bDiplomacyPanel){DepthFail(TEXT("diplomacy input panel did not open"));return;}Act(2);if(State->DiplomaticRelation(TEXT("dwarves")).Stance!=ESoulDiplomaticStance::NonAggression){DepthFail(TEXT("paid NAP: ")+Campaign->LastMessage);return;}VisualStep=2;Elapsed=0;return;}
   if(VisualStep==2){FRBSaveDomainState S;State->CaptureRBSaveDomain_Implementation(S,Error);ExpectedSnapshot=S.Fields[0].StringValue;FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));Key(EKeys::F5);VisualStep=3;Elapsed=0;return;}
   if(VisualStep==3){if(!State->bLastSaveSucceeded){DepthFail(TEXT("diplomacy F5"));return;}Act(4);if(State->DiplomaticRelation(TEXT("dwarves")).Stance!=ESoulDiplomaticStance::War){DepthFail(TEXT("explicit treaty break"));return;}VisualStep=4;Elapsed=0;return;}
   if(VisualStep==4){DepthShot(TEXT("Diplomacy_Betrayal"));VisualStep=5;Elapsed=0;return;}
   if(VisualStep==5){Key(EKeys::F9);VisualStep=6;Elapsed=0;return;}
   if(VisualStep==6){FRBSaveDomainState S;State->CaptureRBSaveDomain_Implementation(S,Error);if(!State->bLastLoadSucceeded||S.Fields[0].StringValue!=ExpectedSnapshot||State->DiplomacyAllowsHostility(TEXT("dwarves"),TEXT("humans"))){DepthFail(TEXT("exact diplomacy F9 treaty/treasury restoration"));return;}if(!Campaign->bDiplomacyPanel)Key(EKeys::L);VisualStep=7;Elapsed=0;return;}
   DepthShot(TEXT("Diplomacy_Restored_Pact"));SetDepthPhase(TEXT("diplomacy_pass"));Elapsed=0;return;
  }
  if(P==TEXT("diplomacy_pass")){UE_LOG(LogTemp,Display,TEXT("SOUL_DIPLOMACY_PROOF_PASS paid_gifts=2 peace=1 real_AI_day=1 pact=1 betrayal=1 F5=1 F9_exact=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}
  return;
 }
 // Render review uses a copied paid checkpoint and ordinary travel/site inputs.
 // It never reveals fog, changes ownership or grants resources for a screenshot.
 if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandPresentationReview")))
 {
  auto FocusSite=[&](FName Id)
  {
   auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget());if(!C)return;
   FBox Bounds(ForceInit);
   for(TActorIterator<ASoulPlaytestRegionActor> It(GetWorld());It;++It)if(It->RegionId==Id)
   {TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);for(auto* Part:Parts)if(Part->ComponentHasTag(TEXT("Soul.HeartlandSite")))Bounds+=Part->Bounds.GetBox();}
   if(!Bounds.IsValid){DepthFail(TEXT("site art bounds missing"));return;}
   C->Focus(Bounds.GetCenter());C->Zoom(-100);C->Zoom(10);C->Orbit(1,.5f);
  };
  auto Move=[&](FName Id){Campaign->SelectCompany();Campaign->HandleRegionClicked(Id);if(State->PlayerRegion!=Id){DepthFail(TEXT("site review legal travel: ")+Id.ToString());return false;}FocusSite(Id);return true;};
  if(P.IsEmpty())
  {
   if(VisualStep==0){Key(EKeys::F9);VisualStep=1;Elapsed=0;return;}
   if(!State->bLastLoadSucceeded||Level(TEXT("human.market"))!=2){DepthFail(TEXT("review paid checkpoint"));return;}
   if(VisualStep==1){if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget())){C->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("human_capital")));C->Orbit(1,2);C->Zoom(8);}Campaign->ToggleTownPanel();VisualStep=2;Elapsed=0;return;}
   if(VisualStep==2){DepthShot(TEXT("Final_UI_1280x800"));VisualStep=3;Elapsed=0;return;}
   Campaign->CancelPanel();SetDepthPhase(TEXT("upgraded_visit"));Campaign->VisitSettlement();return;
  }
  if(P==TEXT("upgraded_done"))
  {
   if(VisualStep==0){if(!Move(TEXT("crossroads")))return;VisualStep=1;Elapsed=0;return;}
   if(VisualStep==1){DepthShot(TEXT("Windmill_Strategic"));if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget()))C->Zoom(5);VisualStep=2;Elapsed=0;return;}
   if(VisualStep==2){DepthShot(TEXT("Windmill_Close"));VisualStep=3;Elapsed=0;return;}
   if(VisualStep==3){if(!Move(TEXT("old_quarry")))return;VisualStep=4;Elapsed=0;return;}
   if(VisualStep==4){DepthShot(TEXT("Quarry_Strategic"));if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget()))C->Zoom(5);VisualStep=5;Elapsed=0;return;}
   if(VisualStep==5){const int32 Gold=State->Economy.Resources.FindRef(TEXT("gold"));if(!State->InteractHeartlandSite(Error)){DepthFail(Error);return;}DepthShot(TEXT("Quarry_Interaction_Close"));UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_QUARRY_REWARD delta=%d"),State->Economy.Resources.FindRef(TEXT("gold"))-Gold);VisualStep=6;Elapsed=0;return;}
   VisualStep=0;Day(TEXT("review_shrine"));return;
  }
  if(P==TEXT("review_shrine"))
  {
   if(VisualStep==0){if(!Move(TEXT("ancient_shrine")))return;VisualStep=1;Elapsed=0;return;}
   if(VisualStep==1){FRBSaveDomainState Before,After;State->CaptureRBSaveDomain_Implementation(Before,Error);const bool Accepted=State->InteractHeartlandSite(Error);State->CaptureRBSaveDomain_Implementation(After,Error);if(Accepted||Before.Fields[0].StringValue!=After.Fields[0].StringValue){DepthFail(TEXT("full-mana shrine rejection"));return;}DepthShot(TEXT("Shrine_Strategic"));if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget()))C->Zoom(5);VisualStep=2;Elapsed=0;return;}
   if(VisualStep==2){DepthShot(TEXT("Shrine_Close"));VisualStep=3;Elapsed=0;return;}
   if(VisualStep==3){FRBSaveDomainState S;State->CaptureRBSaveDomain_Implementation(S,Error);ExpectedSnapshot=S.Fields[0].StringValue;FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));Key(EKeys::F5);VisualStep=4;Elapsed=0;return;}
   if(VisualStep==4){if(!State->bLastSaveSucceeded){DepthFail(TEXT("site review F5"));return;}Key(EKeys::F9);VisualStep=5;Elapsed=0;return;}
   FRBSaveDomainState S;State->CaptureRBSaveDomain_Implementation(S,Error);if(!State->bLastLoadSucceeded||S.Fields[0].StringValue!=ExpectedSnapshot){DepthFail(TEXT("remote shrine exact restoration"));return;}
   FString VisualFailure;
   if(FFileHelper::LoadFileToString(VisualFailure,*(FPaths::ProjectSavedDir()/TEXT("DepthVisualFailure.txt")))){DepthFail(VisualFailure);return;}
   UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_PRESENTATION_PASS paid_city=1 sites=3 quarry_reward=1 full_mana_reject=1 remote_F5_F9=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
  }
  return;
 }
 // Continue a copied, genuinely paid progression checkpoint through two normal
 // player clicks. No encounter fixture, troop grant, ownership edit or free AP.
 if(FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandCompanyBattle")))
 {
  if(P!=TEXT("company_battle"))
  {
   if(VisualStep==0){Key(EKeys::F9);VisualStep=1;Elapsed=0;return;}
   if(VisualStep==1)
   {
    if(!State->bLastLoadSucceeded||State->PlayerRegion!=TEXT("human_capital")||State->PlayerArmy.FindRef(TEXT("human_archer"))<=0||State->PlayerArmy.FindRef(TEXT("human_guard"))<=0)
    {DepthFail(TEXT("paid company checkpoint unavailable"));return;}
    Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("crossroads"));
    if(State->PlayerRegion!=TEXT("crossroads")){DepthFail(TEXT("paid company legal Crossroads move"));return;}
    VisualStep=2;Elapsed=0;return;
   }
   Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("forest_edge"));
   FSoulCampaignBattleDescriptor Encounter; if(!Campaign->IsBattleAvailable()||!State->BuildBattleDescriptor(TEXT("forest_edge"),Encounter,Error)||Encounter.PlayerFaction!=TEXT("humans")||Encounter.EnemyFaction!=TEXT("orcs")){DepthFail(TEXT("natural Forest Edge exact enemy no longer available"));return;}
   SetDepthPhase(TEXT("company_battle"));Campaign->StartBattle();return;
  }
  const auto& Result=State->LastBattleResult;
  const FName ExpectedRegion=Result.bPlayerWon?FName(TEXT("forest_edge")):FName(TEXT("crossroads"));
  if(Result.TargetRegion!=TEXT("forest_edge")||State->PlayerRegion!=ExpectedRegion||(VisualStep==0&&Result.PlayerCompanies.Num()!=3))
  {DepthFail(TEXT("exact paid-company natural result/return"));return;}
  if(VisualStep==0)for(const auto& Company:Result.PlayerCompanies)if(State->PlayerArmy.FindRef(Company.Key)!=Company.Value)
  {DepthFail(TEXT("company casualties diverged from campaign"));return;}
  FRBSaveDomainState Snapshot;if(!State->CaptureRBSaveDomain_Implementation(Snapshot,Error)){DepthFail(Error);return;}
  if(VisualStep==0){ExpectedSnapshot=Snapshot.Fields[0].StringValue;FFileHelper::SaveStringToFile(ExpectedSnapshot,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));Key(EKeys::F5);VisualStep=1;Elapsed=0;return;}
  if(VisualStep==1){if(!State->bLastSaveSucceeded){DepthFail(TEXT("company result F5"));return;}Key(EKeys::F9);VisualStep=2;Elapsed=0;return;}
  if(!State->bLastLoadSucceeded||Snapshot.Fields[0].StringValue!=ExpectedSnapshot){DepthFail(TEXT("company result exact F9"));return;}
  if(VisualStep==2){DepthShot(TEXT("Company_Battle_Return"));VisualStep=3;Elapsed=0;return;}
  UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_COMPANY_BATTLE_PASS natural_win=%d infantry=%d archers=%d guards=%d region=%s paid_checkpoint=1 F5=1 F9=1"),Result.bPlayerWon,State->PlayerArmy.FindRef(TEXT("human_knight")),State->PlayerArmy.FindRef(TEXT("human_archer")),State->PlayerArmy.FindRef(TEXT("human_guard")),*State->PlayerRegion.ToString());
  bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);return;
 }
 if(P.IsEmpty())
 {
  if(VisualStep==0){if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget())){C->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("human_capital")));C->Orbit(1,2);C->Zoom(8);}Campaign->ToggleTownPanel();VisualStep=1;Elapsed=0;return;}
  if(VisualStep==1){DepthShot(TEXT("Starting_Miniature_UI"));VisualStep=2;Elapsed=0;return;}
  Campaign->CancelPanel();SetDepthPhase(TEXT("starting_visit"));Campaign->VisitSettlement();return;
 }
 if(P==TEXT("starting_done"))
 {
  if(!Build(TEXT("human.arcane_hall")))return;
  for(int32 I=0;I<3;++I)if(!State->Recruit(TEXT("human_archer"))){DepthFail(TEXT("paid archer recruitment"));return;}
  Day(TEXT("guild"));return;
 }
 if(P==TEXT("guild")){if(!State->Hero.KnownSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard"))){DepthFail(TEXT("guild service unlock"));return;}if(Build(TEXT("human.tavern")))Day(TEXT("tavern"));return;}
 if(P==TEXT("tavern")){if(!State->HireTavernHero()||!State->bCompanionAssigned){DepthFail(TEXT("paid companion hiring/assignment"));return;}if(Build(TEXT("human.barracks")))Day(TEXT("barracks_one"));return;}
 if(P==TEXT("barracks_one")){Day(TEXT("barracks_two"));return;}
 if(P==TEXT("barracks_two"))
 {
  if(Level(TEXT("human.barracks"))<2){DepthFail(TEXT("two-day veteran barracks"));return;}
  for(int32 I=0;I<3;++I)if(!State->Recruit(TEXT("human_guard"))){DepthFail(TEXT("paid veteran recruitment"));return;}
  if(Build(TEXT("human.market")))Day(TEXT("market_one"));return;
 }
 if(P==TEXT("market_one")){Day(TEXT("market_two"));return;}
 if(P==TEXT("market_two"))
 {
  if(Level(TEXT("human.market"))!=2||Level(TEXT("human.barracks"))!=2||Level(TEXT("human.arcane_hall"))!=1||!State->bCompanionAssigned){DepthFail(TEXT("paid three-building progression"));return;}
  if(VisualStep==0){if(auto* C=Cast<ASoulCampaignCamera>(PC->GetViewTarget())){C->Focus(ASoulCampaignWorldActor::Locations().FindRef(TEXT("human_capital")));C->Orbit(1,2);C->Zoom(8);}Campaign->ToggleTownPanel();VisualStep=1;Elapsed=0;return;}
  if(VisualStep==1){DepthShot(TEXT("Upgraded_Miniature_UI"));VisualStep=2;Elapsed=0;return;}
  if(VisualStep==2){Campaign->CancelPanel();FRBSaveDomainState S;if(!State->CaptureRBSaveDomain_Implementation(S,Error)){DepthFail(Error);return;}FFileHelper::SaveStringToFile(S.Fields[0].StringValue,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));Key(EKeys::F5);VisualStep=3;Elapsed=0;return;}
  if(!State->bLastSaveSucceeded){DepthFail(TEXT("progression F5"));return;}
  UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_PAID_STATE day=%d gold=%d infantry=%d ranged=%d guard=%d companion=%d guild=%d barracks=%d market=%d"),State->Economy.Day,State->Economy.Resources.FindRef(TEXT("gold")),State->PlayerArmy.FindRef(TEXT("human_knight")),State->PlayerArmy.FindRef(TEXT("human_archer")),State->PlayerArmy.FindRef(TEXT("human_guard")),State->bCompanionAssigned,Level(TEXT("human.arcane_hall")),Level(TEXT("human.barracks")),Level(TEXT("human.market")));
  SetDepthPhase(TEXT("upgraded_visit"));Campaign->VisitSettlement();return;
 }
 if(P==TEXT("upgraded_done"))
 {
  if(VisualStep==0){Campaign->SelectCompany();Campaign->HandleRegionClicked(TEXT("crossroads"));if(State->PlayerRegion!=TEXT("crossroads")||!State->InteractHeartlandSite(Error)){DepthFail(TEXT("post-city legal move/site"));return;}DepthShot(TEXT("Windmill_Interaction"));VisualStep=1;Elapsed=0;return;}
  if(VisualStep==1){Key(EKeys::F9);VisualStep=2;Elapsed=0;return;}
  FRBSaveDomainState S;FString Expected;State->CaptureRBSaveDomain_Implementation(S,Error);FFileHelper::LoadFileToString(Expected,*(FPaths::ProjectSavedDir()/TEXT("CampaignInputExpectedSnapshot.json")));
  if(!State->bLastLoadSucceeded||S.Fields[0].StringValue!=Expected||Level(TEXT("human.market"))!=2||Level(TEXT("human.barracks"))!=2||!State->bCompanionAssigned){DepthFail(TEXT("exact F9 company/building/companion restoration"));return;}
  DepthShot(TEXT("Restored_Campaign"));SetDepthPhase(TEXT("passed"));Elapsed=0;return;
 }
 if(P==TEXT("passed")){UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_PASS paid_companies=3 physical_states=3 companion=1 visits=2 F5=1 F9_exact=1"));bDone=true;FPlatformMisc::RequestExitWithStatus(false,0);}
}
void ASoulSettlementVisitGameMode::TickDepthVisitQualification(float Seconds)
{
#if WITH_EDITOR
 if(FAssetCompilingManager::Get().GetNumRemainingAssets()>0)return;
#endif
 if(!Walker||!IsStreamingReady())return;
 auto* PC=GetWorld()->GetFirstPlayerController();if(!PC)return;
 const FString Phase=DepthPhase();const bool Upgraded=Phase==TEXT("upgraded_visit");
 const float Before=WalkingProofTime;WalkingProofTime+=Seconds;
 if(Before<8&&WalkingProofTime>=8)
 {
  int32 Groups=0;
  for(TActorIterator<ASoulSettlementBuildingActor> It(GetWorld());It;++It)
   if(It->SettlementId==TEXT("human_capital")&&(It->BuildingId==TEXT("human.arcane_hall")||It->MinimumBuildingLevel==2))
   {
    if(It->IntactActors.IsEmpty())continue;
    for(const auto& Root:It->IntactActors)if(!Root||Root->IsHidden()==Upgraded){DepthFail(TEXT("physical native building state mismatch: ")+It->BuildingId.ToString());return;}
    ++Groups;
   }
  if(Groups!=3){DepthFail(TEXT("three native groups missing"));return;}
  if(Upgraded&&!Companion)FFileHelper::SaveStringToFile(TEXT("Embodied companion missing after city streaming"),*(FPaths::ProjectSavedDir()/TEXT("DepthVisualFailure.txt")));
  UE_LOG(LogTemp,Display,TEXT("SOUL_DEPTH_CITY_STATE upgraded=%d groups=%d companion=%d"),Upgraded,Groups,Companion!=nullptr);
  DepthProofCamera=GetWorld()->SpawnActor<ACameraActor>();DepthProofCamera->GetCameraComponent()->SetFieldOfView(55);PC->SetViewTarget(DepthProofCamera);
 }
 const FVector Focuses[]={FVector(-6370,1820,800),FVector(-7685,541,850),FVector(-3850,-130,850)};
 const TCHAR* Names[]={TEXT("Guild"),TEXT("Barracks"),TEXT("Market")};
 for(int32 I=0;I<3;++I)
 {
  const float At=9+I*8;
  if(Before<At&&WalkingProofTime>=At&&DepthProofCamera){const FVector Position=Focuses[I]+FVector(-1700,-2000,1900);DepthProofCamera->SetActorLocation(Position);DepthProofCamera->SetActorRotation((Focuses[I]-Position).Rotation());}
  if(Before<At+5&&WalkingProofTime>=At+5)DepthShot(FString(Upgraded?TEXT("Upgraded_"):TEXT("Starting_"))+Names[I]);
 }
 if(Before<34&&WalkingProofTime>=34&&Upgraded&&Companion&&DepthProofCamera)
 {
  const FVector F=Companion->GetActorLocation()+FVector(0,0,35);
  FCollisionQueryParams Q(SCENE_QUERY_STAT(SoulCompanionReview),true);Q.AddIgnoredActor(Companion);
  FVector Best=F+FVector(0,0,180);float BestDistance=0;
  for(int32 I=0;I<12;++I){const FVector Offset=FRotator(0,Companion->GetActorRotation().Yaw+45+I*30,0).RotateVector(FVector(380,0,100));FHitResult H;
   GetWorld()->LineTraceSingleByChannel(H,F,F+Offset,ECC_Visibility,Q);
   if(H.bBlockingHit)UE_LOG(LogTemp,Display,TEXT("SOUL_COMPANION_REVIEW_RAY ray=%d actor=%s component=%s penetrating=%d hit=%s"),I,*GetNameSafe(H.GetActor()),*GetNameSafe(H.GetComponent()),H.bStartPenetrating,*H.ImpactPoint.ToCompactString());
   const FVector Position=H.bBlockingHit?H.ImpactPoint-Offset.GetSafeNormal()*30:F+Offset;
   const float D=FVector::DistSquared(F,Position);if(D>BestDistance){BestDistance=D;Best=Position;}if(!H.bBlockingHit)break;}
  UE_LOG(LogTemp,Display,TEXT("SOUL_COMPANION_REVIEW body=%s camera=%s clearance_cm=%.1f"),*Companion->GetActorLocation().ToString(),*Best.ToString(),FMath::Sqrt(BestDistance));
  if(BestDistance<250.f*250.f)
  {
   FFileHelper::SaveStringToFile(TEXT("Companion has no readable nearby camera clearance"),*(FPaths::ProjectSavedDir()/TEXT("DepthVisualFailure.txt")));
   Best=F+FVector(0,-600,200); // Diagnostic only; the final review still fails.
  }
  DepthProofCamera->SetActorLocation(Best);DepthProofCamera->SetActorRotation((F-Best).Rotation());
 }
 if(Before<39&&WalkingProofTime>=39&&Upgraded&&Companion)DepthShot(TEXT("Rowan_Assigned_Companion"));
 const bool GateReview=Upgraded&&FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandPresentationReview"));
 if(GateReview&&DepthProofCamera)
 {
  if(Before<43&&WalkingProofTime>=43)
  {
   const FVector Focus(5160,980,1100),Position=Focus+FVector(6500,6500,6000);
   DepthProofCamera->SetActorLocation(Position);DepthProofCamera->SetActorRotation((Focus-Position).Rotation());
   FString Rows=TEXT("yaw,lane,distance,x,y,z,normal,step,blocked,actor\n");
   FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
   FCollisionQueryParams Q(SCENE_QUERY_STAT(SoulNativeGateSurvey),true);
   Q.AddIgnoredActor(Walker);if(Companion)Q.AddIgnoredActor(Companion);
   for(float Yaw:{0.f,45.f,90.f,135.f})for(float Lane:{-400.f,0.f,400.f})
   {
    FVector Previous=FVector::ZeroVector;bool HasPrevious=false;
    for(int32 D=-3000;D<=3000;D+=200)
    {
     const FVector P=FVector(5160,980,0)+FRotator(0,Yaw,0).RotateVector(FVector(D,Lane,0));FHitResult H,B;
     const bool Ground=GetWorld()->LineTraceSingleByObjectType(H,P+FVector(0,0,1900),P-FVector(0,0,2000),Objects,Q);
     const FVector At=H.ImpactPoint+FVector(0,0,100);
     const bool Blocked=Ground&&HasPrevious&&GetWorld()->SweepSingleByObjectType(B,Previous,At,FQuat::Identity,Objects,FCollisionShape::MakeSphere(45),Q);
     Rows+=FString::Printf(TEXT("%.0f,%.0f,%d,%.1f,%.1f,%.1f,%.3f,%.1f,%d,%s\n"),Yaw,Lane,D,P.X,P.Y,Ground?H.ImpactPoint.Z:-9999.f,H.ImpactNormal.Z,HasPrevious?At.Z-Previous.Z:0.f,Blocked,*GetNameSafe(Blocked?B.GetActor():H.GetActor()));
     Previous=At;HasPrevious=Ground;
    }
   }
   FFileHelper::SaveStringToFile(Rows,*(FPaths::ProjectSavedDir()/TEXT("HumanGatePhysicalSurvey.csv")));
  }
  if(Before<48&&WalkingProofTime>=48)DepthShot(TEXT("Human_Gate_Context"));
  if(Before<51&&WalkingProofTime>=51)
  {const FVector Focus(5160,980,1100),Position=Focus+FVector(2800,2800,1200);DepthProofCamera->SetActorLocation(Position);DepthProofCamera->SetActorRotation((Focus-Position).Rotation());}
  if(Before<56&&WalkingProofTime>=56)DepthShot(TEXT("Human_Gate_Approach"));
 }
 const float ReturnAt=GateReview?60.f:43.f;
 if(Before<ReturnAt&&WalkingProofTime>=ReturnAt){SetDepthPhase(Upgraded?TEXT("upgraded_done"):TEXT("starting_done"));HandleAction(TEXT("Return"));}
}
