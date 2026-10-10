#include "SoulHeartlandBridgeGeometry.h"
#include "SoulRealtimeBattleArena.h"
#include "SoulBattleControlRules.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
bool ASoulRealtimeArenaGameMode::IsBattleHero(int32 I) const
{return Combatants.IsValidIndex(I)&&(Combatants[I].bPlayerHero||Combatants[I].bNonPlayerHero);}
bool ASoulRealtimeArenaGameMode::SeparateBattleHeroesFromFormations()
{
    // Build and validate the complete replacement before mutating membership.
    // Singleton hero groups keep the existing active-body accounting intact,
    // but are deliberately absent from TacticalFormations and its troop cards.
    TArray<FRBHostGroup> Candidate = Groups;
    TMap<int32, int32> Assignments;
    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        if (!IsBattleHero(I)) continue;
        const int32 OldGroup = Combatants[I].GroupIndex;
        FRBCombatGroup Troops;
        if (!Candidate.IsValidIndex(OldGroup) || !Candidate[OldGroup].ToCore(Troops) ||
            !Actors.IsValidIndex(I) || !IsValid(Actors[I])) return false;
        const FRBCombatantRef Hero = IdentityAt(I).Core();
        if (Troops.Members.Remove(Hero) != 1) return false;
        int32 HeroGroup = OldGroup;
        if (!Troops.Members.IsEmpty())
        {
            if (Troops.Leader == Hero) Troops.Leader = Troops.Members[0];
            ++Troops.Revision;
            if (!FRBHostGroup::FromCore(Troops, Candidate[OldGroup])) return false;
            HeroGroup = Candidate.AddDefaulted();
        }
        FRBCombatGroup Commander;
        Commander.Id = FGuid::NewGuid();
        Commander.Leader = Hero;
        Commander.Members.Add(Hero);
        Commander.Anchor = Actors[I]->GetActorLocation();
        Commander.Facing = Actors[I]->GetActorForwardVector().GetSafeNormal2D();
        Commander.Command = Combatants[I].bPlayerHero ? ERBGroupCommand::Hold : ERBGroupCommand::Charge;
        if (!FRBHostGroup::FromCore(Commander, Candidate[HeroGroup])) return false;
        Assignments.Add(I, HeroGroup);
    }
    Groups = MoveTemp(Candidate);
    for (const auto& Assignment : Assignments) Combatants[Assignment.Key].GroupIndex = Assignment.Value;
    TacticalFormations.RemoveAll([this](const FSoulBattleFormationState& State)
    {
        const auto& Members = Groups[State.GroupIndex].Members;
        return Members.Num() == 1 && IsBattleHero(Index(Members[0]));
    });
    for (auto& State : TacticalFormations)
    {
        State.InitialBodies = Groups[State.GroupIndex].Members.Num();
        State.PreviousAlive = State.InitialBodies;
    }
    UE_LOG(LogTemp, Display, TEXT("SOUL_HERO_MEMBERSHIP: heroes=%d troopFormations=%d groups=%d"),
        Assignments.Num(), TacticalFormations.Num(), Groups.Num());
    return true;
}

bool ASoulRealtimeArenaGameMode::ReadDriveGroup(int32 GroupIndex, FRBCombatGroup& Out) const
{
    if (!Groups.IsValidIndex(GroupIndex) || !Groups[GroupIndex].ToCore(Out)) return false;
    // Never lend the possessed hero (or a temporarily unpossessed player hero)
    // to a group driver. Enemy commanders retain the same normal RB driver.
    for (const auto& Member : Out.Members)
    {
        const int32 I = Index(FRBHostIdentity::From(Member));
        if (Combatants.IsValidIndex(I) && Combatants[I].bPlayerHero) return false;
    }
    if (Out.Command == ERBGroupCommand::Follow)
    {
        const int32 State = FindFormationState(GroupIndex);
        if (!TacticalFormations.IsValidIndex(State)) return false;
        // RB's native Follow follows its own member-leader, not an external
        // commander. Keep the requested order and revision; feed RB the latest
        // validated commander-relative anchor as an ordinary movement snapshot.
        Out.Command = ERBGroupCommand::Advance;
        Out.Anchor = TacticalFormations[State].TacticalAnchor;
    }
    if(IsHeartlandBridgeBattle()&&(Out.Command==ERBGroupCommand::Advance||Out.Command==ERBGroupCommand::Charge))
    {
        FVector Waypoint;
        if(SoulHeartlandBridge::Approach(GroupCenter(GroupIndex)-ArenaOrigin,Out.Anchor-ArenaOrigin,Waypoint))
        {Out.Command=ERBGroupCommand::Advance;Out.Anchor=ArenaOrigin+Waypoint;Out.Facing=(Out.Anchor-GroupCenter(GroupIndex)).GetSafeNormal2D();}
    }
    if(bSiege&&!SiegeDriveGroup(GroupIndex,Out))return false;
    return Out.IsValid();
}

bool USoulRealtimeArenaGroupDriver::ReadGroup(FRBCombatGroup& Out) const
{
    const auto* Host = GetWorld() ? GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>() : nullptr;
    return Host && Host->ReadDriveGroup(GroupIndex, Out);
}

