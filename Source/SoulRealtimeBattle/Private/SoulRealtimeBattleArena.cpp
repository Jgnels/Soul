#include "SoulRealtimeBattleArena.h"
#include "SoulRealtimeBattlePBIL.h"
#include "SoulCampaignBattleBridge.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "RBCombatRangedComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "RBMagicLibrary.h"
#include "RBMagicPresentationProfile.h"
#include "RBMagicSpellDefinition.h"

namespace
{
    const FName ArenaDomain(TEXT("Soul.Battle"));
    const FName MagicArenaDomain(TEXT("Soul.Arena"));

    FName RealtimeSideId(int32 Side)
    {
        return Side == 0 ? FName(TEXT("Human")) : FName(TEXT("Enemy"));
    }

    FName GroupFormationId(int32 Side, int32 GroupIndex)
    {
        return FName(*FString::Printf(
            TEXT("Realtime.Side%d.Group%d"), Side, GroupIndex));
    }

    FName ReserveFormationId(int32 Side)
    {
        return FName(*FString::Printf(
            TEXT("Realtime.Side%d.Reserve.Line"), Side));
    }
    constexpr float BattlefieldHalfX = 2600.0f;
    constexpr float BattlefieldHalfY = 2200.0f;
    constexpr float BattlefieldWallThickness = 100.0f;
    constexpr float BattlefieldWallHalfHeight = 2400.0f;
    ASoulRealtimeArenaGameMode* ArenaHost(const UObject* Object)
    {
        return Object && Object->GetWorld()
            ? Cast<ASoulRealtimeArenaGameMode>(
                Object->GetWorld()->GetAuthGameMode())
            : nullptr;
    }

    float AttackDelay(ESoulRealtimeFormationRole Role)
    {
        switch (Role)
        {
            case ESoulRealtimeFormationRole::Hero: return 0.50f;
            case ESoulRealtimeFormationRole::Shock: return 0.62f;
            case ESoulRealtimeFormationRole::Apex: return 0.90f;
            default: return 0.72f;
        }
    }
}

bool USoulRealtimeArenaBinding::HostIdentityExists_Implementation(
    FRBHostIdentity Identity) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->Index(Identity) != INDEX_NONE;
}
bool USoulRealtimeArenaBinding::HostReadEquippedWeapon_Implementation(
    FRBHostIdentity Identity, FRBHostWeapon& Out) const
{
    const auto* Host = ArenaHost(this);
    const int32 I = Host ? Host->Index(Identity) : INDEX_NONE;
    if (I == INDEX_NONE) return false;
    Out = FRBHostWeapon::From(Host->Profile(I));
    return Out.Core().IsValid();
}

bool USoulRealtimeArenaBinding::HostCommitAcceptedHit_Implementation(
    const FRBHostHit& Hit, FString& Error)
{
    auto* Host = ArenaHost(this);
    if (!Host)
    {
        Error = TEXT("Soul arena authority unavailable");
        return false;
    }
    return Host->CommitHit(Hit, Error);
}
bool USoulRealtimeArenaBinding::HostSpendResources_Implementation(
    FRBHostIdentity Identity, FName WeaponId,
    FName Item, int32 Quantity, float Effort, FString& Error)
{
    auto* Host = ArenaHost(this);
    if (!Host)
    {
        Error = TEXT("Soul arena resource authority unavailable");
        return false;
    }
    return Host->SpendResources(
        Identity, WeaponId, Item, Quantity, Effort, Error);
}

bool USoulRealtimeArenaBinding::HostCanAct_Implementation(
    FRBHostIdentity Identity) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->CanAct(Identity);
}
bool USoulRealtimeArenaBinding::HostCanReceiveDamage_Implementation(
    FRBHostIdentity Identity) const
{
    return HostCanAct(Identity);
}

bool USoulRealtimeArenaBinding::HostHasEligibleGuardEquipment_Implementation(
    FRBHostIdentity Identity) const
{
    const auto* Host = ArenaHost(this);
    const int32 I = Host ? Host->Index(Identity) : INDEX_NONE;
    return I != INDEX_NONE && Host->Profile(I).Category != TEXT("Bow");
}

bool USoulRealtimeArenaGroupDriver::HostReadGroup_Implementation(
    FRBHostGroup& Out) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->ReadGroup(GroupIndex, Out);
}
bool USoulRealtimeArenaGroupDriver::HostReadParticipation_Implementation(
    FRBHostIdentity Identity, FRBHostParticipation& Out) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->ReadParticipation(Identity, Out);
}

bool USoulRealtimeArenaGroupDriver::HostAreOpponents_Implementation(
    FRBHostIdentity A, FRBHostIdentity B) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->AreOpponents(A, B);
}

bool USoulRealtimeArenaGroupDriver::HostHasAmmunition_Implementation(
    FRBHostIdentity Identity, FName Item) const
{
    const auto* Host = ArenaHost(this);
    return Host && Host->HasAmmunition(Identity, Item);
}
bool USoulRealtimeArenaGroupDriver::HostRequestMeleeFallback_Implementation(
    URBVariantCombatBindingComponent* Binding)
{
    auto* Host = ArenaHost(this);
    return Host && Host->RequestMeleeFallback(Binding);
}

void USoulRealtimeArenaGroupDriver::HostObserveDecision_Implementation(
    FRBHostIdentity Identity, const FRBHostDecision& Decision)
{
    auto* Host = ArenaHost(this);
    if (!Host) return;
    if (Decision.Target.Core().IsValid() &&
        !Host->AreOpponents(Identity, Decision.Target))
    {
        ++AlliedTargets;
    }
    Host->ObserveDecision(GroupIndex, Identity, Decision);
}
bool USoulRealtimeArenaGroupDriver::HostCommitGroupOrder_Implementation(
    const FRBHostGroup& Previous,
    const FRBHostGroup& Replacement,
    FString& Error)
{
    auto* Host = ArenaHost(this);
    if (!Host)
    {
        Error = TEXT("Soul arena group authority unavailable");
        return false;
    }
    return Host->CommitGroupOrder(
        GroupIndex, Previous, Replacement, Error);
}

ASoulRealtimeArenaGameMode::ASoulRealtimeArenaGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
    HUDClass = ASoulRealtimeArenaHUD::StaticClass();
}
int32 ASoulRealtimeArenaGameMode::Index(
    FRBHostIdentity Identity) const
{
    if (Identity.Domain != ArenaDomain || !Identity.Id.IsValid())
        return INDEX_NONE;
    return Combatants.IndexOfByPredicate(
        [&Identity](const FSoulRealtimeArenaCombatant& C)
        {
            return C.Id == Identity.Id;
        });
}

FRBHostIdentity ASoulRealtimeArenaGameMode::IdentityAt(
    int32 I) const
{
    FRBHostIdentity Out;
    if (!Combatants.IsValidIndex(I)) return Out;
    Out.Domain = ArenaDomain;
    Out.Id = Combatants[I].Id;
    return Out;
}
float ASoulRealtimeArenaGameMode::RoleDamage(
    ESoulRealtimeFormationRole Role)
{
    switch (Role)
    {
        case ESoulRealtimeFormationRole::Guard: return 12.0f;
        case ESoulRealtimeFormationRole::Breaker: return 16.0f;
        case ESoulRealtimeFormationRole::Shock: return 18.0f;
        case ESoulRealtimeFormationRole::Ranged: return 9.0f;
        case ESoulRealtimeFormationRole::Support: return 11.0f;
        case ESoulRealtimeFormationRole::Apex: return 27.0f;
        case ESoulRealtimeFormationRole::Hero: return 22.0f;
        default: return 14.0f;
    }
}

float ASoulRealtimeArenaGameMode::RoleHealth(
    ESoulRealtimeFormationRole Role)
{
    switch (Role)
    {
        case ESoulRealtimeFormationRole::Guard: return 132.0f;
        case ESoulRealtimeFormationRole::Breaker: return 112.0f;
        case ESoulRealtimeFormationRole::Shock: return 115.0f;
        case ESoulRealtimeFormationRole::Ranged: return 82.0f;
        case ESoulRealtimeFormationRole::Support: return 92.0f;
        case ESoulRealtimeFormationRole::Apex: return 190.0f;
        case ESoulRealtimeFormationRole::Hero: return 165.0f;
        default: return 105.0f;
    }
}

float ASoulRealtimeArenaGameMode::RoleWalkSpeed(
    ESoulRealtimeFormationRole Role)
{
    switch (Role)
    {
        case ESoulRealtimeFormationRole::Shock: return 330.0f;
        case ESoulRealtimeFormationRole::Hero: return 320.0f;
        case ESoulRealtimeFormationRole::Guard: return 240.0f;
        case ESoulRealtimeFormationRole::Breaker: return 360.0f;
        case ESoulRealtimeFormationRole::Apex: return 420.0f;
        default: return 270.0f;
    }
}

ESoulRealtimeMovementArchetype
ASoulRealtimeArenaGameMode::ResolveMovementArchetype(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    if (FormationRole == ESoulRealtimeFormationRole::Apex)
        return ESoulRealtimeMovementArchetype::Aerial;
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
        return ESoulRealtimeMovementArchetype::LowProfile;
    return ESoulRealtimeMovementArchetype::Infantry;
}

FString ASoulRealtimeArenaGameMode::RoleLabel(
    ESoulRealtimeFormationRole Role)
{
    switch (Role)
    {
        case ESoulRealtimeFormationRole::Guard: return TEXT("GUARD");
        case ESoulRealtimeFormationRole::Breaker: return TEXT("BREAKER");
        case ESoulRealtimeFormationRole::Shock: return TEXT("SHOCK");
        case ESoulRealtimeFormationRole::Ranged: return TEXT("RANGED");
        case ESoulRealtimeFormationRole::Support: return TEXT("SUPPORT");
        case ESoulRealtimeFormationRole::Apex: return TEXT("APEX");
        case ESoulRealtimeFormationRole::Hero: return TEXT("HERO");
        default: return TEXT("LINE");
    }
}

FRBWeaponProfile ASoulRealtimeArenaGameMode::Profile(int32 I) const
{
    FRBWeaponProfile P;
    if (!Combatants.IsValidIndex(I)) return P;
    const auto& C = Combatants[I];
    P.Id = C.bRanged ? TEXT("Soul.Bow") : TEXT("Soul.Melee");
    P.Category = C.bRanged ? TEXT("Bow")
        : (C.Role == ESoulRealtimeFormationRole::Apex
            ? TEXT("Claw") : TEXT("Sword"));
    P.DamageType = TEXT("Physical");
    P.BaseDamage = RoleDamage(C.Role);
    P.Reach = C.bRanged ? 2600.0f
        : (C.Role == ESoulRealtimeFormationRole::Apex
            ? 245.0f : 175.0f);
    P.BleedingTendency =
        C.Role == ESoulRealtimeFormationRole::Shock ? 0.12f : 0.04f;
    P.StaggerSeconds =
        C.Role == ESoulRealtimeFormationRole::Apex ? 0.35f : 0.08f;
    return P;
}

bool ASoulRealtimeArenaGameMode::CanAct(
    FRBHostIdentity Identity) const
{
    const int32 I = Index(Identity);
    return I != INDEX_NONE && Combatants[I].Health > 0.0f;
}
bool ASoulRealtimeArenaGameMode::CommitHit(
    const FRBHostHit& Hit, FString& Error)
{
    const int32 A = Index(Hit.Attacker);
    const int32 V = Index(Hit.Victim);
    if (A == INDEX_NONE || V == INDEX_NONE || A == V ||
        Combatants[A].Side == Combatants[V].Side ||
        !Hit.ContactId.IsValid() ||
        AcceptedContacts.Contains(Hit.ContactId) ||
        !Hit.bHasExactImpact ||
        !FMath::IsFinite(Hit.AcceptedDamage) ||
        Hit.AcceptedDamage <= 0.0f ||
        Combatants[V].Health <= 0.0f)
    {
        Error = TEXT("Soul arena rejected invalid, allied or repeated hit");
        return false;
    }

    AcceptedContacts.Add(Hit.ContactId);
    const float Before = Combatants[V].Health;
    Combatants[V].Health =
        FMath::Max(0.0f, Before - Hit.AcceptedDamage);
    if (Before > 0.0f && Combatants[V].Health <= 0.0f)
    {
        Status = FString::Printf(
            TEXT("%s %s defeated"),
            Combatants[V].Side == 0 ? TEXT("Human") : TEXT("Enemy"),
            *RoleLabel(Combatants[V].Role));
    }
    Error.Reset();
    return true;
}

bool ASoulRealtimeArenaGameMode::SpendResources(
    FRBHostIdentity Identity, FName WeaponId,
    FName Item, int32 Quantity, float Effort, FString& Error)
{
    const int32 I = Index(Identity);
    if (I == INDEX_NONE || !CanAct(Identity) ||
        !FMath::IsFinite(Effort) || Effort < 0.0f ||
        WeaponId != Profile(I).Id ||
        !((Item.IsNone() && Quantity == 0) ||
          (Item == TEXT("Soul.Arrow") && Quantity == 1)) ||
        Quantity < 0 || Combatants[I].Arrows < Quantity)
    {
        Error = TEXT("Soul arena rejected resource request");
        return false;
    }
    Combatants[I].Arrows -= Quantity;
    Error.Reset();
    return true;
}

bool ASoulRealtimeArenaGameMode::HasAmmunition(
    FRBHostIdentity Identity, FName Item) const
{
    const int32 I = Index(Identity);
    return I != INDEX_NONE && Item == TEXT("Soul.Arrow") &&
        Combatants[I].Arrows > 0;
}

bool ASoulRealtimeArenaGameMode::AreOpponents(
    FRBHostIdentity A, FRBHostIdentity B) const
{
    const int32 IA = Index(A);
    const int32 IB = Index(B);
    return IA != INDEX_NONE && IB != INDEX_NONE &&
        IA != IB && Combatants[IA].Side != Combatants[IB].Side;
}
bool ASoulRealtimeArenaGameMode::ReadParticipation(
    FRBHostIdentity Identity, FRBHostParticipation& Out) const
{
    const int32 I = Index(Identity);
    if (I == INDEX_NONE) return false;
    Out.bCanParticipate = Combatants[I].Health > 0.0f;
    Out.RangedMinimum = 420.0f;
    Out.RangedMaximum = 2600.0f;
    return true;
}

bool ASoulRealtimeArenaGameMode::ReadGroup(
    int32 GroupIndex, FRBHostGroup& Out) const
{
    if (!Groups.IsValidIndex(GroupIndex)) return false;
    Out = Groups[GroupIndex];
    return true;
}

bool ASoulRealtimeArenaGameMode::CommitGroupOrder(
    int32 GroupIndex,
    const FRBHostGroup& Previous,
    const FRBHostGroup& Replacement,
    FString& Error)
{
    if (!Groups.IsValidIndex(GroupIndex))
    {
        Error = TEXT("Soul arena group index invalid");
        return false;
    }

    FRBCombatGroup CurrentCore;
    FRBCombatGroup PreviousCore;
    FRBCombatGroup ReplacementCore;
    if (!Groups[GroupIndex].ToCore(CurrentCore) ||
        !Previous.ToCore(PreviousCore) ||
        !Replacement.ToCore(ReplacementCore) ||
        PreviousCore.Id != CurrentCore.Id ||
        PreviousCore.Revision != CurrentCore.Revision ||
        PreviousCore.Members != CurrentCore.Members ||
        ReplacementCore.Id != CurrentCore.Id ||
        ReplacementCore.Leader != CurrentCore.Leader ||
        ReplacementCore.Members != CurrentCore.Members ||
        ReplacementCore.Revision != CurrentCore.Revision + 1)
    {
        Error = TEXT("Soul arena rejected stale group order");
        return false;
    }
    Groups[GroupIndex] = Replacement;
    Error.Reset();
    return true;
}

bool ASoulRealtimeArenaGameMode::RequestMeleeFallback(
    URBVariantCombatBindingComponent* Binding)
{
    const int32 I = Binding
        ? Index(FRBHostIdentity::From(Binding->GetCombatant()))
        : INDEX_NONE;
    if (I == INDEX_NONE || !Combatants[I].bRanged) return false;
    Combatants[I].bRanged = false;
    if (Ranged.IsValidIndex(I) && Ranged[I])
    {
        Ranged[I]->CancelDraw();
    }
    return true;
}

int32 ASoulRealtimeArenaGameMode::AliveForSide(int32 Side) const
{
    int32 Count = 0;
    for (const auto& C : Combatants)
        Count += C.Side == Side && C.Health > 0.0f ? 1 : 0;
    return Count;
}
int32 ASoulRealtimeArenaGameMode::CasualtiesForSide(
    int32 Side) const
{
    if (Side < 0 || Side > 1) return 0;
    if (!ReinforcementBattle.Formations.IsEmpty())
    {
        return FMath::Max(
            0,
            InitialStrategic[Side] -
            AliveForSide(Side) -
            ReserveBodiesForSide(Side));
    }
    return FMath::Max(0, InitialAlive[Side] - AliveForSide(Side));
}

int32 ASoulRealtimeArenaGameMode::ReserveBodiesForSide(int32 Side) const
{
    if (Side < 0 || Side > 1 || ReinforcementBattle.Formations.IsEmpty())
        return 0;
    return FSoulRealtimeBattleRules::ReserveBodies(
        ReinforcementBattle, RealtimeSideId(Side));
}

int32 ASoulRealtimeArenaGameMode::ReinforcementWavesForSide(int32 Side) const
{
    return Side >= 0 && Side <= 1 ? ReinforcementWaves[Side] : 0;
}

