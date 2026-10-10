#include "SoulBattleControlRules.h"

bool FSoulBattleControlRules::AllowsEngagement(const FRBCombatGroup& Group,
    const FRBCombatantRef& Member, const FVector& Position, float Spacing,
    bool bManualOrder, float ArrivalRadius)
{
    if (!Group.IsValid() || !Group.Members.Contains(Member) || Position.ContainsNaN() ||
        !FMath::IsFinite(Spacing) || Spacing <= 0 || !FMath::IsFinite(ArrivalRadius) || ArrivalRadius < 35.f) return false;
    if (!bManualOrder || Group.Command == ERBGroupCommand::Charge) return true;
    if (Group.Command == ERBGroupCommand::FallBack) return false;
    // RBDecideCombat considers visible melee/ranged targets before formation
    // movement. With a manual order, first regain the assigned slot. This changes
    // engagement eligibility, not hostility or the ability to receive damage.
    // RB navigation uses MoveToLocation(Goal, 35, stopOnOverlap=true).
    // The host supplies that radius plus the pawn's collision footprint; the
    // direct-steering path also works with this conservative arrival envelope.
    FVector Slot;
    return Group.GetSlot(Member, Spacing, Slot) &&
        FVector::DistSquared2D(Position, Slot) <= FMath::Square(FMath::Max(ArrivalRadius, Spacing * .25f));
}
