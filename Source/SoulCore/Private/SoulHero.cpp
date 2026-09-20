#include "SoulHero.h"

int32 FSoulHeroRules::ExperienceForLevel(int32 Level)
{
    Level = FMath::Max(1, Level);
    return (Level - 1) * Level * 125;
}

bool FSoulHeroRules::AddExperience(FSoulHeroState& Hero, int32 Amount)
{
    if (Amount <= 0) return false;
    Hero.Experience += Amount;
    bool bLeveled = false;

    while (Hero.Experience >= ExperienceForLevel(Hero.Level + 1))
    {
        ++Hero.Level;
        ++Hero.UnspentSkillPoints;
        bLeveled = true;
    }
    return bLeveled;
}

bool FSoulHeroRules::SpendSkillPoint(FSoulHeroState& Hero, FName SkillId, int32 MaxRank)
{
    if (Hero.UnspentSkillPoints <= 0 || SkillId.IsNone()) return false;
    int32& Rank = Hero.Skills.FindOrAdd(SkillId);
    if (Rank >= FMath::Max(1, MaxRank)) return false;
    ++Rank;
    --Hero.UnspentSkillPoints;
    return true;
}

bool FSoulHeroRules::LearnSpell(FSoulHeroState& Hero, FName SpellId)
{
    if (SpellId.IsNone() || Hero.KnownSpells.Contains(SpellId)) return false;
    Hero.KnownSpells.Add(SpellId);
    return true;
}

bool FSoulHeroRules::SpendMana(FSoulHeroState& Hero, int32 Cost)
{
    Cost = FMath::Max(0, Cost);
    if (Hero.Mana < Cost) return false;
    Hero.Mana -= Cost;
    return true;
}

void FSoulHeroRules::RestoreMana(FSoulHeroState& Hero, int32 Amount)
{
    Hero.Mana = FMath::Clamp(Hero.Mana + FMath::Max(0, Amount), 0, FMath::Max(0, Hero.MaxMana));
}
