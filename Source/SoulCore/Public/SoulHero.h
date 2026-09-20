#pragma once

#include "CoreMinimal.h"

struct FSoulHeroState
{
    FName HeroId;
    int32 Level = 1;
    int32 Experience = 0;
    int32 UnspentSkillPoints = 0;
    int32 Mana = 0;
    int32 MaxMana = 0;
    TMap<FName, int32> Skills;
    TSet<FName> KnownSpells;
    TArray<FName> ArtifactSlots;
};

class SOULCORE_API FSoulHeroRules
{
public:
    static int32 ExperienceForLevel(int32 Level);
    static bool AddExperience(FSoulHeroState& Hero, int32 Amount);
    static bool SpendSkillPoint(FSoulHeroState& Hero, FName SkillId, int32 MaxRank = 3);
    static bool LearnSpell(FSoulHeroState& Hero, FName SpellId);
    static bool SpendMana(FSoulHeroState& Hero, int32 Cost);
    static void RestoreMana(FSoulHeroState& Hero, int32 Amount);
};