FString ASoulRealtimeArenaGameMode::ReinforcementSummary(int32 Side) const
{
    if (Side < 0 || Side > 1) return FString();
    const int32 Reserve = ReserveBodiesForSide(Side);
    const int32 NextBodies = bFinished ? 0
        : FSoulRealtimeBattleRules::PreviewWave(ReinforcementBattle, RealtimeSideId(Side)).TotalBodies();
    const FString Readiness = bFinished ? TEXT("battle ended")
        : Reserve == 0 ? TEXT("reserves exhausted")
        : NextBodies > 0 ? FString::Printf(TEXT("next +%d ready"), NextBodies)
            : TEXT("waiting for frontline losses");
    FString Summary = FString::Printf(TEXT("%s: %d reserve | %s"),
        Side == 0 ? TEXT("Allies") : TEXT("Enemy"), Reserve, *Readiness);
    if (ReinforcementWaves[Side] > 0)
        Summary += FString::Printf(TEXT(" | Last arrival: +%d (wave %d)"),
            LastReinforcementBodies[Side], ReinforcementWaves[Side]);
    return Summary;
}

FString ASoulRealtimeArenaGameMode::AlliedOrderSummary() const
{
    int32 LivingGroups = 0, MatchingGroups = 0;
    for (const auto& Group : Groups)
    {
        bool AlliedAndAlive = false;
        for (const auto& Member : Group.Members)
        {
            const int32 I = Index(Member);
            if (I != INDEX_NONE && Combatants[I].Side == 0 && Combatants[I].Health > 0)
            { AlliedAndAlive = true; break; }
        }
        if (!AlliedAndAlive) continue;
        ++LivingGroups;
        FRBCombatGroup Core;
        if (Group.ToCore(Core) && (bAlliedCharge
            ? Core.Command == ERBGroupCommand::Charge || Core.Command == ERBGroupCommand::Advance
            : Core.Command == ERBGroupCommand::Hold)) ++MatchingGroups;
    }
    const TCHAR* Order = bAlliedCharge ? TEXT("CHARGE") : TEXT("HOLD");
    return FString::Printf(TEXT("Order %s: %d/%d live groups | arrivals %s | [G] %s"),
        Order, MatchingGroups, LivingGroups, Order,
        bAlliedCharge ? TEXT("Hold") : TEXT("Charge"));
}

FString ASoulRealtimeArenaGameMode::TacticalSummary() const
{
    int32 Routing[2] = {0, 0};
    int32 Living[2] = {0, 0};
    int32 Morale[2] = {0, 0};
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (AliveInGroup(State.GroupIndex) <= 0) continue;
        ++Living[State.Side];
        Morale[State.Side] += State.MoralePermille;
        Routing[State.Side] += State.bRouting ? 1 : 0;
    }
    const int32 AlliedMorale = Living[0] > 0 ? Morale[0] / Living[0] : 0;
    const int32 EnemyMorale = Living[1] > 0 ? Morale[1] / Living[1] : 0;
    return FString::Printf(
        TEXT("%s  |  Morale A %d%% (%d routing)  E %d%% (%d routing)"),
        FSoulRealtimeTacticalRules::PhaseLabel(BattlePhase),
        AlliedMorale / 10, Routing[0], EnemyMorale / 10, Routing[1]);
}

FString ASoulRealtimeArenaGameMode::SpellSummary() const
{
    return TEXT("[1] Firebolt  [2] Chain Lightning  [3] Blizzard  [4] Tidal Ward  [5] Tailwind");
}

int32 ASoulRealtimeArenaGameMode::TotalAlliedTargets() const
{
    int32 Count = 0;
    for (const auto& Driver : Drivers)
        if (Driver) Count += Driver->AlliedTargets;
    return Count;
}

float ASoulRealtimeArenaGameMode::PlayerHealth() const
{
    if (!PlayerHero) return 0.0f;
    const auto* Binding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 I = Binding
        ? Index(FRBHostIdentity::From(Binding->GetCombatant()))
        : INDEX_NONE;
    return I == INDEX_NONE ? 0.0f : Combatants[I].Health;
}

bool ASoulRealtimeArenaGameMode::CanCastMagic(
    const URBMagicSpellDefinition& Spell,
    const FRBMagicCastRequest& Request,
    FString& OutError) const
{
    OutError.Reset();
    if (!PlayerHero || Spell.bStrategicOnly)
    {
        OutError = TEXT("Battle caster or battle spell unavailable.");
        return false;
    }

    const auto* Binding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 CasterIndex = Binding
        ? Index(FRBHostIdentity::From(Binding->GetCombatant()))
        : INDEX_NONE;
    if (CasterIndex == INDEX_NONE || Combatants[CasterIndex].Health <= 0 ||
        Request.Caster.Domain != MagicArenaDomain ||
        Request.Caster.Id != Combatants[CasterIndex].Id)
    {
        OutError = TEXT("Cast request does not match the player hero.");
        return false;
    }

    const FName SpellName = Spell.SpellTag.GetTagName();
    if (SpellCooldowns.FindRef(SpellName) > 0.0f)
    {
        OutError = TEXT("Spell is on cooldown.");
        return false;
    }

    float ManaCost = 0.0f;
    for (const FRBMagicResourceCost& Cost : Spell.Costs)
    {
        if (Cost.ResourceTag.ToString() == TEXT("Magic.Resource.Mana"))
            ManaCost += Cost.Amount;
    }
    if (PlayerMana + KINDA_SMALL_NUMBER < ManaCost)
    {
        OutError = TEXT("Not enough mana.");
        return false;
    }

    if (Spell.TargetMode == ERBMagicTargetMode::Unit)
    {
        bool bFriendlyUnitSpell = false;
        for (const FRBMagicEffectSpec& Effect : Spell.Effects)
        {
            bFriendlyUnitSpell = bFriendlyUnitSpell ||
                Effect.EffectTag.ToString() == TEXT("Magic.Effect.Shield");
        }
        const int32 TargetIndex = Combatants.IndexOfByPredicate(
            [&Request](const FSoulRealtimeArenaCombatant& C)
            {
                return C.Id == Request.Target.Entity.Id;
            });
        const bool bSameSide = TargetIndex != INDEX_NONE &&
            Combatants[TargetIndex].Side == Combatants[CasterIndex].Side;
        if (TargetIndex == INDEX_NONE ||
            Combatants[TargetIndex].Health <= 0.0f ||
            bSameSide != bFriendlyUnitSpell ||
            !Actors.IsValidIndex(TargetIndex) || !Actors[TargetIndex])
        {
            OutError = bFriendlyUnitSpell
                ? TEXT("Ward requires a living allied target.")
                : TEXT("Unit spell requires a living enemy target.");
            return false;
        }
        if (Spell.Range > 0.0f &&
            FVector::Dist2D(
                PlayerHero->GetActorLocation(),
                Actors[TargetIndex]->GetActorLocation()) > Spell.Range)
        {
            OutError = TEXT("Magic target is out of range.");
            return false;
        }
    }
    else if (Spell.TargetMode == ERBMagicTargetMode::Ground &&
             Spell.Range > 0.0f &&
             FVector::Dist2D(
                 PlayerHero->GetActorLocation(),
                 Request.Target.WorldLocation) > Spell.Range)
    {
        OutError = TEXT("Ground target is out of range.");
        return false;
    }
    return true;
}

bool ASoulRealtimeArenaGameMode::ApplyMagicDamage(
    int32 TargetIndex, float Damage)
{
    if (!Combatants.IsValidIndex(TargetIndex) ||
        Damage <= 0.0f || Combatants[TargetIndex].Health <= 0.0f)
        return false;

    FSoulRealtimeArenaCombatant& Target = Combatants[TargetIndex];
    if (Target.WardSeconds > 0.0f && Target.WardPoints > 0.0f)
    {
        const float Absorbed = FMath::Min(Target.WardPoints, Damage);
        Target.WardPoints -= Absorbed;
        Damage -= Absorbed;
        UE_LOG(LogTemp, Display,
            TEXT("SOUL_MAGIC_WARD_ABSORB: side=%d absorbed=%.1f remaining=%.1f"),
            Target.Side, Absorbed, Target.WardPoints);
    }
    const float Before = Target.Health;
    Target.Health = FMath::Max(0.0f, Before - Damage);
    if (Before > 0.0f && Target.Health <= 0.0f)
    {
        UE_LOG(LogTemp, Display,
            TEXT("RB_MAGIC_DEFEAT: side=%d role=%s"),
            Combatants[TargetIndex].Side,
            *RoleLabel(Combatants[TargetIndex].Role));
    }
    return true;
}

bool ASoulRealtimeArenaGameMode::TryCommitMagicCast(
    const URBMagicSpellDefinition& Spell,
    const FRBMagicCastRequest& Request,
    TConstArrayView<FRBMagicEffectIntent> Effects,
    FString& OutError)
{
    if (!CanCastMagic(Spell, Request, OutError) || Effects.IsEmpty())
    {
        if (OutError.IsEmpty())
            OutError = TEXT("Spell produced no effect intents.");
        return false;
    }

    const auto* Binding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 CasterIndex = Binding
        ? Index(FRBHostIdentity::From(Binding->GetCombatant()))
        : INDEX_NONE;
    if (CasterIndex == INDEX_NONE)
    {
        OutError = TEXT("Player caster disappeared before commit.");
        return false;
    }

    float ManaCost = 0.0f;
    for (const FRBMagicResourceCost& Cost : Spell.Costs)
    {
        if (Cost.ResourceTag.ToString() == TEXT("Magic.Resource.Mana"))
            ManaCost += Cost.Amount;
    }

    int32 PrimaryTarget = INDEX_NONE;
    if (Spell.TargetMode == ERBMagicTargetMode::Unit)
    {
        PrimaryTarget = Combatants.IndexOfByPredicate(
            [&Request](const FSoulRealtimeArenaCombatant& C)
            {
                return C.Id == Request.Target.Entity.Id;
            });
        if (PrimaryTarget == INDEX_NONE)
        {
            OutError = TEXT("Primary magic target no longer exists.");
            return false;
        }
    }

    FSoulRealtimeArenaMagicArea Area;
    Area.Center = Request.Target.WorldLocation;
    Area.SourceSide = Combatants[CasterIndex].Side;
    bool bHasArea = false;

    for (const FRBMagicEffectIntent& Intent : Effects)
    {
        const FString EffectTag = Intent.Effect.EffectTag.ToString();
        if (EffectTag == TEXT("Magic.Effect.Damage") &&
            PrimaryTarget != INDEX_NONE)
        {
            TArray<int32> Targets;
            Targets.Add(PrimaryTarget);
            if (Spell.DeliveryMode == ERBMagicDeliveryMode::Chain &&
                Intent.Effect.Radius > 0.0f)
            {
                const FVector Center = Actors[PrimaryTarget]->GetActorLocation();
                for (int32 I = 0; I < Combatants.Num(); ++I)
                {
                    if (I == PrimaryTarget ||
                        Combatants[I].Side == Combatants[CasterIndex].Side ||
                        Combatants[I].Health <= 0.0f ||
                        !Actors.IsValidIndex(I) || !Actors[I])
                        continue;
                    if (FVector::DistSquared2D(
                            Center, Actors[I]->GetActorLocation()) <=
                        FMath::Square(Intent.Effect.Radius))
                        Targets.Add(I);
                }
                Targets.Sort([this, Center](int32 A, int32 B)
                {
                    return FVector::DistSquared2D(
                        Center, Actors[A]->GetActorLocation()) <
                        FVector::DistSquared2D(
                        Center, Actors[B]->GetActorLocation());
                });
            }

            const int32 Limit = FMath::Clamp(
                Intent.Effect.MaxTargets, 1, Targets.Num());
            for (int32 I = 0; I < Limit; ++I)
                ApplyMagicDamage(Targets[I], Intent.Effect.Magnitude);
        }
        else if (EffectTag == TEXT("Magic.Effect.Burning") &&
                 PrimaryTarget != INDEX_NONE)
        {
            ApplyMagicDamage(PrimaryTarget, Intent.Effect.Magnitude);
        }
        else if (EffectTag == TEXT("Magic.Effect.Shield") &&
                 PrimaryTarget != INDEX_NONE)
        {
            FSoulRealtimeArenaCombatant& Target = Combatants[PrimaryTarget];
            Target.WardPoints = FMath::Max(
                Target.WardPoints, Intent.Effect.Magnitude);
            Target.WardSeconds = FMath::Max(
                Target.WardSeconds, Intent.Effect.DurationSeconds);
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_MAGIC_WARD: target=%s strength=%.1f seconds=%.1f"),
                *RoleLabel(Target.Role), Target.WardPoints, Target.WardSeconds);
        }
        else if (EffectTag == TEXT("Magic.Effect.StrategicMovement"))
        {
            const int32 Side = Combatants[CasterIndex].Side;
            SpeedBuffMultiplier[Side] = FMath::Max(
                SpeedBuffMultiplier[Side], 1.0f + Intent.Effect.Magnitude);
            SpeedBuffSeconds[Side] = FMath::Max(
                SpeedBuffSeconds[Side],
                FMath::Max(8.0f, Intent.Effect.DurationSeconds));
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_MAGIC_TAILWIND: side=%d multiplier=%.2f seconds=%.1f"),
                Side, SpeedBuffMultiplier[Side], SpeedBuffSeconds[Side]);
        }
        else if (EffectTag == TEXT("Magic.Effect.DamagePeriodic"))
        {
            Area.DamagePerTick =
                FMath::Max(Area.DamagePerTick, Intent.Effect.Magnitude);
            Area.Radius = FMath::Max(Area.Radius, Intent.Effect.Radius);
            Area.RemainingSeconds =
                FMath::Max(Area.RemainingSeconds, Intent.Effect.DurationSeconds);
            bHasArea = true;
        }
        else if (EffectTag == TEXT("Magic.Effect.Slow"))
        {
            Area.SlowFraction = FMath::Clamp(
                Intent.Effect.Magnitude, 0.0f, 0.85f);
            Area.Radius = FMath::Max(Area.Radius, Intent.Effect.Radius);
            Area.RemainingSeconds =
                FMath::Max(Area.RemainingSeconds, Intent.Effect.DurationSeconds);
            bHasArea = true;
        }
    }

    if (bHasArea && Area.Radius > 0.0f &&
        Area.RemainingSeconds > 0.0f)
        ActiveMagicAreas.Add(Area);

    PlayerMana = FMath::Max(0.0f, PlayerMana - ManaCost);
    SpellCooldowns.Add(
        Spell.SpellTag.GetTagName(), Spell.CooldownSeconds);
    ++MagicCasts;
    Status = FString::Printf(
        TEXT("Cast %s | Mana %.0f"),
        *Spell.DisplayName.ToString(), PlayerMana);
    UE_LOG(LogTemp, Display,
        TEXT("RB_MAGIC_CAST: spell=%s mana=%.1f activeAreas=%d"),
        *Spell.SpellTag.ToString(), PlayerMana, ActiveMagicAreas.Num());
    OutError.Reset();
    return true;
}

