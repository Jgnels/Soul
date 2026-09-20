#include "SoulHex.h"

namespace
{
    const FSoulHex Directions[6] = {
        FSoulHex(1, 0), FSoulHex(1, -1), FSoulHex(0, -1),
        FSoulHex(-1, 0), FSoulHex(-1, 1), FSoulHex(0, 1)
    };

    FSoulHex Add(const FSoulHex& A, const FSoulHex& B)
    {
        return FSoulHex(A.Q + B.Q, A.R + B.R);
    }

    FSoulHex Direction(int32 Facing)
    {
        const int32 Index = ((Facing % 6) + 6) % 6;
        return Directions[Index];
    }
}

int32 FSoulHexRules::Distance(const FSoulHex& A, const FSoulHex& B)
{
    const int32 DQ = A.Q - B.Q;
    const int32 DR = A.R - B.R;
    const int32 DS = (-A.Q - A.R) - (-B.Q - B.R);
    return (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DS)) / 2;
}

TArray<FSoulHex> FSoulHexRules::Neighbors(const FSoulHex& Hex)
{
    TArray<FSoulHex> Result;
    Result.Reserve(6);
    for (const FSoulHex& D : Directions)
    {
        Result.Add(Add(Hex, D));
    }
    return Result;
}

TArray<FSoulHex> FSoulHexRules::Footprint(const FSoulHex& Anchor, int32 FootprintSize, int32 Facing)
{
    TArray<FSoulHex> Result;
    Result.Add(Anchor);
    if (FootprintSize >= 2)
    {
        Result.Add(Add(Anchor, Direction(Facing)));
    }
    if (FootprintSize >= 3)
    {
        Result.Add(Add(Anchor, Direction(Facing + 1)));
    }
    return Result;
}

bool FSoulHexRules::CanOccupy(const FSoulBattlefield& Board, const FSoulHex& Anchor, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied)
{
    for (const FSoulHex& Hex : Footprint(Anchor, FootprintSize, Facing))
    {
        const FSoulHexCell* Cell = Board.Cells.Find(Hex);
        if (!Cell || Occupied.Contains(Hex))
        {
            return false;
        }
        if (Mode == ESoulMovementMode::Ground)
        {
            if (Cell->Terrain == ESoulTerrain::DeepWater || Cell->bBlocksLargeUnits && FootprintSize > 1)
            {
                return false;
            }
        }
    }
    return true;
}

int32 FSoulHexRules::EnterCost(const FSoulHexCell& Cell, ESoulMovementMode Mode)
{
    if (Mode == ESoulMovementMode::Flying)
    {
        return 100;
    }
    if (Cell.Terrain == ESoulTerrain::DeepWater)
    {
        return MAX_int32;
    }
    return FMath::Max(1, Cell.MovementCost);
}

TMap<FSoulHex, int32> FSoulHexRules::Reachable(const FSoulBattlefield& Board, const FSoulHex& Start, int32 Budget, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied)
{
    TMap<FSoulHex, int32> Best;
    TArray<FSoulHex> Open;
    Best.Add(Start, 0);
    Open.Add(Start);

    while (!Open.IsEmpty())
    {
        int32 BestIndex = 0;
        int32 BestCost = Best[Open[0]];
        for (int32 Index = 1; Index < Open.Num(); ++Index)
        {
            const int32 Cost = Best[Open[Index]];
            if (Cost < BestCost || (Cost == BestCost && Open[Index] < Open[BestIndex]))
            {
                BestIndex = Index;
                BestCost = Cost;
            }
        }

        const FSoulHex Current = Open[BestIndex];
        Open.RemoveAt(BestIndex);

        for (const FSoulHex& Next : Neighbors(Current))
        {
            if (!CanOccupy(Board, Next, FootprintSize, Facing, Mode, Occupied))
            {
                continue;
            }
            const FSoulHexCell* Cell = Board.Cells.Find(Next);
            const int32 Step = Cell ? EnterCost(*Cell, Mode) : MAX_int32;
            if (Step == MAX_int32)
            {
                continue;
            }
            const int32 NewCost = BestCost + Step;
            if (NewCost > Budget)
            {
                continue;
            }
            int32* Existing = Best.Find(Next);
            if (!Existing || NewCost < *Existing)
            {
                Best.Add(Next, NewCost);
                Open.AddUnique(Next);
            }
        }
    }
    return Best;
}

