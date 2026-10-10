#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
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
 D.TargetRegion=TEXT("forest_edge");
 TestTrue(TEXT("woodland field binding exists"),F.S->ApplySettlementEnvironment(D,E));
 TestEqual(TEXT("woodland never substitutes the city or generic field"),D.MapPackage,FName(TEXT("/Game/Soul/Maps/Battles/L_Heartland_Woodland")));
 TestFalse(TEXT("woodland is not a settlement siege"),D.BattleContext.bSettlementNearby);
 TestEqual(TEXT("measured native woodland origin"),D.ArenaOrigin,FVector(-9000,-9000,-87.0749478874734));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandCompaniesTest,"Soul.Integration.Heartland.ExactCompaniesAndRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandCompaniesTest::RunTest(const FString&){
 FHeartlandFixture F;auto* S=F.S;FString E;
 const int32 Gold=S->Economy.Resources.FindRef(TEXT("gold")),Stock=S->Economy.RecruitmentPools.FindChecked(TEXT("human_archer")).Available;
 TestTrue(TEXT("paid native ranged company"),S->Recruit(TEXT("human_archer")));
 TestEqual(TEXT("real recruit cost"),S->Economy.Resources.FindRef(TEXT("gold")),Gold-180);
 TestEqual(TEXT("finite ranged pool"),S->Economy.RecruitmentPools.FindChecked(TEXT("human_archer")).Available,Stock-1);
 const FString Before=HeartlandSnapshot(S);
 TestFalse(TEXT("guards require veteran barracks"),S->Recruit(TEXT("human_guard")));
 TestEqual(TEXT("locked recruit leaves state exact"),HeartlandSnapshot(S),Before);
 FSoulCampaignBattleDescriptor D;D.PlayerFaction=TEXT("humans");S->ConfigureHeartlandMagic(D);
 TestEqual(TEXT("exact archer count in descriptor"),D.PlayerCompanies.FindRef(TEXT("human_archer")),1);
 TestEqual(TEXT("army strength sums companies"),S->GetPlayerTroopCount(),S->PlayerArmy.FindRef(TEXT("human_knight"))+1);
 FRBSaveDomainState Save;S->CaptureRBSaveDomain_Implementation(Save,E);FHeartlandFixture Fresh;
 TestTrue(TEXT("mixed roster restores"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,E));
 TestEqual(TEXT("companies and finite pools exact"),HeartlandSnapshot(Fresh.S),Before);
 Fresh.S->LastBattleResult.PlayerCompanies.Add(TEXT("human_guard"),99);
 TestTrue(TEXT("second restore remains valid"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,E));
 TestTrue(TEXT("transient company result cannot leak across checkpoints"),Fresh.S->LastBattleResult.PlayerCompanies.IsEmpty());
 TestEqual(TEXT("restored army remains authoritative"),Fresh.S->PlayerArmy.FindRef(TEXT("human_archer")),1);
 S->PlayerArmy[TEXT("human_knight")]=0;
 TestTrue(TEXT("archer-only army remains living"),S->GetPlayerTroopCount()>0);
 TestTrue(TEXT("archer-only army moves normally"),S->MovePlayerTo(TEXT("crossroads")));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandCompanyResultsTest,"Soul.Integration.Heartland.CompanyResultAdmission",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandCompanyResultsTest::RunTest(const FString&){
 FSoulCampaignBattleDescriptor D;D.EncounterId=TEXT("test");D.SourceRegion=TEXT("a");D.TargetRegion=TEXT("b");D.PlayerFaction=TEXT("humans");D.EnemyFaction=TEXT("dwarves");D.PlayerUnitId=TEXT("human_knight");D.EnemyUnitId=TEXT("dwarf_warrior");D.MapPackage=TEXT("/Game/test");D.ReturnMapPackage=TEXT("/Game/campaign");D.PlayerStrategicCount=8;D.EnemyStrategicCount=6;
 D.PlayerCompanies={{TEXT("human_knight"),4},{TEXT("human_archer"),3},{TEXT("human_guard"),1}};
 TestTrue(TEXT("exact roster admitted"),D.IsValid());
 FSoulCampaignBattleResult R;R.EncounterId=D.EncounterId;R.TargetRegion=D.TargetRegion;R.bPlayerWon=true;R.PlayerSurvivors=4;R.PlayerCompanies={{TEXT("human_knight"),2},{TEXT("human_archer"),2},{TEXT("human_guard"),0}};
 TestTrue(TEXT("real casualty ledger admitted"),R.IsValidFor(D));
 R.PlayerCompanies[TEXT("human_guard")]=1;TestFalse(TEXT("invented survivor rejected"),R.IsValidFor(D));
 R.PlayerCompanies.Remove(TEXT("human_guard"));TestFalse(TEXT("missing company rejected"),R.IsValidFor(D));
 D.PlayerCompanies.Add(TEXT("orc_hammer_warrior"),0);TestFalse(TEXT("wrong faction substitution rejected"),D.IsValid());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandDiplomacyTest,"Soul.Integration.Heartland.DiplomacyTransactionsAndTreaties",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandDiplomacyTest::RunTest(const FString&)
{
 FHeartlandFixture F;auto* S=F.S;FString E;
 const FString Before=HeartlandSnapshot(S);FSoulFactionCampaignState Other;S->InspectFactionArmy(TEXT("orcs"),Other);
 const int32 TheirGold=Other.Economy.Resources.FindRef(TEXT("gold")),OurGold=S->Economy.Resources.FindRef(TEXT("gold"));
 TestFalse(TEXT("unmotivated peace refused"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::Peace,E));
 TestEqual(TEXT("refusal spends nothing"),HeartlandSnapshot(S),Before);
 TestTrue(TEXT("first real gift"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::Gift,E));
 TestTrue(TEXT("second real gift"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::Gift,E));
 TestTrue(TEXT("deterministic peace accepted"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::Peace,E));
 S->InspectFactionArmy(TEXT("orcs"),Other);
 TestEqual(TEXT("player paid gifts"),S->Economy.Resources.FindRef(TEXT("gold")),OurGold-500);
 TestEqual(TEXT("recipient received gifts"),Other.Economy.Resources.FindRef(TEXT("gold")),TheirGold+500);
 TestEqual(TEXT("three accepted actions cost movement"),S->Economy.ActionPoints,0);
 TestFalse(TEXT("peace blocks Human hostility"),S->DiplomacyAllowsHostility(TEXT("humans"),TEXT("orcs")));
 TestFalse(TEXT("peace blocks reverse hostility"),S->DiplomacyAllowsHostility(TEXT("orcs"),TEXT("humans")));
 TestTrue(TEXT("unrelated AI war unchanged"),S->DiplomacyAllowsHostility(TEXT("dwarves"),TEXT("orcs")));
 // Isolated native boundary fixture: undefended adjacent land must not be silently captured at peace.
 S->Economy.ActionPoints=3;S->World.Regions.FindChecked(TEXT("crossroads")).OwnerFactionId=TEXT("orcs");
 const FString Protected=HeartlandSnapshot(S);FSoulControlledCampaignAction A;FSoulCampaignBattleDescriptor B;
 TestFalse(TEXT("direct movement cannot capture peaceful land"),S->MovePlayerTo(TEXT("crossroads")));
 TestFalse(TEXT("controlled action rejects pre-mutation"),S->PrepareControlledAction(TEXT("humans"),TEXT("humans.primary"),TEXT("human_capital"),TEXT("crossroads"),A,E));
 TestFalse(TEXT("battle descriptor cannot evade treaty"),S->BuildBattleDescriptor(TEXT("crossroads"),B,E));
 TestEqual(TEXT("all rejections atomic"),HeartlandSnapshot(S),Protected);
 TestTrue(TEXT("three-day pact"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::NonAggression,E));
 const auto Pact=S->DiplomaticRelation(TEXT("orcs"));
 TestTrue(TEXT("pact stays through end day"),FSoulDiplomacyRules::EffectiveStance(Pact,Pact.PactUntilDay)==ESoulDiplomaticStance::NonAggression);
 TestTrue(TEXT("expiry returns to peace, not surprise war"),FSoulDiplomacyRules::EffectiveStance(Pact,Pact.PactUntilDay+1)==ESoulDiplomaticStance::Peace);
 TestFalse(TEXT("cannot hide treaty betrayal as ordinary declaration"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::DeclareWar,E));
 TestTrue(TEXT("explicit break declares war"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::BreakTreaty,E));
 TestTrue(TEXT("hostility restored explicitly"),S->DiplomacyAllowsHostility(TEXT("orcs"),TEXT("humans")));
 TestTrue(TEXT("betrayal remembered"),S->DiplomaticRelation(TEXT("orcs")).BetrayalDay==S->Economy.Day);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandDiplomacySaveTest,"Soul.Integration.Heartland.DiplomacySaveAndAdversarialRejection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandDiplomacySaveTest::RunTest(const FString&)
{
 FHeartlandFixture F;auto* S=F.S;FString E;FRBSaveDomainState Legacy;S->CaptureRBSaveDomain_Implementation(Legacy,E);
 TestFalse(TEXT("legacy default has no added payload"),Legacy.Fields[0].StringValue.Contains(TEXT("heartland_diplomacy")));
 TestTrue(TEXT("paid gift"),S->ExecuteDiplomacy(TEXT("dwarves"),ESoulDiplomaticAction::Gift,E));
 FRBSaveDomainState Save;S->CaptureRBSaveDomain_Implementation(Save,E);FHeartlandFixture Fresh;
 TestTrue(TEXT("relations and both treasuries restore"),Fresh.S->RestoreRBSaveDomain_Implementation(Save,E));
 TestEqual(TEXT("exact diplomatic checkpoint"),HeartlandSnapshot(Fresh.S),HeartlandSnapshot(S));
 const FString Before=HeartlandSnapshot(S);
 TestFalse(TEXT("passive faction unsupported"),S->ExecuteDiplomacy(TEXT("nature"),ESoulDiplomaticAction::Gift,E));
 TestFalse(TEXT("malformed faction unsupported"),S->ExecuteDiplomacy(TEXT("not-a-faction"),ESoulDiplomaticAction::Peace,E));
 TestEqual(TEXT("unsupported rejection atomic"),HeartlandSnapshot(S),Before);
 for(auto Condition:{ESoulHeroCondition::Wounded,ESoulHeroCondition::Captured})
 {S->Hero.Condition=Condition;const FString Injured=HeartlandSnapshot(S);TestFalse(TEXT("unavailable hero cannot negotiate"),S->ExecuteDiplomacy(TEXT("orcs"),ESoulDiplomaticAction::Gift,E));TestEqual(TEXT("injured rejection atomic"),HeartlandSnapshot(S),Injured);}
 S->Hero.Condition=ESoulHeroCondition::Healthy;
 TSharedPtr<FJsonObject> Root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Save.Fields[0].StringValue),Root);
 auto Bad=Root->GetObjectField(TEXT("heartland_diplomacy"))->GetObjectField(TEXT("dwarves"));Bad->SetNumberField(TEXT("relation"),1001);
 FRBSaveDomainState Corrupt=Save;Corrupt.Fields[0].StringValue.Reset();FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Corrupt.Fields[0].StringValue));
 const FString Good=HeartlandSnapshot(Fresh.S);TestFalse(TEXT("corrupt relation rejected"),Fresh.S->RestoreRBSaveDomain_Implementation(Corrupt,E));TestEqual(TEXT("invalid save leaves authority intact"),HeartlandSnapshot(Fresh.S),Good);
 TestTrue(TEXT("legacy snapshot restores with default war"),Fresh.S->RestoreRBSaveDomain_Implementation(Legacy,E));
 TestTrue(TEXT("legacy clears later relations"),Fresh.S->HumanRelations.IsEmpty());
 TestEqual(TEXT("legacy bytes unchanged"),HeartlandSnapshot(Fresh.S),Legacy.Fields[0].StringValue);
 return true;
}
#endif
