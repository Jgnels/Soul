#pragma once

#include "CoreMinimal.h"
#include "SoulTypes.h"

struct FSoulFactionUnitSlot
{
    FName UnitId;
    ESoulUnitRole Role = ESoulUnitRole::Fighter;
};

struct FSoulFactionDefinition
{
    FName FactionId;
    TArray<FSoulFactionUnitSlot> CoreRoster;
    TArray<FName> HeroIds;
    FName SignatureMagicTheme;
    FName SettlementProfileId;
};

struct FSoulFactionValidation
{
    bool bValid = false;
    TArray<FString> Errors;
};

class SOULCORE_API FSoulFactionRules
{
public:
    static FSoulFactionValidation Validate(const FSoulFactionDefinition& Faction);
};
