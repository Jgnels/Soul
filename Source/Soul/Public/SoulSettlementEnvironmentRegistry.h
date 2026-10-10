#pragma once
#include "CoreMinimal.h"
struct FSoulSettlementEnvironmentBinding
{
    FName SettlementId, Faction, SettlementType;
    FString VisitEnvironment, CityBattleEnvironment, SiegeEnvironment, FieldBattleEnvironment, DevelopmentProfile;
    FVector ArenaOrigin=FVector::ZeroVector;
    bool bAuthoredAvailable=false,bBattleEnabled=false;
};
// Immutable content bindings; no ownership, save, or encounter authority.
class SOUL_API FSoulSettlementEnvironmentRegistry
{
public:
    static bool Load(TMap<FName,FSoulSettlementEnvironmentBinding>& Out,FString& Error);
};
