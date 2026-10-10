#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/CommandLine.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementStateSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
struct FSiegeCampaignFixture
{
 UGameInstance* GI;USoulFounderPlaytestStateSubsystem* S;USoulSettlementStateSubsystem* T;
 FSiegeCampaignFixture(){GI=NewObject<UGameInstance>();GI->AddToRoot();GI->Init();S=GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>();S->InitializeScenario();T=GI->GetSubsystem<USoulSettlementStateSubsystem>();}
 ~FSiegeCampaignFixture(){GI->Shutdown();GI->RemoveFromRoot();}
};
FString SiegeSnapshot(USoulFounderPlaytestStateSubsystem* S){FRBSaveDomainState D;FString E;if(!S->CaptureRBSaveDomain_Implementation(D,E)||D.Fields.IsEmpty())return TEXT("SNAPSHOT_REJECTED:")+E;return D.Fields[0].StringValue;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSiegeCampaignAdmissionTest,"Soul.Integration.SiegeV0.AdmissionAndResults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulSiegeCampaignAdmissionTest::RunTest(const FString&)
{
 const FString Old=FCommandLine::Get();ON_SCOPE_EXIT{FCommandLine::Set(*Old);};
 FCommandLine::Set(TEXT("-SoulComposition -SoulFourFactionAlpha -SoulHeartland -SoulSiegeV0 -SoulSiegeQualification"));
 FSiegeCampaignFixture F;auto* S=F.S;auto* Town=F.T->FindSettlement(TEXT("human_capital"));FString E;FSoulCampaignBattleDescriptor D;
 TestTrue(TEXT("valid siege start"),S->bInitialized&&Town);if(!Town)return false;
 TestTrue(TEXT("fortified enemy capital enters siege"),S->BuildBattleDescriptor(TEXT("human_capital"),D,E));TestTrue(TEXT("distinct context"),D.bSiege&&D.BattlefieldId==TEXT("human_capital_gate_siege"));
 TestEqual(TEXT("native city retained"),D.MapPackage,FName(TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored")));
 const int32 Normal=D.SiegeGateMaximum;Town->FortificationLevel=2;S->BuildBattleDescriptor(TEXT("human_capital"),D,E);TestTrue(TEXT("upgrade strengthens gate"),D.SiegeGateMaximum>Normal);
 Town->FortificationLevel=0;S->BuildBattleDescriptor(TEXT("human_capital"),D,E);TestFalse(TEXT("unfortified approach stays field"),D.bSiege);Town->FortificationLevel=1;
 const auto HealthyHero=S->Hero;
 for(auto Condition:{ESoulHeroCondition::Wounded,ESoulHeroCondition::Captured})
 {S->Hero=HealthyHero;FSoulHeroRules::ApplyBattleInjury(S->Hero,true,Condition==ESoulHeroCondition::Captured?0:10,TEXT("dwarves"),TEXT("human_capital"));const FString Before=SiegeSnapshot(S);TestFalse(TEXT("injured commander rejected before commitment"),S->BeginBattle(TEXT("human_capital")));TestEqual(TEXT("rejection atomic"),SiegeSnapshot(S),Before);}
 S->Hero=HealthyHero;TestTrue(TEXT("legal battle commits"),S->BeginBattle(TEXT("human_capital")));
 const auto Pending=S->PendingBattle;FSoulCampaignBattleResult R;R.EncounterId=Pending.EncounterId;R.TargetRegion=Pending.TargetRegion;R.bPlayerWon=true;R.PlayerSurvivors=FMath::Min(20,Pending.PlayerStrategicCount);R.PlayerManaRemaining=Pending.PlayerMana;for(const auto& C:Pending.PlayerCompanies)R.PlayerCompanies.Add(C.Key,0);R.PlayerCompanies.Add(TEXT("human_knight"),R.PlayerSurvivors);
 const int32 Integrity=Town->WallIntegrityPermille;
 TestFalse(TEXT("field result cannot resolve siege"),S->ApplyBattleResult(R));TestEqual(TEXT("invalid result preserves gate"),Town->WallIntegrityPermille,Integrity);
 R.bSiege=true;R.SiegeGateRemaining=500;R.bCourtyardCaptured=true;
 TestFalse(TEXT("closed-gate courtyard victory rejected"),S->ApplyBattleResult(R));
 R.bCourtyardCaptured=false;R.bTacticalHeroWounded=true;TestTrue(TEXT("natural force-result contract applies"),S->ApplyBattleResult(R));
 TestEqual(TEXT("attacker lawful capture"),S->World.Regions.FindChecked(TEXT("human_capital")).OwnerFactionId,FName(TEXT("humans")));
 TestEqual(TEXT("exact remaining force"),S->GetPlayerTroopCount(),R.PlayerSurvivors);TestEqual(TEXT("gate loss enters existing settlement domain"),Town->WallIntegrityPermille,500);
 TestTrue(TEXT("hero wound preserved"),S->Hero.Condition==ESoulHeroCondition::Wounded);
 TestFalse(TEXT("duplicate result rejected"),S->ApplyBattleResult(R));
 FRBSaveDomainState Campaign,Settlement;if(!TestTrue(TEXT("campaign capture: ")+E,S->CaptureRBSaveDomain_Implementation(Campaign,E)))return false;if(!TestTrue(TEXT("town capture: ")+E,F.T->CaptureRBSaveDomain_Implementation(Settlement,E)))return false;
 FSiegeCampaignFixture Fresh;TestTrue(TEXT("settlement domain restored"),Fresh.T->RestoreRBSaveDomain_Implementation(Settlement,E));TestTrue(TEXT("campaign domain restored"),Fresh.S->RestoreRBSaveDomain_Implementation(Campaign,E));
 TestEqual(TEXT("restored gate"),Fresh.T->GetWallIntegrity(TEXT("human_capital")),500);TestEqual(TEXT("restored campaign exact"),SiegeSnapshot(Fresh.S),SiegeSnapshot(S));
 TestTrue(TEXT("siege result context restored"),Fresh.S->LastBattleResult.bSiege&&Fresh.S->LastBattleResult.SiegeGateRemaining==500&&!Fresh.S->LastBattleResult.bCourtyardCaptured);
 TSharedPtr<FJsonObject> Receipt;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Campaign.Fields[0].StringValue),Receipt);
 FRBSaveDomainState Malformed=Campaign;Receipt->SetNumberField(TEXT("result_siege_gate"),1401);
 Malformed.Fields[0].StringValue.Reset();FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Malformed.Fields[0].StringValue));
 const FString BeforeBad=SiegeSnapshot(Fresh.S);TestFalse(TEXT("invalid saved integrity rejected"),Fresh.S->RestoreRBSaveDomain_Implementation(Malformed,E));TestEqual(TEXT("bad receipt leaves current state unchanged"),SiegeSnapshot(Fresh.S),BeforeBad);
 Receipt->SetNumberField(TEXT("result_siege_gate"),500);Receipt->SetBoolField(TEXT("result_siege_courtyard"),true);
 Malformed.Fields[0].StringValue.Reset();FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Malformed.Fields[0].StringValue));
 TestFalse(TEXT("saved courtyard cannot bypass intact gate"),Fresh.S->RestoreRBSaveDomain_Implementation(Malformed,E));TestEqual(TEXT("contradictory receipt atomic"),SiegeSnapshot(Fresh.S),BeforeBad);
 Receipt->RemoveField(TEXT("result_siege"));Receipt->RemoveField(TEXT("result_siege_gate"));Receipt->RemoveField(TEXT("result_siege_courtyard"));
 Malformed.Fields[0].StringValue.Reset();FJsonSerializer::Serialize(Receipt,TJsonWriterFactory<>::Create(&Malformed.Fields[0].StringValue));
 TestTrue(TEXT("older field-summary checkpoint remains readable"),Fresh.S->RestoreRBSaveDomain_Implementation(Malformed,E));TestTrue(TEXT("old checkpoint clears stale siege receipt"),!Fresh.S->LastBattleResult.bSiege&&Fresh.S->LastBattleResult.SiegeGateRemaining==0&&!Fresh.S->LastBattleResult.bCourtyardCaptured);
 FSiegeCampaignFixture Loss;TestTrue(TEXT("second lawful assault"),Loss.S->BeginBattle(TEXT("human_capital")));const auto LD=Loss.S->PendingBattle;
 FSoulCampaignBattleResult Def;Def.EncounterId=LD.EncounterId;Def.TargetRegion=LD.TargetRegion;Def.bSiege=true;Def.SiegeGateRemaining=LD.SiegeGateIntegrity;Def.PlayerSurvivors=0;Def.EnemySurvivors=FMath::Min(20,LD.EnemyStrategicCount);Def.bTacticalHeroWounded=true;for(const auto& C:LD.PlayerCompanies)Def.PlayerCompanies.Add(C.Key,0);
 TestTrue(TEXT("defender victory applies"),Loss.S->ApplyBattleResult(Def));TestEqual(TEXT("defender retains capital"),Loss.S->World.Regions.FindChecked(TEXT("human_capital")).OwnerFactionId,FName(TEXT("dwarves")));
 TestTrue(TEXT("defeated wounded hero captured"),Loss.S->Hero.Condition==ESoulHeroCondition::Captured);TestEqual(TEXT("captor exact"),Loss.S->Hero.CaptorFaction,FName(TEXT("dwarves")));
 return true;
}
#endif
