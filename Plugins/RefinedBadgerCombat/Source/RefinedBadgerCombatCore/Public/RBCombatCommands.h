#pragma once

#include "RBCombatContracts.h"

enum class ERBGroupCommand : uint8 { Follow, Hold, Face, Advance, FallBack, Charge };
enum class ERBCombatIntent : uint8 { Hold, Move, Face, Attack, Draw, Release, SwitchToMelee, Flee, Surrender, Incapacitated };

// Host stores this alongside its existing people and history when persistence
// is required. It must validate restored IDs against its own population.
struct REFINEDBADGERCOMBATCORE_API FRBCombatGroup
{
    static constexpr int32 MaximumMembers = 20;
    FGuid Id;
    FRBCombatantRef Leader;
    TArray<FRBCombatantRef> Members;
    ERBGroupCommand Command = ERBGroupCommand::Hold;
    FVector Anchor = FVector::ZeroVector;
    FVector Facing = FVector::ForwardVector;
    FRBCombatantRef Focus;
    uint64 Revision = 0;
    bool IsValid() const;
    bool Issue(const FRBCombatantRef& Issuer, ERBGroupCommand Order, const FVector& NewAnchor,
        const FVector& NewFacing, const FRBCombatantRef& NewFocus, uint64 ExpectedRevision);
    bool GetSlot(const FRBCombatantRef& Member, float Spacing, FVector& OutLocation) const;
};

struct REFINEDBADGERCOMBATCORE_API FRBCombatSituation
{
    FRBCombatantRef Person;
    FVector Position = FVector::ZeroVector;
    FVector LeaderPosition = FVector::ZeroVector;
    FVector TargetPosition = FVector::ZeroVector;
    FRBCombatantRef Target;
    bool bCanParticipate = true;
    bool bFleeing = false;
    bool bSurrendered = false;
    bool bTargetHostile = false;
    bool bTargetVisible = false;
    bool bFriendlyObstruction = false;
    bool bActionReady = true;
    bool bRangedEquipped = false;
    bool bHasAmmunition = false;
    bool bDrawing = false;
    bool bDrawReady = false;
    float MeleeReach = 150;
    float RangedMinimum = 220;
    float RangedMaximum = 3200;
};

struct REFINEDBADGERCOMBATCORE_API FRBCombatDecision
{
    ERBCombatIntent Intent = ERBCombatIntent::Hold;
    FVector Destination = FVector::ZeroVector;
    FRBCombatantRef Target;
    FName Reason;
};

// Consumes host/perception facts. Never invents hostility, visibility, morale or
// a new soldier identity. Movement still requires host navigation/avoidance.
REFINEDBADGERCOMBATCORE_API FRBCombatDecision RBDecideCombat(
    const FRBCombatGroup& Group, const FRBCombatSituation& Situation, float Spacing = 140);
