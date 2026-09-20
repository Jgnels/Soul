#include "SoulMagic.h"
#include "SoulHex.h"

bool FSoulMagicRules::IsLegalTarget(const FSoulBattlefield& Board, const FSoulHex& Caster, const FSoulHex& Target, const FSoulSpellDefinition& Spell)
{
    return Board.Cells.Contains(Target) && FSoulHexRules::Distance(Caster, Target) <= FMath::Max(0, Spell.CastRange);
}

TArray<FSoulHex> FSoulMagicRules::AffectedCells(const FSoulBattlefield& Board, const FSoulHex& Target, const FSoulSpellDefinition& Spell)
{
    if (!Board.Cells.Contains(Target))
    {
        return {};
    }

    if (Spell.Shape == ESoulTargetShape::Radius)
    {
        return FSoulHexRules::Radius(Board, Target, FMath::Max(0, Spell.Radius));
    }

    return {Target};
}