TArray<FSoulHex> FSoulHexRules::ShortestPath(const FSoulBattlefield& Board, const FSoulHex& Start, const FSoulHex& Goal, int32 FootprintSize, int32 Facing, ESoulMovementMode Mode, const TSet<FSoulHex>& Occupied)
{
    TMap<FSoulHex, int32> Best;
    TMap<FSoulHex, FSoulHex> Previous;
    TArray<FSoulHex> Open;
    Best.Add(Start, 0);
    Open.Add(Start);

    while (!Open.IsEmpty())
    {
        int32 BestIndex = 0;
        int32 BestScore = Best[Open[0]];
        for (int32 Index = 1; Index < Open.Num(); ++Index)
        {
            const int32 Score = Best[Open[Index]];
            if (Score < BestScore || (Score == BestScore && Open[Index] < Open[BestIndex]))
            {
                BestIndex = Index;
                BestScore = Score;
            }
        }
        const FSoulHex Current = Open[BestIndex];
        Open.RemoveAt(BestIndex);
        if (Current == Goal)
        {
            break;
        }

        for (const FSoulHex& Next : Neighbors(Current))
        {
            if (!CanOccupy(Board, Next, FootprintSize, Facing, Mode, Occupied))
            {
                continue;
            }
            const FSoulHexCell* Cell = Board.Cells.Find(Next);
            const int32 Step = Cell ? EnterCost(*Cell, Mode) : MAX_int32;
            if (Step == MAX_int32)
            {
                continue;
            }
            const int32 NewCost = BestScore + Step;
            int32* Existing = Best.Find(Next);
            if (!Existing || NewCost < *Existing)
            {
                Best.Add(Next, NewCost);
                Previous.Add(Next, Current);
                Open.AddUnique(Next);
            }
        }
    }

    if (!Best.Contains(Goal))
    {
        return {};
    }

    TArray<FSoulHex> Result;
    FSoulHex Current = Goal;
    Result.Add(Current);
    while (Current != Start)
    {
        const FSoulHex* Prev = Previous.Find(Current);
        if (!Prev)
        {
            return {};
        }
        Current = *Prev;
        Result.Add(Current);
    }
    Algo::Reverse(Result);
    return Result;
}

TArray<FSoulHex> FSoulHexRules::Radius(const FSoulBattlefield& Board, const FSoulHex& Center, int32 RadiusValue)
{
    TArray<FSoulHex> Result;
    for (const TPair<FSoulHex, FSoulHexCell>& Pair : Board.Cells)
    {
        if (Distance(Center, Pair.Key) <= RadiusValue)
        {
            Result.Add(Pair.Key);
        }
    }
    Result.Sort();
    return Result;
}

namespace
{
    struct FCubeFloat
    {
        double X = 0.0;
        double Y = 0.0;
        double Z = 0.0;
    };

    FSoulHex CubeRound(const FCubeFloat& Cube)
    {
        int32 X = FMath::RoundToInt(Cube.X);
        int32 Y = FMath::RoundToInt(Cube.Y);
        int32 Z = FMath::RoundToInt(Cube.Z);

        const double DX = FMath::Abs(static_cast<double>(X) - Cube.X);
        const double DY = FMath::Abs(static_cast<double>(Y) - Cube.Y);
        const double DZ = FMath::Abs(static_cast<double>(Z) - Cube.Z);

        if (DX > DY && DX > DZ) X = -Y - Z;
        else if (DY > DZ) Y = -X - Z;
        else Z = -X - Y;

        return FSoulHex(X, Z);
    }
}

TArray<FSoulHex> FSoulHexRules::Line(const FSoulHex& Start, const FSoulHex& End)
{
    const int32 N = Distance(Start, End);
    if (N <= 0)
    {
        return {Start};
    }

    const FCubeFloat A{static_cast<double>(Start.Q), static_cast<double>(-Start.Q - Start.R), static_cast<double>(Start.R)};
    const FCubeFloat B{static_cast<double>(End.Q), static_cast<double>(-End.Q - End.R), static_cast<double>(End.R)};

    TArray<FSoulHex> Result;
    Result.Reserve(N + 1);
    for (int32 Index = 0; Index <= N; ++Index)
    {
        const double T = static_cast<double>(Index) / static_cast<double>(N);
        FCubeFloat L;
        L.X = A.X + (B.X - A.X) * T;
        L.Y = A.Y + (B.Y - A.Y) * T;
        L.Z = A.Z + (B.Z - A.Z) * T;
        Result.AddUnique(CubeRound(L));
    }
    return Result;
}

bool FSoulHexRules::HasLineOfSight(const FSoulBattlefield& Board, const FSoulHex& Start, const FSoulHex& End)
{
    const TArray<FSoulHex> Cells = Line(Start, End);
    for (int32 Index = 1; Index + 1 < Cells.Num(); ++Index)
    {
        const FSoulHexCell* Cell = Board.Cells.Find(Cells[Index]);
        if (!Cell || Cell->bBlocksLineOfSight)
        {
            return false;
        }
    }
    return Board.Cells.Contains(Start) && Board.Cells.Contains(End);
}
