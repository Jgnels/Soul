#include "SoulMemory.h"

namespace
{
    bool HasMemoryId(const FSoulCommanderState& Commander, FName MemoryId)
    {
        return Commander.Memories.ContainsByPredicate([MemoryId](const FSoulCommanderMemory& Memory)
        {
            return Memory.Id == MemoryId;
        });
    }

    int32 Decay(int32 Intensity, int32 Age, int32 Horizon)
    {
        if (Age < 0 || Age >= Horizon)
        {
            return 0;
        }
        return (FMath::Clamp(Intensity, 0, 1000) * (Horizon - Age)) / Horizon;
    }
}

void FSoulMemoryRules::RecordBattle(FSoulCommanderState& Commander, FName MemoryId, FName OpponentId, FName PlaceId, int32 Turn, bool bVictory, int32 IntensityPermille)
{
    if (MemoryId.IsNone() || HasMemoryId(Commander, MemoryId))
    {
        return;
    }

    FSoulCommanderMemory Memory;
    Memory.Id = MemoryId;
    Memory.Kind = bVictory ? ESoulMemoryKind::BattleVictory : ESoulMemoryKind::BattleDefeat;
    Memory.OpponentId = OpponentId;
    Memory.PlaceId = PlaceId;
    Memory.Turn = Turn;
    Memory.IntensityPermille = FMath::Clamp(IntensityPermille, 0, 1000);
    Memory.ValencePermille = bVictory ? 650 : -650;
    Commander.Memories.Add(Memory);
}

void FSoulMemoryRules::RecordPlaceLoss(FSoulCommanderState& Commander, FName MemoryId, FName OpponentId, FName PlaceId, int32 Turn, int32 IntensityPermille)
{
    if (MemoryId.IsNone() || HasMemoryId(Commander, MemoryId))
    {
        return;
    }

    FSoulCommanderMemory Memory;
    Memory.Id = MemoryId;
    Memory.Kind = ESoulMemoryKind::PlaceLoss;
    Memory.OpponentId = OpponentId;
    Memory.PlaceId = PlaceId;
    Memory.Turn = Turn;
    Memory.IntensityPermille = FMath::Clamp(IntensityPermille, 0, 1000);
    Memory.ValencePermille = -650;
    Commander.Memories.Add(Memory);
}

int32 FSoulMemoryRules::RivalBias(const FSoulCommanderState& Commander, FName OpponentId, int32 CurrentTurn)
{
    int32 Bias = 0;
    for (const FSoulCommanderMemory& Memory : Commander.Memories)
    {
        if (Memory.OpponentId != OpponentId)
        {
            continue;
        }

        const int32 Strength = Decay(Memory.IntensityPermille, CurrentTurn - Memory.Turn, 7);
        if (Memory.Kind == ESoulMemoryKind::BattleDefeat || Memory.Kind == ESoulMemoryKind::SiegeDefeat)
        {
            Bias -= (Strength * 800) / 1000;
        }
        else if (Memory.Kind == ESoulMemoryKind::BattleVictory || Memory.Kind == ESoulMemoryKind::SiegeVictory)
        {
            Bias += (Strength * 250) / 1000;
        }
    }
    return FMath::Clamp(Bias, -800, 300);
}

int32 FSoulMemoryRules::PlaceReclaimBonus(const FSoulCommanderState& Commander, FName PlaceId, int32 CurrentTurn)
{
    int32 Bonus = 0;
    for (const FSoulCommanderMemory& Memory : Commander.Memories)
    {
        if (Memory.Kind != ESoulMemoryKind::PlaceLoss || Memory.PlaceId != PlaceId)
        {
            continue;
        }
        const int32 Strength = Decay(Memory.IntensityPermille, CurrentTurn - Memory.Turn, 12);
        Bonus += (Strength * 500) / 1000;
    }
    return FMath::Clamp(Bonus, 0, 500);
}