void ASoulRealtimeArenaGameMode::BeginPlay()
{
    Super::BeginPlay();
    bProof = FParse::Param(
        FCommandLine::Get(), TEXT("SoulRealtimeArenaProof"));
    bMagicProof = FParse::Param(
        FCommandLine::Get(), TEXT("SoulRealtimeMagicProof"));
    bExternalEnvironment = FParse::Param(
        FCommandLine::Get(), TEXT("SoulRealtimeExternalEnvironment"));
    bVisualUnits = FParse::Param(
        FCommandLine::Get(), TEXT("SoulRealtimeVisualUnits"));
    FParse::Value(
        FCommandLine::Get(), TEXT("-SoulArenaOriginX="), ArenaOrigin.X);
    FParse::Value(
        FCommandLine::Get(), TEXT("-SoulArenaOriginY="), ArenaOrigin.Y);
    FParse::Value(
        FCommandLine::Get(), TEXT("-SoulArenaOriginZ="), ArenaOrigin.Z);

    bMapOnly = FParse::Param(FCommandLine::Get(), TEXT("SoulMapOnly"));
    bQualification = FParse::Param(FCommandLine::Get(), TEXT("SoulVerticalQualification"));
    bQualification = bQualification || bProof;
    bAutobattle = bQualification ||
        FParse::Param(FCommandLine::Get(), TEXT("SoulAutobattle"));
    bTacticalMagic = bMagicProof || FParse::Param(FCommandLine::Get(), TEXT("SoulTacticalMagic"));
    bQualification = bQualification || bMagicProof;
    bAutobattle = bAutobattle || bMagicProof;
    // The selected world itself owns environment identity; flags cannot substitute a cube map.
    const bool bDragon = GetWorld()->GetOutermost()->GetName().Contains(TEXT("Dragon_graveyard/Level/L_showcase_level"));
    if (bDragon)
    {
        bExternalEnvironment = true;
        bVisualUnits = true;
        if (ArenaOrigin.IsNearlyZero()) ArenaOrigin = FVector(14000, -10000, 0);
        // Preserve every donor actor. Reduce the near-field fog curtain only in this
        // transient battle world so the showcase's skeletons and lava remain legible.
        for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
        {
            if (auto* Fog = It->GetComponent())
            {
                UE_LOG(LogTemp, Display, TEXT("SOUL_DRAGON_BATTLE_FOG: authored=%g battle=%g"), Fog->FogDensity, FMath::Min(Fog->FogDensity, 0.006f));
                Fog->SetFogDensity(FMath::Min(Fog->FogDensity, 0.006f));
            }
        }
        // This imported map authored EV100 bounds (-0.5..0). Preserve those bounds
        // when Soul's renderer uses the legacy positive-luminance convention.
        // Change this world instance only; never modify the donor asset or global CVars.
        const auto* Extended = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"));
        const auto* Attenuation = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.LensAttenuation"));
        const bool bEV100 = Extended && Extended->GetInt() != 0;
        const float LuminanceMax = 0.78f / FMath::Max(Attenuation ? Attenuation->GetFloat() : 0.78f, 0.01f);
        for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
        {
            auto& Settings = It->Settings;
            UE_LOG(LogTemp, Display, TEXT("SOUL_DRAGON_EXPOSURE: volume=%s extended=%d authoredMin=%g authoredMax=%g bias=%g"),
                *It->GetName(), bEV100, Settings.AutoExposureMinBrightness, Settings.AutoExposureMaxBrightness, Settings.AutoExposureBias);
            if (!bEV100 && Settings.bOverride_AutoExposureMinBrightness && Settings.bOverride_AutoExposureMaxBrightness &&
                Settings.AutoExposureMinBrightness <= 0 && Settings.AutoExposureMaxBrightness <= 0)
            {
                Settings.AutoExposureMinBrightness = LuminanceMax * FMath::Pow(2.0f, Settings.AutoExposureMinBrightness);
                Settings.AutoExposureMaxBrightness = LuminanceMax * FMath::Pow(2.0f, Settings.AutoExposureMaxBrightness);
                UE_LOG(LogTemp, Display, TEXT("SOUL_DRAGON_EXPOSURE_NORMALIZED: min=%g max=%g"),
                    Settings.AutoExposureMinBrightness, Settings.AutoExposureMaxBrightness);
            }
        }
    }
    if (bMapOnly)
    {
        SetupBattleCamera();
        UE_LOG(LogTemp, Display, TEXT("SOUL_G0_READY: map=%s simulation=0 units=0"), *GetWorld()->GetOutermost()->GetName());
        return;
    }
    FParse::Value(FCommandLine::Get(), TEXT("SoulActivePerSide="), ActiveCap);
    ActiveCap = FMath::Clamp(ActiveCap, 1, 35);
    StrategicBodies[0] = StrategicBodies[1] = ActiveCap;
    FParse::Value(FCommandLine::Get(), TEXT("SoulPlayerPool="), StrategicBodies[0]);
    FParse::Value(FCommandLine::Get(), TEXT("SoulEnemyPool="), StrategicBodies[1]);
    StrategicBodies[0] = FMath::Clamp(StrategicBodies[0], 1, 250);
    StrategicBodies[1] = FMath::Clamp(StrategicBodies[1], 1, 250);
    // Campaign contract is applied below after validation by the campaign authority.
    if (auto* Bridge = GetGameInstance()->GetSubsystem<USoulCampaignBattleBridge>())
    {
        if (const auto* Encounter = Bridge->GetPendingEncounter())
        {
            if (!Encounter->IsValid() || Encounter->MapPackage.ToString() != GetWorld()->GetOutermost()->GetName())
            {
                FinishProof(false, TEXT("Campaign battlefield identity mismatch"));
                return;
            }
            bCampaignBattle = true;
            bExternalEnvironment = true;
            bVisualUnits = true;
            // Campaign battles are embodied unless an explicit qualification or
            // -SoulAutobattle run requested the observer commander.
            ArenaOrigin = Encounter->ArenaOrigin;
            ActiveCap = Encounter->ActiveCapPerSide;
            StrategicBodies[0] = Encounter->PlayerStrategicCount;
            StrategicBodies[1] = Encounter->EnemyStrategicCount;
            EnemyVisualFaction = Encounter->EnemyFaction;
            EnemyVisualRegion = Encounter->TargetRegion;
            PlayerMana = static_cast<float>(Encounter->PlayerMana);
            // Automatic casting belongs to explicit qualification. Normal play
            // uses the existing [1] input and must not spend mana on its own.
            bTacticalMagic = bTacticalMagic || bQualification;
            UE_LOG(LogTemp, Display, TEXT("SOUL_CAMPAIGN_BATTLE_BEGIN: encounter=%s target=%s map=%s player=%s:%s:%d enemy=%s:%s:%d cap=%d"),
                *Encounter->EncounterId.ToString(), *Encounter->TargetRegion.ToString(), *Encounter->MapPackage.ToString(),
                *Encounter->PlayerFaction.ToString(), *Encounter->PlayerUnitId.ToString(), StrategicBodies[0],
                *Encounter->EnemyFaction.ToString(), *Encounter->EnemyUnitId.ToString(), StrategicBodies[1], ActiveCap);
        }
    }
    Spatial = NewObject<USoulRealtimeBattlePBIL>(this);
    if (!Spatial->Configure(GetWorld(), ArenaOrigin))
    {
        FinishProof(false, TEXT("PBIL initialization failed"));
        return;
    }
    if (!SetupArena())
    {
        if (bProof || bQualification)
            FinishProof(false, TEXT("arena setup failed"));
        else
            Status = TEXT("Arena setup failed");
        return;
    }

    InitialAlive[0] = AliveForSide(0);
    InitialAlive[1] = AliveForSide(1);
    SetupReinforcementState();
    if (bAutobattle || !PlayerHero) SetupBattleCamera();
    UE_LOG(LogTemp, Display,
        TEXT("SOUL_RT_ARENA_SETUP: actors=%d groups=%d proof=%d external=%d visuals=%d origin=%s"),
        Combatants.Num(), Groups.Num(), bProof,
        bExternalEnvironment, bVisualUnits,
        *ArenaOrigin.ToCompactString());
}

bool ASoulRealtimeArenaGameMode::SetupArena()
{
    if (!bExternalEnvironment)
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(
            nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (!Cube) return false;

        AActor* Floor = GetWorld()->SpawnActor<AActor>();
        UStaticMeshComponent* FloorMesh =
            NewObject<UStaticMeshComponent>(Floor);
        Floor->SetRootComponent(FloorMesh);
        Floor->AddInstanceComponent(FloorMesh);
        FloorMesh->SetStaticMesh(Cube);
        FloorMesh->SetWorldScale3D(FVector(55.0f, 42.0f, 0.2f));
        FloorMesh->SetCollisionProfileName(TEXT("BlockAll"));
        FloorMesh->RegisterComponent();
        Floor->SetActorLocation(FVector(0, 0, -20));

        ADirectionalLight* Light =
            GetWorld()->SpawnActor<ADirectionalLight>();
        if (Light)
        {
            Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            Light->SetActorRotation(FRotator(-55, -35, 0));
            Light->GetLightComponent()->SetIntensity(5.0f);
        }
    }
    if (bExternalEnvironment && !SetupBattlefieldBounds())
        return false;
    if (!SpawnArmy(0) || !SpawnArmy(1) || !SetupDrivers())
        return false;

    if (!bProof && !bAutobattle && PlayerHero)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (!PC) return false;
        PC->Possess(PlayerHero);
        const FRotator InitialView(
            -32.0f, PlayerHero->GetActorRotation().Yaw, 0.0f);
        PC->SetControlRotation(InitialView);
        PC->SetViewTarget(PlayerHero);
        PC->SetShowMouseCursor(false);
        UE_LOG(LogTemp, Display,
            TEXT("SOUL_RT_PLAYER_CONTROL: possessed=%d hero=%s control=%s"),
            PC->GetPawn() == PlayerHero ? 1 : 0,
            *PlayerHero->GetActorLocation().ToCompactString(),
            *PC->GetControlRotation().ToCompactString());
    }

    Status = TEXT("Knights versus Dwarves | bounded active formations");
    return Combatants.Num() == FMath::Min(ActiveCap, StrategicBodies[0]) + FMath::Min(ActiveCap, StrategicBodies[1]);
}

bool ASoulRealtimeArenaGameMode::SetupBattlefieldBounds()
{
    auto AddWall = [this](const FVector& Offset, const FVector& Extent)
    {
        AActor* Wall = GetWorld() ? GetWorld()->SpawnActor<AActor>() : nullptr;
        if (!Wall) return false;
        UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
        if (!Box) return false;
        Wall->SetRootComponent(Box);
        Wall->AddInstanceComponent(Box);
        Box->InitBoxExtent(Extent);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->SetGenerateOverlapEvents(false);
        Box->SetCanEverAffectNavigation(false);
        Box->SetHiddenInGame(true);
        Box->RegisterComponent();
        Wall->SetActorLocation(ArenaOrigin + Offset);
        BattlefieldBounds.Add(Wall);
        return true;
    };

    BattlefieldBounds.Reset();
    const bool bReady =
        AddWall(FVector(BattlefieldHalfX + BattlefieldWallThickness, 0, 0), FVector(BattlefieldWallThickness, BattlefieldHalfY + 200.0f, BattlefieldWallHalfHeight)) &&
        AddWall(FVector(-BattlefieldHalfX - BattlefieldWallThickness, 0, 0), FVector(BattlefieldWallThickness, BattlefieldHalfY + 200.0f, BattlefieldWallHalfHeight)) &&
        AddWall(FVector(0, BattlefieldHalfY + BattlefieldWallThickness, 0), FVector(BattlefieldHalfX + 200.0f, BattlefieldWallThickness, BattlefieldWallHalfHeight)) &&
        AddWall(FVector(0, -BattlefieldHalfY - BattlefieldWallThickness, 0), FVector(BattlefieldHalfX + 200.0f, BattlefieldWallThickness, BattlefieldWallHalfHeight));
    UE_LOG(LogTemp, Display, TEXT("SOUL_RT_BOUNDS_SETUP: halfX=%.0f halfY=%.0f walls=%d"), BattlefieldHalfX, BattlefieldHalfY, BattlefieldBounds.Num());
    return bReady && BattlefieldBounds.Num() == 4;
}

void ASoulRealtimeArenaGameMode::TrackBattlefieldExtent()
{
    if (!bExternalEnvironment) return;
    for (const ACharacter* Actor : Actors)
    {
        if (!Actor) continue;
        const FVector Delta = Actor->GetActorLocation() - ArenaOrigin;
        MaxObservedArenaOffsetX = FMath::Max(
            MaxObservedArenaOffsetX, FMath::Abs(Delta.X));
        MaxObservedArenaOffsetY = FMath::Max(
            MaxObservedArenaOffsetY, FMath::Abs(Delta.Y));
    }
}

bool ASoulRealtimeArenaGameMode::SpawnArmy(int32 Side)
{
    int32 Remaining = FMath::Min(ActiveCap, StrategicBodies[Side]);
    const int32 GroupCount = FMath::Clamp(
        FMath::DivideAndRoundUp(Remaining, 7), 1, 5);

    TArray<ESoulRealtimeFormationRole> Roles;
    TArray<ESoulBattleFormationKind> Kinds;
    if (GroupCount == 1)
    {
        Roles = {ESoulRealtimeFormationRole::Line};
        Kinds = {ESoulBattleFormationKind::FrontLine};
    }
    else if (GroupCount == 2)
    {
        Roles = {ESoulRealtimeFormationRole::Line, ESoulRealtimeFormationRole::Ranged};
        Kinds = {ESoulBattleFormationKind::FrontLine, ESoulBattleFormationKind::MissileSupport};
    }
    else if (GroupCount == 3)
    {
        Roles = {ESoulRealtimeFormationRole::Line, ESoulRealtimeFormationRole::Ranged, ESoulRealtimeFormationRole::Shock};
        Kinds = {ESoulBattleFormationKind::FrontLine, ESoulBattleFormationKind::MissileSupport, ESoulBattleFormationKind::Strike};
    }
    else if (GroupCount == 4)
    {
        Roles = {ESoulRealtimeFormationRole::Line, ESoulRealtimeFormationRole::Ranged,
            ESoulRealtimeFormationRole::Shock, ESoulRealtimeFormationRole::Hero};
        Kinds = {ESoulBattleFormationKind::FrontLine, ESoulBattleFormationKind::MissileSupport,
            ESoulBattleFormationKind::Strike, ESoulBattleFormationKind::CommandReserve};
    }
    else
    {
        Roles = {ESoulRealtimeFormationRole::Line, ESoulRealtimeFormationRole::Guard,
            ESoulRealtimeFormationRole::Ranged, ESoulRealtimeFormationRole::Shock,
            ESoulRealtimeFormationRole::Hero};
        Kinds = {ESoulBattleFormationKind::FrontLine, ESoulBattleFormationKind::FrontLine,
            ESoulBattleFormationKind::MissileSupport, ESoulBattleFormationKind::Strike,
            ESoulBattleFormationKind::CommandReserve};
    }

    const float Direction = Side == 0 ? 1.0f : -1.0f;
    const float BaseX = Side == 0 ? -1200.0f : 1200.0f;
    const float YSlots[5] = {-280.0f, 280.0f, -620.0f, 900.0f, 620.0f};
    const float Rearward[5] = {0.0f, 0.0f, 340.0f, 80.0f, 430.0f};
    for (int32 G = 0; G < GroupCount; ++G)
    {
        const int32 SlotsLeft = GroupCount - G;
        const int32 Count = FMath::Clamp(
            FMath::DivideAndRoundUp(Remaining, SlotsLeft), 1, 8);
        const FVector Anchor = ArenaOrigin + FVector(
            BaseX - Direction * Rearward[G],
            (Side == 0 ? 1.0f : -1.0f) * YSlots[G], 100.0f);
        if (!SpawnFormation(Side, Roles[G], Count, Anchor))
            return false;

        FSoulBattleFormationState State;
        State.GroupIndex = Groups.Num() - 1;
        State.Side = Side;
        State.Kind = Kinds[G];
        State.SpawnAnchor = Anchor;
        State.TacticalAnchor = Anchor + FVector(Direction * 650.0f, 0, 0);
        if (Kinds[G] == ESoulBattleFormationKind::Strike)
            State.TacticalAnchor.Y += (Side == 0 ? 1.0f : -1.0f) * 350.0f;
        State.InitialBodies = Count;
        State.PreviousAlive = Count;
        TacticalFormations.Add(State);
        Remaining -= Count;
    }
    return Remaining == 0;
}

bool ASoulRealtimeArenaGameMode::SpawnFormation(
    int32 Side,
    ESoulRealtimeFormationRole FormationRole,
    int32 Count,
    const FVector& Anchor)
{
    if (Count <= 0 || Count > FRBCombatGroup::MaximumMembers)
        return false;

    const int32 FirstCombatant = Combatants.Num();
    const int32 FirstGroup = Groups.Num();
    const int32 FirstDriver = Drivers.Num();
    const TWeakObjectPtr<ACharacter> PreviousHero(PlayerHero);
    bool bSpawnCommitted = false;
    ON_SCOPE_EXIT
    {
        if (!bSpawnCommitted)
        {
            RollbackSpawnedFormation(FirstCombatant, FirstGroup, FirstDriver);
            PlayerHero = PreviousHero.Get();
        }
    };
    const int32 GroupIndex = Groups.AddDefaulted();
    FRBCombatGroup Core;
    Core.Id = FGuid::NewGuid();
    // Deployment begins organized. The formation commander commits each group
    // after both armies are registered, rather than spawning into an instant mob.
    Core.Command = ERBGroupCommand::Hold;
    Core.Anchor = Anchor;
    Core.Facing = Side == 0
        ? FVector::ForwardVector : -FVector::ForwardVector;

    const int32 Columns = FMath::Min(4, Count);
    for (int32 I = 0; I < Count; ++I)
    {
        const int32 Row = I / Columns;
        const int32 Col = I % Columns;
        const float Center = (Columns - 1) * 0.5f;
        FVector Location = Anchor;
        Location.X += (Side == 0 ? -1.0f : 1.0f)
            * Row * 115.0f;
        Location.Y += (Col - Center) * 115.0f;
        // A formation owns the tactical job; specialist members keep their own
        // combat and locomotion identity inside it. The strike element therefore
        // fields an aerial apex and, for the evil roster, a low-profile beast
        // without fragmenting the army into one-actor "formations".
        ESoulRealtimeFormationRole MemberRole = FormationRole;
        if (FormationRole == ESoulRealtimeFormationRole::Shock && I == 0)
            MemberRole = ESoulRealtimeFormationRole::Apex;
        else if (FormationRole == ESoulRealtimeFormationRole::Shock && I == 1 &&
                 (Side == 0 || UsesEvilVisualRoster()))
            MemberRole = ESoulRealtimeFormationRole::Breaker;
        const bool bPlayer =
            !bProof && !bAutobattle && Side == 0 &&
            FormationRole == ESoulRealtimeFormationRole::Hero && I == 0;
        if (!SpawnCombatant(
                Side, MemberRole, GroupIndex, Location, bPlayer))
            return false;

        const FRBHostIdentity Identity =
            IdentityAt(Combatants.Num() - 1);
        Core.Members.Add(Identity.Core());
        if (I == 0) Core.Leader = Identity.Core();
    }

    FRBHostGroup HostGroup;
    if (!FRBHostGroup::FromCore(Core, HostGroup))
        return false;
    Groups[GroupIndex] = MoveTemp(HostGroup);
    bSpawnCommitted = true;
    return true;
}

