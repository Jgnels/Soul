#pragma once

#include "CoreMinimal.h"
#include "SoulRealtimeBattleRules.h"
#include "RBPBILTypes.h"

struct FSoulSpatialPreference
{
    ERBPBILChannel Channel = ERBPBILChannel::Threat;
    float Weight = 0.0f;
};

struct FSoulFormationSpatialPolicy
{
    TArray<FSoulSpatialPreference> Preferences;
    float SearchRadius = 1600.0f;
    bool bReachableOnly = true;

    const FSoulSpatialPreference* StrongestPreference() const;
    float WeightFor(ERBPBILChannel Channel) const;
};
class SOULREALTIMEBATTLE_API FSoulRealtimeBattleSpatial
{
public:
    static FSoulFormationSpatialPolicy MakePolicy(
        ESoulRealtimeFormationRole Role);

    static FRBPBILQueryRequest MakePrimaryQuery(
        ESoulRealtimeFormationRole Role,
        const FVector& Origin);
};
