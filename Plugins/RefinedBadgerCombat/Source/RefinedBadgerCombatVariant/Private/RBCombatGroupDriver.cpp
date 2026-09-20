#include "RBCombatGroupDriver.h"

static_assert(static_cast<uint8>(ERBHostGroupOrder::Charge) == static_cast<uint8>(ERBGroupCommand::Charge));
static_assert(static_cast<uint8>(ERBHostCombatIntent::Incapacitated) == static_cast<uint8>(ERBCombatIntent::Incapacitated));
#include "RBVariantCombatBindingComponent.h"
#include "RBCombatMeleeComponent.h"
#include "RBCombatRangedComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

URBCombatGroupDriver::URBCombatGroupDriver()
{ PrimaryComponentTick.bCanEverTick = true; }

bool URBCombatGroupDriver::SetRepresentations(const TArray<URBVariantCombatBindingComponent*>& Bindings)
{
    if (Bindings.Num() > 128) { return false; }
    TArray<TWeakObjectPtr<URBVariantCombatBindingComponent>> Candidate;
    TSet<FRBCombatantRef> Ids;
    for (auto* Binding : Bindings)
    {
        if (!IsValid(Binding) || Binding->GetWorld() != GetWorld() || !Binding->HasValidCombatant()
            || Ids.Contains(Binding->GetCombatant())) { return false; }
        Ids.Add(Binding->GetCombatant()); Candidate.Add(Binding);
    }
    for (const auto& Weak : ControlledPawns) { if (auto* Pawn = Weak.Get()) { RevokeControl(Pawn); } }
    ControlledPawns.Reset(); Representations = MoveTemp(Candidate); DirectGoals.Reset();
    OwnedDraws.Reset(); OwnedAttacks.Reset(); return true;
}

URBVariantCombatBindingComponent* URBCombatGroupDriver::Find(const FRBCombatantRef& Person) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBCombatGroupDriver_FindRepresentation);
    for (const auto& Weak : Representations)
    {
        ++RepresentationFindVisits;
        auto* Binding = Weak.Get();
        if (Binding && IsValid(Binding->GetOwner()) && Binding->GetCombatant() == Person) { return Binding; }
    }
    return nullptr;
}

void URBCombatGroupDriver::ResetWorkCounters()
{
    RepresentationFindVisits = 0;
    CandidateVisits = 0;
    OpponentQueries = 0;
    SightTraces = 0;
}

void URBCombatGroupDriver::StopMotion(APawn* Pawn)
{
    DirectGoals.Remove(Pawn);
    if (auto* AI = Cast<AAIController>(Pawn->GetController())) { AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); }
}

void URBCombatGroupDriver::RevokeControl(APawn* Pawn)
{
    StopMotion(Pawn);
    if (auto* Ranged = Pawn->FindComponentByClass<URBCombatRangedComponent>())
    {
        const FGuid* Owned = OwnedDraws.Find(Ranged);
        if (Owned && Owned->IsValid() && *Owned == Ranged->GetDrawSessionId()) { Ranged->CancelDraw(); }
        OwnedDraws.Remove(Ranged);
    }
    if (auto* Melee = Pawn->FindComponentByClass<URBCombatMeleeComponent>())
    {
        const FGuid* Owned = OwnedAttacks.Find(Melee);
        if (Owned && Owned->IsValid() && *Owned == Melee->GetActionId()) { Melee->InterruptAttack(); }
        OwnedAttacks.Remove(Melee);
    }
}

void URBCombatGroupDriver::Move(APawn* Pawn, const FVector& Goal)
{
    if (auto* AI = Cast<AAIController>(Pawn->GetController()))
    {
        const auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (Navigation && Navigation->GetDefaultNavDataInstance())
        {
            const auto Result = AI->MoveToLocation(Goal, 35, true, true, false, true, nullptr, false);
            if (Result != EPathFollowingRequestResult::Failed) { DirectGoals.Remove(Pawn); return; }
        }
    }
    if (bAllowDirectSteering) { DirectGoals.Add(Pawn, Goal); }
    else { StopMotion(Pawn); }
}

