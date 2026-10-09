#include "SoulRealtimeBattleArena.h"
#include "RBMagicSpellDefinition.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Engine/Texture2D.h"

namespace
{
const TCHAR* BattleSpellName(int32 Slot)
{
    static const TCHAR* Names[]={TEXT("Firebolt"),TEXT("ChainLightning"),TEXT("Blizzard"),TEXT("TidalWard"),TEXT("Tailwind")};
    return Slot>=0 && Slot<5 ? Names[Slot] : nullptr;
}
}
void ASoulRealtimeArenaGameMode::PushBattleNotice(const FString& Text,int32 Side)
{
    if(Text.IsEmpty()) return;
    BattleNotices.RemoveAll([&](const FSoulBattleNotice& Notice)
        { return Notice.ExpiresAt<=BattleElapsed || (Notice.Text==Text && Notice.Side==Side); });
    FSoulBattleNotice Notice;
    Notice.Text=Text;Notice.Side=Side;Notice.ExpiresAt=BattleElapsed+12.f;
    BattleNotices.Insert(MoveTemp(Notice),0);
    if(BattleNotices.Num()>3) BattleNotices.SetNum(3);
}
void ASoulRealtimeArenaGameMode::SetupSpellBar()
{
    BattleSpells.Reset();
    // Licensed donor references only; preload once, never load from DrawHUD.
    SpellIcons.Reset();
    const TCHAR* IconPaths[]={
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_12"),
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_02"),
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_16"),
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_24"),
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_20"),
        TEXT("/Game/Spell_Mix/frame/Textures/T_spells_mix_frame_36")};
    for(const TCHAR* Path:IconPaths) SpellIcons.Add(LoadObject<UTexture2D>(nullptr,Path));
    // Load presentation before deployment rather than blocking the first cast
    // while its short effect expires during shader/asset preparation.
    BattlePresentationAssets.Reset();
    const TCHAR* Particles[]={
        TEXT("/Game/ParagonIggyScorch/FX/Particles/IggyScorch/Abilities/Turret/FX/P_IggyScorch_Turret_Flamethrower"),
        TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Primary/FX/P_Aurora_Melee_SucessfulImpact"),
        TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Whrilwind"),
        TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Rooted"),
        TEXT("/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_JumpPad_Swirl")};
    for(const TCHAR* Path:Particles)
        if(auto* Asset=LoadObject<UParticleSystem>(nullptr,Path)) BattlePresentationAssets.Add(Asset);
    if(auto* Asset=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/MagicSpells/Fire/FX/NS_Fireball.NS_Fireball")))
        BattlePresentationAssets.Add(Asset);
    for(int32 I=0;I<5;++I)
    {
        const FString Name=BattleSpellName(I);
        BattleSpells.Add(LoadObject<URBMagicSpellDefinition>(nullptr,
            *FString::Printf(TEXT("/Game/Soul/Magic/Spells/DA_Soul_%s.DA_Soul_%s"),*Name,*Name)));
    }
}
float ASoulRealtimeArenaGameMode::SpellManaCost(const URBMagicSpellDefinition& Spell)
{
    float Cost=0;
    for(const auto& Resource:Spell.Costs)
        if(Resource.ResourceTag.ToString()==TEXT("Magic.Resource.Mana")) Cost+=Resource.Amount;
    return Cost;
}
FString ASoulRealtimeArenaGameMode::SpellBlockReason(int32 Slot,bool bAllowPausedTargeting) const
{
    const auto* Spell=BattleSpells.IsValidIndex(Slot) ? BattleSpells[Slot].Get() : nullptr;
    if(!Spell || Spell->bStrategicOnly) return TEXT("Spell unavailable");
    if(bFinished) return TEXT("Battle resolved");
    if(PlayerHealth()<=0) return TEXT("Hero fallen");
    const float Cooldown=SpellCooldowns.FindRef(Spell->SpellTag.GetTagName());
    if(Cooldown>0) return FString::Printf(TEXT("Ready in %.0fs"),FMath::CeilToFloat(Cooldown));
    const float Cost=SpellManaCost(*Spell);
    if(PlayerMana+KINDA_SMALL_NUMBER<Cost)
        return FString::Printf(TEXT("Need %.0f more mana"),FMath::CeilToFloat(Cost-PlayerMana));
    if(bBattlePaused && !bAllowPausedTargeting) return TEXT("Resume to cast");
    return FString();
}
FString ASoulRealtimeArenaGameMode::SpellButtonLabel(int32 Slot) const
{
    const auto* Spell=BattleSpells.IsValidIndex(Slot) ? BattleSpells[Slot].Get() : nullptr;
    if(!Spell) return TEXT("Unavailable | Spell unavailable");
    const FString Reason=SpellBlockReason(Slot);
    return FString::Printf(TEXT("[%d] %s | %s"),Slot+1,*Spell->DisplayName.ToString(),
        Reason.IsEmpty() ? *FString::Printf(TEXT("%.0f mana | Ready"),SpellManaCost(*Spell)) : *Reason);
}
bool ASoulRealtimeArenaGameMode::CastPlayerSpellSlot(int32 Slot,const FHitResult* AimHit)
{
    const TCHAR* Name=BattleSpellName(Slot);
    if(!Name || bFinished) return false;
    return CastPlayerSpell(*FString::Printf(TEXT("/Game/Soul/Magic/Spells/DA_Soul_%s.DA_Soul_%s"),Name,Name),
        *FString::Printf(TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_%s.DA_SoulPresentation_%s"),Name,Name),AimHit);
}
void ASoulRealtimeArenaGameMode::SelectPlayerSpell(int32 Slot)
{
    if(bFinished) return;
    // Explicitly cancel an old target before choosing another spell. A rejected
    // new choice must never leave a different spell armed under the same pointer.
    SelectedSpellSlot=INDEX_NONE;
    bPlaceFormationOrder=false;
    const FString Reason=SpellBlockReason(Slot,true);
    if(!Reason.IsEmpty()) { Status=Reason; return; }
    bSpellbookOpen=false;
    SelectedSpellSlot=Slot;
    Status=Slot==2 ? TEXT("Blizzard selected: click ground within hero range. RMB cancels.") :
        TEXT("Spell selected: click an enemy within hero range. RMB cancels.");
    if(Slot>=3) Status=Slot==3 ? TEXT("Tidal ward ready: click battlefield to shield your hero. RMB cancels.") :
        TEXT("Tailwind ready: click battlefield to hasten your army. RMB cancels.");
    if(bBattlePaused) Status=TEXT("Spell readied. Resume [P], then click to cast. No mana spent.");
}
void ASoulRealtimeArenaGameMode::HandleBattleAction(FName Action)
{
    if(bCampaignAutoResolve)return; // The Human observer cannot command either AI army.
    const FString Name=Action.ToString();
    UE_LOG(LogTemp,Display,TEXT("SOUL_BATTLE_UI: action=%s"),*Name);
    if(Action==TEXT("CloseGrimoire")) { bSpellbookOpen=false; return; }
    if(Action==TEXT("Grimoire"))
    {
        bSpellbookOpen=!bSpellbookOpen;
        if(bSpellbookOpen) { SelectedSpellSlot=INDEX_NONE; bPlaceFormationOrder=false; }
        return; // Opening or closing a book never changes simulation time.
    }
    if(Action==TEXT("PauseRule"))
    {
        if(bBattlePaused && BattleElapsed<=0.f)
            bAllowTacticalPause=!bAllowTacticalPause;
        else Status=TEXT("Choose tactical pause during deployment, before starting battle.");
        return;
    }
    if(Action==TEXT("Pause")) { ToggleBattlePause(); return; }
    if(Action==TEXT("Camera")) { ToggleBattleCamera(); return; }
    if(Action==TEXT("View")) { ToggleFirstPersonCamera(); return; }
    if(Action==TEXT("Focus"))
    {
        if(!bTacticalCameraActive) ToggleBattleCamera();
        if(bSelectAllAllies) SetupBattleCamera();
        else if(AliveInGroup(SelectedAlliedFormation)>0)
        {
            TacticalFocus=GroupCenter(SelectedAlliedFormation);
            TacticalDistance=2600;
            TacticalRotation.Pitch=-32;
        }
        return;
    }
    if(Action==TEXT("Help")) { bShowBattleHelp=!bShowBattleHelp; return; }
    if(bFinished) return;
    if(Name.StartsWith(TEXT("Unit")))
    {
        const int32 I=FCString::Atoi(*Name.Mid(4));
        if(!Combatants.IsValidIndex(I) || Combatants[I].Health<=0 ||
            !Actors.IsValidIndex(I) || !IsValid(Actors[I])) return;
        if(SelectedSpellSlot>=0)
        {
            FHitResult Aim(Actors[I],Actors[I]->GetCapsuleComponent(),Actors[I]->GetActorLocation(),FVector::UpVector);
            Aim.bBlockingHit=true;
            if(CastPlayerSpellSlot(SelectedSpellSlot,&Aim)) SelectedSpellSlot=INDEX_NONE;
            return;
        }
        if(bPlaceFormationOrder) { Status=TEXT("Choose clear ground for the formation waypoint."); return; }
        if(Combatants[I].Side==ControlledSide)
        {
            bSelectAllAllies=false;
            SelectedAlliedFormation=Combatants[I].GroupIndex;
            Status=TEXT("Formation selected. Choose Move, Hold, Advance, Charge or Fall back.");
        }
        else Status=FString::Printf(TEXT("Enemy %s | health %.0f / %.0f"),*RoleLabel(Combatants[I].Role),
            Combatants[I].Health,Combatants[I].MaxHealth);
        return;
    }
    if(Name.StartsWith(TEXT("Formation"))) { SelectAlliedFormationSlot(FCString::Atoi(*Name.Mid(9))); return; }
    if(Name.StartsWith(TEXT("BookSpell"))) { SelectPlayerSpell(FCString::Atoi(*Name.Mid(9))); return; }
    if(Name.StartsWith(TEXT("Spell"))) { SelectPlayerSpell(FCString::Atoi(*Name.Mid(5))); return; }
    if(Action==TEXT("Move")) { bPlaceFormationOrder=true; SelectedSpellSlot=INDEX_NONE; Status=TEXT("Click clear ground to move selected formations. RMB cancels."); return; }
    if(Action==TEXT("All")) { bSelectAllAllies=true; SelectedAlliedFormation=INDEX_NONE; Status=TEXT("All allied formations selected"); return; }
    if(Action==TEXT("AI")) { ReturnSelectedAlliesToAI(); return; }
    if(Action==TEXT("Hold")) CommandSelectedAllies(ERBHostGroupOrder::Hold);
    if(Action==TEXT("Advance")) CommandSelectedAllies(ERBHostGroupOrder::Advance);
    if(Action==TEXT("Charge")) CommandSelectedAllies(ERBHostGroupOrder::Charge);
    if(Action==TEXT("Fallback")) CommandSelectedAllies(ERBHostGroupOrder::FallBack);
    if(Action==TEXT("Face")) CommandSelectedAllies(ERBHostGroupOrder::Face);
}
bool ASoulRealtimeArenaGameMode::ReadPointerHit(FHitResult& Hit) const
{
    auto* PC=GetWorld()->GetFirstPlayerController();
    if(!PC) return false;
    FVector Origin,Direction;
    if(!bGamepadActive && (bTacticalCameraActive || PC->bShowMouseCursor))
    {
        if(!PC->DeprojectMousePositionToWorld(Origin,Direction)) return false;
    }
    else
    {
        FRotator Rotation;
        PC->GetPlayerViewPoint(Origin,Rotation);
        Direction=Rotation.Vector();
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulBattlePointer),true,PlayerHero);
    return GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*50000,ECC_Visibility,Query);
}
void ASoulRealtimeArenaGameMode::MoveSelectedToPointer()
{
    FHitResult Hit;
    if(bFinished) return;
    if(!ReadPointerHit(Hit)) { Status=TEXT("Choose visible ground inside the battlefield"); return; }
    if(FMath::Abs(Hit.ImpactPoint.X-ArenaOrigin.X)>BattlefieldHalfX ||
        FMath::Abs(Hit.ImpactPoint.Y-ArenaOrigin.Y)>BattlefieldHalfY)
    { Status=TEXT("Choose a position inside the battlefield"); return; }
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Projected;
    if(Hit.ImpactNormal.Z<0.7f) { Status=TEXT("Choose walkable ground, away from steep walls"); return; }
    if(Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
    {
        if(!Nav->ProjectPointToNavigation(Hit.ImpactPoint,Projected,FVector(160,160,400)))
        { Status=TEXT("That position is not traversable"); return; }
    }
    else Projected.Location=Hit.ImpactPoint;
    int32 Changed=0;
    FVector SelectionCenter=FVector::ZeroVector;
    int32 SelectedCount=0;
    for(const auto& State:TacticalFormations)
        if(State.Side==ControlledSide && AliveInGroup(State.GroupIndex)>0 &&
            (bSelectAllAllies || State.GroupIndex==SelectedAlliedFormation))
        { SelectionCenter+=GroupCenter(State.GroupIndex); ++SelectedCount; }
    if(SelectedCount==0) return;
    SelectionCenter/=SelectedCount;
    for(const auto& State:TacticalFormations)
    {
        if(State.Side!=ControlledSide || State.bRouting || AliveInGroup(State.GroupIndex)<=0 ||
            (!bSelectAllAllies && State.GroupIndex!=SelectedAlliedFormation)) continue;
        const FVector Center=GroupCenter(State.GroupIndex);
        const FVector Anchor=Projected.Location+(bSelectAllAllies ? Center-SelectionCenter : FVector::ZeroVector);
        const bool Clear=IsDirectGroundRouteClear(GetWorld(),Center,Anchor);
        if(Clear && IssueFormationOrder(State.GroupIndex,ERBHostGroupOrder::Advance,
            Anchor,(Anchor-Center).GetSafeNormal2D(),true)) ++Changed;
    }
    bPlaceFormationOrder=Changed==0;
    Status=Changed ? FString::Printf(TEXT("Move ordered: %d formation(s). [R] returns control to AI."),Changed) :
        TEXT("No clear ground route. Choose a closer waypoint around the obstruction.");
    UE_LOG(LogTemp,Display,TEXT("SOUL_PLAYER_MOVE_ORDER: groups=%d anchor=%s"),Changed,*Projected.Location.ToCompactString());
}



// The authored showcase has no navigation data. Reuse a bounded physical
// corridor check for player waypoints and commander maneuvers. This validates
// the direct route only; it is deliberately not another pathfinding system.
bool ASoulRealtimeArenaGameMode::IsDirectGroundRouteClear(
    UWorld* World,const FVector& From,const FVector& To,float Radius)
{
    if(!World || From.ContainsNaN() || To.ContainsNaN() || !FMath::IsFinite(Radius) || Radius<=0 ||
        FVector::Dist2D(From,To)>10000) return false;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulGroundCorridor),false);
    FVector Previous=FVector::ZeroVector;
    const int32 Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(From,To)/150.0));
    for(int32 Step=0;Step<=Steps;++Step)
    {
        const FVector Point=FMath::Lerp(From,To,float(Step)/Steps);
        FHitResult Ground;
        if(!World->LineTraceSingleByObjectType(Ground,Point+FVector(0,0,500),
            Point-FVector(0,0,700),Objects,Query) || Ground.ImpactNormal.Z<0.7f)
            return false;
        const FVector At=Ground.ImpactPoint+FVector(0,0,Radius+45);
        FHitResult Obstacle;
        if(Step>0 && (FMath::Abs(At.Z-Previous.Z)>110 ||
            World->SweepSingleByObjectType(Obstacle,Previous,At,FQuat::Identity,
                Objects,FCollisionShape::MakeSphere(Radius),Query)))
            return false;
        Previous=At;
    }
    return true;
}

