#include "SoulRealtimeBattleArena.h"
#include "Engine/Canvas.h"
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
    const float W = Canvas->ClipX, H = Canvas->ClipY;
    const FLinearColor Back(0.018f,0.027f,0.034f,0.92f);
    const FLinearColor Gold(0.66f,0.49f,0.18f,1);
    const FColor Ink(238,236,222), Muted(177,188,190), Accent(255,220,105);
    float MX = -1, MY = -1;
    if (GetOwningPlayerController()) GetOwningPlayerController()->GetMousePosition(MX, MY);
    auto Button = [&](FName Action, const FString& Label, float X, float Y, float BW, float BH, bool Selected = false)
    {
        const bool Hover = MX >= X && MX <= X+BW && MY >= Y && MY <= Y+BH;
        DrawRect(Selected ? FLinearColor(0.20f,0.16f,0.065f,0.97f) :
            Hover ? FLinearColor(0.13f,0.21f,0.25f,0.98f) : Back, X,Y,BW,BH);
        DrawRect(Selected ? Gold : FLinearColor(0.23f,0.32f,0.35f,1), X,Y,BW,2);
        DrawText(Label, Selected ? Accent : Ink, X+9,Y+9,nullptr,0.85f,false);
        AddHitBox(FVector2D(X,Y),FVector2D(BW,BH),Action,true,10);
    };
    // Subordinate, shape-distinct allegiance markers. Health uses combat authority.
    for (int32 I=0; I<Host->Combatants.Num(); ++I)
    {
        const auto& Unit = Host->Combatants[I];
        ACharacter* Actor = Host->Actors.IsValidIndex(I) ? Host->Actors[I].Get() : nullptr;
        if (!IsValid(Actor) || Unit.Health<=0 || Actor->IsHidden()) continue;
        if (Unit.bPlayerHero && Host->bFirstPersonCamera && !Host->bTacticalCameraActive) continue;
        const auto& Bounds = Actor->GetMesh()->Bounds;
        const FVector P = Project(Bounds.Origin+FVector(0,0,Bounds.BoxExtent.Z+24));
        if (P.Z<=0 || P.X<8 || P.X>W-8 || P.Y<106 || P.Y>H-205) continue;
        AddHitBox(FVector2D(P.X-14,P.Y-19),FVector2D(28,30),
            FName(*FString::Printf(TEXT("Unit%d"),I)),true,5);
        const FLinearColor Team = Unit.Side==0 ? FLinearColor(0.2f,0.85f,1) : FLinearColor(1,0.3f,0.15f);
        DrawLine(P.X-5,P.Y-4,P.X,P.Y+1,Team,2);
        DrawLine(P.X,P.Y+1,P.X+5,P.Y-4,Team,2);
        if(Unit.Side==1) DrawLine(P.X-5,P.Y-6,P.X+5,P.Y-6,Team,2);
        if(Unit.Side==0 && (Host->bSelectAllAllies || Unit.GroupIndex==Host->SelectedAlliedFormation))
        {
            DrawRect(FLinearColor(0.02f,0.02f,0.02f,0.9f),P.X-13,P.Y-13,26,4);
            DrawRect(Team,P.X-12,P.Y-12,24*FMath::Clamp(Unit.Health/FMath::Max(1.0f,Unit.MaxHealth),0.0f,1.0f),2);
        }
    }
    // Show the accepted order destination, never an independently simulated route.
    if(Host->bTacticalCameraActive && !Host->bFinished)
    for(const auto& Formation:Host->TacticalFormations)
    {
        if(Formation.Side!=0 || Formation.bRouting ||
            Formation.ManualOverrideUntil<=Host->BattleElapsed || Host->AliveInGroup(Formation.GroupIndex)<=0 ||
            (!Host->bSelectAllAllies && Formation.GroupIndex!=Host->SelectedAlliedFormation)) continue;
        const FVector P=Project(Formation.TacticalAnchor+FVector(0,0,18));
        if(P.Z<=0 || P.X<40 || P.X>W-90 || P.Y<150 || P.Y>H-220) continue;
        const FLinearColor Target(1.f,.72f,.20f,1.f);
        DrawLine(P.X-12,P.Y,P.X,P.Y-7,Target,2);
        DrawLine(P.X,P.Y-7,P.X+12,P.Y,Target,2);
        DrawLine(P.X+12,P.Y,P.X,P.Y+7,Target,2);
        DrawLine(P.X,P.Y+7,P.X-12,P.Y,Target,2);
        DrawText(FString::Printf(TEXT("%s ORDER"),FSoulRealtimeTacticalRules::FormationKindLabel(Formation.Kind)),
            Accent,P.X+17,P.Y-7,nullptr,.75f,false);
    }
    DrawRect(Back,12,12,W-24,62);
    DrawText(TEXT("SOUL | DRAGON GRAVEYARD"),Accent,24,20);
    DrawText(FString::Printf(TEXT("ALLIES %d (+%d)   ENEMY %d (+%d)   HERO %.0f   MANA %.0f"),
        Host->AliveForSide(0),Host->ReserveBodiesForSide(0),Host->AliveForSide(1),
        Host->ReserveBodiesForSide(1),Host->PlayerHealth(),Host->PlayerManaValue()),Ink,24,46,nullptr,0.9f,false);
    Button(TEXT("Pause"),Host->bFinished ? TEXT("Resolved") :
        Host->bBattlePaused ? TEXT("Start / Resume [P]") : TEXT("Pause [P]"),W-455,22,148,36);
    Button(TEXT("Camera"),Host->bTacticalCameraActive ? TEXT("Hero view [C]") : TEXT("Commander [C]"),W-301,22,140,36);
    Button(TEXT("View"),TEXT("1st / 3rd [X]"),W-155,22,131,36);
    DrawText(Host->bFinished ? TEXT("BATTLE RESOLVED") :
        Host->bBattlePaused ? TEXT("PAUSED - inspect troops and issue orders, then Resume") : Host->TacticalSummary(),
        Host->bBattlePaused ? Accent : Ink,24,86,nullptr,0.95f,false);
    DrawText(TEXT("v ALLIES"),FColor(51,217,255),W-205,86,nullptr,0.8f,false);
    DrawText(TEXT("v ENEMIES"),FColor(255,77,38),W-110,86,nullptr,0.8f,false);

    const int32 Count = FMath::Max(1,Host->AlliedFormationCount());
    const float CardW = FMath::Min(235.0f,(W-252-(Count-1)*6)/Count);
    for(int32 Slot=0;Slot<Count;++Slot)
    {
        const FString Summary=Host->AlliedFormationSummary(Slot);
        if(Summary.IsEmpty()) continue;
        const float X=12+Slot*(CardW+6);
        const bool Selected=Host->bSelectAllAllies || Summary.Contains(TEXT("< SELECTED"));
        Button(FName(*FString::Printf(TEXT("Formation%d"),Slot)),Summary.Replace(TEXT("  < SELECTED"),TEXT("")),
            X,H-198,CardW,64,Selected);
        DrawText(Selected ? TEXT("SELECTED | click orders below") : TEXT("Click to select"),Selected ? Accent : Muted,
            X+9,H-165,nullptr,0.78f,false);
    }
    Button(TEXT("Focus"),TEXT("Focus selected [Home]"),W-228,H-198,216,36);
    static const TCHAR* Actions[]={TEXT("All"),TEXT("Move"),TEXT("Hold"),TEXT("Advance"),TEXT("Charge"),TEXT("Fallback"),TEXT("Face"),TEXT("AI")};
    static const TCHAR* Labels[]={TEXT("Select all"),TEXT("Move here"),TEXT("Hold [H]"),TEXT("Advance [V]"),TEXT("Charge [G]"),TEXT("Fall back [B]"),TEXT("Face [F]"),TEXT("AI control [R]")};
    const float OrderW=FMath::Min(130.0f,(W-174)/8);
    for(int32 I=0;I<8;++I) Button(Actions[I],Labels[I],12+I*(OrderW+4),H-126,OrderW,32);
    Button(TEXT("Help"),TEXT("Controls [F10]"),W-132,H-126,120,32,Host->bShowBattleHelp);
    const float SpellW=FMath::Min(193.0f,(W-24-24)/5);
    for(int32 I=0;I<5;++I)
        Button(FName(*FString::Printf(TEXT("Spell%d"),I)),Host->SpellButtonLabel(I),
            12+I*(SpellW+6),H-86,SpellW,42,Host->SelectedSpellSlot==I);
    DrawRect(Back,12,H-38,W-24,26);
    DrawText(Host->Status,Accent,24,H-33,nullptr,0.88f,false);
    if(Host->bPlaceFormationOrder)
        DrawText(TEXT("MOVE READY: click clear ground | RMB cancels"),Accent,24,113);
    else if(Host->SelectedSpellSlot>=0)
        DrawText(TEXT("SPELL READY: click target | RMB cancels"),Accent,24,113);
    else DrawText(Host->bTacticalCameraActive ? TEXT("Click ally to select | WASD pan | wheel zoom | RMB orbit | Shift+RMB move") :
        TEXT("WASD move | mouse aim | LMB attack | RMB block | hold Alt for buttons"),
        Muted,24,113,nullptr,0.8f,false);
    if(Host->bShowBattleHelp)
    {
        DrawRect(Back,12,145,535,145);
        DrawText(TEXT("PAUSE freely to inspect and give orders. Resume with P or Space."),Ink,24,158,nullptr,0.85f,false);
        DrawText(TEXT("F1-F5 select formation. TAB cycles. Click Select all for the army."),Ink,24,182,nullptr,0.85f,false);
        DrawText(TEXT("Your orders persist until AI control [R]. Broken troops must rally."),Ink,24,206,nullptr,0.85f,false);
        DrawText(TEXT("Spells 1-3: select then click enemy/ground. 4-5: cast on your side."),Ink,24,230,nullptr,0.85f,false);
        DrawText(TEXT("Commander: Q/E rotate. Hero: X switches first/third person."),Ink,24,254,nullptr,0.85f,false);
    }
}

