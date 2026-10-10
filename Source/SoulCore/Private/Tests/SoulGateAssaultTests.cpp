#include "Misc/AutomationTest.h"
#include "SoulSiege.h"
#include "SoulCampaignBattleBridge.h"
#include "SoulHero.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulGateAssaultRulesTest,"Soul.Core.Siege.GateAssaultV0",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSoulGateAssaultRulesTest::RunTest(const FString&)
{
 auto S=FSoulSiegeRules::Begin({});FSoulSiegePreparation P;P.bReinforcedGate=true;auto Strong=FSoulSiegeRules::Begin(P);
 TestTrue(TEXT("upgraded gate stronger"),Strong.GateMaximumIntegrity>S.GateMaximumIntegrity);
 TestFalse(TEXT("cannot capture through closed gate"),FSoulSiegeRules::AdvanceCourtyard(S,1000,4,0));
 TestEqual(TEXT("no closed progress"),S.CourtyardControlMillis,0);
 TestFalse(TEXT("reject negative damage"),FSoulSiegeRules::ApplyGateDamage(S,-10));
 TestFalse(TEXT("reject malformed damage"),FSoulSiegeRules::ApplyGateDamage(S,MAX_int32));
 TestTrue(TEXT("accepted contact damages gate"),FSoulSiegeRules::ApplyGateDamage(S,250));
 TestEqual(TEXT("damage is exact"),S.GateIntegrityPermille,750);TestTrue(TEXT("still blocks"),S.bGatehouseActive);
 TestTrue(TEXT("final damage breaches"),FSoulSiegeRules::ApplyGateDamage(S,750));
 TestFalse(TEXT("breach alone does not win"),S.bVictory);TestEqual(TEXT("one breach"),S.WallBreaches,1);
 TestFalse(TEXT("replayed hit cannot breach again"),FSoulSiegeRules::ApplyGateDamage(S,100));
 for(int I=0;I<5;++I)FSoulSiegeRules::AdvanceCourtyard(S,1000,2,0);
 TestEqual(TEXT("physical attackers accrue control"),S.CourtyardControlMillis,5000);
 FSoulSiegeRules::AdvanceCourtyard(S,1000,2,1);TestEqual(TEXT("defender contests"),S.CourtyardControlMillis,5000);
 FSoulSiegeRules::AdvanceCourtyard(S,1000,0,1);TestEqual(TEXT("abandonment reverses"),S.CourtyardControlMillis,4000);
 TestFalse(TEXT("reject oversized tick"),FSoulSiegeRules::AdvanceCourtyard(S,16000,1,0));
 for(int I=0;I<11;++I)FSoulSiegeRules::AdvanceCourtyard(S,1000,1,0);
 TestTrue(TEXT("uncontested inner capture resolves"),S.bVictory);
 FSoulHeroState H;H.Condition=ESoulHeroCondition::Wounded;TestFalse(TEXT("wounded cannot siege"),FSoulHeroRules::CanCommandSiege(H));
 H.Condition=ESoulHeroCondition::Captured;TestFalse(TEXT("captured cannot siege"),FSoulHeroRules::CanCommandSiege(H));
 return true;
}
#endif