void ASoulRealtimeArenaGameMode::RollbackSpawnedFormation(
    int32 FirstCombatant, int32 FirstGroup, int32 FirstDriver)
{
    for (int32 I = Drivers.Num() - 1; I >= FirstDriver; --I)
    {
        if (IsValid(Drivers[I])) Drivers[I]->DestroyComponent();
    }
    Drivers.SetNum(FirstDriver);
    for (int32 I = Actors.Num() - 1; I >= FirstCombatant; --I)
    {
        ACharacter* Actor = Actors[I];
        if (!IsValid(Actor)) continue;
        if (Spatial) Spatial->UnregisterUnit(Actor);
        if (AAIController* AI = Cast<AAIController>(Actor->GetController()))
        {
            AI->StopMovement();
            AI->UnPossess();
            AI->Destroy();
        }
        if (PlayerHero == Actor) PlayerHero = nullptr;
        Actor->SetActorEnableCollision(false);
        Actor->Destroy();
    }
    Actors.SetNum(FirstCombatant);
    Bindings.SetNum(FirstCombatant);
    Ranged.SetNum(FirstCombatant);
    VisualRunning.SetNum(FirstCombatant);
    Combatants.SetNum(FirstCombatant);
    Groups.SetNum(FirstGroup);
    TacticalFormations.RemoveAll(
        [FirstGroup](const FSoulBattleFormationState& State)
        {
            return State.GroupIndex >= FirstGroup;
        });
    for (auto It = DefeatedRepresentations.CreateIterator(); It; ++It)
    {
        if (*It >= FirstCombatant) It.RemoveCurrent();
    }
    // A failed refresh may already have updated some pre-existing drivers. Restore all of them.
    if (!RefreshDriverRepresentations())
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_RT_SPAWN_ROLLBACK_DRIVER_FAILURE"));
    }
    UE_LOG(LogTemp, Display, TEXT("SOUL_RT_SPAWN_ROLLED_BACK: actors=%d groups=%d drivers=%d"),
        Actors.Num(), Groups.Num(), Drivers.Num());
}

FVector ASoulRealtimeArenaGameMode::ResolveSpawnLocation(
    const FVector& Desired)
{
    if (!bExternalEnvironment || !GetWorld())
        return Desired;

    FHitResult Hit;
    FCollisionQueryParams Query(
        SCENE_QUERY_STAT(SoulRealtimeArenaGround), false);
    // Reinforcement deployment must hit terrain, never a living or retained corpse capsule.
    for (const ACharacter* Existing : Actors)
    {
        if (Existing) Query.AddIgnoredActor(Existing);
    }
    const FVector Start = Desired + FVector(0, 0, 8000.0);
    const FVector End = Desired - FVector(0, 0, 12000.0);
    if (GetWorld()->LineTraceSingleByChannel(
            Hit, Start, End, ECC_Visibility, Query))
    {
        return FVector(
            Desired.X, Desired.Y, Hit.ImpactPoint.Z + 96.0);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("SOUL_RT_GROUND_MISS: desired=%s"),
        *Desired.ToCompactString());
    return Desired;
}

USkeletalMesh* ASoulRealtimeArenaGameMode::ResolveVisualMesh(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Apex)
    {
        Path = Side == 0
            ? TEXT("/Game/QuadrapedCreatures/Griffon/Meshes/SK_Griffon.SK_Griffon")
            : TEXT("/Game/QuadrapedCreatures/MountainDragon/Meshes/SK_MOUNTAIN_DRAGON.SK_MOUNTAIN_DRAGON");
        return LoadObject<USkeletalMesh>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Hero)
    {
        Path = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora");
        return LoadObject<USkeletalMesh>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/AfricanAnimalsPack/Elephant/Meshes/SK_ElephantTusksBig.SK_ElephantTusksBig");
        return LoadObject<USkeletalMesh>(nullptr, Path);
    }
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/Kraken/Meshes/KRAKEN.KRAKEN");
        return LoadObject<USkeletalMesh>(nullptr, Path);
    }
    if (Side == 0)
    {
        switch (FormationRole)
        {
            case ESoulRealtimeFormationRole::Line:
                // Strict parent-chain compatibility with the existing Dwarf melee clips is qualified.
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_02/Mesh_UE4/Full/SK_Knight_02_Full_01.SK_Knight_02_Full_01");
                break;
            case ESoulRealtimeFormationRole::Guard:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_04/Mesh_UE5/Full_Mesh/SKM_Knight_04_Full_01.SKM_Knight_04_Full_01");
                break;
            case ESoulRealtimeFormationRole::Breaker:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_05/Mesh_UE5/Full_Mesh/SKM_Knight_05_Full_01.SKM_Knight_05_Full_01");
                break;
            case ESoulRealtimeFormationRole::Shock:
            case ESoulRealtimeFormationRole::Ranged:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_03/Mesh_UE5/Full/SKM_Knight_03_Full_01.SKM_Knight_03_Full_01");
                break;
            case ESoulRealtimeFormationRole::Hero:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_01/Mesh_UE5/Knight_01_Full/SKM_Knight_01_Full_01.SKM_Knight_01_Full_01");
                break;
            default:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_02/Mesh_UE5/Full/SKM_Knight_02_Full_01.SKM_Knight_02_Full_01");
                break;
        }
    }
    else if (!UsesEvilVisualRoster())
    {
        switch (FormationRole)
        {
            case ESoulRealtimeFormationRole::Guard:
            case ESoulRealtimeFormationRole::Hero:
                Path = TEXT("/Game/Dwarf_Pack/King/Mesh/SK_Dwarf_King_Full.SK_Dwarf_King_Full");
                break;
            case ESoulRealtimeFormationRole::Breaker:
            case ESoulRealtimeFormationRole::Apex:
                Path = TEXT("/Game/Dwarf_Pack/Broddi/Mesh/SK_Dwarf_BroddI_Full.SK_Dwarf_BroddI_Full");
                break;
            case ESoulRealtimeFormationRole::Ranged:
                Path = TEXT("/Game/Dwarf_Pack/Orme/Mesh/SK_Dwarf_Orme_Full.SK_Dwarf_Orme_Full");
                break;
            case ESoulRealtimeFormationRole::Support:
                Path = TEXT("/Game/Dwarf_Pack/Agvid/Mesh/SK/SK_Dwarf_Agvid_Full.SK_Dwarf_Agvid_Full");
                break;
            default:
                Path = TEXT("/Game/Dwarf_Pack/Bedvar/Mesh/SK_Dwarf_Bedvar_Full.SK_Dwarf_Bedvar_Full");
                break;
        }
    }
    else
    {
        switch (FormationRole)
        {
            case ESoulRealtimeFormationRole::Guard:
            case ESoulRealtimeFormationRole::Hero:
                Path = TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer.SK_Orc_Hummer");
                break;
            case ESoulRealtimeFormationRole::Breaker:
            case ESoulRealtimeFormationRole::Apex:
                Path = TEXT("/Game/Fantasy_Pack/Characters/Troll/Mesh/SK_Troll.SK_Troll");
                break;
            case ESoulRealtimeFormationRole::Ranged:
                Path = TEXT("/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full.SK_Ulf_Full");
                break;
            case ESoulRealtimeFormationRole::Support:
                Path = TEXT("/Game/Fantasy_Pack/Characters/Fantasy_Barbarian/Mesh/SK_Fantasy_Barbarian_Full.SK_Fantasy_Barbarian_Full");
                break;
            default:
                Path = TEXT("/Game/Fantasy_Pack/Characters/Barbarian/Mesh/SK_Barbarian_Full.SK_Barbarian_Full");
                break;
        }
    }
    return Path ? LoadObject<USkeletalMesh>(nullptr, Path) : nullptr;
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveVisualAnimation(
    int32 Side, bool bRunning, ESoulRealtimeFormationRole FormationRole) const
{
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Apex)
    {
        Path = Side == 0
            ? (bRunning
                ? TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_FlyNormal.ANIM_Griffon_FlyNormal")
                : TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_FlyStationary.ANIM_Griffon_FlyStationary"))
            : (bRunning
                ? TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_flyNormal.ANIM_MOUNTAIN_DRAGON_flyNormal")
                : TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_FlyStationary.ANIM_MOUNTAIN_DRAGON_FlyStationary"));
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Hero)
    {
        Path = bRunning
            ? TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd.Jog_Fwd")
            : TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Idle.Idle");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = bRunning
            ? TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_Run.ANIM_Elephant_Run")
            : TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_IdleBreathe.ANIM_Elephant_IdleBreathe");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = bRunning
            ? TEXT("/Game/Kraken/Animations/KRAKEN_walk.KRAKEN_walk")
            : TEXT("/Game/Kraken/Animations/KRAKEN_idle.KRAKEN_idle");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0)
    {
        Path = FormationRole == ESoulRealtimeFormationRole::Line
            ? (bRunning
                ? TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run")
                : TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Idle.Anim_Warrior_Idle"))
            : (bRunning
                ? TEXT("/Game/Knights_Pack/Demoscene_UE5/Animations/MM_Run_Fwd.MM_Run_Fwd")
                : TEXT("/Game/Knights_Pack/Demoscene_UE5/Animations/MM_Idle.MM_Idle"));
    }
    else if (!UsesEvilVisualRoster())
    {
        Path = bRunning
            ? TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run")
            : TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Idle.Anim_Warrior_Idle");
    }
    else if (FormationRole == ESoulRealtimeFormationRole::Guard ||
             FormationRole == ESoulRealtimeFormationRole::Hero)
    {
        Path = bRunning
            ? TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Run.Anim_Orc_Hummer_Run")
            : TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Idle.Anim_Orc_Hummer_Idle");
    }
    else if (FormationRole == ESoulRealtimeFormationRole::Breaker ||
             FormationRole == ESoulRealtimeFormationRole::Apex)
    {
        Path = bRunning
            ? TEXT("/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Run.Anim_Troll_Run")
            : TEXT("/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Idle.Anim_Troll_Idle");
    }
    else
    {
        Path = bRunning
            ? TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run")
            : TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Idle.Anim_Warrior_Idle");
    }
    return LoadObject<UAnimationAsset>(nullptr, Path);
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveVisualAttack(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Apex)
    {
        Path = Side == 0
            ? TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_FlyStationaryClawsAttack.ANIM_Griffon_FlyStationaryClawsAttack")
            : TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_FlyStationarySpreadFire.ANIM_MOUNTAIN_DRAGON_FlyStationarySpreadFire");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Hero)
    {
        Path = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_A.Primary_Attack_A");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_TusksAttack1.ANIM_Elephant_TusksAttack1");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/Kraken/Animations/KRAKEN_sweepAttack.KRAKEN_sweepAttack");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0)
    {
        if (FormationRole == ESoulRealtimeFormationRole::Line)
            Path = TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1");
    }
    else if (!UsesEvilVisualRoster())
        Path = TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1");
    else if (FormationRole == ESoulRealtimeFormationRole::Guard ||
             FormationRole == ESoulRealtimeFormationRole::Hero)
        Path = TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_1.Anim_Orc_Hummer_Attack_1");
    else if (FormationRole == ESoulRealtimeFormationRole::Breaker ||
             FormationRole == ESoulRealtimeFormationRole::Apex)
        Path = TEXT("/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Attack_1.Anim_Troll_Attack_1");
    else
        Path = TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1");
    return Path ? LoadObject<UAnimationAsset>(nullptr, Path) : nullptr;
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveVisualDeath(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Apex)
    {
        Path = Side == 0
            ? TEXT("/Game/QuadrapedCreatures/Griffon/Animations/ANIM_Griffon_Death.ANIM_Griffon_Death")
            : TEXT("/Game/QuadrapedCreatures/MountainDragon/Animations/ANIM_MOUNTAIN_DRAGON_deathHitTheGround.ANIM_MOUNTAIN_DRAGON_deathHitTheGround");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Hero)
    {
        Path = TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Death.Death");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_Death.ANIM_Elephant_Death");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = TEXT("/Game/Kraken/Animations/KRAKEN_death.KRAKEN_death");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0)
    {
        if (FormationRole == ESoulRealtimeFormationRole::Line)
            Path = TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1.Anim_Warrior_Dead_1");
    }
    else if (!UsesEvilVisualRoster())
        Path = TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1.Anim_Warrior_Dead_1");
    else if (FormationRole == ESoulRealtimeFormationRole::Guard ||
             FormationRole == ESoulRealtimeFormationRole::Hero)
        Path = TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Dead.Anim_Orc_Hummer_Dead");
    else if (FormationRole == ESoulRealtimeFormationRole::Breaker ||
             FormationRole == ESoulRealtimeFormationRole::Apex)
        Path = TEXT("/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Dead.Anim_Troll_Dead");
    else
        Path = TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1.Anim_Warrior_Dead_1");
    return Path ? LoadObject<UAnimationAsset>(nullptr, Path) : nullptr;
}

bool ASoulRealtimeArenaGameMode::UsesEvilVisualRoster() const
{
    return EnemyVisualFaction == FName(TEXT("orcs")) ||
           EnemyVisualFaction == FName(TEXT("evil")) ||
           EnemyVisualRegion == FName(TEXT("orc_watch")) ||
           EnemyVisualRegion == FName(TEXT("north_pass")) ||
           EnemyVisualRegion == FName(TEXT("orc_camp")) ||
           FParse::Param(FCommandLine::Get(), TEXT("SoulEvilVisualRoster"));
}

void ASoulRealtimeArenaGameMode::UpdateVisualAnimations()
{
    if (!bVisualUnits) return;
    for (int32 I = 0; I < Actors.Num(); ++I)
    {
        if (!Actors[I] || !Combatants.IsValidIndex(I) || Combatants[I].Health <= 0 ||
            !VisualRunning.IsValidIndex(I))
            continue;
        bool bResumeLocomotion = false;
        if (Combatants[I].bVisualAttackPlaying)
        {
            const UAnimSingleNodeInstance* Playback = Actors[I]->GetMesh()->GetSingleNodeInstance();
            if (Playback && Playback->IsPlaying()) continue;
            Combatants[I].bVisualAttackPlaying = false;
            bResumeLocomotion = true;
        }
        const bool bRunning =
            Actors[I]->GetVelocity().SizeSquared2D() > FMath::Square(12.0);
        if (!bResumeLocomotion && VisualRunning[I] == bRunning)
            continue;
        if (UAnimationAsset* Animation =
            ResolveVisualAnimation(Combatants[I].Side, bRunning, Combatants[I].Role))
        {
            Actors[I]->GetMesh()->PlayAnimation(Animation, true);
            Actors[I]->GetMesh()->SetPlayRate(1.0f);
            VisualRunning[I] = bRunning;
        }
    }
}

bool ASoulRealtimeArenaGameMode::SpawnCombatant(
    int32 Side,
    ESoulRealtimeFormationRole FormationRole,
    int32 GroupIndex,
    const FVector& Location,
    bool bPlayer)
{
    UStaticMesh* Cube = nullptr;
    if (!bVisualUnits)
    {
        Cube = LoadObject<UStaticMesh>(
            nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (!Cube) return false;
    }

    const bool bKraken =
        Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker;
    const bool bElephant =
        Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker;

    FSoulRealtimeArenaCombatant Data;
    Data.Id = FGuid::NewGuid();
    Data.Side = Side;
    Data.Role = FormationRole;
    Data.Health = bKraken ? 360.0f :
        (bElephant ? 320.0f : RoleHealth(FormationRole));
    Data.Arrows =
        FormationRole == ESoulRealtimeFormationRole::Ranged ? 32 : 0;
    Data.GroupIndex = GroupIndex;
    Data.bRanged = FormationRole == ESoulRealtimeFormationRole::Ranged;
    Data.bPlayerHero = bPlayer;
    Data.Movement = ResolveMovementArchetype(Side, FormationRole);
    const int32 NewIndex = Combatants.Add(Data);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FRotator Facing(0, Side == 0 ? 0.0f : 180.0f, 0);
    const FVector SpawnLocation = ResolveSpawnLocation(Location);
    ACharacter* Actor = GetWorld()->SpawnActor<ACharacter>(
        ACharacter::StaticClass(), SpawnLocation, Facing, Params);
    if (!Actor) return false;

    // Track ownership before any asset/binding/AI operation can fail, so the enclosing
    // formation transaction can destroy this actor even if setup stops halfway through.
    Actors.Add(Actor);
    Bindings.Add(nullptr);
    Ranged.Add(nullptr);
    VisualRunning.Add(false);
    Actor->SetCanBeDamaged(true);
    Actor->bUseControllerRotationYaw = bPlayer;
    Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
        ECC_Visibility, ECR_Block);
    Actor->GetCharacterMovement()->MaxWalkSpeed =
        bKraken ? 260.0f :
        (bElephant ? 310.0f : RoleWalkSpeed(FormationRole));
    Actor->GetCharacterMovement()->bOrientRotationToMovement = !bPlayer;
    if (Data.Movement == ESoulRealtimeMovementArchetype::Aerial)
    {
        // The capsule remains nav-authoritative while the creature occupies an
        // airborne visual lane. Pawn overlap and higher speed let it cross the
        // infantry frontage without creating an unpathable mid-air AI pawn.
        Actor->GetCapsuleComponent()->SetCapsuleSize(82.0f, 115.0f);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
            ECC_Pawn, ECR_Overlap);
    }
    else if (Data.Movement == ESoulRealtimeMovementArchetype::LowProfile)
    {
        Actor->GetCapsuleComponent()->SetCapsuleSize(
            bKraken ? 150.0f : 58.0f,
            bKraken ? 170.0f : 58.0f);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
            ECC_Pawn, ECR_Overlap);
    }
    else if (bElephant)
    {
        Actor->GetCapsuleComponent()->SetCapsuleSize(115.0f, 150.0f);
        Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
            ECC_Pawn, ECR_Overlap);
    }

    if (bVisualUnits)
    {
        USkeletalMesh* Mesh = ResolveVisualMesh(Side, FormationRole);
        UAnimationAsset* Idle = ResolveVisualAnimation(Side, false, FormationRole);
        if (!Mesh || !Idle)
        {
            UE_LOG(LogTemp, Error,
                TEXT("SOUL_RT_VISUAL_LOAD_FAIL: side=%d role=%s mesh=%d idle=%d"),
                Side, *RoleLabel(FormationRole), Mesh != nullptr, Idle != nullptr);
            return false;
        }

        USkeletalMeshComponent* Visual = Actor->GetMesh();
        Visual->SetSkeletalMeshAsset(Mesh);
        if (Data.Movement == ESoulRealtimeMovementArchetype::Aerial)
        {
            const float ApexScale = Side == 0 ? 0.55f : 0.28f;
            Visual->SetRelativeLocation(FVector(0, 0, 180.0f));
            Visual->SetRelativeRotation(FRotator::ZeroRotator);
            Visual->SetRelativeScale3D(FVector(ApexScale));
        }
        else if (Data.Movement == ESoulRealtimeMovementArchetype::LowProfile)
        {
            // The donor Kraken is enormous. A 20% presentation preserves its
            // broad silhouette while keeping it in the same elite-creature band
            // as the elephant, griffin, and mountain dragon.
            Visual->SetRelativeLocation(FVector(
                0, 0, bKraken ? 105.0f : -58.0f));
            Visual->SetRelativeRotation(FRotator::ZeroRotator);
            Visual->SetRelativeScale3D(FVector(
                bKraken ? 0.20f : 0.78f));
        }
        else if (bElephant)
        {
            // Native bounds are only about 1.3 x 1.7 x 1.1 metres. Three-times
            // scale gives the tusked elephant a five-metre battlefield footprint
            // comparable to the other elite creatures without making it a kaiju.
            Visual->SetRelativeLocation(FVector(0, 0, -440.0f));
            Visual->SetRelativeRotation(FRotator(0, -90.0f, 0));
            Visual->SetRelativeScale3D(FVector(3.0f));
        }
        else
        {
            Visual->SetRelativeLocation(FVector(0, 0, -90.0f));
            Visual->SetRelativeRotation(FRotator(0, -90.0f, 0));
        }
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Visual->PlayAnimation(Idle, true);
        if (FormationRole == ESoulRealtimeFormationRole::Apex ||
            bKraken || bElephant)
        {
            const TCHAR* Creature = bElephant ? TEXT("Elephant") :
                (bKraken ? TEXT("Kraken") :
                    (Side == 0 ? TEXT("Griffin") : TEXT("MountainDragon")));
            const float CreatureScale = bElephant ? 3.0f :
                (bKraken ? 0.20f : (Side == 0 ? 0.55f : 0.28f));
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_LARGE_UNIT_DEPLOYED: type=%s scale=%.2f health=%.0f group=%d"),
                Creature, CreatureScale, Data.Health, GroupIndex);
        }
    }
    else
    {
        UStaticMeshComponent* Body =
            NewObject<UStaticMeshComponent>(Actor);
        Actor->AddInstanceComponent(Body);
        Body->SetupAttachment(Actor->GetRootComponent());
        Body->SetStaticMesh(Cube);
        FVector Scale(0.42f, 0.42f, 1.25f);
        if (FormationRole == ESoulRealtimeFormationRole::Apex)
            Scale = FVector(0.78f, 0.78f, 1.8f);
        else if (FormationRole == ESoulRealtimeFormationRole::Hero)
            Scale = FVector(0.52f, 0.52f, 1.45f);
        else if (FormationRole == ESoulRealtimeFormationRole::Ranged)
            Scale = FVector(0.38f, 0.38f, 1.15f);
        Body->SetRelativeScale3D(Scale);
        Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->RegisterComponent();
    }
    UTextRenderComponent* Label =
        NewObject<UTextRenderComponent>(Actor);
    Actor->AddInstanceComponent(Label);
    Label->SetupAttachment(Actor->GetRootComponent());
    Label->SetRelativeLocation(FVector(
        0, 0, Data.Movement == ESoulRealtimeMovementArchetype::Aerial
            ? 430.0f : 150.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(24.0f);
    Label->SetText(FText::FromString(FString::Printf(
        TEXT("%s %s"),
        Side == 0 ? TEXT("H") : TEXT("E"),
        *RoleLabel(FormationRole))));
    Label->SetTextRenderColor(
        bPlayer ? FColor::Yellow :
        (Side == 0 ? FColor::Cyan : FColor::Red));
    Label->SetVisibility(!bVisualUnits);
    Label->RegisterComponent();

    USoulRealtimeArenaBinding* Binding =
        NewObject<USoulRealtimeArenaBinding>(Actor);
    Actor->AddInstanceComponent(Binding);
    Binding->RegisterComponent();

    Bindings[NewIndex] = Binding;

    if (!Binding->BindCombatant(IdentityAt(NewIndex).Core()))
        return false;
    Binding->SetGuardConfiguration(0.65f, 0.30f, 0.0f);
    if (Data.bRanged)
    {
        URBCombatRangedComponent* Bow =
            NewObject<URBCombatRangedComponent>(Actor);
        Actor->AddInstanceComponent(Bow);
        Bow->RegisterComponent();
        FRBCombatBowSettings Settings;
        Settings.Ammunition = TEXT("Soul.Arrow");
        Settings.MinimumDrawSeconds = 0.14f;
        Settings.FullDrawSeconds = 0.55f;
        Settings.MinimumSpeed = 1750.0f;
        Settings.MaximumSpeed = 3100.0f;
        Settings.GravityScale = 0.35f;
        Settings.LifetimeSeconds = 4.0f;
        Settings.Radius = 3.0f;
        if (!Bow->ConfigureBow(Settings)) return false;
        Ranged[NewIndex] = Bow;
    }

    if (bPlayer)
    {
        PlayerHero = Actor;
        USpringArmComponent* Arm =
            NewObject<USpringArmComponent>(Actor);
        Actor->AddInstanceComponent(Arm);
        Arm->SetupAttachment(Actor->GetRootComponent());
        Arm->TargetArmLength = 620.0f;
        Arm->SetRelativeRotation(FRotator(-32, 0, 0));
        Arm->bUsePawnControlRotation = true;
        Arm->RegisterComponent();

        UCameraComponent* Camera =
            NewObject<UCameraComponent>(Actor);
        Actor->AddInstanceComponent(Camera);
        Camera->SetupAttachment(
            Arm, USpringArmComponent::SocketName);
        Camera->bUsePawnControlRotation = false;
        Camera->RegisterComponent();
        Camera->SetActive(true);
    }
    else
    {
        AAIController* AI = GetWorld()->SpawnActor<AAIController>();
        if (!AI) return false;
        AI->Possess(Actor);
        if (AI->GetPawn() != Actor)
        {
            AI->Destroy();
            return false;
        }
    }
    if (!Spatial || !Spatial->RegisterUnit(Actor, Side == 1))
    {
        UE_LOG(LogTemp, Error, TEXT("SOUL_PBIL_REGISTER_FAIL: side=%d id=%s"), Side, *Data.Id.ToString());
        return false;
    }
    if (Side == 0 && !PlayerHero) PlayerHero = Actor;
    return Actors.Num() == Combatants.Num() &&
        Bindings.Num() == Combatants.Num() &&
        Ranged.Num() == Combatants.Num() &&
        VisualRunning.Num() == Combatants.Num();
}

bool ASoulRealtimeArenaGameMode::SetupDrivers()
{
    for (int32 I = 0; I < Groups.Num(); ++I)
    {
        USoulRealtimeArenaGroupDriver* Driver =
            NewObject<USoulRealtimeArenaGroupDriver>(this);
        if (!Driver) return false;
        AddInstanceComponent(Driver);
        Driver->GroupIndex = I;
        Driver->FormationSpacing = 105.0f;
        Driver->SightDistance = 6200.0f;
        Driver->bAllowDirectSteering = true;
        Driver->RegisterComponent();
        Drivers.Add(Driver);
    }
    return Drivers.Num() == Groups.Num() &&
        RefreshDriverRepresentations();
}

bool ASoulRealtimeArenaGameMode::RefreshDriverRepresentations()
{
    TArray<URBVariantCombatBindingComponent*> Reps;
    Reps.Reserve(Bindings.Num());
    for (int32 I = 0; I < Bindings.Num(); ++I)
        if (IsValid(Bindings[I]) && Combatants.IsValidIndex(I) && Combatants[I].Health > 0)
            Reps.Add(Bindings[I]);

    for (USoulRealtimeArenaGroupDriver* Driver : Drivers)
    {
        if (!Driver || !Driver->SetRepresentations(Reps))
            return false;
    }
    return true;
}

void ASoulRealtimeArenaGameMode::SetupReinforcementState()
{
    ReinforcementBattle = FSoulRealtimeBattleState();
    ReinforcementBattle.MaxActivePerSide = ActiveCap;
    ReinforcementBattle.ReinforcementTriggerPermille = 700;
    ReinforcementBattle.MaxWaveSize = FMath::Min(4, ActiveCap);
    ReinforcementBattle.bReinforcementsEnabled = true;

    for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
    {
        const int32 LeaderIndex = Index(Groups[GroupIndex].Leader);
        if (LeaderIndex == INDEX_NONE) continue;

        const int32 Side = Combatants[LeaderIndex].Side;
        FSoulRealtimeFormation Formation;
        Formation.FormationId = GroupFormationId(Side, GroupIndex);
        Formation.SideId = RealtimeSideId(Side);
        Formation.UnitId = FName(*RoleLabel(Combatants[LeaderIndex].Role));
        Formation.Role = Combatants[LeaderIndex].Role;
        Formation.StrategicCount = Groups[GroupIndex].Members.Num();
        Formation.ActiveCount = Formation.StrategicCount;
        Formation.ReserveCount = 0;
        Formation.MaxActiveRepresentations =
            FMath::Max(1, Formation.ActiveCount);
        ReinforcementBattle.Formations.Add(Formation);

        for (const FRBHostIdentity& Member : Groups[GroupIndex].Members)
        {
            const int32 MemberIndex = Index(Member);
            if (MemberIndex != INDEX_NONE)
                Combatants[MemberIndex].FormationId = Formation.FormationId;
        }
    }

    for (int32 Side = 0; Side < 2; ++Side)
    {
        FSoulRealtimeFormation Reserve;
        Reserve.FormationId = ReserveFormationId(Side);
        Reserve.SideId = RealtimeSideId(Side);
        Reserve.UnitId = TEXT("Soul.Reserve.Line");
        Reserve.Role = ESoulRealtimeFormationRole::Line;
        Reserve.StrategicCount = FMath::Max(0, StrategicBodies[Side] - AliveForSide(Side));
        Reserve.ActiveCount = 0;
        Reserve.ReserveCount = Reserve.StrategicCount;
        Reserve.MaxActiveRepresentations = ActiveCap;
        ReinforcementBattle.Formations.Add(Reserve);
        InitialStrategic[Side] = StrategicBodies[Side];
    }

    Status = TEXT("Strategic reserves ready");
    UE_LOG(LogTemp, Display,
        TEXT("SOUL_RT_RESERVES_READY: human=%d enemy=%d"),
        ReserveBodiesForSide(0), ReserveBodiesForSide(1));
}

FVector ASoulRealtimeArenaGameMode::ChooseReinforcementAnchor(
    int32 Side) const
{
    const float X = Side == 0
        ? -BattlefieldHalfX + 260.0f
        : BattlefieldHalfX - 260.0f;
    const float YOptions[] = {-1400.0f, 0.0f, 1400.0f};

    FVector FriendlyCenter = ArenaOrigin;
    int32 FriendlyGroups = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != Side || AliveInGroup(State.GroupIndex) <= 0)
            continue;
        FriendlyCenter += GroupCenter(State.GroupIndex) - ArenaOrigin;
        ++FriendlyGroups;
    }
    if (FriendlyGroups > 0)
        FriendlyCenter = ArenaOrigin +
            (FriendlyCenter - ArenaOrigin) / FriendlyGroups;

    FVector Best = ArenaOrigin + FVector(X, 0, 100);
    int32 BestScore = MIN_int32;
    float FarthestEnemy = -1.0f;
    FVector Farthest = Best;
    for (const float Y : YOptions)
    {
        const FVector Candidate = ArenaOrigin + FVector(X, Y, 100);
        float NearestEnemy = TNumericLimits<float>::Max();
        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            if (!Actors.IsValidIndex(I) || !Actors[I] ||
                Combatants[I].Health <= 0.0f ||
                Combatants[I].Side == Side)
                continue;
            NearestEnemy = FMath::Min(NearestEnemy,
                FVector::Dist2D(Candidate, Actors[I]->GetActorLocation()));
        }
        if (NearestEnemy > FarthestEnemy)
        {
            FarthestEnemy = NearestEnemy;
            Farthest = Candidate;
        }
        const int32 Score =
            FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(
                NearestEnemy,
                FVector::Dist2D(Candidate, FriendlyCenter),
                0.0f, true);
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Candidate;
        }
    }
    return BestScore == MIN_int32 ? Farthest : Best;
}

