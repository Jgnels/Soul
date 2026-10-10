#include "SoulRealtimeBattleArena.h"
#include "RBUIInputSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Components/InputComponent.h"
#include "SoulRealtimeBattlePBIL.h"
#include "SoulBattleArrow.h"
#include "SoulBattleSpellCue.h"
#include "SoulCampaignBattleBridge.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
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
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LevelStreaming.h"
#include "HAL/PlatformTime.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/App.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "RBCombatRangedComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
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
    return Host && (Host->Index(Identity) != INDEX_NONE || Host->IsSiegeGate(Identity));
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
    const auto* Host=ArenaHost(this);
    return Host && (Host->IsSiegeGate(Identity)?Host->CanDamageSiegeGate(Identity):Host->CanAct(Identity));
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
    return Host && Host->CanDriverEngage(GroupIndex, A, B);
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

ASoulRealtimeArenaPlayerController::ASoulRealtimeArenaPlayerController()
{
    bShouldPerformFullTickWhenPaused = true;
    bEnableClickEvents = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ASoulRealtimeArenaPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    for(const FKey Key:{EKeys::P,EKeys::SpaceBar})
        InputComponent->BindKey(Key,IE_Pressed,this,&ASoulRealtimeArenaPlayerController::PauseBattle).bExecuteWhenPaused=true;
    const TPair<FKey,FName> PadBindings[]={
        {EKeys::Gamepad_Special_Right,TEXT("Pause")},{EKeys::Gamepad_Special_Left,TEXT("Camera")},
        {EKeys::Gamepad_RightShoulder,TEXT("NextFormation")},{EKeys::Gamepad_LeftShoulder,TEXT("All")},
        {EKeys::Gamepad_DPad_Up,TEXT("Follow")},{EKeys::Gamepad_DPad_Down,TEXT("Fallback")},
        {EKeys::Gamepad_DPad_Left,TEXT("Hold")},{EKeys::Gamepad_DPad_Right,TEXT("Charge")},
        {EKeys::Gamepad_LeftThumbstick,TEXT("Focus")},{EKeys::Gamepad_RightThumbstick,TEXT("View")},
        {EKeys::Gamepad_FaceButton_Left,TEXT("NextSpell")},{EKeys::Gamepad_FaceButton_Top,TEXT("Cast")},
        {EKeys::Gamepad_FaceButton_Bottom,TEXT("GroundOrder")},{EKeys::Gamepad_FaceButton_Right,TEXT("Cancel")}};
    for(const auto& Entry:PadBindings)
    {
        FInputKeyBinding Binding(FInputChord(Entry.Key),IE_Pressed);
        Binding.bExecuteWhenPaused=true;
        Binding.KeyDelegate.GetDelegateForManualSet().BindWeakLambda(this,[this,Action=Entry.Value]()
        { if(auto* Host=GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>()) Host->HandleGamepadAction(Action); });
        InputComponent->KeyBindings.Add(MoveTemp(Binding));
    }
}
void ASoulRealtimeArenaPlayerController::PauseBattle()
{
    if(auto* Host=GetWorld()->GetAuthGameMode<ASoulRealtimeArenaGameMode>())
        Host->ToggleBattlePause();
}

void ASoulRealtimeArenaPlayerController::GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const
{
    // Deployment pauses at world time zero. UE's default controller treats a
    // zero camera-cache timestamp as uninitialized and returns the pawn origin,
    // even after its camera manager has evaluated the active camera.
    AActor* Target = GetViewTarget();
    if (PlayerCameraManager && PlayerCameraManager->GetCameraCacheTime() <= 0.0f &&
        Target && Target->HasActiveCameraComponent())
    {
        FMinimalViewInfo View;
        Target->CalcCamera(0.0f, View);
        Location = View.Location;
        Rotation = View.Rotation;
        return;
    }
    Super::GetPlayerViewPoint(Location, Rotation);
}

ASoulRealtimeArenaGameMode::ASoulRealtimeArenaGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    DefaultPawnClass = nullptr;
    bStartPlayersAsSpectators = true;
    HUDClass = ASoulRealtimeArenaHUD::StaticClass();
    PlayerControllerClass = ASoulRealtimeArenaPlayerController::StaticClass();
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
    if(UsesOrcCampaignRoster(C.Side)){P.Id=TEXT("Soul.Orc.Hammer");P.Category=TEXT("Hammer");}
    if(UsesNatureCampaignRoster(C.Side)){P.Id=TEXT("Soul.Nature.BearBlade");P.Category=TEXT("Sword");}
    if(UsesVikingCampaignRoster(C.Side)){P.Id=TEXT("Soul.Viking.Axe");P.Category=TEXT("Axe");}
    P.DamageType = TEXT("Physical");
    P.BaseDamage = RoleDamage(C.Role);
    P.Reach = C.bRanged ? 2600.0f
        : (C.Role == ESoulRealtimeFormationRole::Apex
            ? 245.0f : 175.0f);
    if(!C.bRanged && Actors.IsValidIndex(I) && IsValid(Actors[I]))
    {
        ACharacter* Nearest=nullptr;
        double Distance=TNumericLimits<double>::Max();
        for(int32 J=0;J<Combatants.Num();++J)
        {
            if(Combatants[J].Side==C.Side || Combatants[J].Health<=0 ||
                !Actors.IsValidIndex(J) || !IsValid(Actors[J])) continue;
            const double Candidate=FVector::DistSquared2D(Actors[I]->GetActorLocation(),Actors[J]->GetActorLocation());
            if(Candidate<Distance) { Distance=Candidate; Nearest=Actors[J]; }
        }
        // RB orders compare actor centers. Account for the actual body envelopes
        // so avoidance does not hold large creatures outside their attack range.
        P.Reach=MeleeBodyReach(Actors[I],Nearest,P.Reach);
    }
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
    return !bFinished && !bBattlePaused && I != INDEX_NONE && Combatants[I].Health > 0.0f &&
        Actors.IsValidIndex(I) && IsValid(Actors[I]);
}
bool ASoulRealtimeArenaGameMode::CommitHit(
    const FRBHostHit& Hit, FString& Error)
{
    if(IsSiegeGate(Hit.Victim))return CommitSiegeHit(Hit,Error);
    const int32 A = Index(Hit.Attacker);
    const int32 V = Index(Hit.Victim);
    if (bFinished || bBattlePaused || A == INDEX_NONE || V == INDEX_NONE || A == V ||
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
    // Sound follows the accepted contact, including arrows already in flight
    // after their shooter has switched to melee.
    if (Hit.Weapon.Category == TEXT("Bow"))
        PlayBattleSound(ESoulBattleSound::ArrowHit, Hit.ImpactPoint, A);
    else if (Combatants[A].Role != ESoulRealtimeFormationRole::Apex &&
             Combatants[A].Role != ESoulRealtimeFormationRole::Breaker)
        PlayBattleSound((AcceptedContacts.Num() & 1) ? ESoulBattleSound::SwordHit1 :
            ESoulBattleSound::SwordHit2, Hit.ImpactPoint, A);
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
    PlayAcceptedHitReaction(V);
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
        Side == ControlledSide ? TEXT("Allies") : TEXT("Enemy"), Reserve, *Readiness);
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
            if (I != INDEX_NONE && Combatants[I].Side == ControlledSide && Combatants[I].Health > 0)
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
    const int32 AlliedMorale = Living[ControlledSide] > 0 ? Morale[ControlledSide] / Living[ControlledSide] : 0;
    const int32 EnemyMorale = Living[1-ControlledSide] > 0 ? Morale[1-ControlledSide] / Living[1-ControlledSide] : 0;
    return FString::Printf(
        TEXT("%s  |  Morale A %d%% (%d routing)  E %d%% (%d routing)"),
        FSoulRealtimeTacticalRules::PhaseLabel(BattlePhase),
        AlliedMorale / 10, Routing[ControlledSide], EnemyMorale / 10, Routing[1-ControlledSide]);
}

FString ASoulRealtimeArenaGameMode::SpellSummary() const
{
    return TEXT("[1] Firebolt   [2] Chain Lightning   [3] Blizzard   [4] Tidal Ward   [5] Tailwind");
}

int32 ASoulRealtimeArenaGameMode::AlliedFormationCount() const
{
    int32 Count = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
        if (State.Side == ControlledSide) ++Count;
    return Count;
}

FString ASoulRealtimeArenaGameMode::AlliedFormationSummary(int32 Slot) const
{
    int32 CurrentSlot = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != ControlledSide) continue;
        if (CurrentSlot++ != Slot) continue;
        FRBCombatGroup Core;
        const TCHAR* Order = TEXT("HOLD");
        if (Groups.IsValidIndex(State.GroupIndex) && Groups[State.GroupIndex].ToCore(Core))
        {
            switch (Core.Command)
            {
                case ERBGroupCommand::Follow: Order = TEXT("FOLLOW"); break;
                case ERBGroupCommand::Face: Order = TEXT("FACE"); break;
                case ERBGroupCommand::Advance: Order = TEXT("ADVANCE"); break;
                case ERBGroupCommand::FallBack: Order = TEXT("FALL BACK"); break;
                case ERBGroupCommand::Charge: Order = TEXT("CHARGE"); break;
                default: break;
            }
        }
        const auto Morale = FSoulRealtimeTacticalRules::MoraleState(
            State.MoralePermille, State.bRouting, State.bRallied);
        const bool bSelected = !bSelectAllAllies &&
            SelectedAlliedFormation == State.GroupIndex;
        return FString::Printf(TEXT("F%d  %s  %d/%d  %s  %s%s"),
            Slot + 1,
            FSoulRealtimeTacticalRules::FormationKindLabel(State.Kind),
            AliveInGroup(State.GroupIndex), State.InitialBodies,
            AliveInGroup(State.GroupIndex)>0 ? FSoulRealtimeTacticalRules::MoraleLabel(Morale) : TEXT("DEFEATED"), Order,
            bSelected ? TEXT("  < SELECTED") : TEXT(""));
    }
    return FString();
}

FString ASoulRealtimeArenaGameMode::SelectedFormationSummary() const
{
    if (bSelectAllAllies) return TEXT("ALL FORMATIONS SELECTED");
    int32 Slot = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != ControlledSide) continue;
        if (State.GroupIndex == SelectedAlliedFormation)
            return AlliedFormationSummary(Slot);
        ++Slot;
    }
    return TEXT("ALL FORMATIONS SELECTED");
}

