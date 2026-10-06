#include "SoulFounderPlaytestHUD.h"
#include "SoulHUDTheme.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
#include "SoulSettlementScenarioData.h"
#include "SoulSettlementStateSubsystem.h"
#include "SoulSettlementVisitGameMode.h"
#include "SoulCampaignWorldActor.h"

namespace
{
ASoulFounderPlaytestCampaignActor* FindCampaign(UWorld* World)
{ for(TActorIterator<ASoulFounderPlaytestCampaignActor> It(World);It;++It)return *It;return nullptr; }
}
bool ASoulFounderPlaytestHUD::IsPointerOverPanel(float X,float Y) const
{ for(const auto& Panel:Panels)if(Panel.IsInside(FVector2D(X,Y)))return true;return false; }
void ASoulFounderPlaytestHUD::DrawHUD()
{
    Super::DrawHUD();Panels.Reset();
    if(!Canvas||!GetWorld()||!GEngine)return;
    auto* C=FindCampaign(GetWorld());auto* Visit=GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>();
    auto* S=C?C->GetState():Visit?Visit->GetState():nullptr;if(!S)return;
    FSoulHUDTheme UI(*this,*Canvas);
    const float Scale=UI.Scale,W=UI.W,H=UI.H;
    const FLinearColor Ink=UI.Ink,Muted=UI.Muted,Gold=UI.Bright,Back=UI.Back;
    auto Text=[&](const FString& T,float X,float Y,FLinearColor Color,float Size=1.f){UI.Text(T,X,Y,Color,Size*1.15f);};
    auto Rect=[&](float X,float Y,float Width,float Height,FLinearColor Color){UI.Rect(X,Y,Width,Height,Color);};
    auto Panel=[&](float X,float Y,float Width,float Height){UI.Panel(X,Y,Width,Height);Panels.Add(FBox2D(FVector2D(X,Y)*Scale,FVector2D(X+Width,Y+Height)*Scale));};
    auto Button=[&](FName Id,const FString& T,float X,float Y,float Width){UI.Button(Id,T,X,Y,Width,27);};
    auto Wrapped=[&](const FString& T,float X,float Y,float Width,FLinearColor Color,int32 MaxLines=3)
    {
        TArray<FString> Words;T.ParseIntoArrayWS(Words);FString Line;int32 Count=0;
        for(const FString& Word:Words)
        {
            FString Trial=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float TW,TH;Canvas->StrLen(GEngine->GetSmallFont(),Trial,TW,TH);
            if(TW*1.15f>Width&&!Line.IsEmpty()){Text(Line,X,Y+Count*17,Color);Line=Word;if(++Count>=MaxLines)return;}else Line=Trial;
        }
        if(!Line.IsEmpty())Text(Line,X,Y+Count*17,Color);
    };
    auto Development=[&](float AtX,float AtY,float Width)
    {
        if(!S->IsSettlementDevelopmentReady())
        {Wrapped(S->GetSettlementDevelopmentError(),AtX,AtY,Width,Gold,3);return;}
        const auto* Scenario=S->GetSettlementScenario();
        const auto* Definition=Scenario->FindDevelopmentDefinition(TEXT("human.tavern"));
        const auto* Authority=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
        const auto* Town=Authority?Authority->FindSettlement(Scenario->SettlementId):nullptr;
        const auto* Building=Town?Town->Buildings.Find(TEXT("human.tavern")):nullptr;
        const FString Condition=Authority?Authority->GetBuildingConditionName(Scenario->SettlementId,TEXT("human.tavern")).ToString():TEXT("Unavailable");
        Text(FString::Printf(TEXT("Tavern: %s  /  level %d"),*Condition,Building?Building->Level:0),AtX,AtY,Gold);
        TArray<FName> Resources;Definition->BuildCost.GetKeys(Resources);Resources.Sort(FNameLexicalLess());
        FString Cost;
        for(FName Resource:Resources)
        {if(!Cost.IsEmpty())Cost+=TEXT(", ");Cost+=FString::Printf(TEXT("%d %s"),Definition->BuildCost[Resource],*Resource.ToString());}
        UI.FitText(FString::Printf(TEXT("Build: %s  /  %d days"),Cost.IsEmpty()?TEXT("no resource cost"):*Cost,Definition->BuildDays),AtX,AtY+23,Width,Ink);
        const FString Progress=Building&&Building->Condition==ESoulBuildingCondition::Building
            ?FString::Printf(TEXT("Construction: %d days remaining"),Building->ConstructionDaysRemaining)
            :S->IsTavernOperational()?TEXT("Companion hiring is available."):TEXT("Complete the tavern to unlock companion hiring.");
        UI.FitText(Progress,AtX,AtY+46,Width,Muted);
    };
    if(Visit)
    {
        Panel(14,12,W-28,60);Text(TEXT("SETTLEMENT VISIT"),28,24,Gold,1.3f);
        Text(FString::Printf(TEXT("DAY %d   GOLD %d"),S->Economy.Day,S->Economy.Resources.FindRef(TEXT("gold"))),300,30,Ink);
        Panel(28,108,500,340);Development(45,126,465);
        if(Visit->IsVisitReady())
        {
            Button(TEXT("BuildTavern"),TEXT("[U] Build tavern"),45,212,465);
            Button(TEXT("EndDay"),TEXT("[Space] Advance day"),45,247,465);
            if(S->IsTavernOperational()&&!S->bSecondHeroHired)Button(TEXT("Hire"),TEXT("[H] Hire companion / 1200 gold"),45,282,465);
            else Text(S->bSecondHeroHired?TEXT("Tavern companion hired"):TEXT("Companion hiring locked"),45,290,Muted);
            Button(TEXT("Save"),TEXT("[F5] Save"),45,327,224);Button(TEXT("Load"),TEXT("[F9] Load"),282,327,228);
        }
        Button(TEXT("Return"),TEXT("[Esc] Return to campaign"),45,394,465);
        Panel(14,H-98,W-28,84);Wrapped(Visit->LastMessage,28,H-86,W-56,Ink,2);
        if(!S->LastPersistenceReport.IsEmpty())Wrapped(S->LastPersistenceReport,28,H-45,W-56,Muted,1);
        return;
    }
    Panel(14,12,W-28,60);
    Text(TEXT("S O U L"),28,24,Gold,1.35f);
    Text(FString::Printf(TEXT("DAY %d     GOLD %d     MOVEMENT %d / %d"),S->Economy.Day,S->Economy.Resources.FindRef(TEXT("gold")),S->Economy.ActionPoints,S->Economy.MaxActionPoints),150,30,Ink);
    bool HostileRemains=false;for(const auto& Region:S->World.Regions)HostileRemains|=S->IsHostile(Region.Key);
    Text(HostileRemains?TEXT("SECURE THE STRONGHOLDS"):TEXT("STRONGHOLDS SECURED"),150,52,Muted,.85f);
    int32 Army=0;for(const auto& P:S->PlayerArmy)Army+=P.Value;

    Button(TEXT("Company"),FString::Printf(TEXT("SELECT YOUR ARMY  %d  [Home]"),Army),W-557,26,266);
    Button(TEXT("EndDay"),S->Economy.ActionPoints>0?TEXT("Next day  [Space]"):TEXT("RESTORE MOVEMENT  [Space]"),W-279,26,253);
    // A separate, persistent company label is selectable even beside a city.
    for(TActorIterator<ASoulCampaignWorldActor> It(GetWorld());It;++It)
    {
        FVector2D At;
        if(GetOwningPlayerController()->ProjectWorldLocationToScreen(It->PresentedPartyLocation(),At,true))
        {
            At/=Scale; At+=FVector2D(-70,22);
            if(At.X>20 && At.X<W-440 && At.Y>90 && At.Y<H-130)
            {
                Panel(At.X,At.Y,154,29);
                Button(TEXT("CompanyWorld"),FString::Printf(TEXT("YOUR ARMY  %d"),Army),At.X+1,At.Y+1,152);
            }
        }
        break;
    }
    const float X=W-326;
    const FName Selected=C->GetSelectedRegion();
    const bool Explored=FSoulWorldRules::IsExplored(S->World,S->PlayerFaction,Selected),Visible=FSoulWorldRules::IsVisible(S->World,S->PlayerFaction,Selected);
    Panel(X,94,312,197);
    const FString SelectedTitle=C->IsCompanySelected()&&Selected==S->PlayerRegion
        ? FString::Printf(TEXT("YOUR ARMY / %s"),*C->DisplayName(Selected))
        : (Explored?C->DisplayName(Selected):TEXT("Uncharted territory"));
    UI.FitText(SelectedTitle,X+15,109,282,Gold,1.32f);
    if(Explored)
    {
        const auto* Region=S->World.Regions.Find(Selected);
        Text(C->IsCompanySelected()?FString::Printf(TEXT("ARMY SELECTED / %d MOVEMENT LEFT"),S->Economy.ActionPoints):(Visible?(Region&&Region->OwnerFactionId==S->PlayerFaction?TEXT("YOUR TERRITORY"):S->IsHostile(Selected)?TEXT("HOSTILE TERRITORY"):TEXT("OPEN COUNTRY")):TEXT("SURVEYED / BEYOND SIGHT")),X+15,135,C->IsCompanySelected()?Gold:Muted);
        Text(C->IsCompanySelected()?TEXT("Click a highlighted neighbouring place."):(Selected==S->PlayerRegion?TEXT("Your company is stationed here."):FSoulWorldRules::CanMove(S->World,S->PlayerRegion,Selected)?TEXT("Connected by a traversable road."):TEXT("Reach this place through its neighbours.")),X+15,162,Ink);
        if(C->IsCompanySelected())Text(TEXT("Each road move costs 1 movement."),X+15,185,Muted);
        else if(Visible&&S->HasHostileGarrison(Selected))Text(FString::Printf(TEXT("Defenders + reserves: %d"),S->EnemyArmies.FindRef(Selected)),X+15,185,Ink);
        else Text(Visible?TEXT("Click a nearby place to travel."):TEXT("Return within sight for current forces."),X+15,185,Muted);
        Text(FString::Printf(TEXT("Hero %d  |  XP %d  |  Mana %d/%d"),S->Hero.Level,S->Hero.Experience,S->Hero.Mana,S->Hero.MaxMana),X+15,209,Muted);
    }
    if(C->IsBattleAvailable())
    {
        if(S->PlayerArmy.FindRef(S->PlayerUnitId)<=0)Text(TEXT("Recruit Knights at the capital first."),X+15,249,Gold);
        else if(S->Economy.ActionPoints<=0)Button(TEXT("BattleRest"),TEXT("Next day restores actions  [Space]"),X+15,237,282);
        else Button(TEXT("Battle"),TEXT("Commit 1 action to battle  [B]"),X+15,237,282);
    }
    else if(C->IsCompanySelected())Button(TEXT("Focus"),TEXT("ARMY SELECTED / choose a highlighted road"),X+15,237,282);
    else if(S->PlayerRegion==TEXT("human_capital"))Button(TEXT("Town"),TEXT("Visit the capital  [T]"),X+15,237,282);
    else Button(TEXT("Focus"),TEXT("Select your army  [Home]"),X+15,237,282);
    Panel(14,H-85,W-28,42);
    Wrapped(C->LastMessage,28,H-74,W-56,Ink,2);
    Rect(14,H-36,W-28,28,Back);
    Text(TEXT("[HOME] select army  /  Click highlighted destination  /  [SPACE] next day  /  WASD pan  /  Wheel zoom  /  [T] town  /  [B] battle"),24,H-28,Muted);
    if(!S->LastPersistenceReport.IsEmpty()) { Rect(14,77,640,27,Back);Text(S->LastPersistenceReport.Replace(TEXT(" with RB Save"),TEXT("")),28,84,Muted); }
    if(S->PlayerArmy.FindRef(S->PlayerUnitId)==0&&!C->IsTownPanelOpen())
    {
        Panel(28,112,380,78);Text(TEXT("Your fighting company needs recruits"),43,125,Gold);
        Wrapped(TEXT("Return to the capital: [T] opens town, [1] recruits Knights. [Space] restores travel actions."),43,147,350,Ink,2);
    }
    if(!S->LastBattleResult.EncounterId.IsNone())
    {
        const auto& R=S->LastBattleResult;
        Panel(X,305,312,83);
        Text(R.bPlayerWon?TEXT("VICTORY"):TEXT("DEFEAT"),X+15,319,Gold);
        Text(C->DisplayName(R.TargetRegion),X+15,341,Ink);
        Text(FString::Printf(TEXT("Survivors: %d allied / %d hostile"),R.PlayerSurvivors,R.EnemySurvivors),X+15,363,Muted);
    }
    if(C->IsSkillChoiceOpen())
    {
        Panel(X,H-233,278,134);
        Text(TEXT("A skill point is available"),X+15,H-219,Gold);
        const TCHAR* Skills[]={TEXT("Command"),TEXT("Adventure"),TEXT("Magic")};
        for(int32 I=0;I<3;++I)Button(FName(*FString::Printf(TEXT("Recruit%d"),I+1)),FString::Printf(TEXT("[%d] %s     rank %d / 2"),I+1,Skills[I],S->Hero.Skills.FindRef(Skills[I])),X+15,H-195+I*30,248);
    }
    if(C->IsTownPanelOpen())
    {
        const auto& Roster=USoulFounderPlaytestStateSubsystem::HumanPlaytestRoster();
        const float TavernY=190.f+Roster.Num()*31.f;
        const bool DevelopmentEnabled=S->IsSettlementDevelopmentEnabled();
        Panel(28,112,454,TavernY-102.f+77.f+(DevelopmentEnabled?154.f:0.f));
        Text(TEXT("HUMAN CAPITAL"),45,129,Gold,1.3f);
        Text(TEXT("Recruit from the available weekly pools"),45,158,Muted);
        for(int32 I=0;I<Roster.Num();++I)
        {
            const auto* Pool=S->Economy.RecruitmentPools.Find(Roster[I]);
            FString Name=Roster[I].ToString().Replace(TEXT("human_"),TEXT("")).Replace(TEXT("_"),TEXT(" "));
            Button(FName(*FString::Printf(TEXT("Recruit%d"),I+1)),FString::Printf(TEXT("[%d] %s  %dg  pool %d  army %d"),I+1,*Name,Pool?Pool->CostPerUnit.FindRef(TEXT("gold")):0,Pool?Pool->Available:0,S->PlayerArmy.FindRef(Roster[I])),45,188+I*31,420);
        }
        if(DevelopmentEnabled&&!S->IsTavernOperational())Text(TEXT("Complete the tavern to unlock hiring"),45,TavernY+18,Muted);
        else Button(TEXT("Hire"),S->bSecondHeroHired?TEXT("Tavern companion hired"):TEXT("[H] Hire companion  /  1200 gold"),45,TavernY+10,420);
        if(DevelopmentEnabled)
        {
            Development(45,TavernY+51,420);
            if(S->IsSettlementDevelopmentReady())
            {
                Button(TEXT("BuildTavern"),TEXT("[U] Build tavern"),45,TavernY+125,201);
                Button(TEXT("VisitSettlement"),TEXT("[V] Visit settlement"),254,TavernY+125,211);
            }
            Text(TEXT("[F5] Save / [F9] Load / [T / Esc] Close town"),45,TavernY+167,Muted);
        }
        else Text(TEXT("[T / Esc] Return to the campaign"),45,TavernY+51.f,Muted);
    }
}
void ASoulFounderPlaytestHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if(auto* Visit=GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()){Visit->HandleAction(BoxName);return;}
    auto* C=FindCampaign(GetWorld());if(!C)return;
    if(BoxName==TEXT("EndDay")||BoxName==TEXT("BattleRest"))C->EndDay();
    else if(BoxName==TEXT("Town"))C->ToggleTownPanel();
    else if(BoxName==TEXT("Battle"))C->StartBattle();
    else if(BoxName==TEXT("Hire"))C->HireTavernHero();
    else if(BoxName==TEXT("BuildTavern"))C->BuildTavern();
    else if(BoxName==TEXT("VisitSettlement"))C->VisitSettlement();
    else if(BoxName.ToString().StartsWith(TEXT("Recruit")))C->HandleNumberKey(FCString::Atoi(*BoxName.ToString().Mid(7)));
    else if(BoxName==TEXT("Focus")||BoxName==TEXT("Company")||BoxName==TEXT("CompanyWorld"))if(auto* PC=GetOwningPlayerController())PC->ConsoleCommand(TEXT("SoulFocusCompany"));
}
