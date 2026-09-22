#include "SoulRealtimeBattleRules.h"

int32 FSoulReinforcementWave::TotalBodies() const
{
    int32 Total = 0;
    for (const TPair<FName, int32>& Pair : FormationCounts)
    {
        Total += FMath::Max(0, Pair.Value);
    }
    return Total;
}

int32 FSoulTerrainMagicProfile::MultiplierFor(FName SchoolId) const
{
    if (const int32* Value = SchoolPowerPermille.Find(SchoolId))
    {
        return FMath::Max(0, *Value);
    }
    return 1000;
}

namespace
{
    int32 RolePriority(ESoulRealtimeFormationRole Role)
    {
        switch (Role)
        {
            case ESoulRealtimeFormationRole::Hero: return 2000;
            case ESoulRealtimeFormationRole::Apex: return 1500;
            case ESoulRealtimeFormationRole::Support: return 200;
            default: return 0;
        }
    }
}
void FSoulRealtimeBattleRules::InitializeDeployment(FSoulRealtimeBattleState& Battle)
{
    const int32 Cap = FMath::Max(1, Battle.MaxActivePerSide);
    TSet<FName> Sides;

    for (FSoulRealtimeFormation& Formation : Battle.Formations)
    {
        Formation.StrategicCount = FMath::Max(0, Formation.StrategicCount);
        Formation.ActiveCount = 0;
        Formation.ReserveCount = Formation.StrategicCount;
        if (!Formation.SideId.IsNone())
        {
            Sides.Add(Formation.SideId);
        }
    }

    for (FName SideId : Sides)
    {
        TArray<int32> Indices;
        int32 Total = 0;
        for (int32 Index = 0; Index < Battle.Formations.Num(); ++Index)
        {
            const FSoulRealtimeFormation& F = Battle.Formations[Index];
            if (F.SideId == SideId && F.StrategicCount > 0)
            {
                Indices.Add(Index);
                Total += F.StrategicCount;
            }
        }
        if (!Battle.bReinforcementsEnabled || Total <= Cap)
        {
            for (int32 Index : Indices)
            {
                FSoulRealtimeFormation& F = Battle.Formations[Index];
                F.ActiveCount = F.StrategicCount;
                F.ReserveCount = 0;
            }
            continue;
        }

        Indices.Sort([&Battle](int32 A, int32 B)
        {
            const FSoulRealtimeFormation& FA = Battle.Formations[A];
            const FSoulRealtimeFormation& FB = Battle.Formations[B];
            const int32 PA = FA.ReinforcementPriority + RolePriority(FA.Role);
            const int32 PB = FB.ReinforcementPriority + RolePriority(FB.Role);
            if (PA != PB) return PA > PB;
            return FA.FormationId.LexicalLess(FB.FormationId);
        });

        int32 Slots = Cap;
        for (int32 Index : Indices)
        {
            if (Slots <= 0) break;
            FSoulRealtimeFormation& F = Battle.Formations[Index];
            ++F.ActiveCount;
            --F.ReserveCount;
            --Slots;
        }
        while (Slots > 0)
        {
            bool bAllocated = false;
            for (int32 Index : Indices)
            {
                FSoulRealtimeFormation& F = Battle.Formations[Index];
                if (F.ReserveCount <= 0 || F.ActiveCount >= FMath::Max(1, F.MaxActiveRepresentations)) continue;
                ++F.ActiveCount;
                --F.ReserveCount;
                --Slots;
                bAllocated = true;
                if (Slots <= 0) break;
            }
            if (!bAllocated) break;
        }
    }
}

int32 FSoulRealtimeBattleRules::ActiveBodies(
    const FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    int32 Total = 0;
    for (const FSoulRealtimeFormation& F : Battle.Formations)
    {
        if (F.SideId == SideId) Total += FMath::Max(0, F.ActiveCount);
    }
    return Total;
}
int32 FSoulRealtimeBattleRules::ReserveBodies(
    const FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    int32 Total = 0;
    for (const FSoulRealtimeFormation& F : Battle.Formations)
    {
        if (F.SideId == SideId) Total += FMath::Max(0, F.ReserveCount);
    }
    return Total;
}