bool ASoulRealtimeArenaGameMode::SpawnReinforcementWave(
    int32 Side, int32 Count)
{
    if (Side < 0 || Side > 1 ||
        Count <= 0 || Count > FRBCombatGroup::MaximumMembers)
        return false;

    const int32 FirstCombatant = Combatants.Num();
    const int32 NewGroupIndex = Groups.Num();
    const int32 FirstDriver = Drivers.Num();
    const TWeakObjectPtr<ACharacter> PreviousHero(PlayerHero);
    bool bSpawnCommitted = false;
    ON_SCOPE_EXIT
    {
        if (!bSpawnCommitted)
        {
            RollbackSpawnedFormation(FirstCombatant, NewGroupIndex, FirstDriver);
            PlayerHero = PreviousHero.Get();
        }
    };
    const FVector ArrivalAnchor =
        ChooseReinforcementAnchor(Side);
    if (!SpawnFormation(
            Side,
            ESoulRealtimeFormationRole::Line,
            Count,
            ArrivalAnchor))
        return false;

    FSoulBattleFormationState Tactical;
    Tactical.GroupIndex = NewGroupIndex;
    Tactical.Side = Side;
    Tactical.Kind = ESoulBattleFormationKind::FrontLine;
    Tactical.SpawnAnchor = ArrivalAnchor;
    Tactical.TacticalAnchor = ArrivalAnchor +
        FVector(Side == 0 ? 700.0f : -700.0f, 0, 0);
    Tactical.InitialBodies = Count;
    Tactical.PreviousAlive = Count;
    Tactical.MoralePermille = 1000;
    TacticalFormations.Add(Tactical);

    const FName FormationId = ReserveFormationId(Side);
    for (int32 I = FirstCombatant; I < Combatants.Num(); ++I)
        Combatants[I].FormationId = FormationId;

    USoulRealtimeArenaGroupDriver* Driver =
        NewObject<USoulRealtimeArenaGroupDriver>(this);
    if (!Driver) return false;
    AddInstanceComponent(Driver);
    Driver->GroupIndex = NewGroupIndex;
    Driver->FormationSpacing = 105.0f;
    Driver->SightDistance = 6200.0f;
    Driver->bAllowDirectSteering = true;
    Driver->RegisterComponent();
    Drivers.Add(Driver);
    bSpawnCommitted = RefreshDriverRepresentations();
    return bSpawnCommitted;
}

void ASoulRealtimeArenaGameMode::TickReinforcements()
{
    if (ReinforcementBattle.Formations.IsEmpty()) return;

    for (int32 Side = 0; Side < 2; ++Side)
    {
        const FName SideId = RealtimeSideId(Side);
        if (!FSoulRealtimeBattleRules::ShouldReinforce(
                ReinforcementBattle, SideId))
            continue;

        FSoulRealtimeBattleState Candidate = ReinforcementBattle;
        const FSoulReinforcementWave Wave =
            FSoulRealtimeBattleRules::BuildAndApplyWave(
                Candidate, SideId);
        const int32 Count =
            Wave.FormationCounts.FindRef(ReserveFormationId(Side));
        if (Count <= 0 || Count != Wave.TotalBodies())
            continue;

        if (SpawnReinforcementWave(Side, Count))
        {
            ReinforcementBattle = MoveTemp(Candidate);
            ++ReinforcementWaves[Side];
            LastReinforcementBodies[Side] = Count;
            Status = FString::Printf(
                TEXT("%s reinforcements arrived: %d"),
                Side == 0 ? TEXT("Human") : TEXT("Enemy"),
                Count);
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_RT_REINFORCEMENT_WAVE: side=%d bodies=%d wave=%d"),
                Side, Count, ReinforcementWaves[Side]);
        }
        else
        {
            // Candidate was never committed and all partial physical state was rolled back.
            // A content/driver failure is actionable; do not retry the same failure every tick.
            UE_LOG(LogTemp, Error,
                TEXT("SOUL_RT_REINFORCEMENT_FAILED: side=%d bodies=%d rollback=1 reserveUnchanged=1"),
                Side, Count);
            Status = TEXT("Battle stopped: reinforcement setup failed; reserve state preserved");
            if (bQualification)
            {
                FinishProof(false, Status);
            }
            else
            {
                bFinished = true;
                for (USoulRealtimeArenaGroupDriver* Driver : Drivers)
                {
                    if (Driver) Driver->SetComponentTickEnabled(false);
                }
                for (ACharacter* Actor : Actors)
                {
                    if (!IsValid(Actor)) continue;
                    Actor->GetCharacterMovement()->StopMovementImmediately();
                    if (AAIController* AI = Cast<AAIController>(Actor->GetController())) AI->StopMovement();
                }
            }
            return;
        }
    }
}
int32 ASoulRealtimeArenaGameMode::AcceptedContactCount() const
{
    return AcceptedContacts.Num();
}