void URBCombatGroupDriver::TickComponent(float Seconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBCombatGroupDriver_Tick);
    Super::TickComponent(Seconds, TickType, TickFunction);
    for (auto It = OwnedDraws.CreateIterator(); It; ++It)
    { const auto* Component = It.Key().Get(); if (!Component || Component->GetDrawSessionId() != It.Value()) { It.RemoveCurrent(); } }
    for (auto It = OwnedAttacks.CreateIterator(); It; ++It)
    { const auto* Component = It.Key().Get(); if (!Component || Component->GetActionId() != It.Value()) { It.RemoveCurrent(); } }
    FRBCombatGroup Group;
    const bool bValidGroup = ReadGroup(Group) && Group.IsValid();
    // Membership is host-owned and may change between decisions. Revoke stale
    // movement before consuming cached per-frame steering, including when the
    // host reassigns this driver to a different valid group.
    for (auto It = ControlledPawns.CreateIterator(); It; ++It)
    {
        APawn* Pawn = It->Get();
        const auto* Binding = Pawn ? Pawn->FindComponentByClass<URBVariantCombatBindingComponent>() : nullptr;
        if (!Pawn || Pawn->IsPlayerControlled() || !bValidGroup || !Binding
            || !Group.Members.Contains(Binding->GetCombatant()) || Find(Binding->GetCombatant()) != Binding)
        {
            if (Pawn) { RevokeControl(Pawn); }
            It.RemoveCurrent();
        }
    }
    if (!bValidGroup) { DirectGoals.Reset(); DecisionElapsed = 0; return; }
    for (auto It = DirectGoals.CreateIterator(); It; ++It)
    {
        APawn* Pawn = It.Key().Get();
        if (!Pawn || Pawn->IsPlayerControlled()) { It.RemoveCurrent(); continue; }
        const FVector Direction = (It.Value() - Pawn->GetActorLocation()).GetSafeNormal2D();
        if (FVector::DistSquared2D(It.Value(), Pawn->GetActorLocation()) <= 35 * 35) { It.RemoveCurrent(); continue; }
        Pawn->AddMovementInput(Direction);
        Pawn->SetActorRotation(Direction.Rotation());
    }
    DecisionElapsed += Seconds;
    if (DecisionElapsed < .1f) { return; }
    DecisionElapsed = 0;
    for (const auto& Member : Group.Members)
    { if (auto* Binding = Find(Member)) { DecideAndDrive(Group, Binding); } }
}

void URBCombatGroupDriver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (const auto& Weak : ControlledPawns) { if (auto* Pawn = Weak.Get()) { RevokeControl(Pawn); } }
    ControlledPawns.Reset(); DirectGoals.Reset(); OwnedDraws.Reset(); OwnedAttacks.Reset();
    Super::EndPlay(EndPlayReason);
}

