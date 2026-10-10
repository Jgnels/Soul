#pragma once
#include "CoreMinimal.h"
struct FSoulSettlementEnvironmentBinding
{
    FName SettlementId, Faction, SettlementType;
    FString VisitEnvironment, CityBattleEnvironment, SiegeEnvironment, FieldBattleEnvironment, DevelopmentProfile;
    FVector ArenaOrigin=FVector::ZeroVector;
    FVector SiegeOrigin=FVector::ZeroVector;
    bool bAuthoredAvailable=false,bBattleEnabled=false;
    const FString& BattleEnvironment() const { return FieldBattleEnvironment.IsEmpty()?CityBattleEnvironment:FieldBattleEnvironment; }
    bool IsFieldEncounter() const { return !FieldBattleEnvironment.IsEmpty(); }
};
// Immutable content bindings; no ownership, save, or encounter authority.
class SOUL_API FSoulSettlementEnvironmentRegistry
{
public:
    static bool Load(TMap<FName,FSoulSettlementEnvironmentBinding>& Out,FString& Error);
};