int32 FSoulRealtimeBattleRules::ActivePower(
    const FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    int32 Total = 0;
    for (const FSoulRealtimeFormation& F : Battle.Formations)
    {
        if (F.SideId == SideId)
        {
            Total += FMath::Max(0, F.ActiveCount) * FMath::Max(0, F.PowerPerBody);
        }
    }
    return Total;
}
bool FSoulRealtimeBattleRules::HasLivingForce(
    const FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    return ActiveBodies(Battle, SideId) + ReserveBodies(Battle, SideId) > 0;
}

int32 FSoulRealtimeBattleRules::ApplyCasualties(
    FSoulRealtimeBattleState& Battle,
    FName FormationId,
    int32 Casualties)
{
    if (Casualties <= 0) return 0;
    for (FSoulRealtimeFormation& F : Battle.Formations)
    {
        if (F.FormationId != FormationId) continue;
        const int32 Applied = FMath::Min(F.ActiveCount, Casualties);
        F.ActiveCount -= Applied;
        F.StrategicCount = FMath::Max(0, F.StrategicCount - Applied);
        return Applied;
    }
    return 0;
}
bool FSoulRealtimeBattleRules::ShouldReinforce(
    const FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    if (!Battle.bReinforcementsEnabled || ReserveBodies(Battle, SideId) <= 0)
    {
        return false;
    }

    const int32 Cap = FMath::Max(1, Battle.MaxActivePerSide);
    const int32 Threshold = FMath::Clamp(
        Battle.ReinforcementTriggerPermille, 1, 1000);
    return ActiveBodies(Battle, SideId) * 1000 < Cap * Threshold;
}

FSoulReinforcementWave FSoulRealtimeBattleRules::BuildAndApplyWave(
    FSoulRealtimeBattleState& Battle,
    FName SideId)
{
    FSoulReinforcementWave Wave;
    Wave.SideId = SideId;
    if (!ShouldReinforce(Battle, SideId)) return Wave;
    const int32 Active = ActiveBodies(Battle, SideId);
    int32 Slots = FMath::Min(
        FMath::Max(0, Battle.MaxActivePerSide - Active),
        FMath::Max(1, Battle.MaxWaveSize));
    if (Slots <= 0) return Wave;

    TArray<int32> Indices;
    for (int32 Index = 0; Index < Battle.Formations.Num(); ++Index)
    {
        const FSoulRealtimeFormation& F = Battle.Formations[Index];
        if (F.SideId == SideId && F.ReserveCount > 0)
        {
            Indices.Add(Index);
        }
    }

    Indices.Sort([&Battle](int32 A, int32 B)
    {
        const FSoulRealtimeFormation& FA = Battle.Formations[A];
        const FSoulRealtimeFormation& FB = Battle.Formations[B];
        const int32 EmptyA = FA.ActiveCount == 0 ? 10000 : 0;
        const int32 EmptyB = FB.ActiveCount == 0 ? 10000 : 0;
        const int32 PA = EmptyA + FA.ReinforcementPriority + RolePriority(FA.Role);
        const int32 PB = EmptyB + FB.ReinforcementPriority + RolePriority(FB.Role);
        return PA != PB ? PA > PB : FA.FormationId.LexicalLess(FB.FormationId);
    });
    while (Slots > 0)
    {
        bool bAllocated = false;
        for (int32 Index : Indices)
        {
            FSoulRealtimeFormation& F = Battle.Formations[Index];
            if (F.ReserveCount <= 0 || F.ActiveCount >= FMath::Max(1, F.MaxActiveRepresentations)) continue;
            ++F.ActiveCount;
            --F.ReserveCount;
            Wave.FormationCounts.FindOrAdd(F.FormationId) += 1;
            --Slots;
            bAllocated = true;
            if (Slots <= 0) break;
        }
        if (!bAllocated) break;
    }
    return Wave;
}

FSoulTerrainMagicProfile FSoulRealtimeBattleRules::MakeVolcanicMagicProfile()
{
    FSoulTerrainMagicProfile Profile;
    Profile.ProfileId = TEXT("TerrainMagic.Volcanic");
    Profile.SchoolPowerPermille.Add(TEXT("Magic.School.Fire"), 1100);
    Profile.SchoolPowerPermille.Add(TEXT("Magic.School.Ice"), 900);
    Profile.SchoolPowerPermille.Add(TEXT("Magic.School.Water"), 950);
    return Profile;
}