void ASoulRealtimeArenaGameMode::ObserveDecision(
    int32 GroupIndex,
    FRBHostIdentity Identity,
    const FRBHostDecision& Decision)
{
    if (Decision.Intent != ERBHostCombatIntent::Attack ||
        !Decision.Target.Core().IsValid())
        return;

    const int32 AttackerIndex = Index(Identity);
    if (AttackerIndex == INDEX_NONE) return;
    PerformMelee(AttackerIndex, Decision.Target);
}

bool ASoulRealtimeArenaGameMode::PerformMelee(
    int32 AttackerIndex, FRBHostIdentity IntendedTarget)
{
    const int32 IntendedIndex = Index(IntendedTarget);
    if (!Combatants.IsValidIndex(AttackerIndex) ||
        IntendedIndex == INDEX_NONE ||
        !AreOpponents(IdentityAt(AttackerIndex), IntendedTarget))
        return false;
    auto& AttackerData = Combatants[AttackerIndex];
    if (AttackerData.Health <= 0.0f ||
        AttackerData.MeleeCooldown > 0.0f ||
        AttackerData.bRanged)
        return false;

    ACharacter* Attacker = Actors[AttackerIndex];
    ACharacter* Intended = Actors[IntendedIndex];
    if (!Attacker || !Intended) return false;

    const FVector Direction =
        (Intended->GetActorLocation() -
         Attacker->GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return false;

    const FRBWeaponProfile Weapon = Profile(AttackerIndex);
    const float Distance = FVector::Dist2D(
        Attacker->GetActorLocation(), Intended->GetActorLocation());
    if (Distance > Weapon.Reach + 95.0f) return false;

    Attacker->SetActorRotation(Direction.Rotation());

    const FVector Start =
        Attacker->GetActorLocation() +
        Direction * 35.0f + FVector(0, 0, 45);
    const FVector End =
        Start + Direction * (Weapon.Reach + 85.0f);
    FCollisionQueryParams Query(
        SCENE_QUERY_STAT(SoulRealtimeArenaMelee),
        false, Attacker);
    FHitResult Hit;
    if (!GetWorld()->SweepSingleByChannel(
            Hit, Start, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeSphere(18.0f), Query))
        return false;

    AActor* HitActor = Hit.GetActor();
    auto* VictimBinding = HitActor
        ? HitActor->FindComponentByClass<USoulRealtimeArenaBinding>()
        : nullptr;
    if (!VictimBinding) return false;

    const FRBHostIdentity Victim =
        FRBHostIdentity::From(VictimBinding->GetCombatant());
    if (!AreOpponents(IdentityAt(AttackerIndex), Victim))
        return false;

    USoulRealtimeArenaBinding* AttackerBinding =
        Bindings[AttackerIndex];
    FRBCombatHit Evidence;
    Evidence.ContactId =
        AttackerBinding->BeginNativeAttackContact();
    Evidence.Attacker = AttackerBinding->GetCombatant();
    Evidence.Victim = VictimBinding->GetCombatant();
    Evidence.Weapon = Weapon;
    Evidence.AcceptedDamage = Weapon.BaseDamage;
    Evidence.Bleeding = Weapon.BleedingTendency;
    Evidence.StaggerSeconds = Weapon.StaggerSeconds;
    Evidence.ImpactPoint = Hit.ImpactPoint;
    Evidence.bHasExactImpact = true;

    FString Error;
    const bool bAccepted = VictimBinding->ReceiveProducedImpact(
        Evidence, Hit, Direction, Attacker, Error);
    AttackerBinding->EndNativeAttackContact();
    AttackerData.MeleeCooldown =
        AttackDelay(AttackerData.Role);

    if (bAccepted && bVisualUnits)
    {
        UAnimSingleNodeInstance* Playback = Attacker->GetMesh()->GetSingleNodeInstance();
        if (!AttackerData.bVisualAttackPlaying || !Playback || !Playback->IsPlaying())
        {
            if (auto* Attack = ResolveVisualAttack(AttackerData.Side, AttackerData.Role))
            {
                Attacker->GetMesh()->PlayAnimation(Attack, false);
                // Show the entire existing clip within the existing combat cadence; never truncate
                // it with a guessed cooldown threshold or replace RB Combat's hit authority.
                const float VisualDuration = FMath::Max(0.1f, AttackDelay(AttackerData.Role) * 0.95f);
                Attacker->GetMesh()->SetPlayRate(FMath::Max(0.1f, Attack->GetPlayLength() / VisualDuration));
                AttackerData.bVisualAttackPlaying = true;
            }
        }
    }

    if (bAccepted && !bProof)
    {
        Status = FString::Printf(
            TEXT("%s %s contact accepted"),
            AttackerData.Side == 0 ? TEXT("Human") : TEXT("Enemy"),
            *RoleLabel(AttackerData.Role));
    }
    return bAccepted;
}

void ASoulRealtimeArenaGameMode::SelectNextAlliedFormation()
{
    TArray<int32> AlliedGroups;
    for (const FSoulBattleFormationState& State : TacticalFormations)
        if (State.Side == 0 && AliveInGroup(State.GroupIndex) > 0)
            AlliedGroups.Add(State.GroupIndex);

    if (AlliedGroups.IsEmpty())
    {
        bSelectAllAllies = true;
        SelectedAlliedFormation = INDEX_NONE;
        return;
    }
    if (bSelectAllAllies)
    {
        bSelectAllAllies = false;
        SelectedAlliedFormation = AlliedGroups[0];
    }
    else
    {
        const int32 Current = AlliedGroups.IndexOfByKey(
            SelectedAlliedFormation);
        if (Current == INDEX_NONE || Current + 1 >= AlliedGroups.Num())
        {
            bSelectAllAllies = true;
            SelectedAlliedFormation = INDEX_NONE;
        }
        else SelectedAlliedFormation = AlliedGroups[Current + 1];
    }
    Status = bSelectAllAllies
        ? TEXT("Selected all allied formations")
        : FString::Printf(TEXT("Selected formation %d"), SelectedAlliedFormation + 1);
}

void ASoulRealtimeArenaGameMode::CommandSelectedAllies(
    ERBHostGroupOrder Order)
{
    int32 Changed = 0;
    int32 Eligible = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != 0 || AliveInGroup(State.GroupIndex) <= 0 ||
            (!bSelectAllAllies &&
             State.GroupIndex != SelectedAlliedFormation))
            continue;
        ++Eligible;

        const FVector Center = GroupCenter(State.GroupIndex);
        float EnemyDistance = 0.0f;
        const int32 Enemy = FindNearestEnemyToGroup(
            State.GroupIndex, EnemyDistance);
        const FVector EnemyLocation = Enemy != INDEX_NONE
            ? Actors[Enemy]->GetActorLocation()
            : Center + FVector::ForwardVector * 500.0f;
        const FVector Facing =
            (EnemyLocation - Center).GetSafeNormal2D();
        FVector Anchor = Center;
        if (Order == ERBHostGroupOrder::Advance)
            Anchor = Center + Facing * 600.0f;
        else if (Order == ERBHostGroupOrder::Charge)
            Anchor = EnemyLocation;
        else if (Order == ERBHostGroupOrder::FallBack)
            Anchor = Center - Facing * 650.0f;
        if (IssueFormationOrder(
                State.GroupIndex, Order, Anchor, Facing, true))
            ++Changed;
    }

    const TCHAR* Label = TEXT("FACE");
    switch (Order)
    {
        case ERBHostGroupOrder::Hold: Label = TEXT("HOLD"); break;
        case ERBHostGroupOrder::Advance: Label = TEXT("ADVANCE"); break;
        case ERBHostGroupOrder::Charge: Label = TEXT("CHARGE"); break;
        case ERBHostGroupOrder::FallBack: Label = TEXT("FALL BACK"); break;
        default: break;
    }
    Status = FString::Printf(TEXT("%s order accepted by %d/%d formation(s)"),
        Label, Changed, Eligible);
}

void ASoulRealtimeArenaGameMode::ToggleAlliedOrders()
{
    bAlliedCharge = !bAlliedCharge;
    bSelectAllAllies = true;
    SelectedAlliedFormation = INDEX_NONE;
    CommandSelectedAllies(bAlliedCharge
        ? ERBHostGroupOrder::Charge : ERBHostGroupOrder::Hold);
}

void ASoulRealtimeArenaGameMode::UpdateDefeatedRepresentations()
{
    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        if (Combatants[I].Health > 0.0f ||
            DefeatedRepresentations.Contains(I) ||
            !Actors.IsValidIndex(I) || !Actors[I])
            continue;

        if (!Combatants[I].FormationId.IsNone() &&
            !ReinforcementBattle.Formations.IsEmpty())
        {
            FSoulRealtimeBattleRules::ApplyCasualties(
                ReinforcementBattle,
                Combatants[I].FormationId,
                1);
        }

        DefeatedRepresentations.Add(I);
        const int32 GroupIndex = Combatants[I].GroupIndex;
        if (Groups.IsValidIndex(GroupIndex) && Groups[GroupIndex].Leader.Id == Combatants[I].Id)
        {
            for (const auto& Member : Groups[GroupIndex].Members)
            {
                const int32 Next = Index(Member);
                if (Next != INDEX_NONE && Combatants[Next].Health > 0)
                {
                    Groups[GroupIndex].Leader = Member;
                    ++Groups[GroupIndex].Revision;
                    break;
                }
            }
        }
        ACharacter* Actor = Actors[I];
        Actor->SetActorEnableCollision(false);
        if (Spatial) Spatial->UnregisterUnit(Actor);
        Combatants[I].bVisualAttackPlaying = false;
        Actor->GetMesh()->bPauseAnims = true;
        if (bVisualUnits)
        {
            if (auto* Death = ResolveVisualDeath(Combatants[I].Side, Combatants[I].Role))
            {
                Actor->GetMesh()->bPauseAnims = false;
                Actor->GetMesh()->PlayAnimation(Death, false);
                Actor->GetMesh()->SetPlayRate(1.0f);
            }
            else Actor->GetMesh()->SetRelativeRotation(FRotator(0, -90, 85));
        }
        else Actor->GetMesh()->SetRelativeRotation(FRotator(0, -90, 85));
        UE_LOG(LogTemp, Display, TEXT("SOUL_UNIT_DEFEATED: side=%d id=%s"), Combatants[I].Side, *Combatants[I].Id.ToString());
        Actor->GetCharacterMovement()->DisableMovement();
        if (AAIController* AI = Cast<AAIController>(Actor->GetController()))
        {
            AI->StopMovement();
            AI->SetActorTickEnabled(false);
        }
        const TWeakObjectPtr<ACharacter> Corpse(Actor);
        FTimerHandle Cleanup;
        GetWorld()->GetTimerManager().SetTimer(Cleanup, [Corpse]()
        {
            if (!Corpse.IsValid()) return;
            Corpse->SetActorHiddenInGame(true);
            Corpse->GetMesh()->SetComponentTickEnabled(false);
            Corpse->SetActorTickEnabled(false);
        }, 6.0f, false);
    }
}
int32 ASoulRealtimeArenaGameMode::FindPlayerSpellTarget(
    float Range) const
{
    if (!PlayerHero || Range <= 0.0f) return INDEX_NONE;
    const FVector Origin = PlayerHero->GetActorLocation();
    const FVector Aim =
        PlayerHero->GetActorForwardVector().GetSafeNormal2D();
    int32 Best = INDEX_NONE;
    double BestScore = -TNumericLimits<double>::Max();

    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        if (Combatants[I].Side == 0 ||
            Combatants[I].Health <= 0.0f ||
            !Actors.IsValidIndex(I) || !Actors[I])
            continue;
        const FVector Delta = Actors[I]->GetActorLocation() - Origin;
        const double Distance = Delta.Size2D();
        if (Distance > Range || Distance <= KINDA_SMALL_NUMBER)
            continue;
        const double Facing = FVector::DotProduct(
            Aim, Delta.GetSafeNormal2D());
        if (Facing < 0.15) continue;
        const double Score =
            Facing * 2.0 + (1.0 - Distance / Range) * 0.5;
        if (Score > BestScore)
        {
            Best = I;
            BestScore = Score;
        }
    }
    return Best;
}