void ASoulRealtimeArenaGameMode::HandleGamepadAction(FName Action)
{
    if(bCampaignAutoResolve)return;
    bGamepadActive=true;
    if(Action==TEXT("Cancel"))
    { bSpellbookOpen=false; SelectedSpellSlot=INDEX_NONE; bPlaceFormationOrder=false; Status=TEXT("Targeting cancelled"); return; }
    if(bFinished && Action!=TEXT("Camera") && Action!=TEXT("View") && Action!=TEXT("Focus")) return;
    if(Action==TEXT("NextFormation")) { SelectNextAlliedFormation(); return; }
    if(Action==TEXT("NextSpell"))
    {
        if(PlayerHealth()<=0) { Status=TEXT("Hero fallen - spells unavailable"); return; }
        SelectedSpellSlot=(SelectedSpellSlot+1)%5; bPlaceFormationOrder=false;
        const FString Reason=SpellBlockReason(SelectedSpellSlot);
        Status=Reason.IsEmpty()
            ? (SelectedSpellSlot==3 ? TEXT("[Y] shields your hero. [B] cancels.") : SelectedSpellSlot==4 ? TEXT("[Y] hastens your army. [B] cancels.") :
                TEXT("Aim with the right stick, then [Y] to cast. [B] cancels."))
            : Reason+TEXT(". [X] changes spell; [B] cancels.");
        return;
    }
    if(Action==TEXT("Cast"))
    {
        if(SelectedSpellSlot<0) { Status=TEXT("[X] selects a spell; aim, then [Y] casts."); return; }
        const FString Reason=SpellBlockReason(SelectedSpellSlot);
        if(!Reason.IsEmpty()) { Status=Reason; return; }
        FHitResult Hit;
        if(SelectedSpellSlot<3 && !ReadPointerHit(Hit))
        { Status=TEXT("Aim at a visible target or ground, then [Y] to cast."); return; }
        if(CastPlayerSpellSlot(SelectedSpellSlot,SelectedSpellSlot>=3 ? nullptr : &Hit))
            SelectedSpellSlot=INDEX_NONE;
        return;
    }
    if(Action==TEXT("GroundOrder"))
    {
        if(!bTacticalCameraActive) { Status=TEXT("[View] opens commander camera for ground orders."); return; }
        SelectedSpellSlot=INDEX_NONE; MoveSelectedToPointer(); return;
    }
    HandleBattleAction(Action);
}
