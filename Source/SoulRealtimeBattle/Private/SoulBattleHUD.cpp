#include "SoulRealtimeBattleArena.h"
#include "Engine/Canvas.h"
#include "SoulHUDTheme.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

void ASoulRealtimeArenaHUD::NotifyHitBoxClick(FName Name)
{
    Super::NotifyHitBoxClick(Name);
    if (auto* Host = GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>())
        Host->HandleBattleAction(Name);
}

void ASoulRealtimeArenaHUD::DrawHUD()
{
    Super::DrawHUD();
    const auto* Host = GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>();
    if (!Host || !Canvas) return;
    FSoulHUDTheme UI(*this,*Canvas);
    const float W=UI.W,H=UI.H;
    const FLinearColor Ally(.2f,.78f,.86f), Enemy(.88f,.30f,.18f);
    FString Tooltip;
    // Subordinate, shape-distinct allegiance markers. Health uses combat authority.
    for (int32 I=0; I<Host->Combatants.Num(); ++I)
    {
        const auto& Unit = Host->Combatants[I];
        ACharacter* Actor = Host->Actors.IsValidIndex(I) ? Host->Actors[I].Get() : nullptr;
        if (!IsValid(Actor) || Unit.Health<=0 || Actor->IsHidden()) continue;
        if (Unit.bPlayerHero && Host->bFirstPersonCamera && !Host->bTacticalCameraActive) continue;
        const auto& Bounds = Actor->GetMesh()->Bounds;
        FVector P = Project(Bounds.Origin+FVector(0,0,Bounds.BoxExtent.Z+24));
        P.X/=UI.Scale; P.Y/=UI.Scale;
        if (P.Z<=0 || P.X<8 || P.X>W-8 || P.Y<106 || P.Y>H-230) continue;
        AddHitBox(FVector2D(P.X-14,P.Y-19)*UI.Scale,FVector2D(28,30)*UI.Scale,
            FName(*FString::Printf(TEXT("Unit%d"),I)),true,5);
        const FLinearColor Team = Unit.Side==0 ? FLinearColor(0.2f,0.85f,1) : FLinearColor(1,0.3f,0.15f);
        UI.Line(P.X-5,P.Y-4,P.X,P.Y+1,Team,2);
        UI.Line(P.X,P.Y+1,P.X+5,P.Y-4,Team,2);
        if(Unit.Side==1) UI.Line(P.X-5,P.Y-6,P.X+5,P.Y-6,Team,2);
        if(Unit.Side==0 && (Host->bSelectAllAllies || Unit.GroupIndex==Host->SelectedAlliedFormation))
        {
            UI.Bar(P.X-13,P.Y-13,26,Unit.Health/FMath::Max(1.f,Unit.MaxHealth),Team);
        }
    }
    // Show the accepted order destination, never an independently simulated route.
    if(Host->bTacticalCameraActive && !Host->bFinished)
    for(const auto& Formation:Host->TacticalFormations)
    {
        if(Formation.Side!=0 || Formation.bRouting ||
            Formation.ManualOverrideUntil<=Host->BattleElapsed || Host->AliveInGroup(Formation.GroupIndex)<=0 ||
            (!Host->bSelectAllAllies && Formation.GroupIndex!=Host->SelectedAlliedFormation)) continue;
        FVector P=Project(Formation.TacticalAnchor+FVector(0,0,18));
        P.X/=UI.Scale; P.Y/=UI.Scale;
        if(P.Z<=0 || P.X<40 || P.X>W-90 || P.Y<150 || P.Y>H-220) continue;
        const FLinearColor Target(1.f,.72f,.20f,1.f);
        UI.Line(P.X-12,P.Y,P.X,P.Y-7,Target,2);
        UI.Line(P.X,P.Y-7,P.X+12,P.Y,Target,2);
        UI.Line(P.X+12,P.Y,P.X,P.Y+7,Target,2);
        UI.Line(P.X,P.Y+7,P.X-12,P.Y,Target,2);
        UI.Text(FString::Printf(TEXT("%s ORDER"),FSoulRealtimeTacticalRules::FormationKindLabel(Formation.Kind)),P.X+17,P.Y-7,UI.Bright);
    }

    UI.Panel(12,12,W-24,68);
    UI.Emblem(6,36,38,14,UI.Gold);
    UI.Text(TEXT("S O U L  /  DRAGON GRAVEYARD"),60,22,UI.Bright,1.3f);
    UI.Text(FString::Printf(TEXT("Your army %d  (+%d reserves)    Enemy %d  (+%d)"),
        Host->AliveForSide(0),Host->ReserveBodiesForSide(0),Host->AliveForSide(1),Host->ReserveBodiesForSide(1)),60,50,UI.Ink);
    UI.Button(TEXT("Pause"),Host->bFinished?TEXT("Battle resolved"):Host->bBattlePaused?TEXT("Resume [P]"):TEXT("Pause [P]"),W-455,28,142,34,Host->bBattlePaused);
    UI.Button(TEXT("Camera"),Host->bTacticalCameraActive?TEXT("Hero [C]"):TEXT("Commander [C]"),W-305,28,142,34);
    UI.Button(TEXT("View"),TEXT("1st / 3rd [X]"),W-155,28,131,34);
    UI.Panel(W*.5f-210,92,420,32,Host->bBattlePaused);
    UI.FitText(Host->bFinished?TEXT("BATTLE RESOLVED"):Host->bBattlePaused?TEXT("PAUSED  /  Plan orders, then resume"):Host->TacticalSummary(),W*.5f-198,101,396,UI.Bright);
    UI.Text(TEXT("ALLIES"),24,94,Ally); UI.Text(TEXT("ENEMIES"),112,94,Enemy);

    // Stable formation slots show strength and morale, including defeated formations.
    const int32 Count=FMath::Max(1,Host->AlliedFormationCount());
    const float Available=W-264, CardW=(Available-(Count-1)*8)/Count;
    int32 Slot=0;
    for(const auto& Formation:Host->TacticalFormations)
    {
        if(Formation.Side!=0) continue;
        const float X=12+Slot*(CardW+8), Y=H-228;
        const bool Selected=Host->bSelectAllAllies||Host->SelectedAlliedFormation==Formation.GroupIndex;
        const FName Action(*FString::Printf(TEXT("Formation%d"),Slot));
        UI.Button(Action,TEXT(""),X,Y,CardW,88,Selected);
        UI.Emblem(Formation.Kind==ESoulBattleFormationKind::FrontLine?3:Formation.Kind==ESoulBattleFormationKind::Strike?5:Formation.Kind==ESoulBattleFormationKind::MissileSupport?4:6,X+24,Y+30,13,Selected?UI.Bright:UI.Gold);
        UI.FitText(FString::Printf(TEXT("F%d  %s"),Slot+1,FSoulRealtimeTacticalRules::FormationKindLabel(Formation.Kind)),X+48,Y+12,CardW-60,UI.Ink,1.25f);
        const int32 Alive=Host->AliveInGroup(Formation.GroupIndex);
        const auto Morale=FSoulRealtimeTacticalRules::MoraleState(Formation.MoralePermille,Formation.bRouting,Formation.bRallied);
        UI.Text(FString::Printf(TEXT("%d / %d  |  %s"),Alive,Formation.InitialBodies,Alive?FSoulRealtimeTacticalRules::MoraleLabel(Morale):TEXT("DEFEATED")),X+48,Y+34,Alive?UI.Muted:Enemy);
        FString Summary=Host->AlliedFormationSummary(Slot);
        // The authority's summary supplies the order; present it on a separate line.
        const TCHAR* Order=TEXT("Hold");
        if(Summary.Contains(TEXT("ADVANCE"))) Order=TEXT("Advance");
        else if(Summary.Contains(TEXT("CHARGE"))) Order=TEXT("Charge");
        else if(Summary.Contains(TEXT("FALL BACK"))) Order=TEXT("Fall back");
        else if(Summary.Contains(TEXT("FOLLOW"))) Order=TEXT("Follow");
        else if(Summary.Contains(TEXT("FACE"))) Order=TEXT("Face");
        UI.Text(FString::Printf(TEXT("%s  /  %s"),Order,Formation.ManualOverrideUntil>Host->BattleElapsed?TEXT("Your orders"):TEXT("Commander")),X+12,Y+59,Selected?UI.Bright:UI.Muted,1.05f);
        UI.Bar(X+12,Y+79,CardW-24,Formation.MoralePermille/1000.f,Formation.bRouting?Enemy:Ally);
        ++Slot;
    }
    UI.Panel(W-240,H-228,228,88);
    UI.Text(FString::Printf(TEXT("HERO  %.0f HP"),Host->PlayerHealth()),W-224,H-215,UI.Ink,1.25f);
    UI.Text(FString::Printf(TEXT("MANA  %.0f"),Host->PlayerManaValue()),W-224,H-192,FLinearColor(.36f,.70f,1));
    UI.Button(TEXT("Focus"),TEXT("Focus selected [Home]"),W-228,H-168,204,24);

    static const TCHAR* Actions[]={TEXT("All"),TEXT("Move"),TEXT("Hold"),TEXT("Advance"),TEXT("Charge"),TEXT("Fallback"),TEXT("Face"),TEXT("AI")};
    static const TCHAR* Labels[]={TEXT("Select all"),TEXT("Move here"),TEXT("Hold [H]"),TEXT("Advance [V]"),TEXT("Charge [G]"),TEXT("Fall back [B]"),TEXT("Face [F]"),TEXT("AI orders [R]")};
    static const TCHAR* Hints[]={TEXT("Select every surviving friendly formation."),TEXT("Select, then click clear ground for this formation's destination."),TEXT("Hold this ground; engage nearby threats without chasing."),TEXT("Approach the enemy while keeping ranks together."),TEXT("Commit to contact. Troops may break ranks to pursue."),TEXT("Withdraw toward a safe position."),TEXT("Face the threat without advancing."),TEXT("Return selected formations to their commander's control.")};
    const float OrderW=(W-176)/8.f;
    for(int32 I=0;I<8;++I)
        if(UI.Button(Actions[I],Labels[I],12+I*(OrderW+4),H-132,OrderW,32)) Tooltip=Hints[I];
    UI.Button(TEXT("Help"),TEXT("Help [F10]"),W-124,H-132,112,32,Host->bShowBattleHelp);
    const float SpellW=(W-48)/5;
    const FLinearColor SpellColors[]={FLinearColor(1,.43f,.17f),FLinearColor(.80f,.62f,1),FLinearColor(.48f,.82f,1),FLinearColor(.25f,.65f,1),FLinearColor(.49f,.90f,.65f)};
    static const TCHAR* SpellNames[]={TEXT("Firebolt"),TEXT("Chain lightning"),TEXT("Blizzard"),TEXT("Tidal ward"),TEXT("Tailwind")};
    static const TCHAR* SpellHints[]={TEXT("Select, then click an enemy. A physical firebolt can hit intervening bodies or scenery."),TEXT("Select, then click an enemy near other enemies to chain lightning."),TEXT("Select, then click ground in hero range for a sustained area spell."),TEXT("Cast immediately: protect nearby allies."),TEXT("Cast immediately: hasten your surviving army.")};
    for(int32 I=0;I<5;++I)
    {
        const float X=12+I*(SpellW+6),Y=H-92;
        if(UI.Button(FName(*FString::Printf(TEXT("Spell%d"),I)),TEXT(""),X,Y,SpellW,58,Host->SelectedSpellSlot==I)) Tooltip=SpellHints[I];
        UI.Emblem(I,X+25,Y+29,15,Host->PlayerHealth()>0?SpellColors[I]:UI.Muted);
        UI.FitText(FString::Printf(TEXT("%d  %s"),I+1,SpellNames[I]),X+50,Y+10,SpellW-60,UI.Ink,1.2f);
        FString Name,Detail;
        Host->SpellButtonLabel(I).Split(TEXT(" | "),&Name,&Detail);
        UI.FitText(Host->PlayerHealth()>0?Detail:TEXT("Hero fallen"),X+50,Y+34,SpellW-60,UI.Muted,1.05f);
    }
    UI.FitText(Host->Status,24,H-24,W-48,UI.Bright,1.05f);
    const FString Context=Host->bPlaceFormationOrder?TEXT("MOVE READY: click clear ground | RMB cancels"):
        Host->SelectedSpellSlot>=0?TEXT("SPELL READY: click target | RMB cancels"):
        Host->bTacticalCameraActive?TEXT("Click ally to select  /  WASD pan  /  Wheel zoom  /  RMB orbit"):
        TEXT("WASD move  /  Mouse aim  /  LMB attack  /  RMB block  /  Hold Alt for buttons");
    UI.FitText(Host->bGamepadActive ? TEXT("RB: formation / LB: all / D-pad: orders / A: move / X: spell / Y: cast / B: cancel / Menu: pause") : Context,24,132,W-48,UI.Muted,1.05f);
    if(Host->bGamepadActive)
    {
        UI.Line(W*.5f-8,H*.5f,W*.5f-3,H*.5f,UI.Bright,2);
        UI.Line(W*.5f+3,H*.5f,W*.5f+8,H*.5f,UI.Bright,2);
        UI.Line(W*.5f,H*.5f-8,W*.5f,H*.5f-3,UI.Bright,2);
        UI.Line(W*.5f,H*.5f+3,W*.5f,H*.5f+8,UI.Bright,2);
    }
    if(!Tooltip.IsEmpty())
    {
        UI.Panel(22,H-277,W-44,34);
        UI.FitText(Tooltip,34,H-267,W-68,UI.Ink);
    }
    if(Host->bShowBattleHelp)
    {
        UI.Panel(22,162,680,158);
        UI.Text(TEXT("COMMAND YOUR BATTLE"),38,175,UI.Bright,1.35f);
        UI.Text(TEXT("P / Space pauses. Inspect the battle and issue orders while paused."),38,203,UI.Ink);
        UI.Text(TEXT("F1-F5 select formations. Tab cycles. Your orders persist until [R]."),38,228,UI.Ink);
        UI.Text(TEXT("Spells 1-3 need a target click. Spells 4-5 cast on your army immediately."),38,253,UI.Ink);
        UI.Text(TEXT("C: hero / commander. X: first / third person. Q/E: commander rotation."),38,278,UI.Ink);
    }
}
