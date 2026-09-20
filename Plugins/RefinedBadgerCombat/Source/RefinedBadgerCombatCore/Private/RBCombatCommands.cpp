#include "RBCombatCommands.h"

bool FRBCombatGroup::IsValid() const
{
    if (!Id.IsValid() || !Leader.IsValid() || Members.IsEmpty() || Members.Num() > MaximumMembers
        || !Members.Contains(Leader) || Anchor.ContainsNaN() || Facing.ContainsNaN()
        || Facing.GetSafeNormal2D().IsNearlyZero()
        || static_cast<uint8>(Command) > static_cast<uint8>(ERBGroupCommand::Charge)) { return false; }
    TSet<FRBCombatantRef> Seen;
    for (const auto& Member : Members)
    {
        if (!Member.IsValid() || Member.Domain != Leader.Domain || Seen.Contains(Member)) { return false; }
        Seen.Add(Member);
    }
    return (!Focus.Id.IsValid() && Focus.Domain.IsNone())
        || (Focus.IsValid() && Focus.Domain == Leader.Domain && !Members.Contains(Focus));
}

bool FRBCombatGroup::Issue(const FRBCombatantRef& Issuer, ERBGroupCommand Order,
    const FVector& NewAnchor, const FVector& NewFacing, const FRBCombatantRef& NewFocus, uint64 ExpectedRevision)
{
    if (!IsValid() || Issuer != Leader || ExpectedRevision != Revision || Revision == MAX_uint64) { return false; }
    auto Candidate = *this;
    Candidate.Command = Order; Candidate.Anchor = NewAnchor; Candidate.Facing = NewFacing;
    Candidate.Focus = NewFocus; ++Candidate.Revision;
    if (!Candidate.IsValid()) { return false; }
    *this = MoveTemp(Candidate); return true;
}

bool FRBCombatGroup::GetSlot(const FRBCombatantRef& Member, float Spacing, FVector& OutLocation) const
{
    const int32 Index = Members.IndexOfByKey(Member);
    if (!IsValid() || Index == INDEX_NONE || !FMath::IsFinite(Spacing) || Spacing <= 0) { return false; }
    const FVector Forward = Facing.GetSafeNormal2D();
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
    const int32 Columns = FMath::Min(4, Members.Num());
    const int32 Row = Index / Columns;
    const int32 RowMembers = FMath::Min(Columns, Members.Num() - Row * Columns);
    OutLocation = Anchor + Right * ((Index % Columns) - .5 * (RowMembers - 1)) * Spacing - Forward * Row * Spacing;
    return true;
}

FRBCombatDecision RBDecideCombat(const FRBCombatGroup& Group, const FRBCombatSituation& S, float Spacing)
{
    FRBCombatDecision D;
    D.Destination = S.Position;
    if (!Group.IsValid() || !Group.Members.Contains(S.Person) || S.Position.ContainsNaN()
        || S.LeaderPosition.ContainsNaN() || S.TargetPosition.ContainsNaN()
        || !FMath::IsFinite(S.MeleeReach) || S.MeleeReach <= 0
        || !FMath::IsFinite(S.RangedMinimum) || !FMath::IsFinite(S.RangedMaximum)
        || S.RangedMinimum <= 0 || S.RangedMaximum < S.RangedMinimum)
    { D.Reason = TEXT("InvalidContext"); return D; }
    if (!S.bCanParticipate) { D.Intent = ERBCombatIntent::Incapacitated; D.Reason = TEXT("HostParticipation"); return D; }
    if (S.bSurrendered) { D.Intent = ERBCombatIntent::Surrender; D.Reason = TEXT("HostSurrender"); return D; }
    FVector Slot;
    if (!Group.GetSlot(S.Person, Spacing, Slot)) { D.Reason = TEXT("InvalidSpacing"); return D; }
    if (S.bFleeing || Group.Command == ERBGroupCommand::FallBack)
    { D.Intent = S.bFleeing ? ERBCombatIntent::Flee : ERBCombatIntent::Move; D.Destination = Slot; D.Reason = TEXT("Withdrawal"); return D; }
    const bool TargetMatchesFocus = !Group.Focus.IsValid() || Group.Focus == S.Target;
    const bool CanEngage = S.Target.IsValid() && S.Target != S.Person && !Group.Members.Contains(S.Target)
        && S.bTargetHostile && S.bTargetVisible && TargetMatchesFocus;
    if (CanEngage)
    {
        D.Target = S.Target;
        const double Distance = FVector::Dist2D(S.Position, S.TargetPosition);
        if (S.bRangedEquipped)
        {
            if (!S.bHasAmmunition || Distance < S.RangedMinimum)
            { D.Intent = ERBCombatIntent::SwitchToMelee; D.Reason = TEXT("RangedUnavailable"); return D; }
            if (Distance <= S.RangedMaximum)
            {
                if (S.bFriendlyObstruction || !S.bActionReady)
                { D.Reason = TEXT("UnsafeOrBusyShot"); return D; }
                D.Intent = S.bDrawing ? (S.bDrawReady ? ERBCombatIntent::Release : ERBCombatIntent::Hold) : ERBCombatIntent::Draw;
                D.Reason = TEXT("VisibleRangedTarget"); return D;
            }
        }
        else if (Distance <= S.MeleeReach)
        {
            D.Intent = S.bActionReady ? ERBCombatIntent::Attack : ERBCombatIntent::Hold;
            D.Reason = TEXT("VisibleMeleeTarget"); return D;
        }
        if (Group.Command == ERBGroupCommand::Charge)
        {
            D.Intent = ERBCombatIntent::Move;
            const FVector Away = (S.Position - S.TargetPosition).GetSafeNormal2D();
            D.Destination = S.TargetPosition + Away * S.MeleeReach * .8f;
            D.Reason = TEXT("ChargeVisibleTarget"); return D;
        }
    }
    if (Group.Command == ERBGroupCommand::Face)
    { D.Intent = ERBCombatIntent::Face; D.Destination = S.Position + Group.Facing.GetSafeNormal2D(); D.Reason = TEXT("FaceOrder"); return D; }
    if (Group.Command == ERBGroupCommand::Follow)
    {
        if (S.Person == Group.Leader) { D.Reason = TEXT("LeaderHoldsOwnPosition"); return D; }
        FVector LeaderSlot;
        if (!Group.GetSlot(Group.Leader, Spacing, LeaderSlot)) { D.Reason = TEXT("InvalidLeaderSlot"); return D; }
        Slot += S.LeaderPosition - LeaderSlot;
    }
    D.Destination = Slot;
    D.Intent = FVector::DistSquared2D(S.Position, Slot) > FMath::Square(Spacing * .25f) ? ERBCombatIntent::Move : ERBCombatIntent::Hold;
    D.Reason = TEXT("FormationOrder"); return D;
}