void URBCombatGroupDriver::DecideAndDrive(const FRBCombatGroup& Group, URBVariantCombatBindingComponent* Binding)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(RBCombatGroupDriver_DecideAndDrive);
    auto* Pawn = Cast<APawn>(Binding->GetOwner());
    if (!Pawn) { return; }
    if (Pawn->IsPlayerControlled()) { RevokeControl(Pawn); ControlledPawns.Remove(Pawn); return; }
    ControlledPawns.Add(Pawn);
    FRBCombatSituation Situation;
    Situation.Person = Binding->GetCombatant();
    if (!ReadParticipant(Situation.Person, Situation)) { RevokeControl(Pawn); return; }
    Situation.Position = Pawn->GetActorLocation();
    auto* Leader = Find(Group.Leader);
    Situation.LeaderPosition = Leader ? Leader->GetOwner()->GetActorLocation() : Group.Anchor;
    auto* Melee = Pawn->FindComponentByClass<URBCombatMeleeComponent>();
    auto* Ranged = Pawn->FindComponentByClass<URBCombatRangedComponent>();
    FRBWeaponProfile Weapon;
    if (Binding->ResolveEquippedWeapon(Weapon))
    {
        Situation.MeleeReach = FMath::Max(1.f, Weapon.Reach);
        Situation.bRangedEquipped = Weapon.Category == FName(TEXT("Bow"));
        Situation.bHasAmmunition = Ranged && HasAmmunition(Situation.Person, Ranged->GetAmmunitionId());
        Situation.bDrawing = Ranged && Ranged->IsDrawing();
        Situation.bDrawReady = Ranged && Ranged->IsDrawReady();
        Situation.bActionReady = !Melee || !Melee->IsAttacking();
    }
    double BestDistance = SightDistance * SightDistance;
    AActor* Target = nullptr;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RBCombatSight), false, Pawn);
    for (const auto& Weak : Representations)
    {
        ++CandidateVisits;
        auto* Other = Weak.Get();
        if (!Other || Other == Binding || !Other->IsAbleToAct()) { continue; }
        ++OpponentQueries;
        if (!AreOpponents(Situation.Person, Other->GetCombatant())
            || (Group.Focus.IsValid() && Group.Focus != Other->GetCombatant())) { continue; }
        AActor* Actor = Other->GetOwner();
        const double Distance = FVector::DistSquared2D(Situation.Position, Actor->GetActorLocation());
        if (Distance > BestDistance) { continue; }
        FHitResult Hit;
        ++SightTraces;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Situation.Position + FVector(0, 0, 40),
            Actor->GetActorLocation() + FVector(0, 0, 40), ECC_Visibility, Query);
        if (bHit && Hit.GetActor() != Actor) { continue; }
        BestDistance = Distance; Target = Actor;
        Situation.Target = Other->GetCombatant(); Situation.TargetPosition = Actor->GetActorLocation();
        Situation.bTargetHostile = true; Situation.bTargetVisible = true;
    }
    const FRBCombatDecision Decision = RBDecideCombat(Group, Situation, FormationSpacing);
    ObserveDecision(Situation.Person, Decision);
    if (Decision.Intent == ERBCombatIntent::Move || Decision.Intent == ERBCombatIntent::Flee)
    { if (Ranged) { Ranged->CancelDraw(); } Move(Pawn, Decision.Destination); return; }
    StopMotion(Pawn);
    if (Target) { Pawn->SetActorRotation((Target->GetActorLocation() - Situation.Position).GetSafeNormal2D().Rotation()); }
    if (Decision.Intent == ERBCombatIntent::Face) { Pawn->SetActorRotation(Group.Facing.Rotation()); }
    if (Decision.Intent == ERBCombatIntent::Attack && Melee)
    { if (!Melee->IsAttacking() && Melee->RequestAttack(ERBWeaponAttack::Light)) { OwnedAttacks.Add(Melee, Melee->GetActionId()); } }
    else if (Decision.Intent == ERBCombatIntent::Draw && Ranged)
    { if (Ranged->BeginDraw()) { OwnedDraws.Add(Ranged, Ranged->GetDrawSessionId()); } }
    else if (Decision.Intent == ERBCombatIntent::Release && Ranged && Target)
    {
        const FVector Muzzle = Situation.Position + FVector(0, 0, 40);
        Ranged->ReleaseShot(Muzzle, Target->GetActorLocation() + FVector(0, 0, 40) - Muzzle);
    }
    else if (Decision.Intent == ERBCombatIntent::SwitchToMelee)
    { if (Ranged) { Ranged->CancelDraw(); } RequestMeleeFallback(Binding); }
    if (!Situation.bCanParticipate || Situation.bFleeing || Situation.bSurrendered || !Target)
    { if (Ranged) { Ranged->CancelDraw(); } if (Melee && Melee->IsAttacking()) { Melee->InterruptAttack(); } }
}

bool FRBHostGroup::ToCore(FRBCombatGroup& Out) const
{
    if (Revision < 0 || Members.Num() > FRBCombatGroup::MaximumMembers
        || static_cast<uint8>(Order) > static_cast<uint8>(ERBHostGroupOrder::Charge)) { return false; }
    FRBCombatGroup Candidate;
    Candidate.Id = Id; Candidate.Leader = Leader.Core(); Candidate.Command = static_cast<ERBGroupCommand>(Order);
    Candidate.Anchor = Anchor; Candidate.Facing = Facing; Candidate.Focus = Focus.Core(); Candidate.Revision = Revision;
    for (const auto& Member : Members) { Candidate.Members.Add(Member.Core()); }
    if (!Candidate.IsValid()) { return false; }
    Out = MoveTemp(Candidate); return true;
}

bool FRBHostGroup::FromCore(const FRBCombatGroup& Value, FRBHostGroup& Out)
{
    if (!Value.IsValid() || Value.Revision > static_cast<uint64>(MAX_int64)) { return false; }
    FRBHostGroup Candidate;
    Candidate.Id = Value.Id; Candidate.Leader = FRBHostIdentity::From(Value.Leader);
    Candidate.Order = static_cast<ERBHostGroupOrder>(Value.Command); Candidate.Anchor = Value.Anchor;
    Candidate.Facing = Value.Facing; Candidate.Focus = FRBHostIdentity::From(Value.Focus); Candidate.Revision = Value.Revision;
    for (const auto& Member : Value.Members) { Candidate.Members.Add(FRBHostIdentity::From(Member)); }
    Out = MoveTemp(Candidate); return true;
}

