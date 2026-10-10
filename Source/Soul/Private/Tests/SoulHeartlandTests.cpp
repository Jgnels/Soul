#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
struct FHeartlandFixture {
 UGameInstance* GI; USoulFounderPlaytestStateSubsystem* S;
 FHeartlandFixture(){const FString Old=FCommandLine::Get();FCommandLine::Set(TEXT("-SoulComposition -SoulFourFactionAlpha -SoulHeartland"));GI=NewObject<UGameInstance>();GI->AddToRoot();GI->Init();S=GI->GetSubsystem<USoulFounderPlaytestStateSubsystem>();S->InitializeScenario();FCommandLine::Set(*Old);}
 ~FHeartlandFixture(){GI->Shutdown();GI->RemoveFromRoot();}
 void Day(){S->AdvanceDay();for(int32 I=0;I<3&&S->IsAlphaTurnActive()&&!S->HasPendingBattle();++I)S->RunNextAlphaAction();}
};
FString HeartlandSnapshot(USoulFounderPlaytestStateSubsystem* S){FRBSaveDomainState D;FString E;S->CaptureRBSaveDomain_Implementation(D,E);return D.Fields[0].StringValue;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandProgressionTest,"Soul.Integration.Heartland.PaidProgressionAndAffinity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandProgressionTest::RunTest(const FString&){
 FHeartlandFixture F;auto* S=F.S;FString E;
 TestTrue(TEXT("isolated profile initializes"),S->bInitialized&&S->IsHeartlandEnabled());
 TestEqual(TEXT("distinct save slot"),S->GetCampaignSaveSlotName(),FString(TEXT("Soul.Composition3500.HeartlandAlpha")));
 TestTrue(TEXT("Frost affinity"),S->HeartlandContent.AllowsSchool(TEXT("Frost")));
 TestFalse(TEXT("Fire forbidden"),S->HeartlandContent.AllowsSchool(TEXT("Fire")));
 TestEqual(TEXT("no free starting spells"),S->Hero.KnownSpells.Num(),0);
 const int32 Gold=S->Economy.Resources.FindRef(TEXT("gold"));
 TestTrue(TEXT("paid guild construction"),S->BeginSettlementConstruction(TEXT("human.arcane_hall"),E));
 TestEqual(TEXT("real cost"),S->Economy.Resources.FindRef(TEXT("gold")),Gold-300);
 const FString Before=HeartlandSnapshot(S);
 TestFalse(TEXT("second construction same day rejected"),S->BeginSettlementConstruction(TEXT("human.tavern"),E));
 TestEqual(TEXT("rejection leaves campaign unchanged"),HeartlandSnapshot(S),Before);
 TestFalse(TEXT("spell unavailable while building"),S->Hero.KnownSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard")));
 F.Day();TestTrue(TEXT("completed one-day guild learns frost"),S->Hero.KnownSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard")));
 FSoulCampaignBattleDescriptor D;S->ConfigureHeartlandMagic(D);
 TestTrue(TEXT("battle receives exact learned spell"),D.AllowedPlayerSpells.Contains(TEXT("Magic.Spell.Ice.Blizzard")));
 TestFalse(TEXT("locked second-tier spell excluded"),D.AllowedPlayerSpells.Contains(TEXT("Magic.Spell.Water.TidalWard")));
 const auto* Scenario=S->GetSettlementScenario();
 TestEqual(TEXT("tier two duration"),Scenario->FindDevelopmentDefinition(TEXT("human.mage_academy"))->BuildDays,2);
 TestEqual(TEXT("tier three duration"),Scenario->FindDevelopmentDefinition(TEXT("human.high_conclave"))->BuildDays,3);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandSiteTest,"Soul.Integration.Heartland.SiteAndTwoDomainRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandSiteTest::RunTest(const FString&){
 FHeartlandFixture F;auto* S=F.S;FString E;
 TestTrue(TEXT("normal paid move to site"),S->MovePlayerTo(TEXT("crossroads")));
 const int32 Gold=S->Economy.Resources.FindRef(TEXT("gold")),AP=S->Economy.ActionPoints;
 TestTrue(TEXT("work owned windmill"),S->InteractHeartlandSite(E));
 TestEqual(TEXT("site reward"),S->Economy.Resources.FindRef(TEXT("gold")),Gold+100);
 TestEqual(TEXT("site costs real AP"),S->Economy.ActionPoints,AP-1);
 const FString Before=HeartlandSnapshot(S);
 TestFalse(TEXT("same-day repeat rejected"),S->InteractHeartlandSite(E));
 TestEqual(TEXT("repeat rejection atomic"),HeartlandSnapshot(S),Before);
 FRBSaveDomainState Campaign,Town;S->CaptureRBSaveDomain_Implementation(Campaign,E);
 auto* Settlements=F.GI->GetSubsystem<USoulSettlementStateSubsystem>();Settlements->CaptureRBSaveDomain_Implementation(Town,E);
 FHeartlandFixture Restored;
 TestTrue(TEXT("settlement domain restores"),Restored.GI->GetSubsystem<USoulSettlementStateSubsystem>()->RestoreRBSaveDomain_Implementation(Town,E));
 TestTrue(TEXT("campaign domain restores"),Restored.S->RestoreRBSaveDomain_Implementation(Campaign,E));
 TestEqual(TEXT("exact campaign including remote region and cooldown"),HeartlandSnapshot(Restored.S),Before);
 TestFalse(TEXT("load cannot farm duplicate site reward"),Restored.S->InteractHeartlandSite(E));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandEnvironmentTest,"Soul.Integration.Heartland.CapitalEnvironmentAdmission",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandEnvironmentTest::RunTest(const FString&){
 FHeartlandFixture F;FString E;FSoulCampaignBattleDescriptor D;D.TargetRegion=TEXT("human_capital");D.MapPackage=TEXT("/Game/UnrelatedFallback");
 TestTrue(TEXT("city binding exists"),F.S->ApplySettlementEnvironment(D,E));
 TestEqual(TEXT("capital uses authored city"),D.MapPackage,FName(TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored")));
 TestEqual(TEXT("measured authored approach"),D.ArenaOrigin,FVector(-9000,21000,400));
 TMap<FName,FSoulSettlementEnvironmentBinding> Registry;
 TestTrue(TEXT("registry parses"),FSoulSettlementEnvironmentRegistry::Load(Registry,E));
 TestTrue(TEXT("unimplemented siege remains unset"),Registry.FindChecked(TEXT("human_capital")).SiegeEnvironment.IsEmpty());
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandInjuryTest,"Soul.Integration.Heartland.HeroInjuryCaptureAndRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandInjuryTest::RunTest(const FString&){
 FHeartlandFixture F;auto* S=F.S;FString E;
 FSoulHeroRules::ApplyBattleInjury(S->Hero,true,5,TEXT("orcs"),TEXT("crossroads"));
 TestTrue(TEXT("surviving army leaves wounded hero"),S->Hero.Condition==ESoulHeroCondition::Wounded);
 TestFalse(TEXT("wounded hero cannot siege"),FSoulHeroRules::CanCommandSiege(S->Hero));
 TestFalse(TEXT("wounded hero cannot diplomacy"),FSoulHeroRules::CanPerformDiplomacy(S->Hero));
 S->Hero.Skills.Add(TEXT("Adventure"),2);S->Economy.ActionPoints=0;FSoulControlledCampaignAction Move;
 TestFalse(TEXT("wounded hero grants no free controlled travel"),S->PrepareControlledAction(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"),Move,E));
 FSoulCampaignBattleDescriptor D;S->ConfigureHeartlandMagic(D);TestFalse(TEXT("wounded hero not instantiated in combat"),D.bPlayerHeroAvailable);
 TestTrue(TEXT("wounded hero grants no spells"),D.AllowedPlayerSpells.IsEmpty());
 FSoulHeroRules::AdvanceRecovery(S->Hero);FSoulHeroRules::AdvanceRecovery(S->Hero);
 TestTrue(TEXT("not recovered early"),S->Hero.Condition==ESoulHeroCondition::Wounded);
 FSoulHeroRules::AdvanceRecovery(S->Hero);TestTrue(TEXT("bounded recovery"),FSoulHeroRules::IsAvailable(S->Hero));
 FSoulHeroRules::ApplyBattleInjury(S->Hero,true,0,TEXT("orcs"),TEXT("crossroads"));
 TestTrue(TEXT("destroyed army captures wounded hero"),S->Hero.Condition==ESoulHeroCondition::Captured);
 TestEqual(TEXT("captor identity"),S->Hero.CaptorFaction,FName(TEXT("orcs")));
 FRBSaveDomainState Save;S->CaptureRBSaveDomain_Implementation(Save,E);
 FHeartlandFixture Fresh;TestTrue(TEXT("captivity persists through restore"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,E));
 TestEqual(TEXT("exact captured state"),HeartlandSnapshot(Fresh.S),HeartlandSnapshot(S));
 FSoulHeroRules::AdvanceRecovery(Fresh.S->Hero);TestTrue(TEXT("captured hero never auto-recovers"),Fresh.S->Hero.Condition==ESoulHeroCondition::Captured);
 FSoulHeroState Injured;FSoulHeroRules::ApplyBattleInjury(Injured,true,8,TEXT("orcs"),TEXT("crossroads"));
 FSoulHeroRules::AdvanceRecovery(Injured);FSoulHeroRules::ApplyBattleInjury(Injured,false,6,TEXT("orcs"),TEXT("crossroads"));
 TestEqual(TEXT("existing wound recovery not restarted"),Injured.RecoveryDays,2);
 FSoulHeroRules::ApplyBattleInjury(Injured,false,0,TEXT("vikings"),TEXT("river_ford"));
 TestTrue(TEXT("wounded commander captured when remaining company destroyed"),Injured.Condition==ESoulHeroCondition::Captured);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandCommanderTest,"Soul.Integration.Heartland.ExactDwarfCommanderAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandCommanderTest::RunTest(const FString&){
 FHeartlandFixture F;auto* S=F.S;FString E;FSoulCampaignBattleDescriptor D;D.PlayerFaction=TEXT("dwarves");D.EnemyFaction=TEXT("humans");
 S->ConfigureHeartlandMagic(D);TestEqual(TEXT("exact commander admitted"),D.NonPlayerHeroId,FName(TEXT("dwarf_king_commander")));
 TestEqual(TEXT("commander has correct faction"),D.NonPlayerHeroFaction,FName(TEXT("dwarves")));
 FSoulHeroRules::ApplyBattleInjury(S->DwarfCommander,true,0,TEXT("humans"),TEXT("human_capital"));
 FSoulCampaignBattleDescriptor Captured;Captured.PlayerFaction=D.PlayerFaction;Captured.EnemyFaction=D.EnemyFaction;S->ConfigureHeartlandMagic(Captured);
 TestTrue(TEXT("captured enemy commander not re-instantiated"),Captured.NonPlayerHeroId.IsNone());
 FRBSaveDomainState Save;S->CaptureRBSaveDomain_Implementation(Save,E);FHeartlandFixture Fresh;
 TestTrue(TEXT("enemy commander restores"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,E));
 TestEqual(TEXT("both heroes persist exactly"),HeartlandSnapshot(Fresh.S),HeartlandSnapshot(S));
 TestEqual(TEXT("captor recorded"),Fresh.S->DwarfCommander.CaptorFaction,FName(TEXT("humans")));
 return true;
}
#endif
