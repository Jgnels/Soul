#pragma once

#include "CoreMinimal.h"

// Both kinds are persistent hero entities, never core troop slots or ranks.
enum class ESoulHeroKind : uint8
{
    Hero,
    Paragon
};

enum class ESoulHeroCondition : uint8 { Healthy, Wounded, Captured };

struct FSoulHeroState
{
    FName HeroId;
    ESoulHeroCondition Condition = ESoulHeroCondition::Healthy;
    int32 RecoveryDays = 0;
    FName CaptorFaction;
    FName CaptureRegion;
    ESoulHeroKind Kind = ESoulHeroKind::Hero;
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
    static bool IsAvailable(const FSoulHeroState& Hero) { return Hero.Condition==ESoulHeroCondition::Healthy; }
    static bool CanCommandSiege(const FSoulHeroState& Hero) { return IsAvailable(Hero); }
    static bool CanPerformDiplomacy(const FSoulHeroState& Hero) { return IsAvailable(Hero); }
    static void ApplyBattleInjury(FSoulHeroState& Hero, bool Wounded, int32 ArmySurvivors, FName Opponent, FName Region);
    static void AdvanceRecovery(FSoulHeroState& Hero);
    static int32 ExperienceForLevel(int32 Level);
    static bool AddExperience(FSoulHeroState& Hero, int32 Amount);
    static bool SpendSkillPoint(FSoulHeroState& Hero, FName SkillId, int32 MaxRank = 3);
    static bool LearnSpell(FSoulHeroState& Hero, FName SpellId);
    static bool SpendMana(FSoulHeroState& Hero, int32 Cost);
    static void RestoreMana(FSoulHeroState& Hero, int32 Amount);
};
