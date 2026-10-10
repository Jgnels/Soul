#include "SoulHeartlandBridgeGeometry.h"
#include "SoulRealtimeBattleArena.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"

// Qualification drives the same command handlers as the tactical HUD. Never changes damage/results.
void ASoulRealtimeArenaGameMode::TickHumanDefenseQualification()
{
    const bool Defense=FParse::Param(FCommandLine::Get(),TEXT("SoulHumanDefenseProof")) || FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaDefenseProof"));
    const bool Alpha=FParse::Param(FCommandLine::Get(),TEXT("SoulAlphaQualification"));
    if(Alpha && bCampaignAutoResolve && !bDefenseControlChecked && BattleElapsed>=3)
    {
        bDefenseControlChecked=true;HandleBattleAction(TEXT("Hold"));HandleGamepadAction(TEXT("Charge"));
        const bool Safe=!bBattlePaused && !TacticalFormations.ContainsByPredicate([&](const auto& F){return F.ManualOverrideUntil>BattleElapsed;});
        UE_LOG(LogTemp,Display,TEXT("SOUL_ALPHA_AUTOBATTLE_CONTROL pass=%d player_commands_rejected=1"),Safe);
        if(!Safe){FinishProof(false,TEXT("AI-only battle accepted Human commands"));return;}
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Alpha_AI_Battle.png"),true,false);
    }
    // The isolated Heartland fixture demonstrates the ordinary player melee input.
    // Only movement/aim/input are automated: collision and RBCombat decide every hit.
    if((FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandBattleQualification"))||FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandCompanyBattle"))) && bDefenseControlChecked
        && !bFinished && PlayerHero && PlayerHealth()>0 && BattleElapsed>6)
    {
        if(bTacticalCameraActive)ToggleBattleCamera();
        int32 Nearest=INDEX_NONE;double Distance=MAX_dbl;
        for(int32 I=0;I<Actors.Num();++I)if(Actors[I]&&Combatants[I].Side!=ControlledSide&&Combatants[I].Health>0)
        {const double D=FVector::DistSquared2D(PlayerHero->GetActorLocation(),Actors[I]->GetActorLocation());if(D<Distance){Distance=D;Nearest=I;}}
        if(Nearest!=INDEX_NONE)
        {
            FVector Goal=Actors[Nearest]->GetActorLocation(),Via;
            if(IsHeartlandBridgeBattle()&&SoulHeartlandBridge::Approach(PlayerHero->GetActorLocation()-ArenaOrigin,Goal-ArenaOrigin,Via))Goal=ArenaOrigin+Via;
            const FVector Direction=(Goal-PlayerHero->GetActorLocation()).GetSafeNormal2D();
            auto* PC=GetWorld()->GetFirstPlayerController();PC->SetControlRotation(Direction.Rotation());PlayerHero->SetActorRotation(Direction.Rotation());
            if(Distance>180*180)PlayerHero->AddMovementInput(Direction,1);
            static TWeakObjectPtr<ASoulRealtimeArenaGameMode> LastHost;static float LastAttack=0;
            if(LastHost.Get()!=this){LastHost=this;LastAttack=0;}
            if(Distance<320*320&&BattleElapsed-LastAttack>1.6f)
            {LastAttack=BattleElapsed;PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1));PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0));}
        }
    }
    if(Alpha && IsHeartlandBridgeBattle() && !bFinished)
    {
        static TWeakObjectPtr<ASoulRealtimeArenaGameMode> LastBridge;
        static float LastDiagnostic=0;
        if(LastBridge.Get()!=this){LastBridge=this;LastDiagnostic=0;}
        if(BattleElapsed-LastDiagnostic>=30)
        {
            LastDiagnostic=BattleElapsed;
            for(int32 I=0;I<Actors.Num();++I)if(Actors[I]&&Combatants[I].Health>0)
            {
                FRBCombatGroup Drive;ReadDriveGroup(Combatants[I].GroupIndex,Drive);
                UE_LOG(LogTemp,Display,TEXT("SOUL_BRIDGE_PHYSICAL side=%d unit=%d hp=%.1f local=%s anchor=%s"),
                    Combatants[I].Side,I,Combatants[I].Health,*(Actors[I]->GetActorLocation()-ArenaOrigin).ToCompactString(),*(Drive.Anchor-ArenaOrigin).ToCompactString());
            }
        }
    }
    if((!Defense&&!Alpha) || bDefenseControlChecked || bAutobattle) return;
    if(bBattlePaused) ToggleBattlePause();
    if(BattleElapsed<3) return;
    bDefenseControlChecked=true;
    auto* PC=GetWorld()->GetFirstPlayerController();
    const auto* Binding=PlayerHero?PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>():nullptr;
    const int32 HeroIndex=Binding?Index(FRBHostIdentity::From(Binding->GetCombatant())):INDEX_NONE;
    bool Valid=(!Defense || ControlledSide==1) && !bAutobattle && PC && PC->GetPawn()==PlayerHero && HeroIndex!=INDEX_NONE;
    Valid=Valid && Combatants[HeroIndex].Side==ControlledSide && Combatants[HeroIndex].bPlayerHero
        && CampaignFactionForSide(ControlledSide)==TEXT("humans") && CampaignUnitForSide(ControlledSide)==TEXT("human_knight")
        && GetNameSafe(PlayerHero->GetMesh()->GetSkeletalMeshAsset())==TEXT("Aurora") && (!Defense || PlayerMana==80);
    HandleGamepadAction(TEXT("All"));
    HandleBattleAction(TEXT("Hold"));
    int32 ManualHuman=0,ManualEnemy=0;
    for(const auto& F:TacticalFormations) if(F.ManualOverrideUntil>BattleElapsed) (F.Side==ControlledSide?ManualHuman:ManualEnemy)++;
    Valid=Valid && ManualHuman>0 && ManualEnemy==0;
    SelectAlliedFormationSlot(0);
    const int32 State=FindFormationState(SelectedAlliedFormation);
    Valid=Valid && State!=INDEX_NONE && TacticalFormations[State].Side==ControlledSide;
    // Tidal Ward is a normal self-targeted player cast; RBMagic validates and charges mana.
    const bool Cast=CastPlayerSpellSlot(3);
    Valid=Valid && (!Defense || (Cast && MagicCasts==1 && PlayerMana<80));
    HandleGamepadAction(TEXT("All"));HandleBattleAction(TEXT("Charge"));
    UE_LOG(LogTemp,Display,TEXT("SOUL_HUMAN_DEFENSE_CONTROL: pass=%d side=%d hero=%s possessed=%d humanManual=%d enemyManual=%d mana=%.0f casts=%d"),
        Valid,ControlledSide,PlayerHero?*GetNameSafe(PlayerHero->GetMesh()->GetSkeletalMeshAsset()):TEXT("None"),PC&&PC->GetPawn()==PlayerHero,ManualHuman,ManualEnemy,PlayerMana,MagicCasts);
    if(!Valid){FinishProof(false,TEXT("Human defensive control gate"));return;}
    ToggleBattleCamera();
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Human_Defense_Control.png"),true,false);
}
