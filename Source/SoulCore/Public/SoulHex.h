#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

class SOULCORE_API FSoulHexRules
{
public:
    static int32 Distance(const FSoulHex& A, const FSoulHex& B);
    static TArray<FSoulHex> Neighbors(const FSoulHex& Hex);
    static TArray<FSoulHex> Footprint(const FSoulHex& Anchor, int32 FootprintSize, int32 Facing);
    static bool CanOccupy(const FSoulBattlefield& Board, const FSoulHex& Anchor, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied);
    static int32 EnterCost(const FSoulHexCell& Cell, ESoulMovementMode Mode);
    static TMap<FSoulHex, int32> Reachable(const FSoulBattlefield& Board, const FSoulHex& Start, int32 Budget, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied);
    static TArray<FSoulHex> ShortestPath(const FSoulBattlefield& Board, const FSoulHex& Start, const FSoulHex& Goal, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied);
    static TArray<FSoulHex> Radius(const FSoulBattlefield& Board, const FSoulHex& Center, int32 Radius);
    static TArray<FSoulHex> Line(const FSoulHex& Start, const FSoulHex& End);
    static bool HasLineOfSight(const FSoulBattlefield& Board, const FSoulHex& Start, const FSoulHex& End);
};
