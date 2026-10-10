#pragma once

#include "CoreMinimal.h"
#include "RBCombatCommands.h"

// Battle command policy only. RB Combat still owns decisions, contacts and damage.
class SOULREALTIMEBATTLE_API FSoulBattleControlRules
{
public:
    static bool AllowsEngagement(const FRBCombatGroup& DriveGroup,
        const FRBCombatantRef& Member, const FVector& Position, float Spacing,
        bool bManualOrder, float ArrivalRadius = 35.f);
};
