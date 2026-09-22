#include "SoulRealtimeBattleArena.h"

#include "AIController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
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

namespace
{
    const FName ArenaDomain(TEXT("Soul.Battle"));
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
    return InitialAlive[Side] - AliveForSide(Side);
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
void ASoulRealtimeArenaGameMode::BeginPlay()
{
    Super::BeginPlay();
    bProof = FParse::Param(
        FCommandLine::Get(), TEXT("SoulRealtimeArenaProof"));

    if (!SetupArena())
    {
        if (bProof)
            FinishProof(false, TEXT("arena setup failed"));
        else
            Status = TEXT("Arena setup failed");
        return;
    }

    InitialAlive[0] = AliveForSide(0);
    InitialAlive[1] = AliveForSide(1);
    UE_LOG(LogTemp, Display,
        TEXT("SOUL_RT_ARENA_SETUP: actors=%d groups=%d proof=%d"),
        Combatants.Num(), Groups.Num(), bProof);
}

bool ASoulRealtimeArenaGameMode::SetupArena()
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
        Light->SetActorRotation(FRotator(-55, -35, 0));
        Light->GetLightComponent()->SetIntensity(5.0f);
    }
    if (!SpawnArmy(0) || !SpawnArmy(1) || !SetupDrivers())
        return false;

    if (!bProof && PlayerHero)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (!PC) return false;
        PC->Possess(PlayerHero);
        PC->SetViewTarget(PlayerHero);
        PC->SetShowMouseCursor(false);
    }

    Status = TEXT("24v24 physical battle ready - seven unit families plus hero");
    return Combatants.Num() == 48 && Groups.Num() == 16;
}

