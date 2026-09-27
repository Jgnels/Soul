#pragma once

#include "CoreMinimal.h"

enum class ESoulTerrain : uint8
{
    Open,
    Road,
    Rough,
    Forest,
    HighGround,
    ShallowWater,
    DeepWater,
    Fortification,
    Hazard
};

enum class ESoulUnitRole : uint8
{
    Fighter,
    Ranged,
    SupportMagic,
    // Apex occupies the seventh core slot. Body plan is independent of role.
    Apex
};

enum class ESoulRegimentRank : uint8
{
    Recruit,
    Seasoned,
    Veteran,
    Elite,
    Legendary
};

enum class ESoulMovementMode : uint8
{
    Ground,
    Flying
};

struct FSoulHex
{
    int32 Q = 0;
    int32 R = 0;

    FSoulHex() = default;
    FSoulHex(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

    bool operator==(const FSoulHex& Other) const { return Q == Other.Q && R == Other.R; }
    bool operator!=(const FSoulHex& Other) const { return !(*this == Other); }
    bool operator<(const FSoulHex& Other) const { return Q == Other.Q ? R < Other.R : Q < Other.Q; }

    FString ToString() const { return FString::Printf(TEXT("%d,%d"), Q, R); }
};

FORCEINLINE uint32 GetTypeHash(const FSoulHex& Hex)
{
    return HashCombine(::GetTypeHash(Hex.Q), ::GetTypeHash(Hex.R));
}

struct FSoulHexCell
{
    FSoulHex Coord;
    ESoulTerrain Terrain = ESoulTerrain::Open;
    int32 MovementCost = 100;
    int32 Elevation = 0;
    bool bBlocksLineOfSight = false;
    bool bBlocksLargeUnits = false;
};

struct FSoulBattlefield
{
    TMap<FSoulHex, FSoulHexCell> Cells;
};

struct FSoulRegimentDefinition
{
    FName Id;
    ESoulUnitRole Role = ESoulUnitRole::Fighter;
    ESoulMovementMode MovementMode = ESoulMovementMode::Ground;
    int32 Speed = 5;
    int32 MovePoints = 5;
    int32 Attack = 5;
    int32 Defense = 5;
    int32 MinDamage = 1;
    int32 MaxDamage = 2;
    int32 HitPointsPerUnit = 10;
    int32 AttackRange = 1;
    int32 Morale = 0;
    int32 Luck = 0;
    int32 FootprintSize = 1;
};

struct FSoulRegimentState
{
    FName GroupId;
    FName DefinitionId;
    FName SideId;
    int32 Count = 1;
    int32 CurrentHitPoints = 10;
    int32 Experience = 0;
    ESoulRegimentRank Rank = ESoulRegimentRank::Recruit;
    FSoulHex Anchor;
    int32 Facing = 0;
    bool bRetaliated = false;
    bool bWaited = false;
    bool bDefending = false;
    bool bAlive = true;
};
