#pragma once

#include "CoreMinimal.h"

enum class ESoulMemoryKind : uint8
{
    BattleVictory,
    BattleDefeat,
    PlaceLoss,
    SiegeVictory,
    SiegeDefeat
};

struct FSoulCommanderMemory
{
    FName Id;
    ESoulMemoryKind Kind = ESoulMemoryKind::BattleDefeat;
    FName OpponentId;
    FName PlaceId;
    int32 Turn = 1;
    int32 IntensityPermille = 700;
    int32 ValencePermille = 0;
};

struct FSoulCommanderState
{
    FName CommanderId;
    TArray<FSoulCommanderMemory> Memories;
};

class SOULCORE_API FSoulMemoryRules
{
public:
    static void RecordBattle(FSoulCommanderState& Commander, FName MemoryId, FName OpponentId, FName PlaceId, int32 Turn, bool bVictory, int32 IntensityPermille = 800);
    static void RecordPlaceLoss(FSoulCommanderState& Commander, FName MemoryId, FName OpponentId, FName PlaceId, int32 Turn, int32 IntensityPermille = 700);
    static int32 RivalBias(const FSoulCommanderState& Commander, FName OpponentId, int32 CurrentTurn);
    static int32 PlaceReclaimBonus(const FSoulCommanderState& Commander, FName PlaceId, int32 CurrentTurn);
};