bool ASoulRealtimeArenaGameMode::SpawnArmy(int32 Side)
{
    const float BaseX = Side == 0 ? -1450.0f : 1450.0f;
    const float Back = Side == 0 ? -280.0f : 280.0f;
    return
        SpawnFormation(Side, ESoulRealtimeFormationRole::Line,
            5, FVector(BaseX, 0, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Guard,
            4, FVector(BaseX, 520, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Breaker,
            4, FVector(BaseX, -520, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Shock,
            3, FVector(BaseX, 1020, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Ranged,
            4, FVector(BaseX + Back, -1020, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Support,
            2, FVector(BaseX + Back, 1420, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Hero,
            1, FVector(BaseX, 260, 100)) &&
        SpawnFormation(Side, ESoulRealtimeFormationRole::Apex,
            1, FVector(BaseX, -1420, 120));
}

bool ASoulRealtimeArenaGameMode::SpawnFormation(
    int32 Side,
    ESoulRealtimeFormationRole FormationRole,
    int32 Count,
    const FVector& Anchor)
{
    if (Count <= 0 || Count > FRBCombatGroup::MaximumMembers)
        return false;

    const int32 GroupIndex = Groups.AddDefaulted();
    FRBCombatGroup Core;
    Core.Id = FGuid::NewGuid();
    Core.Command = ERBGroupCommand::Charge;
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
        const bool bPlayer =
            !bProof && Side == 0 &&
            FormationRole == ESoulRealtimeFormationRole::Hero && I == 0;
        if (!SpawnCombatant(
                Side, FormationRole, GroupIndex, Location, bPlayer))
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
    return true;
}

bool ASoulRealtimeArenaGameMode::SpawnCombatant(
    int32 Side,
    ESoulRealtimeFormationRole FormationRole,
    int32 GroupIndex,
    const FVector& Location,
    bool bPlayer)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return false;

    FSoulRealtimeArenaCombatant Data;
    Data.Id = FGuid::NewGuid();
    Data.Side = Side;
    Data.Role = FormationRole;
    Data.Health = RoleHealth(FormationRole);
    Data.Arrows =
        FormationRole == ESoulRealtimeFormationRole::Ranged ? 32 : 0;
    Data.GroupIndex = GroupIndex;
    Data.bRanged = FormationRole == ESoulRealtimeFormationRole::Ranged;
    Data.bPlayerHero = bPlayer;
    const int32 NewIndex = Combatants.Add(Data);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FRotator Facing(0, Side == 0 ? 0.0f : 180.0f, 0);
    ACharacter* Actor = GetWorld()->SpawnActor<ACharacter>(
        ACharacter::StaticClass(), Location, Facing, Params);
    if (!Actor) return false;

    Actor->SetCanBeDamaged(true);
    Actor->bUseControllerRotationYaw = bPlayer;
    Actor->GetCapsuleComponent()->SetCollisionResponseToChannel(
        ECC_Visibility, ECR_Block);
    Actor->GetCharacterMovement()->MaxWalkSpeed =
        FormationRole == ESoulRealtimeFormationRole::Shock ? 330.0f :
        FormationRole == ESoulRealtimeFormationRole::Hero ? 320.0f :
        FormationRole == ESoulRealtimeFormationRole::Guard ? 240.0f :
        FormationRole == ESoulRealtimeFormationRole::Breaker ? 275.0f :
        FormationRole == ESoulRealtimeFormationRole::Apex ? 285.0f :
        270.0f;
    Actor->GetCharacterMovement()->bOrientRotationToMovement = !bPlayer;

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

    UTextRenderComponent* Label =
        NewObject<UTextRenderComponent>(Actor);
    Actor->AddInstanceComponent(Label);
    Label->SetupAttachment(Actor->GetRootComponent());
    Label->SetRelativeLocation(FVector(0, 0, 150));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(24.0f);
    Label->SetText(FText::FromString(FString::Printf(
        TEXT("%s %s"),
        Side == 0 ? TEXT("H") : TEXT("E"),
        *RoleLabel(FormationRole))));
    Label->SetTextRenderColor(
        bPlayer ? FColor::Yellow :
        (Side == 0 ? FColor::Cyan : FColor::Red));
    Label->RegisterComponent();

    USoulRealtimeArenaBinding* Binding =
        NewObject<USoulRealtimeArenaBinding>(Actor);
    Actor->AddInstanceComponent(Binding);
    Binding->RegisterComponent();

    Actors.Add(Actor);
    Bindings.Add(Binding);
    Ranged.Add(nullptr);
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
    }
    return Actors.Num() == Combatants.Num() &&
        Bindings.Num() == Combatants.Num() &&
        Ranged.Num() == Combatants.Num();
}

bool ASoulRealtimeArenaGameMode::SetupDrivers()
{
    TArray<URBVariantCombatBindingComponent*> Reps;
    Reps.Reserve(Bindings.Num());
    for (USoulRealtimeArenaBinding* Binding : Bindings)
        Reps.Add(Binding);

    for (int32 I = 0; I < Groups.Num(); ++I)
    {
        USoulRealtimeArenaGroupDriver* Driver =
            NewObject<USoulRealtimeArenaGroupDriver>(this);
        AddInstanceComponent(Driver);
        Driver->GroupIndex = I;
        Driver->FormationSpacing = 105.0f;
        Driver->SightDistance = 6200.0f;
        Driver->bAllowDirectSteering = true;
        Driver->RegisterComponent();
        if (!Driver->SetRepresentations(Reps))
            return false;
        Drivers.Add(Driver);
    }
    return Drivers.Num() == Groups.Num();
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

    if (bAccepted && !bProof)
    {
        Status = FString::Printf(
            TEXT("%s %s contact accepted"),
            AttackerData.Side == 0 ? TEXT("Human") : TEXT("Enemy"),
            *RoleLabel(AttackerData.Role));
    }
    return bAccepted;
}

void ASoulRealtimeArenaGameMode::ToggleAlliedOrders()
{
    bAlliedCharge = !bAlliedCharge;
    const ERBHostGroupOrder Order = bAlliedCharge
        ? ERBHostGroupOrder::Charge : ERBHostGroupOrder::Hold;

    int32 Changed = 0;
    for (int32 I = 0; I < Groups.Num(); ++I)
    {
        FRBCombatGroup Core;
        if (!Groups[I].ToCore(Core)) continue;
        const int32 LeaderIndex = Index(Groups[I].Leader);
        if (LeaderIndex == INDEX_NONE ||
            Combatants[LeaderIndex].Side != 0)
            continue;

        FString Error;
        const bool bOk = Drivers.IsValidIndex(I) && Drivers[I] &&
            Drivers[I]->RequestGroupOrder(
                Groups[I].Leader, Order,
                Groups[I].Anchor, FVector::ForwardVector,
                FRBHostIdentity(), Groups[I].Revision, Error);
        if (bOk) ++Changed;
    }

    Status = FString::Printf(
        TEXT("Allied formations: %s (%d groups)"),
        bAlliedCharge ? TEXT("CHARGE") : TEXT("HOLD"),
        Changed);
}

void ASoulRealtimeArenaGameMode::UpdateDefeatedRepresentations()
{
    for (int32 I = 0; I < Combatants.Num(); ++I)
    {
        if (Combatants[I].Health > 0.0f ||
            DefeatedRepresentations.Contains(I) ||
            !Actors.IsValidIndex(I) || !Actors[I])
            continue;

        DefeatedRepresentations.Add(I);
        ACharacter* Actor = Actors[I];
        Actor->SetActorEnableCollision(false);
        Actor->SetActorScale3D(FVector(1.0f, 1.0f, 0.28f));
        Actor->GetCharacterMovement()->DisableMovement();
        if (AAIController* AI =
            Cast<AAIController>(Actor->GetController()))
            AI->StopMovement();
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

    if (PC->WasInputKeyJustPressed(EKeys::G))
        ToggleAlliedOrders();
    if (PC->WasInputKeyJustPressed(EKeys::Escape))
        FPlatformMisc::RequestExit(false);
}
void ASoulRealtimeArenaGameMode::Tick(float Seconds)
{
    Super::Tick(Seconds);
    if (bFinished || Combatants.IsEmpty()) return;

    for (auto& C : Combatants)
        C.MeleeCooldown =
            FMath::Max(0.0f, C.MeleeCooldown - Seconds);

    UpdateDefeatedRepresentations();

    if (!bProof)
    {
        PlayerTick(Seconds);
        if (AliveForSide(1) == 0)
            Status = TEXT("VICTORY - enemy force defeated");
        else if (AliveForSide(0) == 0)
            Status = TEXT("DEFEAT - human force defeated");
        return;
    }

    ProofElapsed += Seconds;
    if (ProofElapsed < 22.0f) return;
    int32 RemainingArrows = 0;
    for (const auto& C : Combatants)
        RemainingArrows += C.Arrows;

    const int32 HumanCasualties = CasualtiesForSide(0);
    const int32 EnemyCasualties = CasualtiesForSide(1);
    const int32 Contacts = AcceptedContacts.Num();
    const int32 Allied = TotalAlliedTargets();

    const bool bPassed =
        Combatants.Num() == 48 &&
        Groups.Num() == 16 &&
        InitialAlive[0] == 24 &&
        InitialAlive[1] == 24 &&
        Contacts >= 12 &&
        HumanCasualties + EnemyCasualties >= 4 &&
        Allied == 0 &&
        RemainingArrows < 256;

    const FString Detail = FString::Printf(
        TEXT("actors=%d groups=%d contacts=%d ")
        TEXT("casualtiesHuman=%d casualtiesEnemy=%d ")
        TEXT("alliedTargets=%d arrowsRemaining=%d"),
        Combatants.Num(), Groups.Num(), Contacts,
        HumanCasualties, EnemyCasualties,
        Allied, RemainingArrows);

    FinishProof(bPassed, Detail);
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
    if (!Host) return;

    DrawRect(FLinearColor(0, 0, 0, 0.80f),
        12, 12, 1120, 132);
    DrawText(
        TEXT("SOUL | REAL-TIME FANTASY BATTLE PROTOTYPE | 24v24"),
        FColor::White, 24, 20);
    DrawText(
        TEXT("WASD move | mouse turn | LMB melee | RMB guard | G allied CHARGE/HOLD | Esc exit"),
        FColor::White, 24, 45);
    DrawText(
        FString::Printf(
            TEXT("Human alive %d | Enemy alive %d | ")
            TEXT("Human casualties %d | Enemy casualties %d | ")
            TEXT("Hero HP %.0f | contacts %d"),
            Host->AliveForSide(0),
            Host->AliveForSide(1),
            Host->CasualtiesForSide(0),
            Host->CasualtiesForSide(1),
            Host->PlayerHealth(),
            Host->AcceptedContactCount()),
        FColor::White, 24, 70);
    DrawText(
        TEXT("RB Combat physical execution | RBAI + RB PBIL formation policy qualified"),
        FColor::Cyan, 24, 95);
    DrawText(
        Host->Status,
        FColor::Yellow, 24, 118);
}
