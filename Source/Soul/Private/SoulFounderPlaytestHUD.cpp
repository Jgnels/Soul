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
    if(S->bPersistenceBusy||S->IsAlphaTurnActive())UI.CursorState=TEXT("busy");
    else if(C&&!C->HoveredRegion.IsNone())
    {
        const FName R=C->HoveredRegion;
        UI.CursorState=R==S->PlayerRegion?TEXT("interact"):
            !FSoulWorldRules::CanMove(S->World,S->PlayerRegion,R)||S->Economy.ActionPoints<=0||!S->DiplomacyAllowsHostility(S->PlayerFaction,S->World.Regions.FindChecked(R).OwnerFactionId)?TEXT("disabled"):
            S->IsHostile(R)?TEXT("attack"):TEXT("move");
    }
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
        const FName TavernId=S->GetTavernBuildingId();
        const auto* Definition=Scenario->FindDevelopmentDefinition(TavernId);
        const auto* Authority=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();
        const auto* Town=Authority?Authority->FindSettlement(Scenario->SettlementId):nullptr;
        const auto* Building=Town?Town->Buildings.Find(TavernId):nullptr;
        const FString Condition=Authority?Authority->GetBuildingConditionName(Scenario->SettlementId,TavernId).ToString():TEXT("Unavailable");
        Text(FString::Printf(TEXT("%s: %s  /  level %d"),*S->GetDevelopmentBuildingName(),*Condition,Building?Building->Level:0),AtX,AtY,Gold);
        TArray<FName> Resources;Definition->BuildCost.GetKeys(Resources);Resources.Sort(FNameLexicalLess());
        FString Cost;
        for(FName Resource:Resources)
        {if(!Cost.IsEmpty())Cost+=TEXT(", ");Cost+=FString::Printf(TEXT("%d %s"),Definition->BuildCost[Resource],*Resource.ToString());}
        UI.FitText(FString::Printf(TEXT("Build: %s  /  %d days"),Cost.IsEmpty()?TEXT("no resource cost"):*Cost,Definition->BuildDays),AtX,AtY+23,Width,Ink);
        const FString Progress=Building&&Building->Condition==ESoulBuildingCondition::Building
            ?FString::Printf(TEXT("Construction: %d days remaining"),Building->ConstructionDaysRemaining)
            :S->IsTavernOperational()?TEXT("Companion hiring is available."):TEXT("Complete construction to unlock companion hiring.");
        UI.FitText(Progress,AtX,AtY+46,Width,Muted);
    };
    auto HeartlandDevelopment=[&]()
    {
        if(!S->IsHeartlandEnabled()||!S->IsSettlementDevelopmentReady())return;
        const auto* Scenario=S->GetSettlementScenario();const auto* Authority=GetGameInstance()->GetSubsystem<USoulSettlementStateSubsystem>();const auto* Town=Authority->FindSettlement(Scenario->SettlementId);
        const float Left=W-475;Panel(Left,108,447,415);Text(TEXT("HEARTLAND DEVELOPMENT"),Left+15,121,Gold);
        int32 Row=0;for(const auto& D:Scenario->DevelopmentDefinitions)
        {
            const auto* B=Town?Town->Buildings.Find(D.BuildingId):nullptr;
            const FString Label=FString::Printf(TEXT("%s | %dg / %dd | %s"),*D.DisplayName.ToString(),D.BuildCost.FindRef(TEXT("gold")),D.BuildDays,B&&B->Condition==ESoulBuildingCondition::Building?*FString::Printf(TEXT("%dd left"),B->ConstructionDaysRemaining):B&&B->Level>=D.MaxLevel?TEXT("complete"):TEXT("build"));
            Button(FName(*(TEXT("Develop:")+D.BuildingId.ToString())),Label,Left+15,154+Row*43,417);
            const TCHAR* Effect=D.BuildingId==TEXT("human.arcane_hall")?TEXT("Learns Frost Blizzard"):
                D.BuildingId==TEXT("human.mage_academy")?TEXT("Learns Water Ward / requires Arcane Hall"):
                D.BuildingId==TEXT("human.high_conclave")?TEXT("Learns Air Tailwind / requires Mage Academy"):
                D.BuildingId==TEXT("human.market")?TEXT("+100 gold each day"):
                D.BuildingId==TEXT("human.barracks")?TEXT("Unlocks Veteran Guard; +2 weekly stock"):
                TEXT("Unlocks paid companion hiring");
            UI.FitText(Effect,Left+20,183+Row*43,407,Muted,.75f);++Row;
        }
        Wrapped(TEXT("Aurora: Frost primary; Water / Air secondary. Fire / Lightning forbidden. Guild, veteran barracks and expanded market change the city."),Left+15,425,417,Muted,4);
    };
    if(C&&C->bDiplomacyPanel)
    {
        UI.Panel(28,24,W-56,H-48,true);Panels.Add(FBox2D(FVector2D(28,24)*Scale,FVector2D(W-28,H-24)*Scale));Text(TEXT("DIPLOMACY / HUMAN HEARTLAND"),48,42,Gold,1.25f);
        Button(TEXT("DiplomacyClose"),TEXT("Close [Esc]"),W-230,40,170);
        int32 I=0;for(FName Id:{FName(TEXT("dwarves")),FName(TEXT("orcs")),FName(TEXT("vikings"))})
            Button(FName(*(TEXT("DiplomacyFaction:")+Id.ToString())),Id.ToString().ToUpper(),48+I++*220,90,205);
        const auto Relation=S->DiplomaticRelation(C->DiplomaticFaction);const auto Stance=FSoulDiplomacyRules::EffectiveStance(Relation,S->Economy.Day);
        Text(FString::Printf(TEXT("%s | %s | relations %+d | gold %d | movement %d"),*C->DiplomaticFaction.ToString().ToUpper(),*FSoulDiplomacyRules::StanceName(Stance),Relation.RelationPermille,S->Economy.Resources.FindRef(TEXT("gold")),S->Economy.ActionPoints),48,137,Gold);
        Text(Stance==ESoulDiplomaticStance::NonAggression?FString::Printf(TEXT("Pact through day %d inclusive; then peace. No military access."),Relation.PactUntilDay):TEXT("Peace blocks attacks and occupation in both directions; it grants no military access."),48,161,Muted);
        const TCHAR* Names[]={TEXT("Declare war"),TEXT("Offer peace"),TEXT("3-day non-aggression pact"),TEXT("Gift 250 gold / +100 relation"),TEXT("Break pact / declare war")};
        for(int32 A=0;A<5;++A)
        {
            const auto D=S->PreviewDiplomacy(C->DiplomaticFaction,static_cast<ESoulDiplomaticAction>(A));
            const float Y=202+A*88;Button(FName(*FString::Printf(TEXT("DiplomacyAction:%d"),A)),Names[A],48,Y,325);
            Wrapped(FString::Join(D.Reasons,TEXT(" ")),395,Y+3,W-450,D.bAccepted?Ink:Muted,4);
        }
        Wrapped(C->LastMessage,48,H-112,W-96,Gold,3);
        return;
    }
    if(Visit)
    {
        Panel(14,12,W-28,60);Text(TEXT("SETTLEMENT VISIT"),28,24,Gold,1.3f);
        Text(FString::Printf(TEXT("DAY %d   GOLD %d"),S->Economy.Day,S->Economy.Resources.FindRef(TEXT("gold"))),300,30,Ink);
        if(Visit->bManagePanel||!Visit->IsWalking())
        {
        Panel(28,108,500,S->IsHeartlandEnabled()?430:380);Development(45,126,465);HeartlandDevelopment();
        if(Visit->IsVisitReady())
        {
            Button(TEXT("BuildTavern"),TEXT("[U] Build ")+S->GetDevelopmentBuildingName(),45,212,465);
            Button(TEXT("EndDay"),TEXT("[Space] Advance day"),45,247,465);
            if(S->IsTavernOperational()&&!S->bSecondHeroHired)Button(TEXT("Hire"),TEXT("[H] Hire companion / 1200 gold"),45,282,465);
            else Text(S->bSecondHeroHired?TEXT("Tavern companion hired"):TEXT("Companion hiring locked"),45,290,Muted);
            Button(TEXT("Save"),TEXT("[F5] Save"),45,327,224);Button(TEXT("Load"),TEXT("[F9] Load"),282,327,228);
        }
        if(S->IsHeartlandEnabled())
        {int32 I=0;for(FName Id:S->AvailableHumanRoster()){const auto* P=S->Economy.RecruitmentPools.Find(Id);Button(FName(*(TEXT("Company:")+Id.ToString())),FString::Printf(TEXT("%s | %dg | stock %d | army %d"),Id==TEXT("human_archer")?TEXT("Archers"):Id==TEXT("human_guard")?TEXT("Veteran Guard"):TEXT("Infantry"),P?P->CostPerUnit.FindRef(TEXT("gold")):0,P?P->Available:0,S->PlayerArmy.FindRef(Id)),45,363+I++*31,465);}}
        else Button(TEXT("Recruit"),TEXT("Recruit Eastern Knight / paid weekly pool"),45,364,465);
        if(S->IsHeartlandEnabled()&&S->bSecondHeroHired)Button(TEXT("CompanionAssign"),S->CompanionStatus(),45,459,465);
        Button(TEXT("Return"),TEXT("[Esc] Return to campaign"),45,S->IsHeartlandEnabled()?494:425,465);
        }
        else {Button(TEXT("Manage"),TEXT("[Tab] Manage settlement"),28,84,270);}
        Panel(14,H-98,W-28,84);Wrapped(Visit->LastMessage,28,H-86,W-56,Ink,2);
        if(!S->LastPersistenceReport.IsEmpty())Wrapped(S->LastPersistenceReport,28,H-45,W-56,Muted,1);
        return;
    }
    Panel(14,12,W-28,60);
    Text(TEXT("S O U L"),28,24,Gold,1.35f);
    Text(FString::Printf(TEXT("DAY %d     GOLD %d     MOVEMENT %d / %d"),S->Economy.Day,S->Economy.Resources.FindRef(TEXT("gold")),S->Economy.ActionPoints,S->Economy.MaxActionPoints),150,30,Ink);
    bool HostileRemains=false;for(const auto& Region:S->World.Regions)HostileRemains|=S->IsHostile(Region.Key);
    Text(S->IsFourFactionAlpha()?(S->IsAlphaTurnActive()?TEXT("FOUR-FACTION ALPHA | AI TURN IN PROGRESS"):TEXT("FOUR-FACTION ALPHA | YOUR TURN")):S->IsSixFactionProfile()?TEXT("SIX-FACTION SANDBOX | AI OFF | [I] INSPECT ARMIES"):(HostileRemains?TEXT("SECURE THE STRONGHOLDS"):TEXT("STRONGHOLDS SECURED")),150,52,Muted,.85f);
    int32 Army=0;for(const auto& P:S->PlayerArmy)Army+=P.Value;

    Button(TEXT("Company"),FString::Printf(TEXT("SELECT YOUR ARMY  %d  [Home]"),Army),W-557,26,266);
    Button(TEXT("EndDay"),S->Economy.ActionPoints>0?TEXT("Next day  [Space]"):TEXT("RESTORE MOVEMENT  [Space]"),W-279,26,253);
    if(S->IsHeartlandEnabled()){Panel(W-557,76,174,31);Button(TEXT("Diplomacy"),TEXT("Diplomacy [L]"),W-556,77,172);}
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
    const bool Explored=FSoulWorldRules::IsExplored(S->World,C->ViewFaction(),Selected),Visible=FSoulWorldRules::IsVisible(S->World,C->ViewFaction(),Selected);
    Panel(X,94,312,197);
    const FString SelectedTitle=C->IsCompanySelected()&&Selected==S->PlayerRegion
        ? FString::Printf(TEXT("YOUR ARMY / %s"),*C->DisplayName(Selected))
        : (Explored?C->DisplayName(Selected):TEXT("Uncharted territory"));
    UI.FitText(SelectedTitle,X+15,109,282,Gold,1.32f);
    if(C->IsFactionInspection())
    {
        Text(FString::Printf(TEXT("%s / READ-ONLY ARMY INSPECTION"),*C->ViewFaction().ToString()),X+15,135,Gold);
        UI.FitText(S->ArmyInspectionAtRegion(Selected),X+15,168,282,Ink,.8f);
        Text(TEXT("[I] next faction  /  [HOME] your army"),X+15,207,Muted);
    }
    else if(Explored)
    {
        const auto* Region=S->World.Regions.Find(Selected);
        Text(C->IsCompanySelected()?FString::Printf(TEXT("ARMY SELECTED / %d MOVEMENT LEFT"),S->Economy.ActionPoints):(Visible?(Region&&Region->OwnerFactionId==S->PlayerFaction?TEXT("YOUR TERRITORY"):S->IsHostile(Selected)?TEXT("HOSTILE TERRITORY"):Region&&!Region->OwnerFactionId.IsNone()?TEXT("AT PEACE / NO MILITARY ACCESS"):TEXT("OPEN COUNTRY")):TEXT("SURVEYED / BEYOND SIGHT")),X+15,135,C->IsCompanySelected()?Gold:Muted);
        Text(C->IsCompanySelected()?TEXT("Click a highlighted neighbouring place."):(Selected==S->PlayerRegion?TEXT("Your company is stationed here."):FSoulWorldRules::CanMove(S->World,S->PlayerRegion,Selected)?TEXT("Connected by a traversable road."):TEXT("Reach this place through its neighbours.")),X+15,162,Ink);
        if(C->IsCompanySelected())Text(TEXT("Each road move costs 1 movement."),X+15,185,Muted);
        else if(Visible&&S->IsSixFactionProfile())UI.FitText(S->ArmyInspectionAtRegion(Selected),X+15,185,282,Ink,.8f);
        else if(Visible&&S->HasHostileGarrison(Selected))Text(FString::Printf(TEXT("Defenders + reserves: %d"),S->ArmyCountAtRegion(Selected)),X+15,185,Ink);
        else Text(Visible?TEXT("Click a nearby place to travel."):TEXT("Return within sight for current forces."),X+15,185,Muted);
        Text(FString::Printf(TEXT("Hero %d  |  XP %d  |  Mana %d/%d"),S->Hero.Level,S->Hero.Experience,S->Hero.Mana,S->Hero.MaxMana),X+15,209,Muted);
    }
    if(C->IsFactionInspection()){}
    else if(C->IsBattleAvailable())
    {
        if(S->GetPlayerTroopCount()<=0)Text(S->IsFourFactionAlpha()?TEXT("Routed: see recovery guidance."):TEXT("Recruit Knights at the capital first."),X+15,249,Gold);
        else if(S->Economy.ActionPoints<=0)Button(TEXT("BattleRest"),TEXT("Next day restores actions  [Space]"),X+15,237,282);
        else Button(TEXT("Battle"),TEXT("Commit 1 action to battle  [B]"),X+15,237,282);
    }
    else if(C->IsCompanySelected())Button(TEXT("Focus"),TEXT("ARMY SELECTED / choose a highlighted road"),X+15,237,282);
    else if(S->PlayerRegion==S->GetDevelopmentRegion())
    {
        FString Reason;
        if(S->IsFourFactionAlpha() && !S->CanOpenHumanSettlementServices(Reason))UI.FitText(Reason,X+15,249,282,Gold,.8f);
        else Button(TEXT("Town"),TEXT("Settlement services  [T]"),X+15,237,282);
    }
    else Button(TEXT("Focus"),TEXT("Select your army  [Home]"),X+15,237,282);
    if(!C->IsTownPanelOpen()&&!C->IsFactionInspection()&&S->PlayerRegion==S->GetDevelopmentRegion())
    {
        FString Why;if(ASoulSettlementVisitGameMode::CanVisit(S,Why))
        {Panel(X,300,312,85);Button(TEXT("VisitSettlement"),TEXT("[V] Enter and walk around city"),X+15,310,282);Button(TEXT("TownDirect"),TEXT("[T] Manage / recruit"),X+15,348,282);}
    }
    if(S->IsHeartlandEnabled()&&!C->IsTownPanelOpen())
        if(const auto* Site=S->HeartlandContent.Sites.FindByPredicate([&](const auto& A){return A.Region==S->PlayerRegion;}))
        {
            Panel(X,405,312,80);UI.FitText(Site->Name,X+15,417,282,Gold);
            if(S->HeartlandSiteDays.FindRef(Site->Id)==S->Economy.Day)
                UI.FitText(TEXT("Collected today; available tomorrow"),X+15,447,282,Muted);
            else Button(TEXT("WorkSite"),FString::Printf(TEXT("Collect +%d %s / %d move"),Site->Amount,*Site->Effect.ToString(),Site->ActionCost),X+15,445,282);
        }
    if(S->IsHeartlandEnabled()&&S->Hero.Condition!=ESoulHeroCondition::Healthy)
    {Panel(28,510,600,65);Wrapped(S->Hero.Condition==ESoulHeroCondition::Captured
        ?FString::Printf(TEXT("Aurora CAPTURED by %s at %s. Unavailable; rescue/ransom not implemented."),*S->Hero.CaptorFaction.ToString(),*C->DisplayName(S->Hero.CaptureRegion))
        :FString::Printf(TEXT("Aurora WOUNDED: %d days recovery. No hero bonuses, magic, siege or diplomacy."),S->Hero.RecoveryDays),43,521,565,Gold,3);}
    Panel(14,H-85,W-28,42);
    Wrapped(C->LastMessage,28,H-74,W-56,Ink,2);
    Rect(14,H-36,W-28,28,Back);
    Text(TEXT("Home army / F selected focus / Q E orbit / PgUp Dn pitch / Wheel zoom / T manage / V visit / Space next day"),24,H-28,Muted);
    if(!S->LastPersistenceReport.IsEmpty()) { Rect(14,77,640,27,Back);Text(S->LastPersistenceReport.Replace(TEXT(" with RB Save"),TEXT("")),28,84,Muted); }
    if(S->GetPlayerTroopCount()==0&&!C->IsTownPanelOpen())
    {
        Panel(28,112,490,112);Text(TEXT("Routed company / recovery"),43,125,Gold);
        Wrapped(S->IsFourFactionAlpha()?S->HumanRecoveryGuidance():TEXT("Return to the capital: [T] opens town, [1] recruits Knights. [Space] restores travel actions."),43,150,460,Ink,4);
    }
    if(!S->LastBattleResult.EncounterId.IsNone())
    {
        const auto& R=S->LastBattleResult;
        const float ResultY=S->IsHeartlandEnabled()&&S->PlayerRegion==S->GetDevelopmentRegion()?398.f:305.f;
        Panel(X,ResultY,312,83);
        Text(S->IsSixFactionProfile()?(R.bPlayerWon?TEXT("ATTACKERS WON"):TEXT("DEFENDERS WON")):(R.bPlayerWon?TEXT("VICTORY"):TEXT("DEFEAT")),X+15,ResultY+14,Gold);
        Text(C->DisplayName(R.TargetRegion),X+15,ResultY+36,Ink);
        Text(FString::Printf(TEXT("Survivors: %d %s / %d %s"),R.PlayerSurvivors,S->IsSixFactionProfile()?TEXT("attackers"):TEXT("allied"),R.EnemySurvivors,S->IsSixFactionProfile()?TEXT("defenders"):TEXT("hostile")),X+15,ResultY+58,Muted);
    }
    if(C->IsSkillChoiceOpen())
    {
        Panel(X,H-233,278,134);
        Text(TEXT("A skill point is available"),X+15,H-219,Gold);
        const TCHAR* Skills[]={TEXT("Command"),TEXT("Adventure"),TEXT("Magic")};
        for(int32 I=0;I<3;++I)Button(FName(*FString::Printf(TEXT("Recruit%d"),I+1)),FString::Printf(TEXT("[%d] %s     rank %d / 2"),I+1,Skills[I],S->Hero.Skills.FindRef(Skills[I])),X+15,H-195+I*30,248);
    }
    if(S->IsFourFactionAlpha() && !C->IsTownPanelOpen())
    {
        TArray<FString> Recap;S->LastAIReport.ParseIntoArrayLines(Recap);const int32 Start=FMath::Max(0,Recap.Num()-7);
        const float RecapHeight=20+23*FMath::Max(1,Recap.Num()-Start),RecapTop=H-115-RecapHeight;
        Panel(28,RecapTop,720,RecapHeight);
        for(int32 I=Start;I<Recap.Num();++I)UI.FitText(Recap[I],43,RecapTop+11+(I-Start)*23,690,Ink,.85f);
    }
    if(C->IsTownPanelOpen())
    {
        HeartlandDevelopment();
        const auto& Roster=S->AvailableHumanRoster();
        const int32 RecruitCount=S->PlayerRegion==TEXT("human_capital")?Roster.Num():0;
        const float TavernY=190.f+RecruitCount*31.f;
        const float CompanionSpace=S->IsHeartlandEnabled()&&S->bSecondHeroHired?36.f:0.f;
        const bool DevelopmentEnabled=S->IsSettlementDevelopmentEnabled();
        Panel(28,112,454,TavernY-102.f+77.f+CompanionSpace+(DevelopmentEnabled?154.f:0.f));
        Text(C->DisplayName(S->PlayerRegion).ToUpper(),45,129,Gold,1.3f);
        Text(RecruitCount?TEXT("Recruit from the available weekly pools"):TEXT("Construction and companion services"),45,158,Muted);
        for(int32 I=0;I<RecruitCount;++I)
        {
            const auto* Pool=S->Economy.RecruitmentPools.Find(Roster[I]);
            FString Name=Roster[I]==TEXT("human_archer")?TEXT("Archers"):Roster[I]==TEXT("human_guard")?TEXT("Veteran Guard"):Roster[I]==TEXT("human_knight")?TEXT("Infantry"):Roster[I].ToString();
            Button(FName(*FString::Printf(TEXT("Recruit%d"),I+1)),FString::Printf(TEXT("[%d] %s  %dg  pool %d  army %d"),I+1,*Name,Pool?Pool->CostPerUnit.FindRef(TEXT("gold")):0,Pool?Pool->Available:0,S->PlayerArmy.FindRef(Roster[I])),45,188+I*31,420);
        }
        if(DevelopmentEnabled&&!S->IsTavernOperational())Text(TEXT("Complete construction to unlock hiring"),45,TavernY+18,Muted);
        else Button(TEXT("Hire"),S->bSecondHeroHired?TEXT("Tavern companion hired"):TEXT("[H] Hire companion  /  1200 gold"),45,TavernY+10,420);
        if(S->IsHeartlandEnabled()&&S->bSecondHeroHired)Button(TEXT("CompanionAssign"),S->CompanionStatus(),45,TavernY+40,420);
        if(DevelopmentEnabled)
        {
            Development(45,TavernY+51+CompanionSpace,420);
            if(S->IsSettlementDevelopmentReady())
            {
                Button(TEXT("BuildTavern"),TEXT("[U] Construct"),45,TavernY+125+CompanionSpace,201);
                Button(TEXT("VisitSettlement"),TEXT("[V] Visit settlement"),254,TavernY+125+CompanionSpace,211);
            }
            Text(TEXT("[F5] Save / [F9] Load / [T / Esc] Close town"),45,TavernY+167+CompanionSpace,Muted);
        }
        else Text(TEXT("[T / Esc] Return to the campaign"),45,TavernY+51.f,Muted);
    }
}
void ASoulFounderPlaytestHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if(auto* Visit=GetWorld()->GetAuthGameMode<ASoulSettlementVisitGameMode>()){Visit->HandleAction(BoxName);return;}
    auto* C=FindCampaign(GetWorld());if(!C)return;
    if(BoxName==TEXT("Diplomacy")){C->ToggleDiplomacy();return;}
    if(BoxName==TEXT("DiplomacyClose")){C->CancelPanel();return;}
    if(C->bDiplomacyPanel)
    {
        const FString Name=BoxName.ToString();
        if(Name.StartsWith(TEXT("DiplomacyFaction:"))){const FName F(*Name.Mid(FString(TEXT("DiplomacyFaction:")).Len()));if(C->GetState()->IsDiplomacyTarget(F))C->DiplomaticFaction=F;return;}
        if(Name.StartsWith(TEXT("DiplomacyAction:"))){const int32 A=FCString::Atoi(*Name.Mid(FString(TEXT("DiplomacyAction:")).Len()));if(A>=0&&A<=4)C->GetState()->ExecuteDiplomacy(C->DiplomaticFaction,static_cast<ESoulDiplomaticAction>(A),C->LastMessage);return;}
        return;
    }
    if(BoxName==TEXT("CompanionAssign")){C->GetState()->AssignHeartlandCompanion(!C->GetState()->bCompanionAssigned);return;}
    if(BoxName==TEXT("WorkSite")){C->GetState()->InteractHeartlandSite(C->LastMessage);return;}
    if(BoxName.ToString().StartsWith(TEXT("Develop:"))){C->GetState()->BeginSettlementConstruction(FName(*BoxName.ToString().Mid(8)),C->LastMessage);return;}
    if(BoxName==TEXT("EndDay")||BoxName==TEXT("BattleRest"))C->EndDay();
    else if(BoxName==TEXT("Town")||BoxName==TEXT("TownDirect"))C->ToggleTownPanel();
    else if(BoxName==TEXT("Battle"))C->StartBattle();
    else if(BoxName==TEXT("Hire"))C->HireTavernHero();
    else if(BoxName==TEXT("BuildTavern"))C->BuildTavern();
    else if(BoxName==TEXT("VisitSettlement"))C->VisitSettlement();
    else if(BoxName.ToString().StartsWith(TEXT("Recruit")))C->HandleNumberKey(FCString::Atoi(*BoxName.ToString().Mid(7)));
    else if(BoxName==TEXT("Focus")||BoxName==TEXT("Company")||BoxName==TEXT("CompanyWorld"))if(auto* PC=GetOwningPlayerController())PC->ConsoleCommand(TEXT("SoulFocusCompany"));
}
