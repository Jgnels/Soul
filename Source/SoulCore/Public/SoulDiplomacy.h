#pragma once
#include "CoreMinimal.h"

enum class ESoulDiplomaticStance : uint8 { War, Peace, NonAggression };
enum class ESoulDiplomaticAction : uint8 { DeclareWar, Peace, NonAggression, Gift, BreakTreaty };
struct FSoulDiplomaticRelation
{
    ESoulDiplomaticStance Stance=ESoulDiplomaticStance::War;
    int32 RelationPermille=0, PactUntilDay=0, BetrayalDay=0, LastActionDay=0;
};
struct FSoulDiplomaticDecision
{
    bool bAccepted=false;
    int32 ScorePermille=0;
    TMap<FName,int32> Components;
    TArray<FString> Reasons;
};
class SOULCORE_API FSoulDiplomacyRules
{
public:
    static ESoulDiplomaticStance EffectiveStance(const FSoulDiplomaticRelation& Relation,int32 Day);
    static FSoulDiplomaticDecision Evaluate(const FSoulDiplomaticRelation& Relation,ESoulDiplomaticAction Action,int32 Day);
    static bool Apply(FSoulDiplomaticRelation& Relation,ESoulDiplomaticAction Action,int32 Day);
    static bool Valid(const FSoulDiplomaticRelation& Relation,int32 Day);
    static FString StanceName(ESoulDiplomaticStance Stance);
};
