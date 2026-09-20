#pragma once

#include "CoreMinimal.h"
#include "SoulMemory.h"

enum class ESoulStrategyAction : uint8
{
    Hold,
    Recover,
    DefendOwnedRegion,
    CaptureResourceRegion,
    AttackVisibleArmy,
    SeizeExposedRegion,
    SiegeSettlement
};

struct FSoulStrategicRegion
{
    FName Id;
    FName OwnerFactionId;
    int32 Threat = 0;
    int32 Value = 0;
    int32 ResourceValue = 0;
    int32 Feasibility = 650;
    int32 TravelCost = 40;
    int32 GarrisonStrength = 0;
    bool bAdjacent = false;
    bool bOccupiedByPlayer = false;
    bool bExposed = false;
    bool bSettlement = false;
};

struct FSoulVisibleArmy
{
    FName Id;
    FName CommanderId;
    FName FactionId;
    FName RegionId;
    int32 Strength = 1000;
    bool bAdjacent = false;
};

struct FSoulStrategySnapshot
{
    FName ActingFactionId;
    FName ActingArmyId;
    FName ActingCommanderId;
    int32 Turn = 1;
    int32 Readiness = 1000;
    int32 Strength = 1000;
    TArray<FSoulStrategicRegion> Regions;
    TArray<FSoulVisibleArmy> VisibleEnemyArmies;
    FSoulCommanderState Commander;
};

struct FSoulStrategyCandidate
{
    ESoulStrategyAction Action = ESoulStrategyAction::Hold;
    FName TargetId;
    int32 TotalScore = 0;
    bool bLegal = false;
    TMap<FName, int32> Components;
    TArray<FName> Reasons;
};

struct FSoulStrategyDecision
{
    bool bOk = false;
    FSoulStrategyCandidate Chosen;
    TArray<FSoulStrategyCandidate> Alternatives;
    TArray<FSoulStrategyCandidate> Candidates;
};

class SOULCORE_API FSoulStrategyAI
{
public:
    static FSoulStrategyDecision Choose(const FSoulStrategySnapshot& Snapshot);

private:
    static FSoulStrategyCandidate Evaluate(ESoulStrategyAction Action, const FSoulStrategySnapshot& Snapshot);
    static int32 Sum(const TMap<FName, int32>& Components);
};