FVector ASoulRealtimeArenaGameMode::SelectedFormationLocation() const
{
    if (!bSelectAllAllies && SelectedAlliedFormation != INDEX_NONE)
        return GroupCenter(SelectedAlliedFormation);
    return PlayerHero ? PlayerHero->GetActorLocation() : ArenaOrigin;
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
    if(bRestrictPlayerSpells&&!AllowedPlayerSpells.Contains(Spell.SpellTag.GetTagName()))
    {OutError=TEXT("Spell unavailable to this hero: affinity or Mage Guild learning required.");return false;}
    if(!Request.CastId.IsValid() || CommittedMagicCasts.Contains(Request.CastId))
    { OutError=TEXT("Cast identity is invalid or already committed."); return false; }
    if (bFinished || bBattlePaused || !PlayerHero || Spell.bStrategicOnly)
    {
        OutError = bBattlePaused ? TEXT("Resume the battle to cast. Targeting stays ready.") : TEXT("Battle caster or battle spell unavailable.");
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

    const float ManaCost = SpellManaCost(Spell);
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
    if (bFinished || bBattlePaused || !Combatants.IsValidIndex(TargetIndex) ||
        !FMath::IsFinite(Damage) || Damage <= 0.0f || Combatants[TargetIndex].Health <= 0.0f)
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
    // Presentation follows actual accepted health loss. A fully absorbed ward
    // hit must not flinch; lethal hits transition through the death path.
    if (Target.Health < Before) PlayAcceptedHitReaction(TargetIndex);
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

    const float ManaCost = SpellManaCost(Spell);

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

    const bool ProjectileDelivery=Spell.DeliveryMode==ERBMagicDeliveryMode::Projectile;
    if(ProjectileDelivery)
    {
        float Damage=0;
        for(const auto& Intent:Effects)
        {
            const FString Tag=Intent.Effect.EffectTag.ToString();
            if(Tag!=TEXT("Magic.Effect.Damage") && Tag!=TEXT("Magic.Effect.Burning"))
            { OutError=TEXT("Projectile payload is not supported in this battle."); return false; }
            Damage+=Intent.Effect.Magnitude;
        }
        if(!LaunchMagicProjectile(CasterIndex,PrimaryTarget,Request.CastId,Damage))
        { OutError=TEXT("Unable to launch spell projectile."); return false; }
    }
    FSoulRealtimeArenaMagicArea Area;
    Area.Center = Request.Target.WorldLocation;
    Area.SourceSide = Combatants[CasterIndex].Side;
    bool bHasArea = false;

    for (const FRBMagicEffectIntent& Intent : Effects)
    {
        const FString EffectTag = Intent.Effect.EffectTag.ToString();
        if(ProjectileDelivery) continue; // The physical sweep delivers this committed payload.
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
            TArray<FVector> Contacts{PlayerHero->GetActorLocation()+FVector(0,0,45)};
            for (int32 I = 0; I < Limit; ++I)
                if(ApplyMagicDamage(Targets[I], Intent.Effect.Magnitude))
                    Contacts.Add(Actors[Targets[I]]->GetActorLocation());
            if(bVisualUnits && Spell.DeliveryMode==ERBMagicDeliveryMode::Chain && Contacts.Num()>1)
                if(auto* Cue=GetWorld()->SpawnActor<ASoulBattleSpellCue>()) Cue->Chain(Contacts);
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
            if(bVisualUnits)
                if(auto* Cue=GetWorld()->SpawnActor<ASoulBattleSpellCue>())
                    Cue->Aura({Actors[PrimaryTarget]},true,Target.WardSeconds);
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
            if(bVisualUnits)
            {
                TArray<AActor*> Recipients;
                for(int32 I=0;I<Combatants.Num();++I)
                    if(Combatants[I].Side==Side && Combatants[I].Health>0 && IsValid(Actors[I]))
                        Recipients.Add(Actors[I]);
                if(auto* Cue=GetWorld()->SpawnActor<ASoulBattleSpellCue>())
                    Cue->Aura(Recipients,false,FMath::Min(SpeedBuffSeconds[Side],5.f));
            }
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
    {
        ActiveMagicAreas.Add(Area);
        if(bVisualUnits)
            if(auto* Cue=GetWorld()->SpawnActor<ASoulBattleSpellCue>())
                Cue->Blizzard(Area.Center,Area.Radius,Area.RemainingSeconds);
    }

    PlayerMana = FMath::Max(0.0f, PlayerMana - ManaCost);
    SpellCooldowns.Add(
        Spell.SpellTag.GetTagName(), Spell.CooldownSeconds);
    CommittedMagicCasts.Add(Request.CastId);
    ++MagicCasts;
    PushBattleNotice(FString::Printf(TEXT("%s cast"),*Spell.DisplayName.ToString()),0);
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
    // The intact Human city has nested authored level instances. Its root
    // begins play before their collision arrives; do not deploy armies into
    // an incomplete world. Other qualified environments retain their path.
    if (GetWorld()->GetOutermost()->GetName() == TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored"))
    {
        bAwaitingAuthoredEnvironment = true;
        AuthoredLoadStarted = FPlatformTime::Seconds();
        // The root world starts before its battle camera and streamed collision.
        // Rendering that temporary default view exhausted the guarded GPU load
        // budget. Keep the existing loading HUD, then restore world rendering
        // before initializing the real battle. No runtime/performance cap changes.
        if (auto* Viewport = GetWorld()->GetGameViewport())
        {
            bPriorWorldRenderingDisabled = Viewport->bDisableWorldRendering;
            Viewport->bDisableWorldRendering = true;
            bAuthoredLoadingRenderingSuppressed = true;
        }
        Status = TEXT("Loading the authored battlefield...");
        return;
    }
    InitializeLoadedArena();
}

void ASoulRealtimeArenaGameMode::RestoreAuthoredLoadingRendering()
{
    if (!bAuthoredLoadingRenderingSuppressed) return;
    if (auto* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
        Viewport->bDisableWorldRendering = bPriorWorldRenderingDisabled;
    bAuthoredLoadingRenderingSuppressed = false;
}

void ASoulRealtimeArenaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestoreAuthoredLoadingRendering();
    Super::EndPlay(EndPlayReason);
}

void ASoulRealtimeArenaGameMode::InitializeLoadedArena()
{
    bControlDiagnostics = FParse::Param(FCommandLine::Get(), TEXT("SoulControlDiagnostics"));
    bReadabilityProof = FParse::Param(FCommandLine::Get(), TEXT("SoulBattleReadabilityProof"));
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
        // Preserve the authored sun as the unique forward-shading primary.
        // A negative priority is clamped to zero by UE, so the fill cannot be
        // made secondary that way. Change only the transient world component.
        UDirectionalLightComponent* AuthoredSun=nullptr;
        int32 SunPriority=0;
        for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)
            if(auto* Light=Cast<UDirectionalLightComponent>(It->GetLightComponent()))
            {
                SunPriority=FMath::Max(SunPriority,Light->ForwardShadingPriority);
                if(!AuthoredSun || Light->Intensity>AuthoredSun->Intensity) AuthoredSun=Light;
            }
        if(AuthoredSun)
        {
            AuthoredSun->SetMobility(EComponentMobility::Movable);
            AuthoredSun->SetForwardShadingPriority(SunPriority+1);
        }
        // A restrained, shadowless fill makes troop silhouettes readable against
        // the dark stone. It exists only in the battle world, never in donor content.
        if (auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>())
        {
            Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            Fill->SetActorRotation(FRotator(-48, 115, 0));
            Fill->GetLightComponent()->SetIntensity(1.4f);
            Fill->GetLightComponent()->SetLightColor(FLinearColor(0.76f,0.84f,1.0f));
            Fill->GetLightComponent()->SetCastShadows(false);
            Fill->GetLightComponent()->SetSpecularScale(0.0f);
            CastChecked<UDirectionalLightComponent>(Fill->GetLightComponent())->SetForwardShadingPriority(0);
        }
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
    if(GetWorld()->GetOutermost()->GetName()==TEXT("/Game/Soul/Maps/Battles/L_Heartland_Woodland"))
    {
        // The authored foliage-generation brush is not tactical architecture.
        // Native probes found it intercepting floor/sweep queries throughout the
        // clearing. Disable only this authoring class in this world instance.
        int32 AuthoringVolumes=0;
        for(TActorIterator<AActor> It(GetWorld());It;++It)
            if(It->GetClass()->GetFName()==TEXT("ProceduralFoliageVolume"))
            {It->SetActorEnableCollision(false);++AuthoringVolumes;}
        UE_LOG(LogTemp,Display,TEXT("SOUL_WOODLAND_AUTHORING_VOLUMES collision_disabled=%d terrain_trees_rocks_unchanged=1"),AuthoringVolumes);
        // Keep reviewed fixed luminance; reduce the cooked sunlit-body clipping by one stop.
        // The donor's +1.1 exposure bias washes out bodies in the cooked renderer.
        // This transient high-priority volume neither saves nor changes donor art.
        if(auto* Review=GetWorld()->SpawnActor<APostProcessVolume>())
        {
            Review->bUnbound=true;Review->Priority=1000;Review->BlendWeight=1;
            Review->Tags.Add(TEXT("Soul.Runtime.AuthoredExposureNormalized"));
            auto& P=Review->Settings;
            const auto* Extended=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"));
            const float Bound=Extended&&Extended->GetInt()!=0?0.f:1.f;
            P.bOverride_AutoExposureMinBrightness=P.bOverride_AutoExposureMaxBrightness=true;
            P.AutoExposureMinBrightness=P.AutoExposureMaxBrightness=Bound;
            P.bOverride_AutoExposureBias=true;P.AutoExposureBias=-1;
            P.bOverride_MotionBlurAmount=true;P.MotionBlurAmount=0;
            UE_LOG(LogTemp,Display,TEXT("SOUL_WOODLAND_PRESENTATION exposure_bound=%g bias=-1 motion_blur=0 source_unchanged=1"),Bound);
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
            bSiege=Encounter->bSiege;
            if(bSiege){SiegeState=FSoulSiegeRules::Begin({});SiegeState.GateMaximumIntegrity=Encounter->SiegeGateMaximum;SiegeState.GateIntegrityPermille=Encounter->SiegeGateIntegrity;}
            ActiveCap = Encounter->ActiveCapPerSide;
            StrategicBodies[0] = Encounter->PlayerStrategicCount;
            StrategicBodies[1] = Encounter->EnemyStrategicCount;
            PlayerVisualFaction = Encounter->PlayerFaction;
            PlayerVisualUnitId = Encounter->PlayerUnitId;
            CampaignCompanies[0]=Encounter->PlayerCompanies;CampaignCompanies[1]=Encounter->EnemyCompanies;
            EnemyVisualFaction = Encounter->EnemyFaction;
            EnemyVisualUnitId = Encounter->EnemyUnitId;
            EnemyVisualRegion = Encounter->TargetRegion;
            ControlledSide = Encounter->TacticalPlayerSide;
            bCampaignAutoResolve = Encounter->bAutoResolve;
            bAutobattle = bAutobattle || Encounter->bAutoResolve;
            PlayerMana = static_cast<float>(Encounter->PlayerMana);
            bCampaignHeroAvailable=Encounter->bPlayerHeroAvailable;NonPlayerHeroId=Encounter->NonPlayerHeroId;NonPlayerHeroFaction=Encounter->NonPlayerHeroFaction;
            bRestrictPlayerSpells=Encounter->bRestrictPlayerSpells;AllowedPlayerSpells=Encounter->AllowedPlayerSpells;
            // Automatic casting belongs to explicit qualification. Normal play
            // uses the existing [1] input and must not spend mana on its own.
            bTacticalMagic = bTacticalMagic || bQualification;
            UE_LOG(LogTemp, Display, TEXT("SOUL_CAMPAIGN_BATTLE_BEGIN: encounter=%s target=%s map=%s player=%s:%s:%d enemy=%s:%s:%d cap=%d"),
                *Encounter->EncounterId.ToString(), *Encounter->TargetRegion.ToString(), *Encounter->MapPackage.ToString(),
                *Encounter->PlayerFaction.ToString(), *Encounter->PlayerUnitId.ToString(), StrategicBodies[0],
                *Encounter->EnemyFaction.ToString(), *Encounter->EnemyUnitId.ToString(), StrategicBodies[1], ActiveCap);
        }
    }
    // The reviewed native city has wooded banks, not a ready-made arena. Keep
    // its architecture, ground and low vegetation; clear only tree trunks from
    // the bounded northwestern approach in this transient battle world. Native
    // packages and the independently loaded visit world are never modified.
    if (GetWorld()->GetOutermost()->GetName() == TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored")
        && ArenaOrigin.Equals(FVector(-9000, 21000, 400), 1.0))
    {
        int32 Removed = 0;
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            TInlineComponentArray<UInstancedStaticMeshComponent*> Components(*It);
            for (auto* Component : Components)
            {
                const UStaticMesh* Mesh = Component->GetStaticMesh();
                if (!Mesh) continue;
                const FString Path = Mesh->GetPathName();
                if (!Path.StartsWith(TEXT("/Game/CastleTown/Scanned_Foliage/Foliage/"))
                    || !(Mesh->GetName().StartsWith(TEXT("SM_EuropeanBeech_"))
                        || Mesh->GetName().StartsWith(TEXT("SM_SilverFir_")))) continue;
                TArray<int32> Indices;
                for (int32 I = 0; I < Component->GetInstanceCount(); ++I)
                {
                    FTransform Transform;
                    if (!Component->GetInstanceTransform(I, Transform, true)) continue;
                    const FVector Delta = Transform.GetLocation() - ArenaOrigin;
                    // Elliptical edge keeps the surrounding authored woodland.
                    if (FMath::Square(Delta.X / 5000.) + FMath::Square(Delta.Y / 4300.) < 1.)
                        Indices.Add(I);
                }
                if (!Indices.IsEmpty() && Component->RemoveInstances(Indices)) Removed += Indices.Num();
            }
        }
        UE_LOG(LogTemp, Display, TEXT("SOUL_HUMAN_APPROACH_CLEARING trees=%d origin=%s radii_cm=5000,4300 donor_writes=0"), Removed, *ArenaOrigin.ToCompactString());
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

    if(bDragon) DressDragonBattlefield();
    SetupSpellBar();
    SetupBattleAudio();
    if(bVisualUnits)
        for(int32 Side=0;Side<2;++Side)
            for(int32 RoleIndex=0;RoleIndex<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++RoleIndex)
                for(int32 Variant=0;Variant<4;++Variant)
                    if(auto* Clip=ResolveVisualAttack(Side,static_cast<ESoulRealtimeFormationRole>(RoleIndex),Variant))
                        BattlePresentationAssets.AddUnique(Clip);
    if(bVisualUnits)
        for(int32 Side=0;Side<2;++Side)
        {
            if(auto* Fall=ResolveAerialFall(Side)) BattlePresentationAssets.AddUnique(Fall);
            for(int32 RoleIndex=0;RoleIndex<=static_cast<int32>(ESoulRealtimeFormationRole::Hero);++RoleIndex)
            {
                const auto VisualRole=static_cast<ESoulRealtimeFormationRole>(RoleIndex);
                if(auto* Clip=ResolveVisualReaction(Side,VisualRole)) BattlePresentationAssets.AddUnique(Clip);
                if(auto* Clip=ResolveVisualDeath(Side,VisualRole)) BattlePresentationAssets.AddUnique(Clip);
                for(int32 Mode=0;Mode<6;++Mode)
                    if(auto* Clip=ResolveStanceAnimation(Side,VisualRole,Mode)) BattlePresentationAssets.AddUnique(Clip);
            }
        }
    InitialAlive[0] = AliveForSide(0);
    InitialAlive[1] = AliveForSide(1);
    SetupReinforcementState();
    if (!bAutobattle && PlayerHero)
    {
        ToggleBattlePause();
        bSelectAllAllies = true;
        SelectedAlliedFormation = INDEX_NONE;
        ToggleBattleCamera();
        Status = TEXT("DEPLOYMENT PAUSED - click a formation and an order, then Resume");
    }
    if (bAutobattle || !PlayerHero) {SetupBattleCamera();if(!bAutobattle&&!PlayerHero)bTacticalCameraActive=true;}
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
    if(bSiege&&!SetupSiege())return false;
    if (!SpawnArmy(0) || !SpawnArmy(1) || !SeparateBattleHeroesFromFormations() || !SetupDrivers())
        return false;

    if (!bProof && !bAutobattle && PlayerHero)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (!PC) return false;
        PC->Possess(PlayerHero);
        const FRotator InitialView(
            -12.0f, PlayerHero->GetActorRotation().Yaw, 0.0f);
        PC->SetControlRotation(InitialView);
        PC->SetViewTarget(PlayerHero);
        if (PlayerCameraArm)
            PlayerCameraArm->TickComponent(0.0f, LEVELTICK_All, &PlayerCameraArm->PrimaryComponentTick);
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        AddTickPrerequisiteActor(PC);
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
        // Invisible containment must not intercept the command/spell ray from
        // the commander camera, which can sit outside the physical arena.
        Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
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
    return bReady && BattlefieldBounds.Num() == 4 && (!IsHeartlandBridgeBattle() || SetupHeartlandBridgeCollision());
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
    if (!CampaignCompanies[Side].IsEmpty())
    {
        // Allocate the active cap fairly, then keep each exact company in its
        // own command group. Paid archers must not become a mixed melee card.
        const TArray<FName> Ids={TEXT("human_knight"),TEXT("human_archer"),TEXT("human_guard")};
        TMap<FName,int32> Counts;
        for(int32 N=0;N<Remaining;++N)
        {
            FName Best;int32 Lowest=MAX_int32;
            for(FName Id:Ids)if(Counts.FindRef(Id)<CampaignCompanies[Side].FindRef(Id)&&Counts.FindRef(Id)<Lowest)
                {Best=Id;Lowest=Counts.FindRef(Id);}
            if(Best.IsNone())return false;
            ++Counts.FindOrAdd(Best);
        }
        const float Direction=Side==0?1.f:-1.f;
        int32 Slot=0;
        for(FName Id:Ids)
        {
            int32 CompanyRemaining=Counts.FindRef(Id);
            while(CompanyRemaining>0)
            {
            const int32 Count=FMath::Min(CompanyRemaining,FRBCombatGroup::MaximumMembers);
            const bool Archer=Id==TEXT("human_archer");
            const auto CompanyRole=Archer?ESoulRealtimeFormationRole::Ranged:Id==TEXT("human_guard")?ESoulRealtimeFormationRole::Guard:ESoulRealtimeFormationRole::Line;
            const FVector Anchor=bSiege?SiegeDeployment(Side,Slot++):ArenaOrigin+FVector(-Direction*(Archer?2120.f:1500.f),Direction*(Slot++-1)*650.f,100.f);
            if(!SpawnFormation(Side,CompanyRole,Count,Anchor,Id))return false;
            FSoulBattleFormationState State;State.GroupIndex=Groups.Num()-1;State.Side=Side;
            State.Kind=Archer?ESoulBattleFormationKind::MissileSupport:ESoulBattleFormationKind::FrontLine;
            State.SpawnAnchor=Anchor;State.TacticalAnchor=bSiege?Anchor+SiegeForward*(Direction*650):Anchor+FVector(Direction*650,0,0);
            State.InitialBodies=State.PreviousAlive=Count;
            State.DisplayName=Archer?TEXT("Heartland Archers"):Id==TEXT("human_guard")?TEXT("Veteran Guard"):TEXT("Heartland Infantry");
            TacticalFormations.Add(State);
            CompanyRemaining-=Count;
            }
        }
        return true;
    }
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

    if (UsesControlledExactInfantry()||bSiege)
    {
        Roles.Init(ESoulRealtimeFormationRole::Line,GroupCount);
        Kinds.Init(ESoulBattleFormationKind::FrontLine,GroupCount);
    }
    const float Direction = Side == 0 ? 1.0f : -1.0f;
    const float BaseX = Side == 0 ? -1500.0f : 1500.0f;
    int32 FrontLineSlot = 0;
    for (int32 G = 0; G < GroupCount; ++G)
    {
        const int32 SlotsLeft = GroupCount - G;
        const int32 Count = FMath::Clamp(
            FMath::DivideAndRoundUp(Remaining, SlotsLeft), 1, 8);
        float Rearward = 0.0f, Lateral = 0.0f;
        switch (Kinds[G])
        {
            case ESoulBattleFormationKind::MissileSupport: Rearward = 620.0f; Lateral = 250.0f; break;
            case ESoulBattleFormationKind::Strike: Rearward = 150.0f; Lateral = -650.0f; break;
            case ESoulBattleFormationKind::CommandReserve: Rearward = 850.0f; Lateral = 700.0f; break;
            default: Lateral = GroupCount == 5 ? (FrontLineSlot++ == 0 ? -260.0f : 360.0f) : 250.0f; break;
        }
        if (UsesControlledExactInfantry())
            Lateral=(G-(GroupCount-1)*.5f)*650.f;
        const FVector Anchor = bSiege?SiegeDeployment(Side,G):ArenaOrigin + FVector(
            BaseX - Direction * Rearward, Direction * Lateral, 100.0f);
        if (!SpawnFormation(Side, Roles[G], Count, Anchor))
            return false;

        FSoulBattleFormationState State;
        State.GroupIndex = Groups.Num() - 1;
        State.Side = Side;
        State.Kind = Kinds[G];
        State.SpawnAnchor = Anchor;
        State.TacticalAnchor = bSiege?Anchor+SiegeForward*(Direction*650):Anchor + FVector(Direction * 650.0f, 0, 0);
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
    const FVector& Anchor, FName ExactCompany)
{
    if (Count <= 0 || Count > FRBCombatGroup::MaximumMembers)
        return false;
    // The admitted Orc proof fields hammer infantry only. Tactical deployment
    // and reserve delivery cannot silently introduce unrelated creature/archer assets.
    FormationRole = CampaignFormationRole(Side,FormationRole);

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

    if(bSiege)Core.Facing=Side==0?SiegeForward:-SiegeForward;
    const int32 Columns = FMath::Min(bSiege?2:4, Count);
    const float Spacing = FormationRole == ESoulRealtimeFormationRole::Shock ? 350.0f :
        FormationRole == ESoulRealtimeFormationRole::Ranged ? 175.0f : 135.0f;
    for (int32 I = 0; I < Count; ++I)
    {
        const int32 Row = I / Columns;
        const int32 Col = I % Columns;
        const int32 RowMembers = FMath::Min(Columns, Count - Row * Columns);
        const float Center = (RowMembers - 1) * 0.5f;
        FVector Location = Anchor;
        Location.X += (Side == 0 ? -1.0f : 1.0f) * Row * Spacing;
        Location.Y += (Side == 0 ? 1.0f : -1.0f) * (Col - Center) * Spacing;
        if(bSiege)Location=Anchor+SiegeForward*((Side==0?-1.f:1.f)*Row*105.f)+SiegeSide*((Col-Center)*95.f);
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
        SpawningCompany=ExactCompany.IsNone()?NextCampaignCompany(Side):ExactCompany;
        if(!CampaignCompanies[Side].IsEmpty())
        {
            if(SpawningCompany.IsNone())return false;
            MemberRole=SpawningCompany==TEXT("human_archer")?ESoulRealtimeFormationRole::Ranged:
                SpawningCompany==TEXT("human_guard")?ESoulRealtimeFormationRole::Guard:ESoulRealtimeFormationRole::Line;
        }
        // Reserve one existing active slot for the player even in small armies.
        // A separate command formation is optional; an embodied camera is not.
        const bool bPlayer =
            !bProof && !bAutobattle && bCampaignHeroAvailable && Side == ControlledSide && !PlayerHero && I == 0;
        if (bPlayer) MemberRole = ESoulRealtimeFormationRole::Hero;
        else if(!NonPlayerHeroId.IsNone()&&CampaignFactionForSide(Side)==NonPlayerHeroFaction
            &&!Combatants.ContainsByPredicate([](const auto& C){return C.bNonPlayerHero;}))MemberRole=ESoulRealtimeFormationRole::Hero;
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
    const FVector& Desired) const
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
    const FVector Start = bSiege?FVector(Desired.X,Desired.Y,SiegeGateBase.Z+180):Desired + FVector(0, 0, 8000.0);
    const FVector End = Desired - FVector(0, 0, 12000.0);
    FCollisionObjectQueryParams GroundObjects;
    GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
    // Authored walkway pieces may ignore Visibility but are real collision floors.
    // Siege deployment must seat on that deck rather than landscape below it.
    const bool GroundHit=bSiege
        ? GetWorld()->LineTraceSingleByObjectType(Hit,Start,End,GroundObjects,Query)
        : GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Query);
    if (GroundHit)
    {
        return FVector(Desired.X, Desired.Y, Hit.ImpactPoint.Z + 96.0);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("SOUL_RT_GROUND_MISS: desired=%s"),
        *Desired.ToCompactString());
    return Desired;
}

USkeletalMesh* ASoulRealtimeArenaGameMode::ResolveVisualMesh(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))return LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Animals_Warrior_Pack/Mesh/Bear/SK_Bear_Full.SK_Bear_Full"));
    if(UsesVikingCampaignRoster(Side))return LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Fantasy_Pack/Characters/Viking_Ulf/Mesh/SK_Ulf_Full.SK_Ulf_Full"));
    if(UsesOrcCampaignRoster(Side))return LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Mesh/SK_Orc_Hummer.SK_Orc_Hummer"));
    Side=CampaignVisualSide(Side);
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Ranged)
        return LoadObject<USkeletalMesh>(nullptr, Side == 0
            ? TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow.Sparrow")
            : TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Skins/Raven/Meshes/Sparrow_Raven.Sparrow_Raven"));
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
        Path = TEXT("/Game/AfricanAnimalsPack/Elephant/Meshes/SK_Elephant.SK_Elephant");
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
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_04/Mesh_UE4/Full_Mesh/SK_Knight_04_Full_01.SK_Knight_04_Full_01");
                break;
            case ESoulRealtimeFormationRole::Breaker:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_05/Mesh_UE5/Full_Mesh/SKM_Knight_05_Full_01.SKM_Knight_05_Full_01");
                break;
            case ESoulRealtimeFormationRole::Shock:
            case ESoulRealtimeFormationRole::Ranged:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_03/Mesh_UE4/Full/SK_Knight_03_Full_01.SK_Knight_03_Full_01");
                break;
            case ESoulRealtimeFormationRole::Hero:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_01/Mesh_UE5/Knight_01_Full/SKM_Knight_01_Full_01.SKM_Knight_01_Full_01");
                break;
            default:
                Path = TEXT("/Game/Knights_Pack/Meshes/Knight_02/Mesh_UE4/Full/SK_Knight_02_Full_01.SK_Knight_02_Full_01");
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
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))return NatureInfantryAnimation(bRunning?TEXT("Run"):TEXT("Idle"));
    if(UsesVikingCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,bRunning
        ? TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run")
        : TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Idle.Anim_Warrior_Idle"));
    if(UsesOrcCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,bRunning
        ? TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Run.Anim_Orc_Hummer_Run")
        : TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Idle.Anim_Orc_Hummer_Idle"));
    Side=CampaignVisualSide(Side);
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Ranged)
        return LoadObject<UAnimationAsset>(nullptr, bRunning
            ? TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Jog_Fwd.Jog_Fwd")
            : TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/idle.idle"));
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
            ? TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_Walk.ANIM_Elephant_Walk")
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
        Path = bRunning
            ? TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Run.Anim_Warrior_Run")
            : TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Idle.Anim_Warrior_Idle");
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
    int32 Side, ESoulRealtimeFormationRole FormationRole, int32 Variation) const
{
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))
    {
        static const TCHAR* Attacks[]={TEXT("Attack_1"),TEXT("Attack_2"),TEXT("Attack_3"),TEXT("Attack_4")};
        return NatureInfantryAnimation(Attacks[FMath::Abs(Variation%4)]);
    }
    if(UsesVikingCampaignRoster(Side))
    {
        static const TCHAR* Attacks[]={TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_2.Anim_Warrior_Attack_2"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_3.Anim_Warrior_Attack_3"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_4.Anim_Warrior_Attack_4")};
        return LoadObject<UAnimationAsset>(nullptr,Attacks[FMath::Abs(Variation%4)]);
    }
    if(UsesOrcCampaignRoster(Side))
    {
        static const TCHAR* Attacks[]={TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_1.Anim_Orc_Hummer_Attack_1"),TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_2.Anim_Orc_Hummer_Attack_2"),TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_3.Anim_Orc_Hummer_Attack_3")};
        return LoadObject<UAnimationAsset>(nullptr,Attacks[FMath::Abs(Variation)%UE_ARRAY_COUNT(Attacks)]);
    }
    Side=CampaignVisualSide(Side);
    if (FormationRole == ESoulRealtimeFormationRole::Ranged) return nullptr;
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
        static const TCHAR* Attacks[]={
            TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_A.Primary_Attack_A"),
            TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_B.Primary_Attack_B"),
            TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_C.Primary_Attack_C")};
        Path=Attacks[Variation%3];
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 0 && FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = Variation%2 ? TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_TusksAttack2.ANIM_Elephant_TusksAttack2") : TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/ANIM_Elephant_TusksAttack1.ANIM_Elephant_TusksAttack1");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    if (Side == 1 && UsesEvilVisualRoster() &&
        FormationRole == ESoulRealtimeFormationRole::Breaker)
    {
        Path = Variation%2 ? TEXT("/Game/Kraken/Animations/KRAKEN_smashAttack.KRAKEN_smashAttack") : TEXT("/Game/Kraken/Animations/KRAKEN_sweepAttack.KRAKEN_sweepAttack");
        return LoadObject<UAnimationAsset>(nullptr, Path);
    }
    static const TCHAR* WarriorAttacks[]={TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_2.Anim_Warrior_Attack_2"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_3.Anim_Warrior_Attack_3"),TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_4.Anim_Warrior_Attack_4")};
    if (Side == 0)
    {
        Path = WarriorAttacks[Variation%4];
    }
    else if (!UsesEvilVisualRoster())
        Path = WarriorAttacks[Variation%4];
    else if (FormationRole == ESoulRealtimeFormationRole::Guard ||
             FormationRole == ESoulRealtimeFormationRole::Hero)
        {
        static const TCHAR* Attacks[]={TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_1.Anim_Orc_Hummer_Attack_1"),TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_2.Anim_Orc_Hummer_Attack_2"),TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Attack_3.Anim_Orc_Hummer_Attack_3")};
        Path=Attacks[Variation%3];
    }
    else if (FormationRole == ESoulRealtimeFormationRole::Breaker ||
             FormationRole == ESoulRealtimeFormationRole::Apex)
        Path = TEXT("/Game/Fantasy_Pack/Characters/Troll/Animations/Anim_Troll_Attack_1.Anim_Troll_Attack_1");
    else
        {
        static const TCHAR* Attacks[]={TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_1.Anim_Warrior_Attack_1"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_2.Anim_Warrior_Attack_2"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_3.Anim_Warrior_Attack_3"),TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Attack_4.Anim_Warrior_Attack_4")};
        Path=Attacks[Variation%4];
    }
    return Path ? LoadObject<UAnimationAsset>(nullptr, Path) : nullptr;
}

