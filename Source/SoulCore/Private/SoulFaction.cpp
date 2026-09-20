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
    int32 Beasts = 0;
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

        switch (Slot.Role)
        {
            case ESoulUnitRole::Fighter: ++Fighters; break;
            case ESoulUnitRole::Ranged: ++Ranged; break;
            case ESoulUnitRole::Beast: ++Beasts; break;
            case ESoulUnitRole::SupportMagic: ++Support; break;
        }
    }

    if (Fighters != 4) Result.Errors.Add(FString::Printf(TEXT("Faction needs 4 fighter slots; found %d."), Fighters));
    if (Ranged != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 ranged slot; found %d."), Ranged));
    if (Beasts != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 beast slot; found %d."), Beasts));
    if (Support != 1) Result.Errors.Add(FString::Printf(TEXT("Faction needs 1 support/magic slot; found %d."), Support));

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
    }

    Result.bValid = Result.Errors.IsEmpty();
    return Result;
}