bool URBCombatBlueprintGroupDriver::HostReadGroup_Implementation(FRBHostGroup& Group) const { return false; }
bool URBCombatBlueprintGroupDriver::HostReadParticipation_Implementation(FRBHostIdentity Person, FRBHostParticipation& Participation) const { return false; }
bool URBCombatBlueprintGroupDriver::HostAreOpponents_Implementation(FRBHostIdentity Person, FRBHostIdentity Other) const { return false; }
bool URBCombatBlueprintGroupDriver::HostHasAmmunition_Implementation(FRBHostIdentity Person, FName Item) const { return false; }
bool URBCombatBlueprintGroupDriver::HostRequestMeleeFallback_Implementation(URBVariantCombatBindingComponent* Binding) { return false; }
void URBCombatBlueprintGroupDriver::HostObserveDecision_Implementation(FRBHostIdentity Person, const FRBHostDecision& Decision) {}
bool URBCombatBlueprintGroupDriver::HostCommitGroupOrder_Implementation(const FRBHostGroup& Previous, const FRBHostGroup& Replacement, FString& Error)
{ Error = TEXT("Host has not implemented atomic group-order persistence."); return false; }

bool URBCombatBlueprintGroupDriver::ReadGroup(FRBCombatGroup& Out) const
{ FRBHostGroup Snapshot; return HostReadGroup(Snapshot) && Snapshot.ToCore(Out); }

bool URBCombatBlueprintGroupDriver::ReadParticipant(const FRBCombatantRef& Person, FRBCombatSituation& Out) const
{
    FRBHostParticipation State;
    if (!Person.IsValid() || !HostReadParticipation(FRBHostIdentity::From(Person), State)
        || !FMath::IsFinite(State.RangedMinimum) || !FMath::IsFinite(State.RangedMaximum)
        || State.RangedMinimum <= 0 || State.RangedMaximum < State.RangedMinimum) { return false; }
    Out.Person = Person; Out.bCanParticipate = State.bCanParticipate; Out.bFleeing = State.bFleeing;
    Out.bSurrendered = State.bSurrendered; Out.bFriendlyObstruction = State.bFriendlyObstruction;
    Out.RangedMinimum = State.RangedMinimum; Out.RangedMaximum = State.RangedMaximum; return true;
}

bool URBCombatBlueprintGroupDriver::AreOpponents(const FRBCombatantRef& Person, const FRBCombatantRef& Other) const
{ return Person.IsValid() && Other.IsValid() && Person != Other && HostAreOpponents(FRBHostIdentity::From(Person), FRBHostIdentity::From(Other)); }
bool URBCombatBlueprintGroupDriver::HasAmmunition(const FRBCombatantRef& Person, FName Item) const
{ return Person.IsValid() && !Item.IsNone() && HostHasAmmunition(FRBHostIdentity::From(Person), Item); }
bool URBCombatBlueprintGroupDriver::RequestMeleeFallback(URBVariantCombatBindingComponent* Binding)
{ return IsValid(Binding) && HostRequestMeleeFallback(Binding); }
void URBCombatBlueprintGroupDriver::ObserveDecision(const FRBCombatantRef& Person, const FRBCombatDecision& Decision)
{
    FRBHostDecision Result; Result.Intent = static_cast<ERBHostCombatIntent>(Decision.Intent);
    Result.Destination = Decision.Destination; Result.Target = FRBHostIdentity::From(Decision.Target); Result.Reason = Decision.Reason;
    HostObserveDecision(FRBHostIdentity::From(Person), Result);
}

bool URBCombatBlueprintGroupDriver::RequestGroupOrder(FRBHostIdentity Issuer, ERBHostGroupOrder Order,
    FVector Anchor, FVector Facing, FRBHostIdentity Focus, int64 ExpectedRevision, FString& Error)
{
    Error.Reset();
    FRBHostGroup Previous, Replacement;
    FRBCombatGroup Candidate;
    if (ExpectedRevision < 0 || ExpectedRevision == MAX_int64 || !HostReadGroup(Previous) || !Previous.ToCore(Candidate)
        || !Candidate.Issue(Issuer.Core(), static_cast<ERBGroupCommand>(Order), Anchor, Facing, Focus.Core(), ExpectedRevision)
        || !FRBHostGroup::FromCore(Candidate, Replacement))
    { Error = TEXT("Group order requires a valid current group, leader, revision and command."); return false; }
    return HostCommitGroupOrder(Previous, Replacement, Error);
}