UAnimationAsset* ASoulRealtimeArenaGameMode::ResolveVisualDeath(
    int32 Side, ESoulRealtimeFormationRole FormationRole) const
{
    FormationRole=CampaignFormationRole(Side,FormationRole);
    if(UsesNatureCampaignRoster(Side))return NatureInfantryAnimation(TEXT("Dead_1"));
    if(UsesVikingCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Fantasy_Pack/Animations/1With_Weapon/Anim_Warrior_Dead_1.Anim_Warrior_Dead_1"));
    if(UsesOrcCampaignRoster(Side))return LoadObject<UAnimationAsset>(nullptr,TEXT("/Game/Fantasy_Pack/Characters/Orc_Hummer/Animations/Anim_Orc_Hummer_Dead.Anim_Orc_Hummer_Dead"));
    Side=CampaignVisualSide(Side);
    const TCHAR* Path = nullptr;
    if (FormationRole == ESoulRealtimeFormationRole::Ranged)
        return LoadObject<UAnimationAsset>(nullptr,
            TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/Death_Fwd.Death_Fwd"));
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

FName ASoulRealtimeArenaGameMode::NextCampaignCompany(int32 Side) const
{
    if(CampaignCompanies[Side].IsEmpty())return NAME_None;
    // Stable interleaving lets small active caps expose every recruited company.
    FName Best;int32 BestDeployed=MAX_int32;
    for(FName Id:{FName(TEXT("human_knight")),FName(TEXT("human_archer")),FName(TEXT("human_guard"))})
    {
        int32 Deployed=0;for(const auto& C:Combatants)if(C.Side==Side&&C.CampaignCompany==Id)++Deployed;
        if(Deployed<CampaignCompanies[Side].FindRef(Id)&&Deployed<BestDeployed){Best=Id;BestDeployed=Deployed;}
    }
    return Best;
}
TMap<FName,int32> ASoulRealtimeArenaGameMode::SurvivingCampaignCompanies(int32 Side,bool Defeated) const
{
    auto Result=CampaignCompanies[Side];
    for(auto& P:Result)
    {
        if(Defeated){P.Value=0;continue;}
        for(const auto& C:Combatants)if(C.Side==Side&&C.CampaignCompany==P.Key&&C.Health<=0)--P.Value;
    }
    return Result;
}
bool ASoulRealtimeArenaGameMode::UsesControlledExactInfantry() const
{
    return bCampaignBattle && ((!PlayerVisualFaction.IsNone() && PlayerVisualFaction!=TEXT("humans")) || UsesNatureCampaignRoster(1));
}
FName ASoulRealtimeArenaGameMode::CampaignFactionForSide(int32 Side) const
{ return Side==0?PlayerVisualFaction:Side==1?EnemyVisualFaction:NAME_None; }
FName ASoulRealtimeArenaGameMode::CampaignUnitForSide(int32 Side) const
{ return Side==0?PlayerVisualUnitId:Side==1?EnemyVisualUnitId:NAME_None; }
int32 ASoulRealtimeArenaGameMode::CampaignVisualSide(int32 Side) const
{
    if (!UsesControlledExactInfantry()) return Side;
    // Reuse the already qualified Human/Dwarf visual families independently of spawn side.
    return CampaignFactionForSide(Side)==TEXT("humans")?0:1;
}
bool ASoulRealtimeArenaGameMode::UsesOrcCampaignRoster(int32 Side) const
{
    return bCampaignBattle && (Side==0 || Side==1) && CampaignFactionForSide(Side)==TEXT("orcs")
        && CampaignUnitForSide(Side)==TEXT("orc_hammer_warrior");
}
bool ASoulRealtimeArenaGameMode::UsesNatureCampaignRoster(int32 Side) const
{
    return bCampaignBattle && (Side==0 || Side==1) && CampaignFactionForSide(Side)==TEXT("nature")
        && CampaignUnitForSide(Side)==TEXT("nature_bear_warrior");
}
UAnimationAsset* ASoulRealtimeArenaGameMode::NatureInfantryAnimation(const TCHAR* Clip) const
{
    const FString Name=FString(TEXT("Anim_Warrior_"))+Clip;
    return LoadObject<UAnimationAsset>(nullptr,*(FString(TEXT("/Game/Animals_Warrior_Pack/Animations/1With_Weapon/"))+Name+TEXT(".")+Name));
}
bool ASoulRealtimeArenaGameMode::UsesVikingCampaignRoster(int32 Side) const
{
    return bCampaignBattle && (Side==0 || Side==1) && CampaignFactionForSide(Side)==TEXT("vikings")
        && CampaignUnitForSide(Side)==TEXT("viking_axe_warrior");
}
ESoulRealtimeFormationRole ASoulRealtimeArenaGameMode::CampaignFormationRole(int32 Side,ESoulRealtimeFormationRole Requested) const
{
    if(Side>=0&&Side<=1&&!CampaignCompanies[Side].IsEmpty()&&CampaignFactionForSide(Side)==TEXT("humans")
        &&(Requested==ESoulRealtimeFormationRole::Line||Requested==ESoulRealtimeFormationRole::Guard||Requested==ESoulRealtimeFormationRole::Ranged||Requested==ESoulRealtimeFormationRole::Hero))return Requested;
    if(Requested==ESoulRealtimeFormationRole::Hero&&NonPlayerHeroId==TEXT("dwarf_king_commander")&&CampaignFactionForSide(Side)==NonPlayerHeroFaction)return Requested;
    if (!bAutobattle && ControlledSide==1 && Side==ControlledSide && Requested==ESoulRealtimeFormationRole::Hero && CampaignFactionForSide(Side)==TEXT("humans")) return Requested;
    return (UsesControlledExactInfantry()||UsesOrcCampaignRoster(Side)||UsesVikingCampaignRoster(Side)||UsesNatureCampaignRoster(Side))?ESoulRealtimeFormationRole::Line:Requested;
}
bool ASoulRealtimeArenaGameMode::UsesEvilVisualRoster() const
{
    if (UsesControlledExactInfantry()) return false; // exact Orc/Viking paths are resolved before legacy families.

    return EnemyVisualFaction == FName(TEXT("orcs")) ||
           EnemyVisualFaction == FName(TEXT("evil")) ||
           EnemyVisualRegion == FName(TEXT("orc_watch")) ||
           EnemyVisualRegion == FName(TEXT("north_pass")) ||
           EnemyVisualRegion == FName(TEXT("orc_camp")) ||
           FParse::Param(FCommandLine::Get(), TEXT("SoulEvilVisualRoster"));
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
    Data.CampaignCompany=SpawningCompany;
    Data.Health = bKraken ? 360.0f :
        (bElephant ? 320.0f : RoleHealth(FormationRole));
    // Give a small physical army time to maneuver and the player time to react.
    // This is encounter balance, shared by both sides and reserve arrivals.
    Data.Health *= 1.6f;
    Data.MaxHealth = Data.Health;
    Data.BaseWalkSpeed = bKraken ? 260.0f : (bElephant ? 310.0f : RoleWalkSpeed(FormationRole));
    Data.Arrows =
        FormationRole == ESoulRealtimeFormationRole::Ranged ? 32 : 0;
    Data.GroupIndex = GroupIndex;
    Data.bRanged = FormationRole == ESoulRealtimeFormationRole::Ranged;
    Data.bPlayerHero = bPlayer;
    Data.bNonPlayerHero=!bPlayer&&FormationRole==ESoulRealtimeFormationRole::Hero&&!NonPlayerHeroId.IsNone()&&CampaignFactionForSide(Side)==NonPlayerHeroFaction;
    Data.Movement = ResolveMovementArchetype(Side, FormationRole);
    const int32 NewIndex = Combatants.Add(Data);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FRotator Facing=bSiege?(Side==0?SiegeForward:-SiegeForward).Rotation():FRotator(0, Side == 0 ? 0.0f : 180.0f, 0);
    const FVector SpawnLocation = ResolveSpawnLocation(Location);
    if(bSiege)UE_LOG(LogTemp,Display,TEXT("SOUL_SIEGE_DEPLOY side=%d company=%s position=%s"),Side,*Data.CampaignCompany.ToString(),*SpawnLocation.ToCompactString());
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
    // Camera collision respects scenery, not other troops' capsules.
    Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
        ECC_Camera, ECR_Ignore);
    Actor->GetCharacterMovement()->MaxWalkSpeed =
        Data.BaseWalkSpeed;
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

    if(!bPlayer)
    {
        auto* Movement=Actor->GetCharacterMovement();
        FNavAvoidanceMask Lane;
        Lane.Packed=Data.Movement==ESoulRealtimeMovementArchetype::Aerial ? 2 : 1;
        Movement->SetAvoidanceGroupMask(Lane);
        Movement->SetGroupsToAvoidMask(Lane);
        Movement->AvoidanceConsiderationRadius=650.0f;
        Movement->SetAvoidanceEnabled(true);
    }

    // ResolveSpawnLocation returns terrain + the standard 96 cm capsule.
    // Larger creatures must start with their own capsule base on that terrain,
    // including the deployment frame before physics has had a chance to settle.
    Actor->AddActorWorldOffset(FVector(0, 0,
        Actor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 96.0f));

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
        Visual->SetRelativeRotation(VisualMeshRotation(Data.Movement));
        if (Data.Movement == ESoulRealtimeMovementArchetype::Aerial)
        {
            const float ApexScale = Side == 0 ? 0.55f : 0.28f;
            Visual->SetRelativeLocation(FVector(0, 0, 180.0f));
            Visual->SetRelativeScale3D(FVector(ApexScale));
        }
        else if (Data.Movement == ESoulRealtimeMovementArchetype::LowProfile)
        {
            // The donor Kraken is enormous. A 20% presentation preserves its
            // broad silhouette while keeping it in the same elite-creature band
            // as the elephant, griffin, and mountain dragon.
            const float BodyScale=bKraken ? .20f : .78f;
            const auto Bounds=Mesh->GetBounds();
            const float GroundOffset=-Actor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
                -(Bounds.Origin.Z-Bounds.BoxExtent.Z)*BodyScale;
            Visual->SetRelativeLocation(FVector(0,0,GroundOffset));
            UE_LOG(LogTemp,Display,TEXT("SOUL_LOW_PROFILE_GROUND: mesh=%s offset=%.2f"),
                *Mesh->GetName(),GroundOffset);
            Visual->SetRelativeScale3D(FVector(
                bKraken ? 0.20f : 0.78f));
        }
        else if (bElephant)
        {
            // Use the complete body's bounds, not the separate tusk accessory.
            // Uniform scaling preserves anatomy; place the mesh's feet at the
            // capsule base rather than burying it with an accessory's offset.
            const FBoxSphereBounds Bounds = Mesh->GetBounds();
            const float Scale = 330.0f / FMath::Max(1.0f, float(Bounds.BoxExtent.Z * 2.0));
            Visual->SetRelativeScale3D(FVector(Scale));
            Visual->SetRelativeLocation(FVector(0, 0,
                -Actor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
                - (Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale));
            UE_LOG(LogTemp, Display, TEXT("SOUL_ELEPHANT_BODY: mesh=%s bounds=%s scale=%.3f footOffset=%.1f"),
                *Mesh->GetName(), *Bounds.BoxExtent.ToCompactString(), Scale,
                Visual->GetRelativeLocation().Z);
        }
        else
        {
            Visual->SetRelativeLocation(FVector(0, 0, -90.0f));
        }
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Visual->PlayAnimation(Idle, true);
        Visual->SetPosition(Idle->GetPlayLength() * FMath::Frac((NewIndex + 1) * 0.618034f));
        // Deployment freezes world time before the first skeletal tick. Evaluate
        // the selected pose now so troops and head sockets are ready at spawn.
        Visual->TickAnimation(0.0f, false);
        Visual->RefreshBoneTransforms();
        Visual->UpdateComponentToWorld();
        EquipVisualWeapons(Actor, Side, FormationRole);
        if(Data.bNonPlayerHero)UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_COMMANDER id=%s faction=%s mesh=%s idle=%s role=Hero uses_existing_slot=1"),*NonPlayerHeroId.ToString(),*NonPlayerHeroFaction.ToString(),*Mesh->GetPathName(),*Idle->GetPathName());
        if (bCampaignBattle && !Data.bNonPlayerHero)
            UE_LOG(LogTemp,Display,TEXT("SOUL_EXACT_ROSTER_BODY side=%d faction=%s unit=%s index=%d mesh=%s role=%s health=%.1f"),
                Side,*CampaignFactionForSide(Side).ToString(),*(Data.CampaignCompany.IsNone()?CampaignUnitForSide(Side):Data.CampaignCompany).ToString(),NewIndex,*Mesh->GetPathName(),*RoleLabel(FormationRole),Data.Health);
        if(UsesVikingCampaignRoster(Side))
            UE_LOG(LogTemp,Display,TEXT("SOUL_VIKING_ROSTER_BODY index=%d faction=vikings unit=viking_axe_warrior mesh=%s idle=%s role=%s health=%.1f group=%d scale=%s facing=%s"),NewIndex,*Mesh->GetPathName(),*Idle->GetPathName(),*RoleLabel(FormationRole),Data.Health,GroupIndex,*Visual->GetRelativeScale3D().ToCompactString(),*Visual->GetRelativeRotation().ToCompactString());
        if(UsesOrcCampaignRoster(Side))
            UE_LOG(LogTemp,Display,TEXT("SOUL_ORC_ROSTER_BODY index=%d faction=orcs unit=orc_hammer_warrior mesh=%s idle=%s role=%s health=%.1f group=%d"),NewIndex,*Mesh->GetPathName(),*Idle->GetPathName(),*RoleLabel(FormationRole),Data.Health,GroupIndex);

        if (FormationRole == ESoulRealtimeFormationRole::Apex ||
            bKraken || bElephant)
        {
            const TCHAR* Creature = bElephant ? TEXT("Elephant") :
                (bKraken ? TEXT("Kraken") :
                    (Side == 0 ? TEXT("Griffin") : TEXT("MountainDragon")));
            const float CreatureScale = bElephant ? Visual->GetRelativeScale3D().X :
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
        Settings.FullDrawSeconds = 1.2f;
        Settings.MinimumSpeed = 1750.0f;
        Settings.MaximumSpeed = 3100.0f;
        Settings.GravityScale = 0.35f;
        Settings.LifetimeSeconds = 4.0f;
        Settings.Radius = 3.0f;
        if (!Bow->ConfigureBow(Settings)) return false;
        Bow->ProjectileClass = ASoulBattleArrow::StaticClass();
        Ranged[NewIndex] = Bow;
    }

    if (bPlayer)
    {
        PlayerHero = Actor;
        USpringArmComponent* Arm =
            NewObject<USpringArmComponent>(Actor);
        PlayerCameraArm = Arm;
        Actor->AddInstanceComponent(Arm);
        Arm->SetupAttachment(Actor->GetRootComponent());
        Arm->TargetArmLength = 460.0f;
        Arm->TargetOffset = FVector(0, 0, 55.0f);
        Arm->PrimaryComponentTick.bTickEvenWhenPaused = true;
        Arm->SetRelativeRotation(FRotator(-12, 0, 0));
        Arm->bUsePawnControlRotation = true;
        Arm->RegisterComponent();

        UCameraComponent* Camera =
            NewObject<UCameraComponent>(Actor);
        Actor->AddInstanceComponent(Camera);
        Camera->SetupAttachment(
            Arm, USpringArmComponent::SocketName);
        Camera->bUsePawnControlRotation = false;
        Camera->PrimaryComponentTick.bTickEvenWhenPaused = true;
        Camera->RegisterComponent();
        Camera->AttachToComponent(Arm, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            USpringArmComponent::SocketName);
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
    if (bAutobattle && Side == ControlledSide && !PlayerHero) PlayerHero = Actor;
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
        const int32 StateIndex = FindFormationState(I);
        const ESoulBattleFormationKind Kind = TacticalFormations.IsValidIndex(StateIndex)
            ? TacticalFormations[StateIndex].Kind : ESoulBattleFormationKind::FrontLine;
        Driver->FormationSpacing = Kind == ESoulBattleFormationKind::Strike ? 350.0f :
            Kind == ESoulBattleFormationKind::MissileSupport ? 175.0f : 135.0f;
        if(IsHeartlandBridgeBattle()||bSiege)Driver->FormationSpacing=90.f;
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
        Formation.UnitId = (UsesControlledExactInfantry()||UsesOrcCampaignRoster(Side)||UsesVikingCampaignRoster(Side)||UsesNatureCampaignRoster(Side))?CampaignUnitForSide(Side):FName(*RoleLabel(Combatants[LeaderIndex].Role));
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
        Reserve.UnitId = (UsesControlledExactInfantry()||UsesOrcCampaignRoster(Side)||UsesVikingCampaignRoster(Side)||UsesNatureCampaignRoster(Side))?CampaignUnitForSide(Side):FName(TEXT("Soul.Reserve.Line"));
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

bool ASoulRealtimeArenaGameMode::ChooseReinforcementAnchor(
    int32 Side, FVector& OutAnchor) const
{
    if(bSiege){OutAnchor=SiegeDeployment(Side,2);return true;}
    const float X = Side == 0
        ? -BattlefieldHalfX + 600.0f
        : BattlefieldHalfX - 600.0f;
    const bool Woodland=GetWorld()->GetOutermost()->GetName()==TEXT("/Game/Soul/Maps/Battles/L_Heartland_Woodland");
    // Use the three physically measured clearing lanes, not the demo's rocks
    // at the generic field's 14m flank entry. Safety checks remain unchanged.
    const TArray<float> YOptions=Woodland?TArray<float>{0.f,-650.f,650.f}:TArray<float>{-1400.f,0.f,1400.f};

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
        const int32 Score =
            FSoulRealtimeTacticalRules::ScoreReinforcementAnchor(
                NearestEnemy,
                FVector::Dist2D(Candidate, FriendlyCenter),
                0.0f, NearestEnemy >= 1000.0f &&
                    IsDirectGroundRouteClear(GetWorld(),Candidate,
                        Candidate+FVector(Side==0 ? 350.0f : -350.0f,0,0)));
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Candidate;
        }
    }
    if(BestScore==MIN_int32) return false;
    OutAnchor=Best;
    return true;
}

bool ASoulRealtimeArenaGameMode::SpawnReinforcementWave(
    int32 Side, int32 Count, const FVector& ArrivalAnchor, FName ExactCompany)
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
    if(!CampaignCompanies[Side].IsEmpty() && ExactCompany.IsNone())
    {
        const TArray<FName> Ids={TEXT("human_knight"),TEXT("human_archer"),TEXT("human_guard")};
        TMap<FName,int32> Deployed,WaveCounts;
        for(const auto& C:Combatants)if(C.Side==Side)++Deployed.FindOrAdd(C.CampaignCompany);
        for(int32 N=0;N<Count;++N)
        {
            FName Best;int32 Fewest=MAX_int32;
            for(FName Id:Ids)if(Deployed.FindRef(Id)<CampaignCompanies[Side].FindRef(Id)&&Deployed.FindRef(Id)<Fewest)
                {Best=Id;Fewest=Deployed.FindRef(Id);}
            if(Best.IsNone())return false;
            ++Deployed.FindOrAdd(Best);++WaveCounts.FindOrAdd(Best);
        }
        int32 Lane=0,Companies=0;for(FName Id:Ids)if(WaveCounts.FindRef(Id)>0)++Companies;
        for(FName Id:Ids)if(const int32 Bodies=WaveCounts.FindRef(Id);Bodies>0)
        {
            FVector Entry=ArrivalAnchor;if(bSiege)Entry=SiegeDeployment(Side,2+Lane++);else Entry.Y=FMath::Clamp(Entry.Y+(Lane++-(Companies-1)*.5f)*650.f,ArenaOrigin.Y-BattlefieldHalfY+600.f,ArenaOrigin.Y+BattlefieldHalfY-600.f);
            if(!SpawnReinforcementWave(Side,Bodies,Entry,Id))return false;
        }
        bSpawnCommitted=true;return true;
    }
    const auto ArrivalRole=ExactCompany==TEXT("human_archer")?ESoulRealtimeFormationRole::Ranged:
        ExactCompany==TEXT("human_guard")?ESoulRealtimeFormationRole::Guard:ESoulRealtimeFormationRole::Line;
    if (!SpawnFormation(Side,ArrivalRole,Count,ArrivalAnchor,ExactCompany))return false;

    // Arrivals walk in from the edge but retain the established frontline's
    // identity and player order. Avoid accumulating one tiny UI group per wave.
    for(auto& Existing:TacticalFormations)
    {
        if(!ExactCompany.IsNone())break; // Exact-company arrivals keep homogeneous command groups.
        if(Existing.Side!=Side || Existing.Kind!=ESoulBattleFormationKind::FrontLine ||
            Existing.bRouting || AliveInGroup(Existing.GroupIndex)<=0) continue;
        FRBCombatGroup Candidate;
        const FRBHostGroup Original=Groups[Existing.GroupIndex];
        if(!Original.ToCore(Candidate)) continue;
        Candidate.Members.RemoveAll([this](const FRBCombatantRef& Member)
        {
            const int32 I=Index(FRBHostIdentity::From(Member));
            return I==INDEX_NONE || Combatants[I].Health<=0;
        });
        FRBCombatGroup NewCore;
        if(!Groups[NewGroupIndex].ToCore(NewCore)) continue;
        Candidate.Members.Append(NewCore.Members);
        ++Candidate.Revision;
        FRBHostGroup Merged;
        if(!FRBHostGroup::FromCore(Candidate,Merged)) continue;
        Groups[Existing.GroupIndex]=MoveTemp(Merged);
        for(int32 I=FirstCombatant;I<Combatants.Num();++I)
        {
            Combatants[I].GroupIndex=Existing.GroupIndex;
            Combatants[I].FormationId=ReserveFormationId(Side);
        }
        if(!RefreshDriverRepresentations())
        {
            Groups[Existing.GroupIndex]=Original;
            for(int32 I=FirstCombatant;I<Combatants.Num();++I) Combatants[I].GroupIndex=NewGroupIndex;
            return false;
        }
        Groups.Pop();
        Existing.InitialBodies+=Count;
        Existing.PreviousAlive+=Count;
        Existing.MoralePermille=FMath::Min(1000,Existing.MoralePermille+100);
        UE_LOG(LogTemp,Display,TEXT("SOUL_REINFORCEMENT_JOIN: side=%d group=%d bodies=%d entry=%s"),
            Side,Existing.GroupIndex,Count,*ArrivalAnchor.ToCompactString());
        bSpawnCommitted=true;
        PushBattleNotice(FString::Printf(TEXT("%s reinforcements: %d troops"),
            Side==ControlledSide?TEXT("Your"):TEXT("Enemy"),Count),Side);
        return true;
    }

    FSoulBattleFormationState Tactical;
    Tactical.GroupIndex = NewGroupIndex;
    Tactical.Side = Side;
    Tactical.Kind = ArrivalRole==ESoulRealtimeFormationRole::Ranged?ESoulBattleFormationKind::MissileSupport:ESoulBattleFormationKind::FrontLine;
    if(!ExactCompany.IsNone())Tactical.DisplayName=ExactCompany==TEXT("human_archer")?TEXT("Heartland Archers"):ExactCompany==TEXT("human_guard")?TEXT("Veteran Guard"):TEXT("Heartland Infantry");
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
    Driver->FormationSpacing = (IsHeartlandBridgeBattle()||bSiege)?90.f:135.0f;
    Driver->SightDistance = 6200.0f;
    Driver->bAllowDirectSteering = true;
    Driver->RegisterComponent();
    Drivers.Add(Driver);
    bSpawnCommitted = RefreshDriverRepresentations();
    if(bSpawnCommitted) PushBattleNotice(FString::Printf(TEXT("%s reinforcements: %d troops"),
        Side==ControlledSide?TEXT("Your"):TEXT("Enemy"),Count),Side);
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

        FVector ArrivalAnchor;
        if(!ChooseReinforcementAnchor(Side,ArrivalAnchor))
        {
            Status=TEXT("Reserves are waiting for a safe battlefield entry");
            continue;
        }
        FSoulRealtimeBattleState Candidate = ReinforcementBattle;
        const FSoulReinforcementWave Wave =
            FSoulRealtimeBattleRules::BuildAndApplyWave(
                Candidate, SideId);
        const int32 Count =
            Wave.FormationCounts.FindRef(ReserveFormationId(Side));
        if (Count <= 0 || Count != Wave.TotalBodies())
            continue;

        if (SpawnReinforcementWave(Side, Count, ArrivalAnchor))
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
    const int32 VisualIndex = Index(Identity);
    if (bVisualUnits && Combatants.IsValidIndex(VisualIndex) &&
        Combatants[VisualIndex].bRanged && IsValid(Actors[VisualIndex]) &&
        (Decision.Intent == ERBHostCombatIntent::Draw || Decision.Intent == ERBHostCombatIntent::Release))
    {
        const bool bDraw = Decision.Intent == ERBHostCombatIntent::Draw;
        const TCHAR* Path = bDraw
            ? TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/RMB_Drawback.RMB_Drawback")
            : TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations/RMB_Fire.RMB_Fire");
        if (auto* Clip = LoadObject<UAnimationAsset>(nullptr, Path))
        {
            Actors[VisualIndex]->GetMesh()->PlayAnimation(Clip, false);
            Actors[VisualIndex]->GetMesh()->SetPlayRate(Clip->GetPlayLength() / (bDraw ? 1.2f : 0.35f));
            Combatants[VisualIndex].bVisualAttackPlaying = true;
        }
    }
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
    if(IsSiegeGate(IntendedTarget))return PerformGateMelee(AttackerIndex);
    const int32 TargetIndex = Index(IntendedTarget);
    if (bBattlePaused || !Combatants.IsValidIndex(AttackerIndex) ||
        TargetIndex == INDEX_NONE || !AreOpponents(IdentityAt(AttackerIndex), IntendedTarget))
        return false;
    auto& Data = Combatants[AttackerIndex];
    ACharacter* Actor = Actors[AttackerIndex];
    ACharacter* Target = Actors[TargetIndex];
    if (!IsValid(Actor) || !IsValid(Target) || Data.Health <= 0 ||
        Combatants[TargetIndex].Health <= 0 || Data.MeleeCooldown > 0 || Data.bRanged)
        return false;
    if (FVector::Dist2D(Actor->GetActorLocation(), Target->GetActorLocation()) >
        Profile(AttackerIndex).Reach + 95.0f)
        return false;
    UAnimationAsset* Clip = bVisualUnits ? ResolveVisualAttack(Data.Side, Data.Role, AttackerIndex + Data.VisualAttackSequence++) : nullptr;
    const float Duration = Clip ? FMath::Clamp(Clip->GetPlayLength(), 0.8f, 2.8f) : AttackDelay(Data.Role);
    if (Data.Role != ESoulRealtimeFormationRole::Apex && Data.Role != ESoulRealtimeFormationRole::Breaker)
        PlayBattleSound((Data.VisualAttackSequence & 1) ? ESoulBattleSound::SwordSwing1 :
            ESoulBattleSound::SwordSwing2, Actor->GetActorLocation(), AttackerIndex);
    if(Bindings.IsValidIndex(AttackerIndex) && Bindings[AttackerIndex]) Bindings[AttackerIndex]->SetGuardIntent(false);
    Data.MeleeCooldown = Duration;
    Data.PendingMeleeTarget = IntendedTarget;
    Data.PendingMeleeSeconds = Duration * 0.35f;
    Actor->SetActorRotation((Target->GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal2D().Rotation());
    Actor->GetCharacterMovement()->StopMovementImmediately();
    if (Clip)
    {
        Actor->GetMesh()->PlayAnimation(Clip, false);
        Actor->GetMesh()->SetPlayRate(Clip->GetPlayLength() / Duration);
        Data.bVisualAttackPlaying = true;
    }
    return true;
}

float ASoulRealtimeArenaGameMode::MeleeBodyReach(ACharacter* Attacker,ACharacter* Target,float BaseReach)
{
    if(!Attacker || !Target) return BaseReach;
    const float Extra=FMath::Max(0.f,Attacker->GetCapsuleComponent()->GetScaledCapsuleRadius()-42.f)+
        FMath::Max(0.f,Target->GetCapsuleComponent()->GetScaledCapsuleRadius()-42.f);
    return BaseReach+Extra+(Extra>0 ? 24.f : 0.f);
}

bool ASoulRealtimeArenaGameMode::TraceMeleeContact(
    ACharacter* Attacker, ACharacter* Intended, float Reach, FHitResult& OutHit)
{
    if (!IsValid(Attacker) || !IsValid(Intended) || !Attacker->GetWorld()) return false;
    const FVector Direction = (Intended->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return false;
    const float HalfHeight = Intended->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FVector Start = Attacker->GetActorLocation() + Direction * 35.0f;
    // Strike the target's body even when the attacker is a tall creature.
    Start.Z = FMath::Clamp(Start.Z + 45.0, Intended->GetActorLocation().Z - HalfHeight * 0.65,
        Intended->GetActorLocation().Z + HalfHeight * 0.65);
    const FVector End = Start + Direction * (Reach + 85.0f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SoulRealtimeArenaMelee), false, Attacker);
    // Movement overlaps are not attack immunity. All battle capsules block
    // Visibility, as does scenery; the first physical blocker still owns contact.
    return Attacker->GetWorld()->SweepSingleByChannel(OutHit, Start, End,
        FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(18.0f), Query);
}

bool ASoulRealtimeArenaGameMode::CommitMeleeImpact(
    int32 AttackerIndex, FRBHostIdentity IntendedTarget)
{
    if(IsSiegeGate(IntendedTarget))return CommitGateMelee(AttackerIndex);
    const int32 IntendedIndex = Index(IntendedTarget);
    if (!Combatants.IsValidIndex(AttackerIndex) ||
        IntendedIndex == INDEX_NONE ||
        !AreOpponents(IdentityAt(AttackerIndex), IntendedTarget))
        return false;
    auto& AttackerData = Combatants[AttackerIndex];
    if (AttackerData.Health <= 0.0f ||
        bBattlePaused ||
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

    FHitResult Hit;
    if (!TraceMeleeContact(Attacker, Intended, Weapon.Reach, Hit))
        return false;

    AActor* HitActor = Hit.GetActor();
    auto* VictimBinding = HitActor
        ? HitActor->FindComponentByClass<USoulRealtimeArenaBinding>()
        : nullptr;
    if (!VictimBinding) return false;

    const FRBHostIdentity Victim =
        FRBHostIdentity::From(VictimBinding->GetCombatant());
    if(IsSiegeGate(Victim))return CommitGateMelee(AttackerIndex);
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
    if(bAccepted && AttackerData.bPlayerHero && (FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandBattleQualification"))||FParse::Param(FCommandLine::Get(),TEXT("SoulHeartlandCompanyBattle"))))
    {
        UE_LOG(LogTemp,Display,TEXT("SOUL_HEARTLAND_HERO_MELEE accepted=1 authority=RBCombat attacker=%s victim=%s"),*GetNameSafe(Attacker),*GetNameSafe(HitActor));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Heartland_Hero_Melee.png"),true,false);
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
    bHeroSelected=false;
    TArray<int32> AlliedGroups;
    for (const FSoulBattleFormationState& State : TacticalFormations)
        if (State.Side == ControlledSide && AliveInGroup(State.GroupIndex) > 0)
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
        ? TEXT("ALL FORMATIONS selected")
        : SelectedFormationSummary();
}

void ASoulRealtimeArenaGameMode::SelectAlliedFormationSlot(int32 Slot)
{
    bHeroSelected=false;
    int32 CurrentSlot = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != ControlledSide) continue;
        if (CurrentSlot++ != Slot) continue;
        bSelectAllAllies = false;
        SelectedAlliedFormation = State.GroupIndex;
        Status = SelectedFormationSummary();
        return;
    }
    bSelectAllAllies = true;
    SelectedAlliedFormation = INDEX_NONE;
    Status = TEXT("ALL FORMATIONS selected");
}

void ASoulRealtimeArenaGameMode::ToggleBattlePause()
{
    if(bFinished) return;
    if(!bBattlePaused && !bAllowTacticalPause)
    { Status=TEXT("Tactical pause is disabled for this battle."); return; }
    const bool bPause = !bBattlePaused;
    if (!UGameplayStatics::SetGamePaused(this, bPause))
    {
        Status = TEXT("Unable to change battle pause");
        return;
    }
    bBattlePaused = bPause;
    UE_LOG(LogTemp, Display,
        TEXT("SOUL_BATTLE_PAUSE: paused=%d worldPaused=%d elapsed=%.2f alive=%d/%d hits=%d mana=%.1f"),
        bBattlePaused, UGameplayStatics::IsGamePaused(this), BattleElapsed,
        AliveForSide(0), AliveForSide(1), AcceptedContacts.Num(), PlayerMana);
    Status = bBattlePaused
        ? TEXT("BATTLE PAUSED - select a formation and issue orders")
        : bAllowTacticalPause ? TEXT("BATTLE RUNNING - [Space/P] pauses at any time") : TEXT("BATTLE RUNNING - tactical pause disabled");
}

void ASoulRealtimeArenaGameMode::ToggleBattleCamera()
{
    if (!PlayerHero || bAutobattle) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    if (bTacticalCameraActive && PlayerHealth() <= 0.0f)
    {
        Status = TEXT("Hero defeated - continue commanding your army");
        return;
    }
    if (bTacticalCameraActive)
    {
        PC->SetViewTarget(PlayerHero);
        PlayerHero->GetMesh()->SetOwnerNoSee(bFirstPersonCamera);
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        bTacticalCameraActive = false;
        Status = TEXT("HERO CAMERA - WASD move, mouse aim, LMB attack, RMB block");
    }
    else
    {
        SetupBattleCamera();
        PlayerHero->GetMesh()->SetOwnerNoSee(false);
        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
        bTacticalCameraActive = IsValid(TacticalCamera);
        Status = TEXT("COMMANDER - WASD pan, wheel zoom, Q/E or RMB orbit; F1-F5 select");
    }
}

void ASoulRealtimeArenaGameMode::ToggleFirstPersonCamera()
{
    if (!PlayerHero || !PlayerCameraArm || bAutobattle || PlayerHealth() <= 0.0f) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (bTacticalCameraActive)
    {
        PC->SetViewTarget(PlayerHero);
        PC->SetShowMouseCursor(false);
        PC->SetInputMode(FInputModeGameOnly());
        bTacticalCameraActive = false;
    }

    bFirstPersonCamera = !bFirstPersonCamera;
    PlayerCameraArm->TargetArmLength = bFirstPersonCamera ? 0.0f : 460.0f;
    const FVector EyeOffset = PlayerHero->GetMesh()->DoesSocketExist(TEXT("head"))
        ? PlayerHero->GetMesh()->GetSocketLocation(TEXT("head")) - PlayerHero->GetActorLocation()
            + FVector(0, 0, 8.0f)
        : FVector(0, 0, PlayerHero->BaseEyeHeight);
    PlayerCameraArm->TargetOffset = bFirstPersonCamera
        ? EyeOffset : FVector(0, 0, 55.0f);
    // The donor has no separate first-person body. Hide it only from its owner.
    PlayerHero->GetMesh()->SetOwnerNoSee(bFirstPersonCamera);
    UE_LOG(LogTemp, Display, TEXT("SOUL_HERO_VIEW: firstPerson=%d eyeOffset=%s"),
        bFirstPersonCamera, *PlayerCameraArm->TargetOffset.ToCompactString());
    PlayerCameraArm->SetRelativeRotation(
        bFirstPersonCamera ? FRotator::ZeroRotator : FRotator(-12.0f, 0.0f, 0.0f));
    PlayerCameraArm->bDoCollisionTest = !bFirstPersonCamera;
    Status = bFirstPersonCamera
        ? TEXT("FIRST PERSON HERO - WASD move, mouse aim, LMB attack, RMB block")
        : TEXT("THIRD PERSON HERO - WASD move, mouse aim, LMB attack, RMB block");
}

void ASoulRealtimeArenaGameMode::CommandSelectedAllies(
    ERBHostGroupOrder Order)
{
    if(bHeroSelected){Status=TEXT("Hero selected: WASD controls the hero. Select a formation first.");return;}
    int32 Changed = 0;
    int32 Eligible = 0;
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Side != ControlledSide || State.bRouting || AliveInGroup(State.GroupIndex) <= 0 ||
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
        else if(Order==ERBHostGroupOrder::Follow)
        {
            if(!IsValid(PlayerHero)||PlayerHealth()<=0)continue;
            Anchor=PlayerHero->GetActorLocation()-PlayerHero->GetActorForwardVector()*350+FVector(0,(State.GroupIndex%3-1)*220,0);
            if(!IsDirectGroundRouteClear(GetWorld(),Center,Anchor))continue;
        }
        if (IssueFormationOrder(
                State.GroupIndex, Order, Anchor, Facing, true))
            ++Changed;
    }

    const TCHAR* Label = TEXT("FACE");
    switch (Order)
    {
        case ERBHostGroupOrder::Hold: Label = TEXT("HOLD"); break;
        case ERBHostGroupOrder::Advance: Label = TEXT("MOVE"); break;
        case ERBHostGroupOrder::Follow: Label = TEXT("FOLLOW"); break;
        case ERBHostGroupOrder::Charge: Label = TEXT("CHARGE"); break;
        case ERBHostGroupOrder::FallBack: Label = TEXT("FALL BACK"); break;
        default: break;
    }
    Status = FString::Printf(TEXT("%s accepted by %d/%d formation(s). [R] returns control to AI."),
        Label, Changed, Eligible);
    UE_LOG(LogTemp, Display, TEXT("SOUL_PLAYER_ORDER: order=%s groups=%d/%d persistent=1"), Label, Changed, Eligible);
}

void ASoulRealtimeArenaGameMode::ReturnSelectedAlliesToAI()
{
    if (bFinished) return;
    int32 Changed = 0;
    for (auto& State : TacticalFormations)
    {
        if (State.Side != ControlledSide || (!bSelectAllAllies && State.GroupIndex != SelectedAlliedFormation)) continue;
        State.ManualOverrideUntil = -1.0f;
        ++Changed;
    }
    Status = FString::Printf(TEXT("%d formation(s) returned to commander AI"), Changed);
    UE_LOG(LogTemp, Display, TEXT("SOUL_PLAYER_AI_CONTROL: groups=%d"), Changed);
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
        if(Actors[I]==PlayerHero) PushBattleNotice(TEXT("Hero fallen - command your army"),ControlledSide);
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
        if (Actor == PlayerHero && !bAutobattle && !bTacticalCameraActive)
        {
            ToggleBattleCamera();
            Status = TEXT("Hero defeated - continue commanding your army");
        }
        if (Spatial) Spatial->UnregisterUnit(Actor);
        Combatants[I].bVisualAttackPlaying = false;
        Actor->GetMesh()->bPauseAnims = true;
        if (bVisualUnits)
        {
            const bool Airborne=Combatants[I].Movement==ESoulRealtimeMovementArchetype::Aerial;
            if(Airborne)
            {
                Combatants[I].AerialDeathSeconds=0.f;
                Combatants[I].AerialDeathStartZ=Actor->GetMesh()->GetRelativeLocation().Z;
            }
            if (auto* Death = Airborne ? ResolveAerialFall(Combatants[I].Side) : ResolveVisualDeath(Combatants[I].Side, Combatants[I].Role))
            {
                Actor->GetMesh()->bPauseAnims = false;
                Actor->GetMesh()->PlayAnimation(Death, Airborne);
                Actor->GetMesh()->SetPlayRate(1.0f);
            }
            else Actor->GetMesh()->SetRelativeRotation(FRotator(0, -90, 85));
        }
        else Actor->GetMesh()->SetRelativeRotation(FRotator(0, -90, 85));
        UE_LOG(LogTemp, Display, TEXT("SOUL_UNIT_DEFEATED: side=%d id=%s"), Combatants[I].Side, *Combatants[I].Id.ToString());
        Actor->GetCharacterMovement()->SetAvoidanceEnabled(false);
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
        }, 120.0f, false);
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
        if (Combatants[I].Side == ControlledSide ||
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
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Whrilwind.P_Aurora_Freeze_Whrilwind");
    else if (Tag == TEXT("Magic.Spell.Water.TidalWard"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Rooted.P_Aurora_Freeze_Rooted");
    else if (Tag == TEXT("Magic.Spell.Air.Tailwind"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_JumpPad_Swirl.P_Aurora_JumpPad_Swirl");
    else if (Tag == TEXT("Magic.Spell.Earth.StoneSentinel"))
        FallbackPath = TEXT("/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Wall_Base.P_Aurora_Wall_Base");

    if (FallbackPath)
    {
        if (UParticleSystem* Fallback = LoadObject<UParticleSystem>(
                nullptr, FallbackPath, nullptr, LOAD_NoWarn))
        {
            if(auto* Emitter=UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),Fallback,FTransform(Location),true,EPSCPoolMethod::None,false))
            {
                // These hero assets were authored for a nearby third-person
                // camera. Keep a bounded spell readable at commander distance.
                Emitter->bOverrideLODMethod=true;
                Emitter->LODMethod=PARTICLESYSTEMLODMETHOD_DirectSet;
                Emitter->SetLODLevel(0);
                Emitter->SecondsBeforeInactive=0;
                Emitter->SetWorldScale3D(FVector(Tag==TEXT("Magic.Spell.Ice.Blizzard") ? 2.0f : 1.4f));
                Emitter->SetVectorParameter(TEXT("TeamColor"),FVector(0.3,0.75,1.0));
                Emitter->SetActorParameter(TEXT("BoneSocketActor"),PlayerHero);
                if(PlayerHero && (Tag==TEXT("Magic.Spell.Water.TidalWard") || Tag==TEXT("Magic.Spell.Air.Tailwind")))
                    Emitter->AttachToComponent(PlayerHero->GetRootComponent(),FAttachmentTransformRules::KeepWorldTransform);
                Emitter->ActivateSystem(true);
                const TWeakObjectPtr<UParticleSystemComponent> Weak(Emitter);
                FTimerHandle Receipt,Cleanup;
                if(bControlDiagnostics)
                    GetWorld()->GetTimerManager().SetTimer(Receipt,[Weak,Tag]()
                    {
                        if(Weak.IsValid())
                            UE_LOG(LogTemp,Display,TEXT("SOUL_SPELL_FX: spell=%s particles=%d active=%d bounds=%s"),
                                *Tag,Weak->GetNumActiveParticles(),Weak->IsActive(),*Weak->Bounds.BoxExtent.ToCompactString());
                    },0.5f,false);
                GetWorld()->GetTimerManager().SetTimer(Cleanup,[Weak]()
                {
                    if(Weak.IsValid()) Weak->DestroyComponent();
                },10.0f,false);
            }
            UE_LOG(LogTemp, Display,
                TEXT("SOUL_MAGIC_PARAGON_PRESENTATION: spell=%s"), *Tag);
        }
    }
}

bool ASoulRealtimeArenaGameMode::CastPlayerSpell(
    const TCHAR* SpellPath,
    const TCHAR* PresentationPath,
    const FHitResult* AimHit)
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
        const auto* AimedBinding = AimHit && AimHit->GetActor()
            ? AimHit->GetActor()->FindComponentByClass<USoulRealtimeArenaBinding>() : nullptr;
        const int32 TargetIndex = bFriendlyUnitSpell ? CasterIndex :
            AimHit ? (AimedBinding ? Index(FRBHostIdentity::From(AimedBinding->GetCombatant())) : INDEX_NONE) :
            FindPlayerSpellTarget(Spell->Range);
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
        Request.Target.WorldLocation = AimHit ? FVector(AimHit->ImpactPoint) :
            PlayerHero->GetActorLocation() +
            PlayerHero->GetActorForwardVector().GetSafeNormal2D() * Distance;
        Request.Target.WorldLocation.Z = ResolveSpawnLocation(Request.Target.WorldLocation).Z - 94.0f;
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

    if (bVisualUnits)
    {
        if (auto* Cast = LoadObject<UAnimationAsset>(nullptr,
            TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Cast.Cast")))
        {
            PlayerHero->GetMesh()->PlayAnimation(Cast, false);
            PlayerHero->GetMesh()->SetPlayRate(1.0f);
            Combatants[CasterIndex].bVisualAttackPlaying = true;
            Combatants[CasterIndex].MeleeCooldown = FMath::Max(Combatants[CasterIndex].MeleeCooldown, Cast->GetPlayLength());
        }
    }
    const FString SpellName = Spell->SpellTag.ToString();
    const ESoulBattleSound CastSound = SpellName.Contains(TEXT("Firebolt")) ? ESoulBattleSound::Firebolt :
        SpellName.Contains(TEXT("Blizzard")) ? ESoulBattleSound::Blizzard :
        SpellName.Contains(TEXT("TidalWard")) || SpellName.Contains(TEXT("Tailwind")) ? ESoulBattleSound::TidalWard :
        ESoulBattleSound::MagicImpact;
    PlayBattleSound(CastSound, PlayerHero->GetActorLocation());
    // RB Magic intents and the existing commit path own gameplay. Presentation
    // may be unassigned during spell development and cannot veto a valid cast.
    auto* Profile = PresentationPath && *PresentationPath
        ? LoadObject<URBMagicPresentationProfile>(nullptr, PresentationPath, nullptr, LOAD_NoWarn)
        : nullptr;
    if (Profile && Profile->SpellTag == Spell->SpellTag)
    {
        if(Spell->DeliveryMode!=ERBMagicDeliveryMode::Projectile)
            SpawnSpellPresentation(Profile, PresentationLocation);
    }
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
            Combatant.BaseWalkSpeed *
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
                    Combatants[I].BaseWalkSpeed *
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
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (PC->WasInputKeyJustPressed(EKeys::One)) SelectPlayerSpell(0);
    if (PC->WasInputKeyJustPressed(EKeys::Two)) SelectPlayerSpell(1);
    if (PC->WasInputKeyJustPressed(EKeys::Three)) SelectPlayerSpell(2);
    if (PC->WasInputKeyJustPressed(EKeys::Four)) SelectPlayerSpell(3);
    if (PC->WasInputKeyJustPressed(EKeys::Five)) SelectPlayerSpell(4);

    if (PC->WasInputKeyJustPressed(EKeys::K)) HandleBattleAction(TEXT("Grimoire"));
    if (PC->WasInputKeyJustPressed(EKeys::F10)) bShowBattleHelp = !bShowBattleHelp;
    auto PadAxis=[&](FKey Key){const float V=PC->GetInputAnalogKeyState(Key);return FMath::Abs(V)<.20f?0.f:FMath::Sign(V)*(FMath::Abs(V)-.20f)/.80f;};
    const float PadX=PadAxis(EKeys::Gamepad_LeftX),PadY=PadAxis(EKeys::Gamepad_LeftY);
    const float PadLookX=PadAxis(EKeys::Gamepad_RightX),PadLookY=PadAxis(EKeys::Gamepad_RightY);
    // RB owns device detection; the battle only consumes its current input method.
    if (const auto* LocalPlayer=PC->GetLocalPlayer())
        if (const auto* InputMethod=LocalPlayer->GetSubsystem<URBUIInputSubsystem>())
            bGamepadActive=InputMethod->IsGamepadActive();
    const bool WantsCursor = !bGamepadActive && (bSpellbookOpen || bTacticalCameraActive || SelectedSpellSlot >= 0 || bPlaceFormationOrder || PC->IsInputKeyDown(EKeys::LeftAlt));
    if (PC->bShowMouseCursor != WantsCursor)
    {
        PC->SetShowMouseCursor(WantsCursor);
        if (WantsCursor)
            PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
        else PC->SetInputMode(FInputModeGameOnly());
    }
    float PointerX = -1, PointerY = -1;
    PC->GetMousePosition(PointerX, PointerY);
    const FVector2D Pointer(PointerX, PointerY);
    const bool OverUI = WantsCursor && PC->GetHUD() && PC->GetHUD()->GetHitBoxAtCoordinates(Pointer, true);
    if (bPlaceFormationOrder && !OverUI && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    { MoveSelectedToPointer(); return; }
    if (bPlaceFormationOrder && PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
    { bPlaceFormationOrder=false; Status=TEXT("Move targeting cancelled"); }
    if (SelectedSpellSlot >= 0 && PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
    { SelectedSpellSlot = INDEX_NONE; Status = TEXT("Spell targeting cancelled"); }
    if (SelectedSpellSlot >= 0 && !OverUI && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        FHitResult Hit;
        if ((SelectedSpellSlot>=3 || ReadPointerHit(Hit)) &&
            CastPlayerSpellSlot(SelectedSpellSlot, SelectedSpellSlot>=3 ? nullptr : &Hit))
            SelectedSpellSlot = INDEX_NONE;
        return;
    }
    if(bTacticalCameraActive && !OverUI && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        FHitResult Hit;
        if(ReadPointerHit(Hit) && Hit.GetActor())
            if(auto* Clicked=Hit.GetActor()->FindComponentByClass<USoulRealtimeArenaBinding>())
            {
                const int32 I=Index(FRBHostIdentity::From(Clicked->GetCombatant()));
                if(I!=INDEX_NONE) HandleBattleAction(FName(*FString::Printf(TEXT("Unit%d"),I)));
            }
    }
    if (bTacticalCameraActive && PC->IsInputKeyDown(EKeys::LeftShift) &&
        !OverUI && PC->WasInputKeyJustPressed(EKeys::RightMouseButton)) MoveSelectedToPointer();

    if (PC->WasInputKeyJustPressed(EKeys::J))HandleBattleAction(TEXT("Hero"));
    if (PC->WasInputKeyJustPressed(EKeys::C))
        ToggleBattleCamera();
    if (PC->WasInputKeyJustPressed(EKeys::X))
        ToggleFirstPersonCamera();
    if (PC->WasInputKeyJustPressed(EKeys::Home)) HandleBattleAction(TEXT("Focus"));
    if (PC->WasInputKeyJustPressed(EKeys::F1)) SelectAlliedFormationSlot(0);
    if (PC->WasInputKeyJustPressed(EKeys::F2)) SelectAlliedFormationSlot(1);
    if (PC->WasInputKeyJustPressed(EKeys::F3)) SelectAlliedFormationSlot(2);
    if (PC->WasInputKeyJustPressed(EKeys::F4)) SelectAlliedFormationSlot(3);
    if (PC->WasInputKeyJustPressed(EKeys::F5)) SelectAlliedFormationSlot(4);
    if (PC->WasInputKeyJustPressed(EKeys::Tab)) SelectNextAlliedFormation();
    if (PC->WasInputKeyJustPressed(EKeys::R)) ReturnSelectedAlliesToAI();
    if (PC->WasInputKeyJustPressed(EKeys::H)) CommandSelectedAllies(ERBHostGroupOrder::Hold);
    if (PC->WasInputKeyJustPressed(EKeys::V)) CommandSelectedAllies(ERBHostGroupOrder::Follow);
    if (PC->WasInputKeyJustPressed(EKeys::G)) CommandSelectedAllies(ERBHostGroupOrder::Charge);
    if (PC->WasInputKeyJustPressed(EKeys::B)) CommandSelectedAllies(ERBHostGroupOrder::FallBack);
    if (PC->WasInputKeyJustPressed(EKeys::F)) CommandSelectedAllies(ERBHostGroupOrder::Face);
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PC->GetInputMouseDelta(MouseX, MouseY);
    if (!bTacticalCameraActive && !WantsCursor)
    {
        FRotator View = PC->GetControlRotation();
        View.Yaw += MouseX * 0.45f + PadLookX*100.f*FMath::Clamp(float(FApp::GetDeltaTime()),0.f,.05f);
        View.Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch) - MouseY * 0.35f + PadLookY*80.f*FMath::Clamp(float(FApp::GetDeltaTime()),0.f,.05f), -75.0f, 70.0f);
        PC->SetControlRotation(View);
    }

    if (bTacticalCameraActive && TacticalCamera)
    {
        const float CameraSeconds = FMath::Clamp(float(FApp::GetDeltaTime()), 0.0f, 0.05f);
        const float ForwardAxis = float(PC->IsInputKeyDown(EKeys::W)) - float(PC->IsInputKeyDown(EKeys::S)) + PadY;
        const float RightAxis = float(PC->IsInputKeyDown(EKeys::D)) - float(PC->IsInputKeyDown(EKeys::A)) + PadX;
        TacticalRotation.Yaw += (float(PC->IsInputKeyDown(EKeys::E)) - float(PC->IsInputKeyDown(EKeys::Q))) * 65.0f * CameraSeconds;
        if (PC->IsInputKeyDown(EKeys::RightMouseButton) && !PC->IsInputKeyDown(EKeys::LeftShift))
        {
            TacticalRotation.Yaw += MouseX * 0.45f;
            TacticalRotation.Pitch = FMath::Clamp(TacticalRotation.Pitch - MouseY * 0.35f, -75.0f, -20.0f);
        }
        const FRotationMatrix Basis(FRotator(0, TacticalRotation.Yaw, 0));
        TacticalFocus += (Basis.GetUnitAxis(EAxis::X) * ForwardAxis + Basis.GetUnitAxis(EAxis::Y) * RightAxis).GetClampedToMaxSize(1.0f)
            * TacticalDistance * 0.65f * CameraSeconds;
        TacticalFocus.X = FMath::Clamp(TacticalFocus.X, ArenaOrigin.X - 3200.0, ArenaOrigin.X + 3200.0);
        TacticalFocus.Y = FMath::Clamp(TacticalFocus.Y, ArenaOrigin.Y - 2400.0, ArenaOrigin.Y + 2400.0);
        TacticalRotation.Yaw+=PadLookX*80.f*CameraSeconds;
        TacticalRotation.Pitch=FMath::Clamp(TacticalRotation.Pitch+PadLookY*50.f*CameraSeconds,-75.f,-20.f);
        TacticalDistance*=1.f+(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis)-PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis))*CameraSeconds;
        if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp)) TacticalDistance /= 1.15f;
        if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown)) TacticalDistance *= 1.15f;
        TacticalDistance = FMath::Clamp(TacticalDistance, 650.0f, 6000.0f);
        FVector CameraPosition = TacticalFocus - TacticalRotation.Vector() * TacticalDistance;
        CameraPosition.Z = FMath::Max(CameraPosition.Z, ResolveSpawnLocation(CameraPosition).Z + 180.0);
        TacticalCamera->SetActorLocationAndRotation(CameraPosition, TacticalRotation);
    }
    if(!PlayerHero)return; // Formation commands/camera above remain available when the commander is wounded/captured.
    const FRotator YawRotation(
        0, PC->GetControlRotation().Yaw, 0);
    const FVector Forward =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    const float ForwardInput = bTacticalCameraActive ? 0.0f :
        (PC->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) -
        (PC->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f) + PadY;
    const float RightInput = bTacticalCameraActive ? 0.0f :
        (PC->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) -
        (PC->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f) + PadX;
    if (bFinished || bBattlePaused || PlayerHealth() <= 0.0f) return;
    PlayerHero->AddMovementInput(Forward, ForwardInput);
    PlayerHero->AddMovementInput(Right, RightInput);

    auto* PlayerBinding =
        PlayerHero->FindComponentByClass<USoulRealtimeArenaBinding>();
    const int32 PlayerIndex = PlayerBinding
        ? Index(FRBHostIdentity::From(PlayerBinding->GetCombatant()))
        : INDEX_NONE;
    if (PlayerIndex == INDEX_NONE) return;

    PlayerBinding->SetGuardIntent(
        !bTacticalCameraActive && !WantsCursor && !OverUI &&
        (PC->IsInputKeyDown(EKeys::RightMouseButton) || PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis)>.3f));

    if (!bTacticalCameraActive && !WantsCursor && !OverUI &&
        (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis)>.3f))
    {
        int32 Best = INDEX_NONE;
        double BestDistance = 340.0 * 340.0;
        const FVector PlayerLocation = PlayerHero->GetActorLocation();
        const FVector Aim = PlayerHero->GetActorForwardVector();
        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            if (Combatants[I].Side == ControlledSide ||
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
        else if(!bSiege || !PerformGateMelee(PlayerIndex))
            Status = TEXT("No enemy or assault gate in melee reach");
    }

}
void ASoulRealtimeArenaGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if (bAwaitingAuthoredEnvironment)
    {
        const double Now = FPlatformTime::Seconds();
        int32 Pending = 0;
        for (const auto* Level : GetWorld()->GetStreamingLevels())
            if (Level && (Level->IsStreamingStatePending()
                || (Level->ShouldBeLoaded() && !Level->IsLevelLoaded())
                || (Level->ShouldBeVisible() && !Level->IsLevelVisible()))) ++Pending;
        if (Now - AuthoredLoadStarted > 1200)
        {
            // Keep the encounter pending; never invent a result for a load
            // failure. The bounded qualification launcher records this error.
            Status = TEXT("The authored battlefield did not finish loading.");
            UE_LOG(LogTemp, Error, TEXT("SOUL_AUTHORED_BATTLE_LOAD_FAIL pending=%d async=%d"), Pending, IsAsyncLoading());
            SetActorTickEnabled(false);
            return;
        }
        if (Pending > 0 || IsAsyncLoading()) { AuthoredLoadQuietSince = 0; return; }
        if (AuthoredLoadQuietSince == 0) AuthoredLoadQuietSince = Now;
        if (Now - AuthoredLoadQuietSince < 2) return;
        bAwaitingAuthoredEnvironment = false;
        RestoreAuthoredLoadingRendering();
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_BATTLE_LOAD_READY seconds=%.2f levels=%d"), Now - AuthoredLoadStarted, GetWorld()->GetStreamingLevels().Num());
        InitializeLoadedArena();
        return;
    }
    if (auto* PC = GetWorld()->GetFirstPlayerController())
        if (PC->WasInputKeyJustPressed(EKeys::Escape))
        {
            if(bSpellbookOpen || SelectedSpellSlot>=0 || bPlaceFormationOrder || bShowBattleHelp)
            {
                bSpellbookOpen=false; SelectedSpellSlot=INDEX_NONE;
                bPlaceFormationOrder=false; bShowBattleHelp=false;
                Status=TEXT("Closed without casting.");
            }
            else FPlatformMisc::RequestExit(false);
        }
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
    if (bReadabilityProof) TickReadabilityProof();
    if(bSiege)TickSiegeQualification();else TickHumanDefenseQualification();
    // Camera inspection remains available after resolution as well as during pause.
    if (!bAutobattle && !Combatants.IsEmpty()) PlayerTick(Seconds);
    TickCombatPresentation(bBattlePaused ? 0.f : Seconds);
    if (bFinished)
    {
        if (bBattlePaused && PlayerCameraArm)
            PlayerCameraArm->TickComponent(0.0f, LEVELTICK_All, &PlayerCameraArm->PrimaryComponentTick);
        UpdateVisualAnimations();
        if (ResultHoldSeconds >= 0)
        {
            ResultHoldSeconds += Seconds;
            if (ResultHoldSeconds > 60) { bFinished = false; FinishProof(true, TEXT("Resolved real battle plus 60-second observation")); }
        }
        return;
    }
    if (Combatants.IsEmpty()) return;
    // Paused component ticks can depend on the non-ticking character. Update
    // only this presentation component so deployment remains inspectable.
    if (bBattlePaused && PlayerCameraArm)
        PlayerCameraArm->TickComponent(0.0f, LEVELTICK_All, &PlayerCameraArm->PrimaryComponentTick);
    if (bControlDiagnostics && FPlatformTime::Seconds() >= NextControlReceipt)
    {
        NextControlReceipt = FPlatformTime::Seconds() + 5.0;
        float HealthSum = 0, CooldownSum = 0;
        int32 Ammo = 0;
        uint32 PositionHash = 0;
        for (int32 I = 0; I < Combatants.Num(); ++I)
        {
            HealthSum += Combatants[I].Health;
            CooldownSum += Combatants[I].MeleeCooldown;
            Ammo += Combatants[I].Arrows;
            if (Actors.IsValidIndex(I) && IsValid(Actors[I]))
            {
                ACharacter* A = Actors[I];
                PositionHash = HashCombine(PositionHash, GetTypeHash(A->GetActorLocation()));
                auto* Mesh = A->GetMesh();
                const FVector Root = Mesh->GetSocketLocation(Mesh->GetBoneName(0));
                auto* Animation = Mesh->GetSingleNodeInstance();
                UE_LOG(LogTemp, Display, TEXT("SOUL_UNIT_RECEIPT: i=%d side=%d role=%s hp=%.1f hidden=%d visible=%d actor=%s root=%s bounds=%s ammo=%d clip=%s animTime=%.3f playing=%d pauseAnims=%d"),
                    I, Combatants[I].Side, *RoleLabel(Combatants[I].Role), Combatants[I].Health,
                    A->IsHidden(), Mesh->IsVisible(), *A->GetActorLocation().ToCompactString(),
                    *Root.ToCompactString(), *Mesh->Bounds.Origin.ToCompactString(), Combatants[I].Arrows,
                    *GetNameSafe(Animation ? Animation->GetCurrentAsset() : nullptr),
                    Animation ? Animation->GetCurrentTime() : 0.f, Animation && Animation->IsPlaying(), Mesh->bPauseAnims);
            }
        }
        FVector CameraLocation;
        FRotator CameraRotation;
        if (auto* PC = GetWorld()->GetFirstPlayerController())
        {
            PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
            const auto* HeroCamera = PlayerHero ? PlayerHero->FindComponentByClass<UCameraComponent>() : nullptr;
            UE_LOG(LogTemp, Display, TEXT("SOUL_CAMERA_COMPONENTS: controller=%s target=%s control=%s camera=%s registered=%d active=%d armSocket=%s"),
                *PC->GetClass()->GetName(), *GetNameSafe(PC->GetViewTarget()),
                *PC->GetControlRotation().ToCompactString(),
                HeroCamera ? *HeroCamera->GetComponentLocation().ToCompactString() : TEXT("missing"),
                HeroCamera && HeroCamera->IsRegistered(), HeroCamera && HeroCamera->IsActive(),
                PlayerCameraArm ? *PlayerCameraArm->GetSocketLocation(USpringArmComponent::SocketName).ToCompactString() : TEXT("missing"));
        }
        UE_LOG(LogTemp, Display,
            TEXT("SOUL_CONTROL_RECEIPT: paused=%d worldPaused=%d elapsed=%.3f health=%.3f cooldown=%.3f ammo=%d positions=%u hits=%d mana=%.1f casts=%d alive=%d/%d camera=%s rotation=%s commander=%d firstPerson=%d arm=%d"),
            bBattlePaused, UGameplayStatics::IsGamePaused(this), BattleElapsed, HealthSum, CooldownSum,
            Ammo, PositionHash, AcceptedContacts.Num(), PlayerMana, MagicCasts, AliveForSide(0), AliveForSide(1),
            *CameraLocation.ToCompactString(), *CameraRotation.ToCompactString(), bTacticalCameraActive,
            bFirstPersonCamera, IsValid(PlayerCameraArm));
    }
    if (bBattlePaused)
    {
        UpdateVisualAnimations();
        return;
    }
    if ((bQualification || FParse::Param(FCommandLine::Get(),TEXT("SoulRosterCapture"))) && !bFirstCapture && BattleElapsed > 3)
    {
        bFirstCapture = true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Battle.png"), false, false);
    }

    for (auto& C : Combatants)
        C.MeleeCooldown =
            FMath::Max(0.0f, C.MeleeCooldown - Seconds);

    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        auto& C = Combatants[I];
        if (C.PendingMeleeSeconds <= 0.0f) continue;
        C.PendingMeleeSeconds = FMath::Max(0.0f, C.PendingMeleeSeconds - Seconds);
        if (C.PendingMeleeSeconds <= 0.0f)
        {
            const FRBHostIdentity Target = C.PendingMeleeTarget;
            C.PendingMeleeTarget = FRBHostIdentity();
            if (C.Health > 0.0f)
            {
                if (C.Side == 1 && C.Role == ESoulRealtimeFormationRole::Apex)
                    PlayDragonBreath(I, Index(Target));
                CommitMeleeImpact(I, Target);
            }
        }
    }
    TickMagic(Seconds);
    UpdateVisualAnimations();
    UpdateDefeatedRepresentations();
    TickReinforcements();
    if (bFinished) return; // A failed reserve transaction must not proceed to result publication.
    TrackBattlefieldExtent();
    TickSpatialOrders(Seconds);
    if(bSiege)TickSiege(Seconds);
    if(bFinished)return;
    TickBattleResolution(Seconds);
    if (bFinished) return;
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
void ASoulRealtimeArenaGameMode::SetupBattleCamera()
{
    const FVector Center = ResolveSpawnLocation(ArenaOrigin + FVector(0, 0, 100));
    const bool bDragonShowcase = GetWorld()->GetOutermost()->GetName().Contains(TEXT("Dragon_graveyard"));
    const bool bHumanApproach = GetWorld()->GetOutermost()->GetName() == TEXT("/Game/Soul/Maps/Settlements/L_HumanCapital_Authored");
    const FVector CameraOffset = bHumanApproach ? FVector(-850, 3400, 2400) : bDragonShowcase
        ? (bMapOnly ? FVector(3200, -4200, 3000) : FVector(0, -4200, 2500))
        : (bMapOnly ? FVector(-1800, -3200, 1700) : FVector(-850, -1800, 1000));
    const FVector FocusOffset = bHumanApproach ? FVector(350, -300, 150) : bDragonShowcase
        ? (bMapOnly ? FVector(-200, 500, 150) : FVector(0, -450, 100))
        : (bMapOnly ? FVector(500, 1800, 600) : FVector(250, 0, 100));
    if (!IsValid(TacticalCamera))
        TacticalCamera = GetWorld()->SpawnActor<ACameraActor>(
            Center + CameraOffset, FRotator::ZeroRotator);
    if (!TacticalCamera) return;
    TacticalCamera->SetActorLocation(Center + CameraOffset);
    TacticalFocus = Center + FocusOffset;
    TacticalDistance = FVector::Dist(TacticalFocus, TacticalCamera->GetActorLocation());
    TacticalRotation = (TacticalFocus - TacticalCamera->GetActorLocation()).Rotation();
    TacticalCamera->SetActorRotation(TacticalRotation);
    TacticalCamera->GetCameraComponent()->SetFieldOfView(
        bDragonShowcase ? (bMapOnly ? 60.0f : 55.0f) : bMapOnly ? 65.0f : 68.0f);
    if (auto* PC = GetWorld()->GetFirstPlayerController())
        PC->SetViewTarget(TacticalCamera);
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
    if(TacticalFormations.IsValidIndex(StateIndex) &&
        TacticalFormations[StateIndex].Kind==ESoulBattleFormationKind::FrontLine)
        Drivers[GroupIndex]->FormationSpacing=Order==ERBHostGroupOrder::Hold || Order==ERBHostGroupOrder::Face ? 115.f :
            Order==ERBHostGroupOrder::Charge ? 165.f : 135.f;
    if(IsHeartlandBridgeBattle()||bSiege)Drivers[GroupIndex]->FormationSpacing=90.f;
    if (TacticalFormations.IsValidIndex(StateIndex))
    {
        TacticalFormations[StateIndex].TacticalAnchor = Anchor;
        if (bManual)
        {
            auto& State=TacticalFormations[StateIndex];State.ManualOverrideUntil=TNumericLimits<float>::Max();
            State.ManualOrder=Order;State.ManualAnchor=Anchor;State.ManualFacing=Facing.IsNearlyZero()?FVector::ForwardVector:Facing;
            if(Order==ERBHostGroupOrder::Follow&&IsValid(PlayerHero))State.FollowOffset=Anchor-PlayerHero->GetActorLocation();
            ResetFormationMotion(GroupIndex);
        }
    }
    ++SpatialOrders;
    return true;
}

