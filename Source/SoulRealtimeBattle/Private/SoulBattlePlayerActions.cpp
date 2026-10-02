#include "SoulRealtimeBattleArena.h"
#include "RBMagicSpellDefinition.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

namespace
{
const TCHAR* BattleSpellName(int32 Slot)
{
    static const TCHAR* Names[]={TEXT("Firebolt"),TEXT("ChainLightning"),TEXT("Blizzard"),TEXT("TidalWard"),TEXT("Tailwind")};
    return Slot>=0 && Slot<5 ? Names[Slot] : nullptr;
}
}
void ASoulRealtimeArenaGameMode::SetupSpellBar()
{
    BattleSpells.Reset();
    // Load presentation before deployment rather than blocking the first cast
    // while its short effect expires during shader/asset preparation.
    BattlePresentationAssets.Reset();
    const TCHAR* Particles[]={
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
FString ASoulRealtimeArenaGameMode::SpellButtonLabel(int32 Slot) const
{
    const auto* Spell=BattleSpells.IsValidIndex(Slot) ? BattleSpells[Slot].Get() : nullptr;
    if(!Spell) return TEXT("Spell unavailable");
    float Cost=0;
    for(const auto& Resource:Spell->Costs)
        if(Resource.ResourceTag.ToString()==TEXT("Magic.Resource.Mana")) Cost+=Resource.Amount;
    const float Cooldown=SpellCooldowns.FindRef(Spell->SpellTag.GetTagName());
    return FString::Printf(TEXT("[%d] %s | %s"),Slot+1,*Spell->DisplayName.ToString(),
        Cooldown>0 ? *FString::Printf(TEXT("%.0fs"),FMath::CeilToFloat(Cooldown)) :
        *FString::Printf(TEXT("%.0f mana"),Cost));
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
    if(PlayerHealth()<=0) { Status=TEXT("Hero fallen - spells are unavailable. You can still command surviving formations."); return; }
    bPlaceFormationOrder=false;
    if(Slot>=3)
    {
        SelectedSpellSlot=INDEX_NONE;
        CastPlayerSpellSlot(Slot);
        return;
    }
    SelectedSpellSlot=Slot;
    Status=Slot==2 ? TEXT("Blizzard ready: click ground within hero range. RMB cancels.") :
        TEXT("Spell ready: click an enemy within hero range. RMB cancels.");
}
void ASoulRealtimeArenaGameMode::HandleBattleAction(FName Action)
{
    const FString Name=Action.ToString();
    UE_LOG(LogTemp,Display,TEXT("SOUL_BATTLE_UI: action=%s"),*Name);
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
        if(Combatants[I].Side==0)
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
    if(bTacticalCameraActive || PC->bShowMouseCursor)
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
        if(State.Side==0 && AliveInGroup(State.GroupIndex)>0 &&
            (bSelectAllAllies || State.GroupIndex==SelectedAlliedFormation))
        { SelectionCenter+=GroupCenter(State.GroupIndex); ++SelectedCount; }
    if(SelectedCount==0) return;
    SelectionCenter/=SelectedCount;
    for(const auto& State:TacticalFormations)
    {
        if(State.Side!=0 || State.bRouting || AliveInGroup(State.GroupIndex)<=0 ||
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
