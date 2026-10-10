#pragma once
#include "CoreMinimal.h"
class USoulSettlementScenarioData;
struct FSoulHeartlandSpell { FName Id,School,RequiredBuilding; };
struct FSoulHeartlandSite { FName Id,Region,Effect; FString Name; int32 Amount=0,ActionCost=1; };
struct FSoulHeartlandContent
{
    TArray<FSoulHeartlandSpell> Spells;
    TArray<FSoulHeartlandSite> Sites;
    FName HeroId,PrimarySchool;
    TSet<FName> SecondarySchools,ForbiddenSchools;
    bool AllowsSchool(FName School) const { return !ForbiddenSchools.Contains(School)&&(School==PrimarySchool||SecondarySchools.Contains(School)); }
    static bool Load(USoulSettlementScenarioData* Scenario,FSoulHeartlandContent& Out,FString& Error);
};