void ASoulRealtimeArenaGameMode::UpdateFormationMorale()
{
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
        // A rout on a remote flank must not drain the whole army's morale.
        Input.bFriendlyRoutedNearby = !State.bRouting && TacticalFormations.ContainsByPredicate(
            [this, &State, &Center](const FSoulBattleFormationState& Other)
            {
                return Other.Side == State.Side && Other.GroupIndex != State.GroupIndex &&
                    Other.bRouting && AliveInGroup(Other.GroupIndex) > 0 &&
                    FVector::DistSquared2D(Center, GroupCenter(Other.GroupIndex)) < FMath::Square(850.0f);
            });
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
            PushBattleNotice(FString::Printf(TEXT("%s %s is routing"),
                State.Side==ControlledSide?TEXT("Your"):TEXT("Enemy"),
                FSoulRealtimeTacticalRules::FormationKindLabel(State.Kind)),State.Side);
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
            PushBattleNotice(FString::Printf(TEXT("%s %s has rallied"),
                State.Side==ControlledSide?TEXT("Your"):TEXT("Enemy"),
                FSoulRealtimeTacticalRules::FormationKindLabel(State.Kind)),State.Side);
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
    RefreshManualOrders();

    bool bFrontEngaged[2] = {false, false};
    int32 LivingFronts[2] = {0, 0};
    int32 RoutingFronts[2] = {0, 0};
    int32 LowestFrontMorale[2] = {1000, 1000};
    for (const FSoulBattleFormationState& State : TacticalFormations)
    {
        if (State.Kind != ESoulBattleFormationKind::FrontLine ||
            AliveInGroup(State.GroupIndex) <= 0)
            continue;
        float Distance = 0.0f;
        FindNearestEnemyToGroup(State.GroupIndex, Distance);
        ++LivingFronts[State.Side];
        RoutingFronts[State.Side] += State.bRouting || State.bShattered ? 1 : 0;
        LowestFrontMorale[State.Side] = FMath::Min(LowestFrontMorale[State.Side], State.MoralePermille);
        bFrontEngaged[State.Side] =
            bFrontEngaged[State.Side] || (!State.bRouting && !State.bShattered && Distance < 720.0f);
    }

    for (FSoulBattleFormationState& State : TacticalFormations)
    {
        if (AliveInGroup(State.GroupIndex) <= 0 ||
            (State.ManualOverrideUntil > BattleElapsed && !State.bRouting))
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
        Context.bRangedOperational=Combatants.ContainsByPredicate([&State](const FSoulRealtimeArenaCombatant& Unit)
        { return Unit.GroupIndex==State.GroupIndex && Unit.Health>0 && Unit.bRanged && Unit.Arrows>0; });
        Context.bFriendlyLineCollapsing =
            FSoulRealtimeTacticalRules::IsFrontLineCollapsing(
                LivingFronts[State.Side], RoutingFronts[State.Side], LowestFrontMorale[State.Side]);

        ERBHostGroupOrder Order =
            FSoulRealtimeTacticalRules::ChooseOrder(Context);
        FVector Anchor = State.TacticalAnchor;
        if (Order == ERBHostGroupOrder::FallBack)
        {
            const float Outward = State.Side == 0 ? -1.0f : 1.0f;
            Anchor = State.SpawnAnchor + FVector(Outward * 550.0f, 0, 0);
        }
        else if (Order == ERBHostGroupOrder::Advance)
        {
            Anchor = FSoulRealtimeTacticalRules::AdvanceAnchor(Context,
                State.TacticalAnchor, EnemyLocation, Facing);
            if (State.Kind == ESoulBattleFormationKind::FrontLine ||
                State.Kind == ESoulBattleFormationKind::CommandReserve ||
                (State.Kind == ESoulBattleFormationKind::Strike &&
                 bFrontEngaged[State.Side]))
            {
                FVector Approach;
                if (Spatial && Spatial->QueryApproach(
                        Actors[Index(Groups[State.GroupIndex].Leader)],
                        State.Side == 1, EnemyLocation, Approach))
                    Anchor = Approach;
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

        if(State.Kind==ESoulBattleFormationKind::Strike && !State.bRouting && EnemyDistance>450)
        {
            FVector Target=EnemyLocation;
            double Best=TNumericLimits<double>::Max();
            for(const auto& Opponent:TacticalFormations)
            {
                if(Opponent.Side==State.Side || Opponent.Kind!=ESoulBattleFormationKind::MissileSupport ||
                    AliveInGroup(Opponent.GroupIndex)<=0) continue;
                const FVector Candidate=GroupCenter(Opponent.GroupIndex);
                const double Distance=FVector::DistSquared2D(Center,Candidate);
                if(Distance<Best) { Best=Distance; Target=Candidate; }
            }
            if(State.FlankSign==0) State.FlankSign=State.Side==0 ? -1.0f : 1.0f;
            const int32 PreviousStage=State.FlankStage;
            FVector Waypoint=FSoulRealtimeTacticalRules::StrikeWaypoint(
                State.Side,ArenaOrigin,Center,Target,State.FlankSign,State.FlankStage);
            if(IsDirectGroundRouteClear(GetWorld(),Center,Waypoint,110))
            {
                Anchor=Waypoint;
                Order=State.FlankStage<2 || FVector::Dist2D(Center,Target)>550 ?
                    ERBHostGroupOrder::Advance : ERBHostGroupOrder::Charge;
            }
            else if(State.FlankStage==0)
            {
                State.FlankSign=-State.FlankSign;
                Waypoint=FSoulRealtimeTacticalRules::StrikeWaypoint(
                    State.Side,ArenaOrigin,Center,Target,State.FlankSign,State.FlankStage);
                if(IsDirectGroundRouteClear(GetWorld(),Center,Waypoint,110))
                { Anchor=Waypoint; Order=ERBHostGroupOrder::Advance; }
                // Existing approach remains the fallback when both lanes are blocked.
            }
            if(PreviousStage!=State.FlankStage)
                UE_LOG(LogTemp,Display,TEXT("SOUL_FLANK_PROGRESS: side=%d group=%d stage=%d anchor=%s"),
                    State.Side,State.GroupIndex,State.FlankStage,*Anchor.ToCompactString());
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
            // Let withdrawal be visible and give eligible formations a rally
            // opportunity before ending the battle on army-wide morale collapse.
            const bool bEstablishedRout = State.bRouting && State.RoutingSeconds >= 8.0f;
            RoutingFormations += bEstablishedRout ? 1 : 0;
            ShatteredFormations += bEstablishedRout && State.bShattered ? 1 : 0;
        }
        const bool bActiveArmyShattered = LivingFormations > 0 &&
            ShatteredFormations >= LivingFormations;
        FVector SafeEntry;
        const bool bNoSafeReturn=LivingFormations==0 && ReserveBodiesForSide(Side)>0 &&
            !ChooseReinforcementAnchor(Side,SafeEntry);
        bSideMoraleDefeated[Side] = bNoSafeReturn || bActiveArmyShattered ||
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
    for (auto& Unit : Combatants)
        if (Unit.DragonBreath.IsValid()) Unit.DragonBreath->DestroyComponent();
    for (USoulRealtimeArenaGroupDriver* Driver : Drivers) if (Driver) Driver->SetComponentTickEnabled(false);
    for (ACharacter* Actor : Actors)
    {
        if (!IsValid(Actor)) continue;
        if (auto* AI = Cast<AAIController>(Actor->GetController())) AI->StopMovement();
        Actor->GetCharacterMovement()->StopMovementImmediately();
    }
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
    if(bSiege&&FParse::Param(FCommandLine::Get(),TEXT("SoulSiegeQualification")))
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/Siege_Battle_Result.png"),true,false);
    const bool bHumanWon = ControlledSide == 0 ? bWon : EnemySurvivors > 0 && PlayerSurvivors == 0;
    BattleResultLabel = bHumanWon
        ? (bSideMoraleDefeated[1-ControlledSide] ? TEXT("VICTORY / ENEMY ROUTED") : TEXT("VICTORY"))
        : (bSideMoraleDefeated[ControlledSide] ? TEXT("DEFEAT / ARMY ROUTED") : TEXT("DEFEAT"));
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
        Result.bSiege=bSiege;Result.SiegeGateRemaining=bSiege?SiegeState.GateIntegrityPermille:0;Result.bCourtyardCaptured=bSiege&&SiegeState.bVictory;
        Result.PlayerCompanies=SurvivingCampaignCompanies(0,bSideMoraleDefeated[0]);
        Result.EnemyCompanies=SurvivingCampaignCompanies(1,bSideMoraleDefeated[1]);
        Result.PlayerSurvivors = PlayerSurvivors;
        Result.EnemySurvivors = EnemySurvivors;
        Result.PlayerReinforcements = ReinforcementWaves[0];
        Result.EnemyReinforcements = ReinforcementWaves[1];
        Result.MagicCasts = MagicCasts;
        Result.PlayerManaRemaining = FMath::FloorToInt(FMath::Max(0.0f, PlayerMana));
        Result.bNonPlayerHeroWounded=Combatants.ContainsByPredicate([](const auto& C){return C.bNonPlayerHero&&C.Health<=0;});
        Result.bTacticalHeroWounded=!Pending->bAutoResolve&&Pending->bPlayerHeroAvailable&&PlayerHealth()<=0.f;
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
        Status = BattleResultLabel;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Vertical_Result.png"), false, false);
        return;
    }
    bFinished = true;
    Status = BattleResultLabel;
}
