#include "Misc/AutomationTest.h"
#include "SoulHeartlandBridgeGeometry.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeartlandBridgeRouteTest,"Soul.RealtimeBattle.HeartlandBridge.MeasuredCrossing",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeartlandBridgeRouteTest::RunTest(const FString&)
{
 using namespace SoulHeartlandBridge;
 TestTrue(TEXT("upstream native water blocks walking"),IsWater(FVector(-151.33,1247.15,0)));
 TestFalse(TEXT("measured deck crosses river"),IsWater(FromDeck(FVector(-550,0,0))));
 TestFalse(TEXT("both deployment banks remain usable"),IsWater(FVector(-1500,-650,0))||IsWater(FVector(1500,650,0)));
 TestTrue(TEXT("off-deck shortcut crosses water"),CrossesWater(FVector(-1500,650,0),FVector(1500,650,0)));
 const FVector West=FromDeck(FVector(-1500,0,0)),East=FromDeck(FVector(1500,0,0));
 TestFalse(TEXT("actual deck corridor remains clear"),CrossesWater(West,East));
 FVector Via;
 TestTrue(TEXT("west bank converges at entrance"),Approach(FVector(-1700,900,0),FVector(1500,650,0),Via));
 TestTrue(TEXT("first waypoint is west entrance"),Via.Equals(West,1));
 TestFalse(TEXT("entrance arrival does not cut through water"),CrossesWater(FVector(-1700,900,0),Via));
 TestTrue(TEXT("east bank has reciprocal routing"),Approach(FVector(1500,650,0),FVector(-1500,650,0),Via));
 TestTrue(TEXT("east entrance is reciprocal"),Via.Equals(East,1));
 TestFalse(TEXT("same-bank move is left alone"),Approach(FVector(1500,650,0),FVector(1700,-650,0),Via));
 const FVector Deck=FromDeck(FVector(700,0,0)),Bank=FromDeck(FVector(1500,800,0));
 TestTrue(TEXT("leaving the deck uses its mouth instead of crossing parapet"),Approach(Deck,Bank,Via));
 TestTrue(TEXT("parapet detour reaches the bank-side mouth"),Via.Equals(East,1));
 TestFalse(TEXT("unobstructed longitudinal deck movement is unchanged"),Approach(FromDeck(FVector(-700,0,0)),FromDeck(FVector(700,0,0)),Via));
 return true;
}
#endif