void ASoulRealtimeArenaGameMode::SpawnSpellPresentation(
    const URBMagicPresentationProfile* Profile,
    const FVector& Location)
{
    if (!Profile || !GetWorld()) return;
    UNiagaraSystem* System = nullptr;
    if (!Profile->PersistentSystem.IsNull())
        System = Profile->PersistentSystem.LoadSynchronous();
    if (!System && !Profile->ImpactSystem.IsNull())
        System = Profile->ImpactSystem.LoadSynchronous();
    if (!System && !Profile->CastSystem.IsNull())
        System = Profile->CastSystem.LoadSynchronous();
    if (!System && !Profile->ProjectileSystem.IsNull())
        System = Profile->ProjectileSystem.LoadSynchronous();
    if (System)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(), System, Location);
        return;
    }

    // The owned Paragon hero packs include production spell particles even
    // when a matching Niagara provider pack is not mounted. Keep gameplay
    // authority in RB Magic and use these licensed systems as presentation.
    const FString Tag = Profile->SpellTag.ToString();
    const TCHAR* FallbackPath = nullptr;
    if (Tag == TEXT("Magic.Spell.Electric.ChainLightning"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Primary/FX/P_Aurora_Melee_SucessfulImpact.P_Aurora_Melee_SucessfulImpact");
    else if (Tag == TEXT("Magic.Spell.Ice.Blizzard"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_Ultimate_InitialBlast.P_Aurora_Ultimate_InitialBlast");
    else if (Tag == TEXT("Magic.Spell.Water.TidalWard"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Rooted.P_Aurora_Freeze_Rooted");
    else if (Tag == TEXT("Magic.Spell.Air.Tailwind"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Dash_WarmUp.P_Aurora_Dash_WarmUp");
    else if (Tag == TEXT("Magic.Spell.Earth.StoneSentinel"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Wall_Base.P_Aurora_Wall_Base");

    if (FallbackPath)
    {
        if (UParticleSystem* Fallback = LoadObject<UParticleSystem>(
                nullptr, FallbackPath, nullptr, LOAD_NoWarn))
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(), Fallback, FTransform(Location));
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_MAGIC_PARAGON_PRESENTATION: spell=%s"), *Tag);
        }
    }
}

bool ASoulRealtimeArenaGameMode::CastPlayerSpell(
    const TCHAR* SpellPath,
    const TCHAR* PresentationPath)
{
    auto* Spell = LoadObject<URBMagicSpellDefinition>(
        nullptr, SpellPath);
    if (!Spell || !IsValid(PlayerHero))
    {
        Status = TEXT("Spell definition or living caster unavailable");
        return false;
    }

    const auto* Binding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 CasterIndex = Binding
        ? Index(FRBHostIdentity::From(Binding->GetCombatant()))
        : INDEX_NONE;
    if (CasterIndex == INDEX_NONE)
    {
        Status = TEXT("Player caster unavailable");
        return false;
    }

    FRBMagicCastRequest Request;
    Request.CastId = FGuid::NewGuid();
    Request.SpellTag = Spell->SpellTag;
    Request.Caster.Domain = MagicArenaDomain;
    Request.Caster.Id = Combatants[CasterIndex].Id;

    FVector PresentationLocation = PlayerHero->GetActorLocation();
    if (Spell->TargetMode == ERBMagicTargetMode::Unit)
    {
        bool bFriendlyUnitSpell = false;
        for (const FRBMagicEffectSpec& Effect : Spell->Effects)
        {
            bFriendlyUnitSpell = bFriendlyUnitSpell ||
                Effect.EffectTag.ToString() == TEXT("Magic.Effect.Shield");
        }
        const int32 TargetIndex = bFriendlyUnitSpell
            ? CasterIndex : FindPlayerSpellTarget(Spell->Range);
        if (TargetIndex == INDEX_NONE)
        {
            Status = TEXT("No valid unit in spell targeting arc");
            return false;
        }
        Request.Target.Entity.Domain = MagicArenaDomain;
        Request.Target.Entity.Id = Combatants[TargetIndex].Id;
        Request.Target.WorldLocation = Actors[TargetIndex]->GetActorLocation();
        PresentationLocation = Request.Target.WorldLocation;
    }
    else if (Spell->TargetMode == ERBMagicTargetMode::Ground)
    {
        const float Distance =
            FMath::Min(1600.0f, FMath::Max(600.0f, Spell->Range * 0.65f));
        Request.Target.WorldLocation =
            PlayerHero->GetActorLocation() +
            PlayerHero->GetActorForwardVector().GetSafeNormal2D() * Distance;
        Request.Target.WorldLocation.Z = 10.0f;
        PresentationLocation = Request.Target.WorldLocation;
    }
    else if (Spell->TargetMode == ERBMagicTargetMode::Direction)
    {
        Request.Target.Direction =
            PlayerHero->GetActorForwardVector().GetSafeNormal();
    }

    TArray<FRBMagicEffectIntent> Effects;
    FString Error;
    if (!URBMagicLibrary::BuildEffectIntents(
            Spell, Request, Effects, Error) ||
        !TryCommitMagicCast(*Spell, Request, Effects, Error))
    {
        Status = Error.IsEmpty() ? TEXT("Magic cast rejected") : Error;
        return false;
    }

    // RB Magic intents and the existing commit path own gameplay. Presentation
    // may be unassigned during spell development and cannot veto a valid cast.
    auto* Profile = PresentationPath && *PresentationPath
        ? LoadObject<URBMagicPresentationProfile>(nullptr, PresentationPath, nullptr, LOAD_NoWarn)
        : nullptr;
    if (Profile && Profile->SpellTag == Spell->SpellTag)
        SpawnSpellPresentation(Profile, PresentationLocation);
    else
        UE_LOG(LogTemp, Display, TEXT("SOUL_MAGIC_PRESENTATION_UNAVAILABLE: spell=%s gameplayCommitted=1"),
            *Spell->SpellTag.ToString());
    return true;
}

void ASoulRealtimeArenaGameMode::TickMagic(float Seconds)
{
    for (auto& Pair : SpellCooldowns)
        Pair.Value = FMath::Max(0.0f, Pair.Value - Seconds);
    for (int32 Side = 0; Side < 2; ++Side)
    {
        SpeedBuffSeconds[Side] = FMath::Max(
            0.0f, SpeedBuffSeconds[Side] - Seconds);
        if (SpeedBuffSeconds[Side] <= 0.0f)
            SpeedBuffMultiplier[Side] = 1.0f;
    }

    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        FSoulRealtimeArenaCombatant& Combatant = Combatants[I];
        Combatant.WardSeconds = FMath::Max(
            0.0f, Combatant.WardSeconds - Seconds);
        if (Combatant.WardSeconds <= 0.0f)
            Combatant.WardPoints = 0.0f;
        if (Combatant.Health <= 0.0f ||
            !Actors.IsValidIndex(I) || !Actors[I])
            continue;
        Actors[I]->GetCharacterMovement()->MaxWalkSpeed =
            RoleWalkSpeed(Combatant.Role) *
            SpeedBuffMultiplier[Combatant.Side];
    }

    for (int32 AreaIndex = ActiveMagicAreas.Num() - 1;
         AreaIndex >= 0; --AreaIndex)
    {
        FSoulRealtimeArenaMagicArea& Area =
            ActiveMagicAreas[AreaIndex];
        Area.RemainingSeconds -= Seconds;
        Area.TickAccumulator += Seconds;

        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            if (Combatants[I].Side == Area.SourceSide ||
                Combatants[I].Health <= 0.0f ||
                !Actors.IsValidIndex(I) || !Actors[I])
                continue;
            if (FVector::DistSquared2D(
                    Area.Center, Actors[I]->GetActorLocation()) >
                FMath::Square(Area.Radius))
                continue;
            if (Area.SlowFraction > 0.0f)
            {
                Actors[I]->GetCharacterMovement()->MaxWalkSpeed =
                    RoleWalkSpeed(Combatants[I].Role) *
                    SpeedBuffMultiplier[Combatants[I].Side] *
                    (1.0f - Area.SlowFraction);
            }
        }

        while (Area.TickAccumulator >= 1.0f)
        {
            Area.TickAccumulator -= 1.0f;
            for (int32 I = 0; I < Combatants.Num(); ++I)
            {
                if (Combatants[I].Side == Area.SourceSide ||
                    Combatants[I].Health <= 0.0f ||
                    !Actors.IsValidIndex(I) || !Actors[I])
                    continue;
                if (FVector::DistSquared2D(
                        Area.Center, Actors[I]->GetActorLocation()) <=
                    FMath::Square(Area.Radius))
                    ApplyMagicDamage(I, Area.DamagePerTick);
            }
        }

        if (Area.RemainingSeconds <= 0.0f)
            ActiveMagicAreas.RemoveAtSwap(AreaIndex);
    }
}

void ASoulRealtimeArenaGameMode::PlayerTick(float Seconds)
{
    if (!PlayerHero || PlayerHealth() <= 0.0f) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PC->GetInputMouseDelta(MouseX, MouseY);
    PC->AddYawInput(MouseX * 0.45f);

    const FRotator YawRotation(
        0, PC->GetControlRotation().Yaw, 0);
    const FVector Forward =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    const float ForwardInput =
        (PC->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) -
        (PC->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
    const float RightInput =
        (PC->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) -
        (PC->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);
    PlayerHero->AddMovementInput(Forward, ForwardInput);
    PlayerHero->AddMovementInput(Right, RightInput);

    auto* PlayerBinding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 PlayerIndex = PlayerBinding
        ? Index(FRBHostIdentity::From(PlayerBinding->GetCombatant()))
        : INDEX_NONE;
    if (PlayerIndex == INDEX_NONE) return;

    PlayerBinding->SetGuardIntent(
        PC->IsInputKeyDown(EKeys::RightMouseButton));

    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        int32 Best = INDEX_NONE;
        double BestDistance = 340.0 * 340.0;
        const FVector PlayerLocation = PlayerHero->GetActorLocation();
        const FVector Aim = PlayerHero->GetActorForwardVector();
        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            if (Combatants[I].Side == 0 ||
                Combatants[I].Health <= 0.0f || !Actors[I])
                continue;
            const FVector Delta =
                Actors[I]->GetActorLocation() - PlayerLocation;
            const double Distance = Delta.SizeSquared2D();
            if (Distance >= BestDistance ||
                FVector::DotProduct(
                    Aim.GetSafeNormal2D(),
                    Delta.GetSafeNormal2D()) < 0.15f)
                continue;
            Best = I;
            BestDistance = Distance;
        }
        if (Best != INDEX_NONE)
            PerformMelee(PlayerIndex, IdentityAt(Best));
        else
            Status = TEXT("No enemy in melee arc");
    }

    if (PC->WasInputKeyJustPressed(EKeys::One))
        CastPlayerSpell(
            TEXT("/Game/Soul/Magic/Spells/DA_Soul_Firebolt.DA_Soul_Firebolt"),
            TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Firebolt.DA_SoulPresentation_Firebolt"));
    if (PC->WasInputKeyJustPressed(EKeys::Two))
        CastPlayerSpell(
            TEXT("/Game/Soul/Magic/Spells/DA_Soul_ChainLightning.DA_Soul_ChainLightning"),
            TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_ChainLightning.DA_SoulPresentation_ChainLightning"));
    if (PC->WasInputKeyJustPressed(EKeys::Three))
        CastPlayerSpell(
            TEXT("/Game/Soul/Magic/Spells/DA_Soul_Blizzard.DA_Soul_Blizzard"),
            TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Blizzard.DA_SoulPresentation_Blizzard"));
    if (PC->WasInputKeyJustPressed(EKeys::Four))
        CastPlayerSpell(
            TEXT("/Game/Soul/Magic/Spells/DA_Soul_TidalWard.DA_Soul_TidalWard"),
            TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_TidalWard.DA_SoulPresentation_TidalWard"));
    if (PC->WasInputKeyJustPressed(EKeys::Five))
        CastPlayerSpell(
            TEXT("/Game/Soul/Magic/Spells/DA_Soul_Tailwind.DA_Soul_Tailwind"),
            TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Tailwind.DA_SoulPresentation_Tailwind"));

    if (PC->WasInputKeyJustPressed(EKeys::Tab))
        SelectNextAlliedFormation();
    if (PC->WasInputKeyJustPressed(EKeys::H))
        CommandSelectedAllies(ERBHostGroupOrder::Hold);
    if (PC->WasInputKeyJustPressed(EKeys::V))
        CommandSelectedAllies(ERBHostGroupOrder::Advance);
    if (PC->WasInputKeyJustPressed(EKeys::G))
        CommandSelectedAllies(ERBHostGroupOrder::Charge);
    if (PC->WasInputKeyJustPressed(EKeys::B))
        CommandSelectedAllies(ERBHostGroupOrder::FallBack);
    if (PC->WasInputKeyJustPressed(EKeys::F))
        CommandSelectedAllies(ERBHostGroupOrder::Face);
    if (PC->WasInputKeyJustPressed(EKeys::Escape))
        FPlatformMisc::RequestExit(false);
}
void ASoulRealtimeArenaGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if (auto* PC = GetWorld()->GetFirstPlayerController())
        if (PC->WasInputKeyJustPressed(EKeys::Escape))
            FPlatformMisc::RequestExit(false);
    if (bMapOnly)
    {
        BattleElapsed += Seconds;
        if (!bFirstCapture && BattleElapsed > 20)
        {
            bFirstCapture = true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_G0_A.png"), false, false);
        }
        if (!bSecondCapture && BattleElapsed > 45)
        {
            bSecondCapture = true;
            if (auto* PC = GetWorld()->GetFirstPlayerController())
            {
                const FVector Center = ResolveSpawnLocation(ArenaOrigin + FVector(0, 0, 100));
                PC->GetViewTarget()->SetActorLocation(Center + FVector(1800, -3200, 1700));
                PC->GetViewTarget()->SetActorRotation((Center + FVector(500, 1800, 600) - PC->GetViewTarget()->GetActorLocation()).Rotation());
            }
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_G0_B.png"), false, false);
        }
        if (bQualification && BattleElapsed >= 90.0f) FinishProof(true, TEXT("G0 map-only 90 seconds, no simulation"));
        return;
    }
    if (bFinished)
    {
        UpdateVisualAnimations();
        if (ResultHoldSeconds >= 0)
        {
            ResultHoldSeconds += Seconds;
            if (ResultHoldSeconds > 60) { bFinished = false; FinishProof(true, TEXT("Resolved real battle plus 60-second observation")); }
        }
        return;
    }
    if (Combatants.IsEmpty()) return;
    if (bQualification && !bFirstCapture && BattleElapsed > 3)
    {
        bFirstCapture = true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Battle.png"), false, false);
    }

    for (auto& C : Combatants)
        C.MeleeCooldown =
            FMath::Max(0.0f, C.MeleeCooldown - Seconds);

    TickMagic(Seconds);
    UpdateVisualAnimations();
    UpdateDefeatedRepresentations();
    TickReinforcements();
    if (bFinished) return; // A failed reserve transaction must not proceed to result publication.
    TrackBattlefieldExtent();
    TickSpatialOrders(Seconds);
    TickBattleResolution(Seconds);
    if (bFinished) return;
    if (!bAutobattle) PlayerTick(Seconds);
}

void ASoulRealtimeArenaGameMode::FinishProof(
    bool bPassed, const FString& Detail)
{
    if (bFinished) return;
    bFinished = true;
    UE_LOG(LogTemp, Display,
        TEXT("SOUL_RT_ARENA_%s: %s"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"), *Detail);
    if (GLog)
    {
        GLog->FlushThreadedLogs();
        GLog->Flush();
    }
    FPlatformMisc::RequestExitWithStatus(
        true, bPassed ? 0 : 1);
}
void ASoulRealtimeArenaHUD::DrawHUD()
{
    Super::DrawHUD();
    const auto* Host = ArenaHost(this);
    if (!Host || !Canvas) return;

    const float Bottom = Canvas->ClipY - 114.f;
    DrawRect(FLinearColor(0.025f, 0.035f, 0.04f, 0.88f), 12, 12, Canvas->ClipX - 24, 54);
    DrawRect(FLinearColor(0.025f, 0.035f, 0.04f, 0.82f), 12, Bottom, Canvas->ClipX - 24, 102);
    DrawText(GetWorld()->GetOutermost()->GetName().Contains(TEXT("Dragon_graveyard")) ? TEXT("SOUL  |  DRAGON GRAVEYARD") : TEXT("SOUL  |  FIELD BATTLE"), FColor(227,211,166), 24, 20);
    DrawText(FString::Printf(TEXT("Allies %d + %d reserves    Enemy %d + %d reserves    Losses %d / %d"),
        Host->AliveForSide(0), Host->ReserveBodiesForSide(0), Host->AliveForSide(1), Host->ReserveBodiesForSide(1),
        Host->CasualtiesForSide(0), Host->CasualtiesForSide(1)), FColor::White, 24, 43);
    DrawText(FString::Printf(TEXT("Mana %.0f  |  Casts %d"), Host->PlayerManaValue(), Host->MagicCastCount()), FColor::Cyan, Canvas->ClipX - 220, 20);
    DrawText(Host->TacticalSummary(), FColor(227,211,166), 24, Bottom + 8);
    DrawText(Host->Status, FColor::Yellow, 24, Bottom + 30);
    DrawText(Host->ReinforcementSummary(0), FColor::Cyan, 24, Bottom + 52);
    DrawText(Host->ReinforcementSummary(1), FColor::White, Canvas->ClipX * 0.5f, Bottom + 52);
    DrawText(TEXT("[Tab] Formation/All  [H] Hold  [V] Advance  [G] Charge  [B] Fall back  [F] Face"), FColor(195,202,203), 24, Bottom + 75);
    DrawText(Host->SpellSummary(), FColor(170,215,255), Canvas->ClipX * 0.43f, Bottom + 75);
}

void ASoulRealtimeArenaGameMode::SetupBattleCamera()
{
    const FVector Center = ResolveSpawnLocation(ArenaOrigin + FVector(0, 0, 100));
    // Establish the environment in map-only qualification; frame the actual fighters during play.
    const bool bDragonShowcase = GetWorld()->GetOutermost()->GetName().Contains(TEXT("Dragon_graveyard"));
    // Keep the authored skull, ribs and lava in frame while making formation
    // silhouettes readable at 720p instead of reducing the armies to a dot.
    const FVector CameraOffset = bDragonShowcase ? FVector(3200, -4200, 3000) : (bMapOnly ? FVector(-1800, -3200, 1700) : FVector(-850, -1800, 1000));
    const FVector FocusOffset = bDragonShowcase ? FVector(-200, 500, 150) : (bMapOnly ? FVector(500, 1800, 600) : FVector(250, 0, 100));
    auto* Camera = GetWorld()->SpawnActor<ACameraActor>(Center + CameraOffset, FRotator::ZeroRotator);
    if (!Camera) return;
    Camera->SetActorRotation((Center + FocusOffset - Camera->GetActorLocation()).Rotation());
    Camera->GetCameraComponent()->SetFieldOfView(bDragonShowcase ? 60.0f : bMapOnly ? 65.0f : 75.0f);
    if (auto* PC = GetWorld()->GetFirstPlayerController()) PC->SetViewTarget(Camera);
}

int32 ASoulRealtimeArenaGameMode::AliveInGroup(int32 GroupIndex) const
{
    if (!Groups.IsValidIndex(GroupIndex)) return 0;
    int32 Count = 0;
    for (const FRBHostIdentity& Member : Groups[GroupIndex].Members)
    {
        const int32 I = Index(Member);
        Count += I != INDEX_NONE && Combatants[I].Health > 0.0f ? 1 : 0;
    }
    return Count;
}

FVector ASoulRealtimeArenaGameMode::GroupCenter(int32 GroupIndex) const
{
    if (!Groups.IsValidIndex(GroupIndex)) return ArenaOrigin;
    FVector Center = FVector::ZeroVector;
    int32 Count = 0;
    for (const FRBHostIdentity& Member : Groups[GroupIndex].Members)
    {
        const int32 I = Index(Member);
        if (I == INDEX_NONE || Combatants[I].Health <= 0.0f ||
            !Actors.IsValidIndex(I) || !Actors[I])
            continue;
        Center += Actors[I]->GetActorLocation();
        ++Count;
    }
    return Count > 0 ? Center / Count : Groups[GroupIndex].Anchor;
}

int32 ASoulRealtimeArenaGameMode::FindNearestEnemyToGroup(
    int32 GroupIndex, float& OutDistance) const
{
    OutDistance = TNumericLimits<float>::Max();
    if (!Groups.IsValidIndex(GroupIndex)) return INDEX_NONE;
    const int32 Leader = Index(Groups[GroupIndex].Leader);
    if (Leader == INDEX_NONE) return INDEX_NONE;
    const int32 Side = Combatants[Leader].Side;
    const FVector Center = GroupCenter(GroupIndex);
    int32 Best = INDEX_NONE;
    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        if (!Actors.IsValidIndex(I) || !Actors[I] ||
            Combatants[I].Health <= 0.0f || Combatants[I].Side == Side)
            continue;
        const float Distance = FVector::Dist2D(
            Center, Actors[I]->GetActorLocation());
        if (Distance < OutDistance)
        {
            OutDistance = Distance;
            Best = I;
        }
    }
    return Best;
}

int32 ASoulRealtimeArenaGameMode::FindFormationState(
    int32 GroupIndex) const
{
    return TacticalFormations.IndexOfByPredicate(
        [GroupIndex](const FSoulBattleFormationState& State)
        {
            return State.GroupIndex == GroupIndex;
        });
}

bool ASoulRealtimeArenaGameMode::IssueFormationOrder(
    int32 GroupIndex,
    ERBHostGroupOrder Order,
    const FVector& Anchor,
    const FVector& Facing,
    bool bManual)
{
    if (!Groups.IsValidIndex(GroupIndex) ||
        !Drivers.IsValidIndex(GroupIndex) || !Drivers[GroupIndex] ||
        AliveInGroup(GroupIndex) <= 0)
        return false;

    FString Error;
    const bool bAccepted = Drivers[GroupIndex]->RequestGroupOrder(
        Groups[GroupIndex].Leader, Order, Anchor,
        Facing.IsNearlyZero() ? FVector::ForwardVector : Facing,
        FRBHostIdentity(), Groups[GroupIndex].Revision, Error);
    if (!bAccepted) return false;

    const int32 StateIndex = FindFormationState(GroupIndex);
    if (TacticalFormations.IsValidIndex(StateIndex))
    {
        TacticalFormations[StateIndex].TacticalAnchor = Anchor;
        if (bManual)
            TacticalFormations[StateIndex].ManualOverrideUntil =
                BattleElapsed + 15.0f;
    }
    ++SpatialOrders;
    return true;
}

void ASoulRealtimeArenaGameMode::UpdateFormationMorale()
{
    bool bAnyRouting[2] = {false, false};
    for (const FSoulBattleFormationState& State : TacticalFormations)
        if (State.bRouting && AliveInGroup(State.GroupIndex) > 0)
            bAnyRouting[State.Side] = true;

    for (FSoulBattleFormationState& State : TacticalFormations)
    {
        const int32 Alive = AliveInGroup(State.GroupIndex);
        if (Alive <= 0)
        {
            State.PreviousAlive = 0;
            continue;
        }

        const FVector Center = GroupCenter(State.GroupIndex);
        int32 FriendlyNear = 0;
        int32 EnemyNear = 0;
        bool bHeroSupport = false;
        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            if (!Actors.IsValidIndex(I) || !Actors[I] ||
                Combatants[I].Health <= 0.0f)
                continue;
            const float Distance = FVector::Dist2D(
                Center, Actors[I]->GetActorLocation());
            if (Distance > 850.0f) continue;
            if (Combatants[I].Side == State.Side)
            {
                ++FriendlyNear;
                bHeroSupport = bHeroSupport ||
                    Combatants[I].Role == ESoulRealtimeFormationRole::Hero;
            }
            else ++EnemyNear;
        }

        float EnemyDistance = TNumericLimits<float>::Max();
        const int32 Enemy = FindNearestEnemyToGroup(
            State.GroupIndex, EnemyDistance);
        bool bFlanked = false;
        if (Enemy != INDEX_NONE && Groups.IsValidIndex(State.GroupIndex))
        {
            FRBCombatGroup Core;
            if (Groups[State.GroupIndex].ToCore(Core))
            {
                const FVector ToEnemy =
                    (Actors[Enemy]->GetActorLocation() - Center)
                    .GetSafeNormal2D();
                bFlanked = EnemyDistance < 750.0f &&
                    FVector::DotProduct(Core.Facing.GetSafeNormal2D(),
                        ToEnemy) < -0.25f;
            }
        }

        FSoulBattleMoraleInput Input;
        Input.PreviousAlive = State.PreviousAlive;
        Input.CurrentAlive = Alive;
        Input.bFlanked = bFlanked;
        Input.bLocalDisadvantage = EnemyNear > FriendlyNear + 2;
        Input.bFriendlyRoutedNearby = bAnyRouting[State.Side] &&
            !State.bRouting;
        Input.bHeroSupport = bHeroSupport;
        State.MoralePermille =
            FSoulRealtimeTacticalRules::UpdateMorale(
                State.MoralePermille, Input);
        State.PreviousAlive = Alive;
        if (State.bRouting)
            State.RoutingSeconds += 0.8f;

        if (!State.bRouting && BattleElapsed >= State.RallyGraceUntil &&
            State.MoralePermille < 260)
        {
            State.bRouting = true;
            State.bShattered = State.bRallied ||
                State.MoralePermille <= 0 ||
                Alive * 4 <= State.InitialBodies;
            State.bRallied = false;
            State.RoutingSeconds = 0.0f;
            ++RoutedSides[State.Side];
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_FORMATION_ROUT: side=%d group=%d kind=%s morale=%d"),
                State.Side, State.GroupIndex,
                FSoulRealtimeTacticalRules::FormationKindLabel(State.Kind),
                State.MoralePermille);
        }
        else if (State.bRouting && !State.bShattered &&
                 bHeroSupport && !Input.bLocalDisadvantage &&
                 State.RoutingSeconds >= 6.0f &&
                 State.MoralePermille >= 420)
        {
            State.bRouting = false;
            State.bRallied = true;
            State.RallyGraceUntil = BattleElapsed + 8.0f;
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_FORMATION_RALLY: side=%d group=%d morale=%d"),
                State.Side, State.GroupIndex, State.MoralePermille);
        }
    }
}

void ASoulRealtimeArenaGameMode::RefreshBattlePhase()
{
    float Closest = TNumericLimits<float>::Max();
    int32 MoraleTotal[2] = {0, 0};
    int32 MoraleGroups[2] = {0, 0};
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (AliveInGroup(State.GroupIndex) <= 0) continue;
        MoraleTotal[State.Side] += State.MoralePermille;
        ++MoraleGroups[State.Side];
        float Distance = 0.0f;
        if (FindNearestEnemyToGroup(State.GroupIndex, Distance) != INDEX_NONE)
            Closest = FMath::Min(Closest, Distance);
    }
    const int32 AlliedLoss = InitialStrategic[0] > 0
        ? CasualtiesForSide(0) * 1000 / InitialStrategic[0] : 0;
    const int32 EnemyLoss = InitialStrategic[1] > 0
        ? CasualtiesForSide(1) * 1000 / InitialStrategic[1] : 0;
    const int32 AlliedMorale = MoraleGroups[0] > 0
        ? MoraleTotal[0] / MoraleGroups[0] : 0;
    const int32 EnemyMorale = MoraleGroups[1] > 0
        ? MoraleTotal[1] / MoraleGroups[1] : 0;
    const ESoulBattlePhase NewPhase =
        FSoulRealtimeTacticalRules::DeterminePhase(
            BattleElapsed, Closest, AlliedLoss, EnemyLoss,
            AlliedMorale, EnemyMorale);
    if (NewPhase != BattlePhase)
    {
        BattlePhase = NewPhase;
        UE_LOG(LogTemp, Display, TEXT("SOUL_BATTLE_PHASE: %s"),
            FSoulRealtimeTacticalRules::PhaseLabel(BattlePhase));
    }
}

void ASoulRealtimeArenaGameMode::TickFormationTactics()
{
    UpdateFormationMorale();
    RefreshBattlePhase();

    bool bFrontEngaged[2] = {false, false};
    bool bFrontCollapsing[2] = {false, false};
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Kind != ESoulBattleFormationKind::FrontLine ||
            AliveInGroup(State.GroupIndex) <= 0)
            continue;
        float Distance = 0.0f;
        FindNearestEnemyToGroup(State.GroupIndex, Distance);
        bFrontEngaged[State.Side] =
            bFrontEngaged[State.Side] || Distance < 720.0f;
        bFrontCollapsing[State.Side] =
            bFrontCollapsing[State.Side] ||
            State.MoralePermille < 480;
    }

    for (FSoulBattleFormationState& State : TacticalFormations)
    {
        if (AliveInGroup(State.GroupIndex) <= 0 ||
            State.ManualOverrideUntil > BattleElapsed)
            continue;

        float EnemyDistance = 0.0f;
        const int32 Enemy = FindNearestEnemyToGroup(
            State.GroupIndex, EnemyDistance);
        if (Enemy == INDEX_NONE) continue;
        const FVector Center = GroupCenter(State.GroupIndex);
        const FVector EnemyLocation = Actors[Enemy]->GetActorLocation();
        const FVector Facing =
            (EnemyLocation - Center).GetSafeNormal2D();

        FSoulBattleOrderContext Context;
        Context.Kind = State.Kind;
        Context.Morale = FSoulRealtimeTacticalRules::MoraleState(
            State.MoralePermille, State.bRouting, State.bRallied);
        Context.Phase = BattlePhase;
        Context.EnemyDistance = EnemyDistance;
        Context.bFrontLineEngaged = bFrontEngaged[State.Side];
        Context.bMeleeThreat = EnemyDistance < 600.0f;
        Context.bFriendlyLineCollapsing =
            bFrontCollapsing[State.Side];

        const ERBHostGroupOrder Order =
            FSoulRealtimeTacticalRules::ChooseOrder(Context);
        FVector Anchor = State.TacticalAnchor;
        if (Order == ERBHostGroupOrder::FallBack)
        {
            const float Outward = State.Side == 0 ? -1.0f : 1.0f;
            Anchor = State.SpawnAnchor + FVector(Outward * 550.0f, 0, 0);
        }
        else if (Order == ERBHostGroupOrder::Advance)
        {
            Anchor = State.TacticalAnchor;
            if (State.Kind == ESoulBattleFormationKind::FrontLine ||
                (State.Kind == ESoulBattleFormationKind::Strike &&
                 bFrontEngaged[State.Side]))
            {
                FVector Approach;
                if (Spatial && Spatial->QueryApproach(
                        Actors[Index(Groups[State.GroupIndex].Leader)],
                        State.Side == 1, EnemyLocation, Approach))
                    Anchor = Approach;
                else
                    Anchor = EnemyLocation - Facing * 420.0f;
            }
        }
        else if (Order == ERBHostGroupOrder::Charge)
        {
            Anchor = EnemyLocation;
        }
        else if (Order == ERBHostGroupOrder::Hold)
        {
            Anchor = State.TacticalAnchor;
        }

        IssueFormationOrder(
            State.GroupIndex, Order, Anchor, Facing, false);
    }
}

void ASoulRealtimeArenaGameMode::TickSpatialOrders(float Seconds)
{
    SpatialElapsed += Seconds;
    if (!Spatial || SpatialElapsed < 0.8f) return;
    SpatialElapsed = 0.0f;
    TickFormationTactics();
}

void ASoulRealtimeArenaGameMode::TickBattleResolution(float Seconds)
{
    BattleElapsed += Seconds;
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const int32 Logical = FSoulRealtimeBattleRules::ActiveBodies(ReinforcementBattle, RealtimeSideId(Side));
        if (Logical != AliveForSide(Side) || Logical > ActiveCap)
        {
            FinishProof(false, TEXT("Real actor / reinforcement count mismatch"));
            return;
        }
    }
    for (int32 Side = 0; Side < 2; ++Side)
    {
        int32 LivingFormations = 0;
        int32 RoutingFormations = 0;
        int32 ShatteredFormations = 0;
        for (const FSoulBattleFormationState& State : TacticalFormations)
        {
            if (State.Side != Side || AliveInGroup(State.GroupIndex) <= 0)
                continue;
            ++LivingFormations;
            RoutingFormations += State.bRouting ? 1 : 0;
            ShatteredFormations += State.bShattered ? 1 : 0;
        }
        const bool bActiveArmyShattered = LivingFormations > 0 &&
            ShatteredFormations >= LivingFormations;
        bSideMoraleDefeated[Side] = bActiveArmyShattered ||
            FSoulRealtimeTacticalRules::IsMoraleDefeated(
                LivingFormations, RoutingFormations,
                ReserveBodiesForSide(Side));
    }
    if (bSideMoraleDefeated[0] || bSideMoraleDefeated[1])
    {
        UE_LOG(LogTemp, Display,
            TEXT("SOUL_MORALE_RESOLUTION: alliedRouted=%d enemyRouted=%d"),
            bSideMoraleDefeated[0], bSideMoraleDefeated[1]);
        FinishBattle();
        return;
    }

    // Qualification exercises the full current battle spell vocabulary. Each cast
    // still passes through RB Magic authority; the sequence only supplies inputs.
    if (bTacticalMagic && PlayerHealth() > 0)
    {
        bool bCast = false;
        if (MagicProofStage == 0 && BattleElapsed > 1.0f)
            bCast = CastPlayerSpell(
                TEXT("/Game/Soul/Magic/Spells/DA_Soul_Tailwind.DA_Soul_Tailwind"),
                TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Tailwind.DA_SoulPresentation_Tailwind"));
        else if (MagicProofStage == 1 && BattleElapsed > 2.0f)
            bCast = CastPlayerSpell(
                TEXT("/Game/Soul/Magic/Spells/DA_Soul_TidalWard.DA_Soul_TidalWard"),
                TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_TidalWard.DA_SoulPresentation_TidalWard"));
        else if (MagicProofStage == 2 && BattleElapsed > 3.0f)
            bCast = CastPlayerSpell(
                TEXT("/Game/Soul/Magic/Spells/DA_Soul_Blizzard.DA_Soul_Blizzard"),
                TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Blizzard.DA_SoulPresentation_Blizzard"));
        else if (MagicProofStage == 3 &&
                 FindPlayerSpellTarget(2400.0f) != INDEX_NONE)
            bCast = CastPlayerSpell(
                TEXT("/Game/Soul/Magic/Spells/DA_Soul_Firebolt.DA_Soul_Firebolt"),
                TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_Firebolt.DA_SoulPresentation_Firebolt"));
        else if (MagicProofStage == 4 &&
                 FindPlayerSpellTarget(2200.0f) != INDEX_NONE)
            bCast = CastPlayerSpell(
                TEXT("/Game/Soul/Magic/Spells/DA_Soul_ChainLightning.DA_Soul_ChainLightning"),
                TEXT("/Game/Soul/Magic/Presentation/DA_SoulPresentation_ChainLightning.DA_SoulPresentation_ChainLightning"));
        if (bCast) ++MagicProofStage;
    }
    if (!FSoulRealtimeBattleRules::HasLivingForce(ReinforcementBattle, RealtimeSideId(0)) ||
        !FSoulRealtimeBattleRules::HasLivingForce(ReinforcementBattle, RealtimeSideId(1)))
    {
        FinishBattle();
        return;
    }
    if (bQualification && BattleElapsed > 180)
        FinishProof(false, TEXT("Battle exceeded 180 seconds: unresolved living force; no fabricated winner"));
}

void ASoulRealtimeArenaGameMode::FinishBattle()
{
    const int32 PhysicalPlayerSurvivors =
        AliveForSide(0) + ReserveBodiesForSide(0);
    const int32 PhysicalEnemySurvivors =
        AliveForSide(1) + ReserveBodiesForSide(1);
    // A fully routed force has lost strategic cohesion and exits this encounter.
    // The existing campaign bridge represents that defeated force as zero effective
    // survivors while the arena presents the living bodies physically withdrawing.
    const int32 PlayerSurvivors =
        bSideMoraleDefeated[0] ? 0 : PhysicalPlayerSurvivors;
    const int32 EnemySurvivors =
        bSideMoraleDefeated[1] ? 0 : PhysicalEnemySurvivors;
    const bool bWon = EnemySurvivors == 0 && PlayerSurvivors > 0;
    UE_LOG(LogTemp, Display, TEXT("SOUL_BATTLE_RESOLVED: won=%d playerSurvivors=%d enemySurvivors=%d physical=%d/%d routed=%d/%d waves=%d/%d magic=%d contacts=%d pbilQueries=%d pbilSuccess=%d pbilOrders=%d seconds=%.2f mana=%d"),
        bWon, PlayerSurvivors, EnemySurvivors,
        PhysicalPlayerSurvivors, PhysicalEnemySurvivors,
        bSideMoraleDefeated[0], bSideMoraleDefeated[1],
        ReinforcementWaves[0], ReinforcementWaves[1], MagicCasts,
        AcceptedContactCount(), Spatial ? Spatial->GetQueryCount() : 0, Spatial ? Spatial->GetSuccessfulQueryCount() : 0, SpatialOrders, BattleElapsed, FMath::FloorToInt(FMath::Max(0.0f, PlayerMana)));
    if (bQualification && (!Spatial || Spatial->GetQueryCount() <= 0 ||
        Spatial->GetSuccessfulQueryCount() <= 0 || SpatialOrders <= 0))
    {
        FinishProof(false, TEXT("Battle resolved without required PBIL query and RB Combat order participation"));
        return;
    }
    if (bCampaignBattle)
    {
        auto* Bridge = GetGameInstance()->GetSubsystem<USoulCampaignBattleBridge>();
        const auto* Pending = Bridge ? Bridge->GetPendingEncounter() : nullptr;
        if (!Pending) { FinishProof(false, TEXT("Lost campaign encounter")); return; }
        const FName ReturnMap = Pending->ReturnMapPackage;
        FSoulCampaignBattleResult Result;
        Result.EncounterId = Pending->EncounterId;
        Result.TargetRegion = Pending->TargetRegion;
        Result.bPlayerWon = bWon;
        Result.PlayerSurvivors = PlayerSurvivors;
        Result.EnemySurvivors = EnemySurvivors;
        Result.PlayerReinforcements = ReinforcementWaves[0];
        Result.EnemyReinforcements = ReinforcementWaves[1];
        Result.MagicCasts = MagicCasts;
        Result.PlayerManaRemaining = FMath::FloorToInt(FMath::Max(0.0f, PlayerMana));
        if (!Bridge->ResolveEncounter(Result)) { FinishProof(false, TEXT("Campaign rejected battle result")); return; }
        bFinished = true;
        if (Spatial) Spatial->Shutdown();
        UGameplayStatics::OpenLevel(this, ReturnMap, true, TEXT("game=/Script/Soul.SoulFounderPlaytestGameMode"));
        return;
    }
    if (bQualification)
    {
        if (AcceptedContactCount() <= 0 || TotalAlliedTargets() != 0)
        { FinishProof(false, TEXT("Battle did not meet combat contact checks")); return; }
        bFinished = true;
        ResultHoldSeconds = 0;
        Status = bWon ? TEXT("VICTORY - hostile reserves exhausted") : TEXT("DEFEAT - allied reserves exhausted");
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Result.png"), false, false);
        return;
    }
    bFinished = true;
    Status = bWon ? TEXT("VICTORY") : TEXT("DEFEAT");
}