bool ASoulRealtimeArenaGameMode::CanDriverEngage(int32 GroupIndex,
    FRBHostIdentity Person, FRBHostIdentity Other) const
{
    if (!AreOpponents(Person, Other)) return false;
    const int32 I = Index(Person);
    if (!Actors.IsValidIndex(I) || !IsValid(Actors[I]) || Combatants[I].bPlayerHero) return false;
    FRBCombatGroup DriveGroup;
    if (!ReadDriveGroup(GroupIndex, DriveGroup)) return false;
    if(IsHeartlandBridgeBattle()&&!Combatants[I].bRanged)
    {
        FVector Waypoint;const int32 Target=Index(Other);
        if(Actors.IsValidIndex(Target)&&Actors[Target]&&
            SoulHeartlandBridge::Approach(Actors[I]->GetActorLocation()-ArenaOrigin,Actors[Target]->GetActorLocation()-ArenaOrigin,Waypoint)
            && FVector::Dist2D(Actors[I]->GetActorLocation(),Actors[Target]->GetActorLocation())>280)return false;
    }
    if(bSiege && SiegeState.GateIntegrityPermille>0)return false;
    const int32 S = FindFormationState(GroupIndex);
    const bool Manual = TacticalFormations.IsValidIndex(S) &&
        TacticalFormations[S].ManualOverrideUntil > BattleElapsed && !TacticalFormations[S].bRouting;
    // A manually charged company must still fight a contact reached on the deck,
    // even while its movement snapshot is temporarily an entrance waypoint.
    if((IsHeartlandBridgeBattle() || (bSiege&&SiegeState.GateIntegrityPermille==0)) && Manual && TacticalFormations[S].ManualOrder==ERBHostGroupOrder::Charge)
    {
        const int32 Target=Index(Other);
        if(Actors.IsValidIndex(Target)&&Actors[Target]&&FVector::Dist2D(Actors[I]->GetActorLocation(),Actors[Target]->GetActorLocation())<=280)return true;
    }
    const float Spacing = Drivers.IsValidIndex(GroupIndex) && Drivers[GroupIndex]
        ? Drivers[GroupIndex]->FormationSpacing : 135.f;
    return FSoulBattleControlRules::AllowsEngagement(DriveGroup, Person.Core(),
        Actors[I]->GetActorLocation(), Spacing, Manual, 35.f + Actors[I]->GetCapsuleComponent()->GetScaledCapsuleRadius());
}

void ASoulRealtimeArenaGameMode::RefreshManualOrders()
{
    for (auto& State : TacticalFormations)
    {
        if (State.ManualOverrideUntil <= BattleElapsed || State.bRouting ||
            AliveInGroup(State.GroupIndex) <= 0 || !Groups.IsValidIndex(State.GroupIndex)) continue;
        FVector Anchor = State.ManualAnchor;
        FVector Facing = State.ManualFacing;
        if (State.ManualOrder == ERBHostGroupOrder::Follow)
        {
            if (!IsValid(PlayerHero) || PlayerHealth() <= 0)
            {
                State.ManualOrder = ERBHostGroupOrder::Hold;
                State.ManualAnchor = GroupCenter(State.GroupIndex);
                Anchor = State.ManualAnchor;
                PushBattleNotice(TEXT("Commander downed: followers HOLD position"), State.Side);
            }
            else
            {
                const FVector Wanted = PlayerHero->GetActorLocation() + State.FollowOffset;
                if (FMath::Abs(Wanted.X-ArenaOrigin.X) <= BattlefieldHalfX &&
                    FMath::Abs(Wanted.Y-ArenaOrigin.Y) <= BattlefieldHalfY &&
                    IsDirectGroundRouteClear(GetWorld(), GroupCenter(State.GroupIndex), Wanted))
                {
                    Anchor = Wanted;
                    State.ManualAnchor = Anchor;
                }
                // Blocked routes retain the last accepted safe anchor, not a
                // direct-steering shortcut through a wall or over a cliff.
            }
        }
        const auto& Current = Groups[State.GroupIndex];
        if (Current.Order != State.ManualOrder || !Current.Anchor.Equals(Anchor, 20.f) ||
            !Current.Facing.Equals(Facing, .01f))
            IssueFormationOrder(State.GroupIndex, State.ManualOrder, Anchor, Facing, false);
    }
}

bool ASoulRealtimeArenaGameMode::ResetFormationMotion(int32 GroupIndex)
{
    if (!Drivers.IsValidIndex(GroupIndex) || !Drivers[GroupIndex]) return false;
    TArray<URBVariantCombatBindingComponent*> Reps;
    for (int32 I = 0; I < Bindings.Num(); ++I)
        if (IsValid(Bindings[I]) && Combatants.IsValidIndex(I) && Combatants[I].Health > 0)
            Reps.Add(Bindings[I]);
    if (!Drivers[GroupIndex]->SetRepresentations(Reps)) return false;
    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        auto& Unit = Combatants[I];
        if (Unit.GroupIndex != GroupIndex || IsBattleHero(I)) continue;
        Unit.PendingMeleeSeconds = 0.f;
        Unit.PendingMeleeTarget = FRBHostIdentity();
        if (Actors.IsValidIndex(I) && Actors[I]) Actors[I]->GetCharacterMovement()->StopMovementImmediately();
    }
    return true;
}
