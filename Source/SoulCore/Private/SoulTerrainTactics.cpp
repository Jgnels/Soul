#include "SoulTerrainTactics.h"
#include "SoulHex.h"

namespace
{
    int32 DirectionIndex(const FSoulHex& Origin, const FSoulHex& Target)
    {
        const TArray<FSoulHex> Neighbors = FSoulHexRules::Neighbors(Origin);
        for (int32 Index = 0; Index < Neighbors.Num(); ++Index)
        {
            if (Neighbors[Index] == Target) return Index;
        }
        return INDEX_NONE;
    }

    bool AdjacentFacing(int32 A, int32 B)
    {
        const int32 Delta = FMath::Abs(A - B);
        return Delta == 1 || Delta == 5;
    }
}

bool FSoulTerrainTactics::EntersEnemyZoneOfControl(const FSoulHex& Destination, ESoulMovementMode MoverMode, FName MoverSide, const TArray<FSoulRegimentState>& Regiments, const TMap<FName, FSoulRegimentDefinition>& Definitions)
{
    if (MoverMode == ESoulMovementMode::Flying)
    {
        return false;
    }

    for (const FSoulRegimentState& Regiment : Regiments)
    {
        if (!Regiment.bAlive || Regiment.SideId == MoverSide) continue;
        const FSoulRegimentDefinition* Def = Definitions.Find(Regiment.DefinitionId);
        if (!Def || Def->AttackRange != 1) continue;
        if (FSoulHexRules::Distance(Destination, Regiment.Anchor) <= 1)
        {
            return true;
        }
    }
    return false;
}

int32 FSoulTerrainTactics::FlankModifierPermille(const FSoulRegimentState& Attacker, const FSoulRegimentState& Defender)
{
    if (FSoulHexRules::Distance(Attacker.Anchor, Defender.Anchor) != 1)
    {
        return 0;
    }

    const int32 AttackDirection = DirectionIndex(Defender.Anchor, Attacker.Anchor);
    if (AttackDirection == INDEX_NONE) return 0;
    const int32 Back = (Defender.Facing + 3) % 6;
    if (AttackDirection == Back) return 200;
    if (AdjacentFacing(AttackDirection, Back)) return 100;
    return 0;
}

int32 FSoulTerrainTactics::AttackModifierPermille(const FSoulBattlefield& Board, const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, const FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef)
{
    int32 Modifier = 0;
    const FSoulHexCell* A = Board.Cells.Find(Attacker.Anchor);
    const FSoulHexCell* D = Board.Cells.Find(Defender.Anchor);

    if (A && D && A->Elevation > D->Elevation)
    {
        Modifier += AttackerDef.AttackRange > 1 ? 150 : 75;
    }

    if (D && D->Terrain == ESoulTerrain::Forest && AttackerDef.AttackRange > 1)
    {
        Modifier -= 150;
    }

    Modifier += FlankModifierPermille(Attacker, Defender);
    return FMath::Clamp(Modifier, -400, 400);
}
