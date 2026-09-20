#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

class SOULCORE_API FSoulTerrainTactics
{
public:
    static bool EntersEnemyZoneOfControl(const FSoulHex& Destination, ESoulMovementMode MoverMode, FName MoverSide, const TArray<FSoulRegimentState>& Regiments, const TMap<FName, FSoulRegimentDefinition>& Definitions);
    static int32 AttackModifierPermille(const FSoulBattlefield& Board, const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, const FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef);
    static int32 FlankModifierPermille(const FSoulRegimentState& Attacker, const FSoulRegimentState& Defender);
};
