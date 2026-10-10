#include "SoulRealtimeBattleArena.h"
#include "Engine/Canvas.h"
#include "SoulHUDTheme.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture2D.h"
#include "RBMagicSpellDefinition.h"

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
    if(Host->bCampaignAutoResolve)
    {
        UI.Panel(14,12,W-28,92);
        UI.Text(FString::Printf(TEXT("AI BATTLE  |  %s vs %s"),*Host->CampaignFactionForSide(0).ToString(),*Host->CampaignFactionForSide(1).ToString()),28,24,UI.Gold);
        UI.Text(FString::Printf(TEXT("Attacker: %d active + %d reserve     Defender: %d active + %d reserve"),Host->AliveForSide(0),Host->ReserveBodiesForSide(0),Host->AliveForSide(1),Host->ReserveBodiesForSide(1)),28,49,FLinearColor::White);
        UI.Text(TEXT("Armies fight automatically. You will return to the campaign when the battle ends."),28,74,FLinearColor::White);
        return;
    }
    const FLinearColor Ally(.2f,.78f,.86f), Enemy(.88f,.30f,.18f);
    FString Tooltip;
    // Subordinate, shape-distinct allegiance markers. Health uses combat authority.
    for (int32 I=0; I<Host->Combatants.Num(); ++I)
    {
        const auto& Unit = Host->Combatants[I];
        ACharacter* Actor = Host->Actors.IsValidIndex(I) ? Host->Actors[I].Get() : nullptr;
        if (!IsValid(Actor) || Unit.Health<=0 || Actor->IsHidden()) continue;
        if(Unit.bNonPlayerHero){FVector2D At;if(GetOwningPlayerController()->ProjectWorldLocationToScreen(Actor->GetActorLocation()+FVector(0,0,150),At,true))UI.Text(TEXT("DWARF COMMANDER"),At.X/UI.Scale-65,At.Y/UI.Scale-20,UI.Gold,.8f);}
        if (Unit.bPlayerHero && Host->bFirstPersonCamera && !Host->bTacticalCameraActive) continue;
        const auto& Bounds = Actor->GetMesh()->Bounds;
        FVector P = Project(Bounds.Origin+FVector(0,0,Bounds.BoxExtent.Z+24));
        P.X/=UI.Scale; P.Y/=UI.Scale;
        if (P.Z<=0 || P.X<8 || P.X>W-8 || P.Y<54 || P.Y>H-124) continue;
        AddHitBox(FVector2D(P.X-14,P.Y-19)*UI.Scale,FVector2D(28,30)*UI.Scale,
            FName(*FString::Printf(TEXT("Unit%d"),I)),true,5);
        const FLinearColor Team = Unit.Side==Host->ControlledSide ? FLinearColor(0.2f,0.85f,1) : FLinearColor(1,0.3f,0.15f);
        UI.Line(P.X-5,P.Y-4,P.X,P.Y+1,Team,2);
        UI.Line(P.X,P.Y+1,P.X+5,P.Y-4,Team,2);
        if(Unit.Side==1-Host->ControlledSide) UI.Line(P.X-5,P.Y-6,P.X+5,P.Y-6,Team,2);
        if(Host->IsUnitSelected(I))
        {
            UI.Bar(P.X-13,P.Y-13,26,Unit.Health/FMath::Max(1.f,Unit.MaxHealth),Team);
        }
    }
    // Show the accepted order destination, never an independently simulated route.
    if(Host->bTacticalCameraActive && !Host->bFinished)
    for(const auto& Formation:Host->TacticalFormations)
    {
        if(Formation.Side!=Host->ControlledSide || Formation.bRouting ||
            Formation.ManualOverrideUntil<=Host->BattleElapsed || Host->AliveInGroup(Formation.GroupIndex)<=0 ||
            (!Host->bSelectAllAllies && Formation.GroupIndex!=Host->SelectedAlliedFormation)) continue;
        FVector P=Project(Formation.TacticalAnchor+FVector(0,0,18));
        P.X/=UI.Scale; P.Y/=UI.Scale;
        if(P.Z<=0 || P.X<40 || P.X>W-90 || P.Y<54 || P.Y>H-124) continue;
        const FLinearColor Target(1.f,.72f,.20f,1.f);
        UI.Line(P.X-12,P.Y,P.X,P.Y-7,Target,2);
        UI.Line(P.X,P.Y-7,P.X+12,P.Y,Target,2);
        UI.Line(P.X+12,P.Y,P.X,P.Y+7,Target,2);
        UI.Line(P.X,P.Y+7,P.X-12,P.Y,Target,2);
        UI.Text(FString::Printf(TEXT("%s ORDER"),FSoulRealtimeTacticalRules::FormationKindLabel(Formation.Kind)),P.X+17,P.Y-7,UI.Bright);
    }



    // Read-only targeting preview. Mana, range and effect commitment remain in RB Magic.
    if(Host->SelectedSpellSlot>=0 && !Host->bSpellbookOpen && !Host->bFinished)
    {
        const int32 Slot=Host->SelectedSpellSlot;
        const auto* Spell=Host->BattleSpells.IsValidIndex(Slot)?Host->BattleSpells[Slot].Get():nullptr;
        FHitResult Hit;
        bool HaveTarget=Slot>=3?IsValid(Host->PlayerHero):Host->ReadPointerHit(Hit);
        FVector Target=Slot>=3 && Host->PlayerHero?Host->PlayerHero->GetActorLocation():FVector(Hit.ImpactPoint);
        bool Valid=HaveTarget && Spell && Host->SpellBlockReason(Slot,true).IsEmpty();
        if(Slot<2)
        {
            const auto* Binding=Hit.GetActor()?Hit.GetActor()->FindComponentByClass<USoulRealtimeArenaBinding>():nullptr;
            const int32 Index=Binding?Host->Index(FRBHostIdentity::From(Binding->GetCombatant())):INDEX_NONE;
            Valid=Valid && Host->Combatants.IsValidIndex(Index) && Host->Combatants[Index].Side==1-Host->ControlledSide && Host->Combatants[Index].Health>0;
            if(Host->Actors.IsValidIndex(Index) && Host->Actors[Index]) Target=Host->Actors[Index]->GetActorLocation();
        }
        if(Slot==2 && HaveTarget) Target.Z=Host->ResolveSpawnLocation(Target).Z-94.f;
        if(Slot<3 && Spell && Host->PlayerHero)
            Valid=Valid && FVector::Dist(Host->PlayerHero->GetActorLocation(),Target)<=Spell->Range;
        if(HaveTarget)
        {
            float Radius=75.f;
            if(Slot==2 && Spell)
                for(const auto& Effect:Spell->Effects) Radius=FMath::Max(Radius,Effect.Radius);
            const FLinearColor Color=Valid?FLinearColor(.38f,.85f,1):FLinearColor(.8f,.35f,.2f);
            for(int32 I=0;I<32;++I)
            {
                const float A=2*PI*I/32,B=2*PI*(I+1)/32;
                const FVector P=Project(Target+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,20));
                const FVector Q=Project(Target+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,20));
                if(P.Z>0 && Q.Z>0) UI.Line(P.X/UI.Scale,P.Y/UI.Scale,Q.X/UI.Scale,Q.Y/UI.Scale,Color,2);
            }
            const FVector P=Project(Target+FVector(0,0,120));
            if(P.Z>0) UI.Text(Valid?Host->bBattlePaused?TEXT("Resume to cast"):TEXT("Confirm cast"):TEXT("Choose a valid target"),
                P.X/UI.Scale+12,P.Y/UI.Scale,Color);
        }
    }

    // Compact fixed-height top strip. The world remains the dominant surface.
    UI.Panel(12,12,W-24,38);
    UI.Emblem(6,28,31,10,UI.Gold);
    UI.Text(TEXT("S O U L"),47,24,UI.Bright);
    UI.Text(FString::Printf(TEXT("ALLIES %d (+%d)"),Host->AliveForSide(Host->ControlledSide),Host->ReserveBodiesForSide(Host->ControlledSide)),135,24,Ally);
    UI.Text(FString::Printf(TEXT("ENEMY %d (+%d)"),Host->AliveForSide(1-Host->ControlledSide),Host->ReserveBodiesForSide(1-Host->ControlledSide)),295,24,Enemy);
    FString Phase=Host->TacticalSummary();
    int32 PhaseSeparator=INDEX_NONE;
    if(Phase.FindChar(TCHAR('|'),PhaseSeparator)) Phase.LeftInline(PhaseSeparator);
    Phase.TrimEndInline();
    UI.FitText(Host->bFinished?Host->BattleResultLabel:Host->bBattlePaused?TEXT("PAUSED"):Phase,465,18,W-900,UI.Bright);
    const float Friendly=Host->FieldStrengthEstimate(Host->ControlledSide),Hostile=Host->FieldStrengthEstimate(1-Host->ControlledSide);
    const float Balance=Friendly+Hostile>0 ? Friendly/(Friendly+Hostile) : .5f;
    const float BalanceW=FMath::Min(210.f,W-900);
    UI.Rect(465,39,BalanceW,5,Enemy);
    UI.Rect(465,39,BalanceW*Balance,5,Ally);
    UI.Line(465+BalanceW*.5f,37,465+BalanceW*.5f,46,UI.Ink,1);
    if(UI.Button(TEXT("FieldStrength"),TEXT(""),465,36,BalanceW,12))
        Tooltip=TEXT("FIELD STRENGTH: surviving health, with routing units discounted. Active fighters only; reserves are the (+counts). An estimate, not a victory prediction.");
    // Draw after the transparent hover/click region so the bar remains visible.
    UI.Rect(466,40,BalanceW-2,3,Enemy);
    UI.Rect(466,40,(BalanceW-2)*Balance,3,Ally);
    UI.Button(TEXT("Pause"),Host->bFinished?TEXT("Resolved"):Host->bBattlePaused?TEXT("Start / Resume [P]"):
        Host->bAllowTacticalPause?TEXT("Pause [P]"):TEXT("No pause"),W-422,17,144,28,Host->bBattlePaused);
    UI.Button(TEXT("Camera"),Host->bTacticalCameraActive?TEXT("Hero [C]"):TEXT("Commander [C]"),W-270,17,142,28);
    UI.Button(TEXT("View"),TEXT("View [X]"),W-120,17,100,28);
    if(Host->bBattlePaused && Host->BattleElapsed<=0.f)
    {
        if(UI.Button(TEXT("PauseRule"),Host->bAllowTacticalPause?TEXT("Tactical pause: ON"):TEXT("Tactical pause: OFF"),12,58,185,28))
            Tooltip=TEXT("Choose before starting. OFF keeps battle live when browsing spells and disables tactical pause after deployment.");
    }

    // Wrap details instead of shrinking or truncating them into one long line.
    auto Paragraph=[&](const FString& Value,float X,float Y,float Width,FLinearColor Color,float Size=1.1f)
    {
        TArray<FString> Words;Value.ParseIntoArrayWS(Words);
        FString Line;
        for(const FString& Word:Words)
        {
            const FString Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;
            float TW=0,TH=0;Canvas->StrLen(GEngine->GetSmallFont(),Next,TW,TH);
            if(TW*Size>Width && !Line.IsEmpty())
            { UI.Text(Line,X,Y,Color,Size);Y+=17;Line=Word; }
            else Line=Next;
        }
        if(!Line.IsEmpty()) { UI.Text(Line,X,Y,Color,Size);Y+=17; }
        return Y;
    };
    auto Icon=[&](int32 I,float X,float Y,float Size,bool Ready=true)
    {
        UTexture2D* Texture=Host->SpellIcons.IsValidIndex(I)?Host->SpellIcons[I].Get():nullptr;
        if(Texture) DrawTexture(Texture,X*UI.Scale,Y*UI.Scale,Size*UI.Scale,Size*UI.Scale,0,0,1,1,
            Ready?FLinearColor::White:FLinearColor(.38f,.38f,.38f,1),BLEND_Translucent);
        else UI.Emblem(I,(X+Size*.5f),(Y+Size*.5f),Size*.32f,Ready?UI.Bright:UI.Muted);
    };
    auto ShieldUI=[&](FName Name,float X,float Y,float Width,float Height,int32 Priority=8)
    { AddHitBox(FVector2D(X,Y)*UI.Scale,FVector2D(Width,Height)*UI.Scale,Name,true,Priority); };

    ShieldUI(TEXT("TopStrip"),12,12,W-24,38);
    const int32 Count=FMath::Max(1,Host->AlliedFormationCount());
    const float ArmyWidth=W-474,CardW=FMath::Min(88.f,(ArmyWidth-(Count-1)*6)/Count);
    int32 Slot=0;
    for(const auto& Formation:Host->TacticalFormations)
    {
        if(Formation.Side!=Host->ControlledSide) continue;
        const float X=12+Slot*(CardW+6),Y=H-120;
        const bool Selected=Host->bSelectAllAllies||Host->SelectedAlliedFormation==Formation.GroupIndex;
        if(UI.Button(FName(*FString::Printf(TEXT("Formation%d"),Slot)),TEXT(""),X,Y,CardW,54,Selected))
        {
            int32 Members[8]={},Arrows=0;float Health=0,Maximum=0;
            for(const auto& Unit:Host->Combatants)
                if(Unit.GroupIndex==Formation.GroupIndex && Unit.Health>0)
                {
                    const int32 RoleIndex=static_cast<int32>(Unit.Role);
                    if(RoleIndex>=0 && RoleIndex<8) ++Members[RoleIndex];
                    Health+=Unit.Health;Maximum+=Unit.MaxHealth;
                    if(Unit.bRanged) Arrows+=Unit.Arrows;
                }
            Tooltip=Host->AlliedFormationSummary(Slot)+TEXT(". ");
            for(int32 RoleIndex=0;RoleIndex<8;++RoleIndex)
                if(Members[RoleIndex]) Tooltip+=FString::Printf(TEXT("%d %s. "),Members[RoleIndex],*Host->RoleLabel(static_cast<ESoulRealtimeFormationRole>(RoleIndex)));
            Tooltip+=FString::Printf(TEXT("Health %.0f%%. %d arrows. Click to select; Home focuses."),
                100.f*Health/FMath::Max(1.f,Maximum),Arrows);
        }
        const int32 Alive=Host->AliveInGroup(Formation.GroupIndex);
        const FLinearColor Color=Alive ? Selected?UI.Bright:UI.Ink : UI.Muted;
        const float CX=X+CardW*.5f,CY=Y+23;
        if(Formation.Kind==ESoulBattleFormationKind::FrontLine)
        {
            for(int32 Soldier=-1;Soldier<=1;++Soldier)
            {
                const float SX=CX+Soldier*12;
                UI.Rect(SX-3,CY-13,6,6,Color);
                UI.Line(SX,CY-5,SX,CY+6,Color,3);
                UI.Line(SX-4,CY-3,SX+4,CY-3,Color,2);
                UI.Line(SX,CY+6,SX-4,CY+12,Color,2);
                UI.Line(SX,CY+6,SX+4,CY+12,Color,2);
            }
        }
        else if(Formation.Kind==ESoulBattleFormationKind::MissileSupport)
        {
            // Bow, string and nocked arrow; original vector artwork.
            UI.Line(CX-7,CY-14,CX+4,CY-7,Color,2);
            UI.Line(CX+4,CY-7,CX+7,CY,Color,2);
            UI.Line(CX+7,CY,CX+4,CY+7,Color,2);
            UI.Line(CX+4,CY+7,CX-7,CY+14,Color,2);
            UI.Line(CX-7,CY-14,CX-7,CY+14,Color,1);
            UI.Line(CX-15,CY,CX+17,CY,Color,2);
            UI.Line(CX+17,CY,CX+11,CY-4,Color,2);
            UI.Line(CX+17,CY,CX+11,CY+4,Color,2);
        }
        else if(Formation.Kind==ESoulBattleFormationKind::Strike)
        {
            UI.Line(CX-8,CY+10,CX+11,CY-13,Color,4);
            UI.Line(CX-12,CY+1,CX+1,CY+12,Color,3);
            UI.Line(CX-8,CY+10,CX-13,CY+16,UI.Gold,3);
        }
        else UI.Emblem(6,CX,CY,14,Color);
        UI.Text(FString::Printf(TEXT("F%d"),Slot+1),X+5,Y+4,UI.Muted,.9f);
        UI.Text(FString::FromInt(Alive),X+CardW-22,Y+30,Color,1.1f);
        UI.Bar(X+6,Y+46,CardW-12,Formation.MoralePermille/1000.f,Formation.bRouting?Enemy:Ally);
        ++Slot;
    }
    static const TCHAR* Actions[]={TEXT("All"),TEXT("Move"),TEXT("Hold"),TEXT("Advance"),TEXT("Charge"),TEXT("Fallback"),TEXT("Face"),TEXT("AI")};
    static const TCHAR* Labels[]={TEXT("All"),TEXT("Move"),TEXT("Hold H"),TEXT("Advance V"),TEXT("Charge G"),TEXT("Back B"),TEXT("Face F"),TEXT("AI R")};
    static const TCHAR* Hints[]={TEXT("Select every surviving friendly formation."),TEXT("Select, then click clear ground for the formation destination."),
        TEXT("Brace on this ground in tighter ranks. Infantry guard nearby threats and do not chase."),TEXT("Approach the enemy while keeping ranks together."),
        TEXT("Commit to contact. Troops may break ranks to pursue."),TEXT("Withdraw toward a safe position."),
        TEXT("Face the threat without advancing."),TEXT("Return selected formations to their commander.")};
    const float OrderW=(ArmyWidth-28)/8.f;
    for(int32 I=0;I<8;++I)
        if(UI.Button(Actions[I],Labels[I],12+I*(OrderW+4),H-60,OrderW,28)) Tooltip=Hints[I];

    UI.Panel(W-446,H-120,434,54);
    ShieldUI(TEXT("HeroPanel"),W-446,H-120,434,54);
    UI.Text(FString::Printf(TEXT("HERO  %.0f HP"),Host->PlayerHealth()),W-434,H-111,UI.Ink);
    UI.Text(FString::Printf(TEXT("MANA  %.0f"),Host->PlayerManaValue()),W-278,H-111,FLinearColor(.36f,.70f,1));
    UI.FitText(Host->bTacticalCameraActive?TEXT("[J] Hero: sword / block"):TEXT("LMB sword | RMB block | C command"),W-434,H-90,285,UI.Muted,1.f);
    UI.Button(TEXT("Hero"),TEXT("Hero [J]"),W-140,H-109,116,30);

    int32 HoverSpell=INDEX_NONE;
    for(int32 I=0;I<5;++I)
    {
        const float X=W-446+I*49,Y=H-60;
        const bool Ready=Host->SpellBlockReason(I,true).IsEmpty();
        if(UI.Button(FName(*FString::Printf(TEXT("Spell%d"),I)),TEXT(""),X,Y,44,44,Host->SelectedSpellSlot==I)) HoverSpell=I;
        Icon(I,X+3,Y+3,38,Ready);
        UI.Rect(X+1,Y+27,13,16,FLinearColor(0,0,0,.9f));
        UI.Text(FString::FromInt(I+1),X+3,Y+28,UI.Ink,1.f);
        if(!Ready)
        {
            const auto* Spell=Host->BattleSpells.IsValidIndex(I)?Host->BattleSpells[I].Get():nullptr;
            const float Cooldown=Spell?Host->SpellCooldowns.FindRef(Spell->SpellTag.GetTagName()):0.f;
            UI.Text(Cooldown>0?FString::Printf(TEXT("%.0f"),FMath::CeilToFloat(Cooldown)):TEXT("-"),X+18,Y+15,UI.Bright);
        }
    }
    UI.Button(TEXT("Grimoire"),TEXT("Spellbook [K]"),W-191,H-60,139,44,Host->bSpellbookOpen);
    UI.Button(TEXT("Help"),TEXT("?"),W-46,H-60,34,44,Host->bShowBattleHelp);

    static const TCHAR* SpellNames[]={TEXT("Firebolt"),TEXT("Chain lightning"),TEXT("Blizzard"),TEXT("Tidal ward"),TEXT("Tailwind")};
    static const TCHAR* Descriptions[]={
        TEXT("Launch a firebolt at an enemy. Bodies and scenery can intercept the projectile."),
        TEXT("Strike an enemy and chain lightning to nearby foes."),
        TEXT("Cover an area with damaging frost and slow enemies within it."),
        TEXT("Shield your hero against incoming damage."),
        TEXT("Hasten your surviving army to help it reposition.")};
    static const TCHAR* Targets[]={TEXT("Target: enemy"),TEXT("Target: enemy"),TEXT("Target: ground"),TEXT("Target: your hero"),TEXT("Target: allied army")};
    auto SpellDetails=[&](int32 I,float X,float Y,float Width)
    {
        UI.Text(SpellNames[I],X,Y,UI.Bright,1.25f);Y+=26;
        const auto* Spell=Host->BattleSpells.IsValidIndex(I)?Host->BattleSpells[I].Get():nullptr;
        if(Spell)
        {
            UI.Text(FString::Printf(TEXT("%.0f mana  /  %.0fs cooldown"),Host->SpellManaCost(*Spell),Spell->CooldownSeconds),X,Y,UI.Ink,1.05f);Y+=22;
        }
        UI.Text(Targets[I],X,Y,Ally,1.05f);Y+=23;
        Y=Paragraph(Descriptions[I],X,Y,Width,UI.Ink)+12;
        const FString Reason=Host->SpellBlockReason(I,true);
        Y=Paragraph(Reason.IsEmpty()?Host->bBattlePaused?TEXT("Ready. Resume before casting."):TEXT("Ready to select."):Reason,X,Y,Width,Reason.IsEmpty()?UI.Muted:UI.Bright)+8;
        Paragraph(TEXT("Select, then confirm on the battlefield. Right click cancels."),X,Y,Width,UI.Muted,1.f);
    };
    if(Host->bSpellbookOpen)
    {
        const float X=W-452,Y=H-470,BookW=440,BookH=338;
        UI.Panel(X,Y,BookW,BookH);
        ShieldUI(TEXT("BookBackground"),X,Y,BookW,BookH);
        // Original two-page layout: restrained spine, icon index, readable detail page.
        UI.Rect(X+8,Y+46,205,BookH-56,FLinearColor(.09f,.085f,.067f,1));
        UI.Rect(X+219,Y+46,213,BookH-56,FLinearColor(.105f,.096f,.072f,1));
        UI.Line(X+215,Y+48,X+215,Y+BookH-10,UI.Gold,2);
        Icon(5,X+10,Y+6,32);
        UI.Text(TEXT("GRIMOIRE"),X+51,Y+9,UI.Bright,1.3f);
        UI.Text(Host->bBattlePaused?TEXT("Battle paused manually"):TEXT("Battle continues"),X+51,Y+27,UI.Muted,1.f);
        UI.Button(TEXT("CloseGrimoire"),TEXT("Close"),X+358,Y+8,70,28);
        int32 Detail=Host->SelectedSpellSlot>=0?Host->SelectedSpellSlot:0;
        for(int32 I=0;I<5;++I)
        {
            const float RowY=Y+55+I*52;
            const bool Ready=Host->SpellBlockReason(I,true).IsEmpty();
            if(UI.Button(FName(*FString::Printf(TEXT("BookSpell%d"),I)),TEXT(""),X+14,RowY,192,46,Host->SelectedSpellSlot==I)) Detail=I;
            Icon(I,X+18,RowY+4,38,Ready);
            UI.FitText(SpellNames[I],X+64,RowY+7,133,Ready?UI.Ink:UI.Muted,1.05f);
            const auto* Spell=Host->BattleSpells.IsValidIndex(I)?Host->BattleSpells[I].Get():nullptr;
            UI.Text(Spell?FString::Printf(TEXT("[%d]  %.0f mana"),I+1,Host->SpellManaCost(*Spell)):TEXT("Unavailable"),X+64,RowY+27,UI.Muted,1.f);
        }
        SpellDetails(Detail,X+231,Y+60,189);
    }
    else if(HoverSpell>=0 || (Host->bGamepadActive && Host->SelectedSpellSlot>=0))
    {
        const int32 Detail=HoverSpell>=0?HoverSpell:Host->SelectedSpellSlot;
        UI.Panel(W-342,H-354,330,222);
        ShieldUI(TEXT("SpellTooltip"),W-342,H-354,330,222,20);
        SpellDetails(Detail,W-326,H-340,298);
    }
    else if(!Tooltip.IsEmpty())
    {
        UI.Panel(12,H-238,650,108);
        ShieldUI(TEXT("OrderTooltip"),12,H-238,650,108,20);
        Paragraph(Tooltip,26,H-223,621,UI.Ink);
    }

    const FString Context=Host->bPlaceFormationOrder?TEXT("MOVE: click clear ground / RMB cancel"):
        Host->SelectedSpellSlot>=0?Host->bBattlePaused?TEXT("SPELL READY / resume [P], then click / RMB cancel"):TEXT("SPELL READY / click to cast / RMB cancel"):
        Host->bTacticalCameraActive?TEXT("WASD pan / wheel zoom / RMB orbit / K spellbook"):
        TEXT("WASD move / LMB attack / RMB block / K spellbook / Alt cursor");
    UI.FitText(Host->bGamepadActive?TEXT("RB formation / D-pad orders / X spell / Y cast / B cancel / Menu pause"):Context,22,94,700,UI.Muted,1.f);
    UI.FitText(Host->Status,22,H-23,ArmyWidth-12,UI.Bright,1.f);
    if(Host->bGamepadActive)
    {
        UI.Line(W*.5f-8,H*.5f,W*.5f-3,H*.5f,UI.Bright,2);
        UI.Line(W*.5f+3,H*.5f,W*.5f+8,H*.5f,UI.Bright,2);
        UI.Line(W*.5f,H*.5f-8,W*.5f,H*.5f-3,UI.Bright,2);
        UI.Line(W*.5f,H*.5f+3,W*.5f,H*.5f+8,UI.Bright,2);
    }
    int32 NoticeRow=0;
    for(const auto& Notice:Host->BattleNotices)
    {
        if(Notice.ExpiresAt<=Host->BattleElapsed) continue;
        const float Y=58+NoticeRow*28;
        UI.Panel(W-322,Y,310,24);
        ShieldUI(FName(*FString::Printf(TEXT("Notice%d"),NoticeRow)),W-322,Y,310,24);
        UI.FitText(Notice.Text,W-312,Y+5,290,Notice.Side==Host->ControlledSide?Ally:Notice.Side==1-Host->ControlledSide?Enemy:UI.Bright,1.f);
        ++NoticeRow;
    }
    if(Host->bShowBattleHelp)
    {
        UI.Panel(22,130,625,190);
        ShieldUI(TEXT("HelpBackground"),22,130,625,190,20);
        UI.Text(TEXT("COMMAND YOUR BATTLE"),38,144,UI.Bright,1.3f);
        UI.Text(TEXT("P / Space: start or pause (if enabled during deployment)."),38,174,UI.Ink);
        UI.Text(TEXT("F1-F5: formations. Tab: cycle. R: return orders to AI."),38,199,UI.Ink);
        UI.Text(TEXT("K: spellbook. 1-5: ready spell. Click: cast. RMB: cancel."),38,224,UI.Ink);
        UI.Text(TEXT("Opening the book never pauses. Resume before casting."),38,249,UI.Ink);
        UI.Text(TEXT("C: hero / commander. X: first / third person. Home: focus."),38,274,UI.Ink);
        UI.Text(TEXT("F10 closes help. Esc closes menus or targeting first."),38,299,UI.Muted,1.f);
    }
}
