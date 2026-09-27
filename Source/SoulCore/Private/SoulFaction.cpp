#include "SoulFaction.h"

FSoulFactionValidation FSoulFactionRules::Validate(const FSoulFactionDefinition& Faction)
{
    FSoulFactionValidation Result;
    if (Faction.FactionId.IsNone())
    {
        Result.Errors.Add(TEXT("Faction id is required."));
    }

    if (Faction.CoreRoster.Num() != 7)
    {
        Result.Errors.Add(FString::Printf(TEXT("Core roster must contain exactly 7 unit families; found %d."), Faction.CoreRoster.Num()));
    }

    int32 Fighters = 0;
    int32 Ranged = 0;
    int32 Apex = 0;
    int32 Quadrupeds = 0;
    int32 Support = 0;
    TSet<FName> Seen;

    for (const FSoulFactionUnitSlot& Slot : Faction.CoreRoster)
    {
        if (Slot.UnitId.IsNone())
        {
            Result.Errors.Add(TEXT("Roster slot is missing unit id."));
        }
        else if (Seen.Contains(Slot.UnitId))
        {
            Result.Errors.Add(FString::Printf(TEXT("Duplicate roster unit id: %s"), *Slot.UnitId.ToString()));
        }
        Seen.Add(Slot.UnitId);
        if (Slot.bQuadruped) ++Quadrupeds;

        switch (Slot.Role)
        {
            case ESoulUnitRole::Fighter: ++Fighters; break;
            case ESoulUnitRole::Ranged: ++Ranged; break;
            case ESoulUnitRole::Apex: ++Apex; break;
            case ESoulUnitRole::SupportMagic: ++Support; break;
            default: Result.Errors.Add(TEXT("Unknown core roster role.")); break;
        }
    }

    if (Fighters != 4) Result.Errors.Add(FString::Printf(TEXT("Faction needs 4 fighter slots; found %d."), Fighters));
    if (Ranged != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 ranged slot; found %d."), Ranged));
    if (Apex != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 apex within its seven core slots; found %d."), Apex));
    if (Quadrupeds != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 quadruped body plan; found %d."), Quadrupeds));
    if (Support != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 support/magic slot; found %d."), Support));

    TSet<FName> SeenHeroes;
    for (FName HeroId : Faction.HeroIds)
    {
        if (HeroId.IsNone())
        {
            Result.Errors.Add(TEXT("Hero id may not be empty."));
        }
        if (Seen.Contains(HeroId))
        {
            Result.Errors.Add(FString::Printf(TEXT("Hero %s must remain separate from the seven-unit roster."), *HeroId.ToString()));
        }
        if (SeenHeroes.Contains(HeroId))
        {
            Result.Errors.Add(FString::Printf(TEXT("Duplicate hero id: %s"), *HeroId.ToString()));
        }
        SeenHeroes.Add(HeroId);
    }

    Result.bValid = Result.Errors.IsEmpty();
    return Result;
}
