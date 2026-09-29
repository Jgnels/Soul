#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "SoulTypes.h"

enum class ESoulMoraleOutcome : uint8
{
    None,
    ExtraAction,
    LoseAction
};

enum class ESoulLuckOutcome : uint8
{
    None,
    Critical,
    Glancing
};

struct FSoulDamageRoll
{
    int32 Damage = 0;
    ESoulLuckOutcome Luck = ESoulLuckOutcome::None;
};

struct FSoulAttackResult
{
    int32 Damage = 0;
    int32 DefenderCountBefore = 0;
    int32 DefenderCountAfter = 0;
    bool bDefenderDestroyed = false;
    ESoulLuckOutcome Luck = ESoulLuckOutcome::None;
};

class SOULCORE_API FSoulCombatRules
{
public:
    static TArray<FName> InitiativeOrder(const TArray<FSoulRegimentState>& Regiments, const TMap<FName, FSoulRegimentDefinition>& Definitions);
    static int32 EffectiveSpeed(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition);
    static int32 EffectiveMorale(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition);
    static ESoulMoraleOutcome RollMorale(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition, FRandomStream& Rng);
    static FSoulDamageRoll RollDamage(const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, const FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef, FRandomStream& Rng);
    static FSoulAttackResult ApplyAttack(const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef, FRandomStream& Rng);
    static bool CanRetaliate(const FSoulRegimentState& Defender, const FSoulRegimentState& Attacker);
    static void ResetRound(TArray<FSoulRegimentState>& Regiments);
    static void MarkWait(FSoulRegimentState& Regiment);
};
