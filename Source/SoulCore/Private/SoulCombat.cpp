#include "SoulCombat.h"
#include "SoulHex.h"
#include "SoulVeterancy.h"

namespace
{
    int32 LuckValue(const FSoulRegimentDefinition& Definition)
    {
        return FMath::Clamp(Definition.Luck, -3, 3);
    }

    ESoulLuckOutcome ApplyLuck(int32& Damage, int32 Luck, FRandomStream& Rng)
    {
        if (Luck == 0 || Damage <= 0)
        {
            return ESoulLuckOutcome::None;
        }

        const int32 ChancePermille = FMath::Min(500, FMath::Abs(Luck) * 80);
        if (Rng.RandRange(0, 999) >= ChancePermille)
        {
            return ESoulLuckOutcome::None;
        }

        if (Luck > 0)
        {
            Damage = FMath::Max(Damage + 1, (Damage * 1500 + 500) / 1000);
            return ESoulLuckOutcome::Critical;
        }

        Damage = FMath::Max(1, (Damage * 500 + 500) / 1000);
        return ESoulLuckOutcome::Glancing;
    }

    int32 GroupBaseDamage(const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& Definition, FRandomStream& Rng)
    {
        const int32 Count = FMath::Max(0, Attacker.Count);
        if (Count == 0)
        {
            return 0;
        }

        const int32 MinD = FMath::Max(0, Definition.MinDamage);
        const int32 MaxD = FMath::Max(MinD, Definition.MaxDamage);

        if (Count <= 8)
        {
            int32 Total = 0;
            for (int32 Index = 0; Index < Count; ++Index)
            {
                Total += Rng.RandRange(MinD, MaxD);
            }
            return Total;
        }

        const int32 MeanTimesTwo = MinD + MaxD;
        const int32 MeanTotal = (MeanTimesTwo * Count) / 2;
        const int32 Range = MaxD - MinD;
        const int32 Variance = FMath::Max(1, FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Count)) * Range * 0.5f));
        return FMath::Max(1, MeanTotal + Rng.RandRange(-Variance, Variance));
    }
}

int32 FSoulCombatRules::EffectiveSpeed(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition)
{
    return FMath::Max(1, Definition.Speed);
}

int32 FSoulCombatRules::EffectiveMorale(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition)
{
    return FMath::Clamp(Definition.Morale + FSoulVeterancy::MoraleBonus(Regiment.Rank), -3, 3);
}

TArray<FName> FSoulCombatRules::InitiativeOrder(const TArray<FSoulRegimentState>& Regiments, const TMap<FName, FSoulRegimentDefinition>& Definitions)
{
    TArray<const FSoulRegimentState*> Active;
    for (const FSoulRegimentState& Regiment : Regiments)
    {
        if (Regiment.bAlive && Regiment.Count > 0 && Definitions.Contains(Regiment.DefinitionId))
        {
            Active.Add(&Regiment);
        }
    }

    Active.Sort([&Definitions](const FSoulRegimentState& A, const FSoulRegimentState& B)
    {
        if (A.bWaited != B.bWaited)
        {
            return !A.bWaited;
        }

        const FSoulRegimentDefinition& DefA = Definitions.FindChecked(A.DefinitionId);
        const FSoulRegimentDefinition& DefB = Definitions.FindChecked(B.DefinitionId);
        const int32 SpeedA = EffectiveSpeed(A, DefA);
        const int32 SpeedB = EffectiveSpeed(B, DefB);

        if (SpeedA != SpeedB)
        {
            return A.bWaited ? SpeedA < SpeedB : SpeedA > SpeedB;
        }
        if (A.SideId != B.SideId)
        {
            return A.SideId.LexicalLess(B.SideId);
        }
        return A.GroupId.LexicalLess(B.GroupId);
    });

    TArray<FName> Result;
    for (const FSoulRegimentState* Regiment : Active)
    {
        Result.Add(Regiment->GroupId);
    }
    return Result;
}

ESoulMoraleOutcome FSoulCombatRules::RollMorale(const FSoulRegimentState& Regiment, const FSoulRegimentDefinition& Definition, FRandomStream& Rng)
{
    const int32 Morale = EffectiveMorale(Regiment, Definition);
    if (Morale == 0)
    {
        return ESoulMoraleOutcome::None;
    }

    const int32 ChancePermille = FMath::Min(350, FMath::Abs(Morale) * 100);
    if (Rng.RandRange(0, 999) >= ChancePermille)
    {
        return ESoulMoraleOutcome::None;
    }
    return Morale > 0 ? ESoulMoraleOutcome::ExtraAction : ESoulMoraleOutcome::LoseAction;
}

FSoulDamageRoll FSoulCombatRules::RollDamage(const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, const FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef, FRandomStream& Rng)
{
    int32 Damage = GroupBaseDamage(Attacker, AttackerDef, Rng);

    int32 EffectiveAttack = AttackerDef.Attack;
    int32 EffectiveDefense = DefenderDef.Defense + (Defender.bDefending ? 3 : 0);
    const int32 Delta = EffectiveAttack - EffectiveDefense;
    const int32 AttackDefensePermille = FMath::Clamp(1000 + Delta * 50, 500, 2000);
    Damage = FMath::Max(1, (Damage * AttackDefensePermille + 500) / 1000);

    const int32 RankPermille = 1000 + FSoulVeterancy::CombatBonusPermille(Attacker.Rank);
    Damage = FMath::Max(1, (Damage * RankPermille + 500) / 1000);

    const ESoulLuckOutcome Luck = ApplyLuck(Damage, LuckValue(AttackerDef), Rng);
    return {Damage, Luck};
}

FSoulAttackResult FSoulCombatRules::ApplyAttack(const FSoulRegimentState& Attacker, const FSoulRegimentDefinition& AttackerDef, FSoulRegimentState& Defender, const FSoulRegimentDefinition& DefenderDef, FRandomStream& Rng)
{
    FSoulAttackResult Result;
    Result.DefenderCountBefore = Defender.Count;

    const FSoulDamageRoll Roll = RollDamage(Attacker, AttackerDef, Defender, DefenderDef, Rng);
    Result.Damage = Roll.Damage;
    Result.Luck = Roll.Luck;

    Defender.CurrentHitPoints = FMath::Max(0, Defender.CurrentHitPoints - Roll.Damage);
    Defender.Count = Defender.CurrentHitPoints > 0
        ? FMath::DivideAndRoundUp(Defender.CurrentHitPoints, FMath::Max(1, DefenderDef.HitPointsPerUnit))
        : 0;
    Defender.bAlive = Defender.Count > 0;
    Result.DefenderCountAfter = Defender.Count;
    Result.bDefenderDestroyed = !Defender.bAlive;
    return Result;
}

bool FSoulCombatRules::CanRetaliate(const FSoulRegimentState& Defender, const FSoulRegimentState& Attacker)
{
    return Defender.bAlive
        && Attacker.bAlive
        && !Defender.bRetaliated
        && FSoulHexRules::Distance(Defender.Anchor, Attacker.Anchor) <= 1;
}

void FSoulCombatRules::ResetRound(TArray<FSoulRegimentState>& Regiments)
{
    for (FSoulRegimentState& Regiment : Regiments)
    {
        Regiment.bRetaliated = false;
        Regiment.bWaited = false;
        Regiment.bDefending = false;
    }
}

void FSoulCombatRules::MarkWait(FSoulRegimentState& Regiment)
{
    Regiment.bWaited = true;
}
