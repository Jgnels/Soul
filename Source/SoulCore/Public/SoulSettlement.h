#pragma once

#include "CoreMinimal.h"

enum class ESoulBuildingCondition : uint8
{
    Unbuilt,
    Building,
    Intact,
    Damaged,
    Ruined
};

struct FSoulBuildingDefinition
{
    FName Id;
    FName Category;
    int32 MaxLevel = 1;
    int32 BuildDays = 1;
    TMap<FName, int32> BuildCost;
    TArray<FName> Prerequisites;
    TArray<FName> UnlockIds;
};

struct FSoulBuildingState
{
    FName Id;
    int32 Level = 0;
    int32 IntegrityPermille = 0;
    int32 ConstructionDaysRemaining = 0;
    ESoulBuildingCondition Condition = ESoulBuildingCondition::Unbuilt;
};

struct FSoulSettlementState
{
    FName SettlementId;
    FName FactionId;
    FName RegionId;
    int32 FortificationLevel = 0;
    int32 WallIntegrityPermille = 1000;
    TMap<FName, FSoulBuildingState> Buildings;
    TSet<FName> PermanentScars;
};

struct FSoulSettlementProjection
{
    TSet<FName> VisibleBuildings;
    TSet<FName> DamagedBuildings;
    TSet<FName> RuinedBuildings;
    int32 WallIntegrityPermille = 1000;
    TSet<FName> Scars;
};

class SOULCORE_API FSoulSettlementRules
{
public:
    static bool PrerequisitesMet(const FSoulSettlementState& Settlement, const FSoulBuildingDefinition& Definition);
    static bool BeginConstruction(FSoulSettlementState& Settlement, const FSoulBuildingDefinition& Definition);
    static void AdvanceDay(FSoulSettlementState& Settlement);
    static bool DamageBuilding(FSoulSettlementState& Settlement, FName BuildingId, int32 DamagePermille, FName ScarId = NAME_None);
    static bool RepairBuilding(FSoulSettlementState& Settlement, FName BuildingId, int32 RepairPermille);
    static void DamageWalls(FSoulSettlementState& Settlement, int32 DamagePermille, FName ScarId = NAME_None);
    static void RepairWalls(FSoulSettlementState& Settlement, int32 RepairPermille);
    static FSoulSettlementProjection Project(const FSoulSettlementState& Settlement);
};
