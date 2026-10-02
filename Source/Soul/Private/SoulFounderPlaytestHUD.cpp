#include "SoulFounderPlaytestHUD.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "SoulFounderPlaytestStateSubsystem.h"
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
    auto* C=FindCampaign(GetWorld());auto* S=C?C->GetState():nullptr;if(!S)return;
    const float Scale=FMath::Max(1.f,Canvas->ClipY/900.f),W=Canvas->ClipX/Scale,H=Canvas->ClipY/Scale;
    const FLinearColor Ink(.90f,.88f,.80f),Muted(.55f,.61f,.64f),Gold(.85f,.70f,.39f),Back(.023f,.036f,.046f,.94f);
    auto Text=[&](const FString& T,float X,float Y,FLinearColor Color,float Size=1.f){DrawText(T,Color,X*Scale,Y*Scale,GEngine->GetSmallFont(),Scale*Size,false);};
    auto Rect=[&](float X,float Y,float Width,float Height,FLinearColor Color){DrawRect(Color,X*Scale,Y*Scale,Width*Scale,Height*Scale);};
    auto Panel=[&](float X,float Y,float Width,float Height){Rect(X,Y,Width,Height,Back);Rect(X,Y,Width,1,Gold*.6f);Panels.Add(FBox2D(FVector2D(X,Y)*Scale,FVector2D(X+Width,Y+Height)*Scale));};
    auto Button=[&](FName Id,const FString& T,float X,float Y,float Width){Rect(X,Y,Width,27,FLinearColor(.10f,.15f,.18f,.96f));Text(T,X+9,Y+7,Gold);AddHitBox(FVector2D(X,Y)*Scale,FVector2D(Width,27)*Scale,Id,true,1);};
    auto Wrapped=[&](const FString& T,float X,float Y,float Width,FLinearColor Color,int32 MaxLines=3)
    {
        TArray<FString> Words;T.ParseIntoArrayWS(Words);FString Line;int32 Count=0;
        for(const FString& Word:Words)
        {
            FString Trial=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float TW,TH;Canvas->StrLen(GEngine->GetSmallFont(),Trial,TW,TH);
            if(TW>Width&&!Line.IsEmpty()){Text(Line,X,Y+Count*17,Color);Line=Word;if(++Count>=MaxLines)return;}else Line=Trial;
        }
        if(!Line.IsEmpty())Text(Line,X,Y+Count*17,Color);
    };
    Panel(14,12,W-28,44);
    Text(TEXT("S O U L"),28,24,Gold,1.35f);
    Text(FString::Printf(TEXT("DAY %d     GOLD %d     MOVEMENT %d / %d"),S->Economy.Day,S->Economy.Resources.FindRef(TEXT("gold")),S->Economy.ActionPoints,S->Economy.MaxActionPoints),150,28,Ink);
    bool HostileRemains=false;for(const auto& Region:S->World.Regions)HostileRemains|=S->IsHostile(Region.Key);
    Text(HostileRemains?TEXT("SECURE THE STRONGHOLDS"):TEXT("STRONGHOLDS SECURED"),500,28,Muted);
    int32 Army=0;for(const auto& P:S->PlayerArmy)Army+=P.Value;

    Button(TEXT("Company"),FString::Printf(TEXT("SELECT YOUR ARMY  %d  [Home]"),Army),W-570,21,270);
    Button(TEXT("EndDay"),S->Economy.ActionPoints>0?TEXT("Next day  [Space]"):TEXT("RESTORE MOVEMENT  [Space]"),W-280,21,253);
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
    const float X=W-292;
    const FName Selected=C->GetSelectedRegion();
    const bool Explored=FSoulWorldRules::IsExplored(S->World,S->PlayerFaction,Selected),Visible=FSoulWorldRules::IsVisible(S->World,S->PlayerFaction,Selected);
    Panel(X,75,278,197);
    const FString SelectedTitle=C->IsCompanySelected()&&Selected==S->PlayerRegion
        ? FString::Printf(TEXT("YOUR ARMY — %s"),*C->DisplayName(Selected))
        : (Explored?C->DisplayName(Selected):TEXT("Uncharted territory"));
    Text(SelectedTitle,X+15,90,Gold,1.15f);
    if(Explored)
    {
        const auto* Region=S->World.Regions.Find(Selected);
        Text(C->IsCompanySelected()?FString::Printf(TEXT("ARMY SELECTED — %d MOVEMENT LEFT"),S->Economy.ActionPoints):(Visible?(Region&&Region->OwnerFactionId==S->PlayerFaction?TEXT("YOUR TERRITORY"):S->IsHostile(Selected)?TEXT("HOSTILE TERRITORY"):TEXT("OPEN COUNTRY")):TEXT("SURVEYED / BEYOND SIGHT")),X+15,116,C->IsCompanySelected()?Gold:Muted);
        Text(C->IsCompanySelected()?TEXT("Click a highlighted neighbouring place."):(Selected==S->PlayerRegion?TEXT("Your company is stationed here."):FSoulWorldRules::CanMove(S->World,S->PlayerRegion,Selected)?TEXT("Connected by a traversable road."):TEXT("Reach this place through its neighbours.")),X+15,143,Ink);
        if(C->IsCompanySelected())Text(TEXT("Each road move costs 1 movement."),X+15,166,Muted);
        else if(Visible&&S->HasHostileGarrison(Selected))Text(FString::Printf(TEXT("Defenders + reserves: %d"),S->EnemyArmies.FindRef(Selected)),X+15,166,Ink);
        else Text(Visible?TEXT("Click a nearby place to travel."):TEXT("Return within sight for current forces."),X+15,166,Muted);
        Text(FString::Printf(TEXT("Hero %d  |  XP %d  |  Mana %d/%d"),S->Hero.Level,S->Hero.Experience,S->Hero.Mana,S->Hero.MaxMana),X+15,190,Muted);
    }
    if(C->IsBattleAvailable())
    {
        if(S->PlayerArmy.FindRef(S->PlayerUnitId)<=0)Text(TEXT("Recruit Knights at the capital first."),X+15,230,Gold);
        else if(S->Economy.ActionPoints<=0)Button(TEXT("BattleRest"),TEXT("Next day restores actions  [Space]"),X+15,218,248);
        else Button(TEXT("Battle"),TEXT("Commit 1 action to battle  [B]"),X+15,218,248);
    }
    else if(C->IsCompanySelected())Button(TEXT("Focus"),TEXT("ARMY SELECTED — choose a highlighted road"),X+15,218,248);
    else if(S->PlayerRegion==TEXT("human_capital"))Button(TEXT("Town"),TEXT("Visit the capital  [T]"),X+15,218,248);
    else Button(TEXT("Focus"),TEXT("Select your army  [Home]"),X+15,218,248);
    Panel(14,H-85,W-28,42);
    Wrapped(C->LastMessage,28,H-74,W-56,Ink,2);
    Rect(14,H-36,W-28,28,Back);
    Text(TEXT("[HOME] select army  /  Click highlighted destination  /  [SPACE] next day  /  WASD pan  /  Wheel zoom  /  [T] town  /  [B] battle"),24,H-28,Muted);
    if(!S->LastPersistenceReport.IsEmpty()) { Rect(14,60,640,27,Back);Text(S->LastPersistenceReport.Replace(TEXT(" with RB Save"),TEXT("")),28,67,Muted); }
    if(S->PlayerArmy.FindRef(S->PlayerUnitId)==0&&!C->IsTownPanelOpen())
    {
        Panel(28,102,350,74);Text(TEXT("Your fighting company needs recruits"),43,115,Gold);
        Wrapped(TEXT("Return to the capital: [T] opens town, [1] recruits Knights. [Space] restores travel actions."),43,137,320,Ink,2);
    }
    if(!S->LastBattleResult.EncounterId.IsNone())
    {
        const auto& R=S->LastBattleResult;
        Panel(X,286,278,79);
        Text(R.bPlayerWon?TEXT("VICTORY"):TEXT("DEFEAT"),X+15,300,Gold);
        Text(C->DisplayName(R.TargetRegion),X+15,320,Ink);
        Text(FString::Printf(TEXT("Survivors: %d allied / %d hostile"),R.PlayerSurvivors,R.EnemySurvivors),X+15,340,Muted);
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
        Panel(28,102,390,TavernY-102.f+77.f);
        Text(TEXT("HUMAN CAPITAL"),45,119,Gold,1.3f);
        Text(TEXT("Recruit from the available weekly pools"),45,148,Muted);
        for(int32 I=0;I<Roster.Num();++I)
        {
            const auto* Pool=S->Economy.RecruitmentPools.Find(Roster[I]);
            FString Name=Roster[I].ToString().Replace(TEXT("human_"),TEXT("")).Replace(TEXT("_"),TEXT(" "));
            Button(FName(*FString::Printf(TEXT("Recruit%d"),I+1)),FString::Printf(TEXT("[%d] %s  %dg  pool %d  army %d"),I+1,*Name,Pool?Pool->CostPerUnit.FindRef(TEXT("gold")):0,Pool?Pool->Available:0,S->PlayerArmy.FindRef(Roster[I])),45,178+I*31,354);
        }
        Button(TEXT("Hire"),S->bSecondHeroHired?TEXT("Tavern companion hired"):TEXT("[H] Hire companion  /  1200 gold"),45,TavernY,354);
        Text(TEXT("[T / Esc] Return to the campaign"),45,TavernY+41.f,Muted);
    }
}
void ASoulFounderPlaytestHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);auto* C=FindCampaign(GetWorld());if(!C)return;
    if(BoxName==TEXT("EndDay")||BoxName==TEXT("BattleRest"))C->EndDay();
    else if(BoxName==TEXT("Town"))C->ToggleTownPanel();
    else if(BoxName==TEXT("Battle"))C->StartBattle();
    else if(BoxName==TEXT("Hire"))C->HireTavernHero();
    else if(BoxName.ToString().StartsWith(TEXT("Recruit")))C->HandleNumberKey(FCString::Atoi(*BoxName.ToString().Mid(7)));
    else if(BoxName==TEXT("Focus")||BoxName==TEXT("Company")||BoxName==TEXT("CompanyWorld"))if(auto* PC=GetOwningPlayerController())PC->ConsoleCommand(TEXT("SoulFocusCompany"));
}
