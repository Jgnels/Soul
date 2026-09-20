#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

enum class ESoulEffectKind : uint8
{
    Damage,
    Heal,
    ModifyStat,
    ApplyStatus,
    Displace,
    Summon,
    TerrainChange,
    Reveal
};

enum class ESoulTargetShape : uint8
{
    Single,
    Radius,
    Line,
    Cone
};

struct FSoulEffectSpec
{
    ESoulEffectKind Kind = ESoulEffectKind::Damage;
    int32 Magnitude = 0;
    int32 DurationRounds = 0;
    FName Tag;
};

struct FSoulSpellDefinition
{
    FName Id;
    int32 CastRange = 6;
    ESoulTargetShape Shape = ESoulTargetShape::Single;
    int32 Radius = 0;
    int32 ManaCost = 0;
    TArray<FSoulEffectSpec> Effects;
};

class SOULCORE_API FSoulMagicRules
{
public:
    static bool IsLegalTarget(const FSoulBattlefield& Board, const FSoulHex& Caster, const FSoulHex& Target, const FSoulSpellDefinition& Spell);
    static TArray<FSoulHex> AffectedCells(const FSoulBattlefield& Board, const FSoulHex& Target, const FSoulSpellDefinition& Spell);
};
