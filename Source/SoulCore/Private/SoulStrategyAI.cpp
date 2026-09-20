#include "SoulStrategyAI.h"

namespace
{
    void AddComponent(FSoulStrategyCandidate& Candidate, FName Name, int32 Value)
    {
        Candidate.Components.Add(Name, Value);
    }

    bool BetterCandidate(const FSoulStrategyCandidate& A, const FSoulStrategyCandidate& B)
    {
        if (A.TotalScore != B.TotalScore) return A.TotalScore > B.TotalScore;
        if (A.Action != B.Action) return static_cast<uint8>(A.Action) < static_cast<uint8>(B.Action);
        return A.TargetId.LexicalLess(B.TargetId);
    }
}

int32 FSoulStrategyAI::Sum(const TMap<FName, int32>& Components)
{
    int32 Total = 0;
    for (const TPair<FName, int32>& Pair : Components)
    {
        Total += Pair.Value;
    }
    return Total;
}

FSoulStrategyCandidate FSoulStrategyAI::Evaluate(ESoulStrategyAction Action, const FSoulStrategySnapshot& Snapshot)
{
    FSoulStrategyCandidate C;
    C.Action = Action;

    if (Action == ESoulStrategyAction::Hold)
    {
        C.bLegal = true;
        AddComponent(C, TEXT("urgency"), 50);
        AddComponent(C, TEXT("strategic_value"), 50);
        AddComponent(C, TEXT("feasibility"), 100);
        C.Reasons.Add(TEXT("safe_fallback"));
    }
    else if (Action == ESoulStrategyAction::Recover)
    {
        C.bLegal = Snapshot.Readiness < 700;
        if (C.bLegal)
        {
            AddComponent(C, TEXT("urgency"), 700 - Snapshot.Readiness);
            AddComponent(C, TEXT("strategic_value"), 100);
            AddComponent(C, TEXT("feasibility"), 900);
            C.Reasons.Add(TEXT("army_below_recovery_threshold"));
        }
        else C.Reasons.Add(TEXT("army_ready"));
    }
    else if (Action == ESoulStrategyAction::DefendOwnedRegion)
    {
        FSoulStrategicRegion Best;
        bool bFound = false;
        int32 BestScore = MIN_int32;
        for (const FSoulStrategicRegion& R : Snapshot.Regions)
        {
            if (R.OwnerFactionId != Snapshot.ActingFactionId || R.Threat <= 0 || !R.bAdjacent) continue;
            const int32 Score = R.Threat + R.Value + R.Feasibility - R.TravelCost;
            if (!bFound || Score > BestScore || (Score == BestScore && R.Id.LexicalLess(Best.Id)))
            {
                Best = R; BestScore = Score; bFound = true;
            }
        }
        C.bLegal = bFound;
        if (bFound)
        {
            C.TargetId = Best.Id;
            AddComponent(C, TEXT("urgency"), Best.Threat);
            AddComponent(C, TEXT("strategic_value"), Best.Value);
            AddComponent(C, TEXT("feasibility"), Best.Feasibility);
            AddComponent(C, TEXT("travel_cost"), -Best.TravelCost);
            C.Reasons.Add(TEXT("owned_region_threatened"));
        }
        else C.Reasons.Add(TEXT("no_threatened_owned_region"));
    }
    else if (Action == ESoulStrategyAction::CaptureResourceRegion)
    {
        FSoulStrategicRegion Best;
        bool bFound = false;
        int32 BestScore = MIN_int32;
        for (const FSoulStrategicRegion& R : Snapshot.Regions)
        {
            if (!R.bAdjacent || R.OwnerFactionId == Snapshot.ActingFactionId || R.ResourceValue <= 0) continue;
            const int32 Reclaim = FSoulMemoryRules::PlaceReclaimBonus(Snapshot.Commander, R.Id, Snapshot.Turn);
            const int32 Score = 250 + R.Value + R.ResourceValue + R.Feasibility - R.TravelCost - FMath::Max(0, 700 - Snapshot.Readiness) + Reclaim;
            if (!bFound || Score > BestScore || (Score == BestScore && R.Id.LexicalLess(Best.Id)))
            {
                Best = R; BestScore = Score; bFound = true;
            }
        }
        C.bLegal = bFound;
        if (bFound)
        {
            C.TargetId = Best.Id;
            const int32 Reclaim = FSoulMemoryRules::PlaceReclaimBonus(Snapshot.Commander, Best.Id, Snapshot.Turn);
            AddComponent(C, TEXT("urgency"), 250);
            AddComponent(C, TEXT("strategic_value"), Best.Value + Best.ResourceValue);
            AddComponent(C, TEXT("feasibility"), Best.Feasibility);
            AddComponent(C, TEXT("travel_cost"), -Best.TravelCost);
            AddComponent(C, TEXT("recovery_penalty"), -FMath::Max(0, 700 - Snapshot.Readiness));
            AddComponent(C, TEXT("place_reclaim"), Reclaim);
            C.Reasons.Add(TEXT("visible_resource_region"));
            C.Reasons.Add(TEXT("legal_capture_target"));
            if (Reclaim > 0) C.Reasons.Add(TEXT("reclaiming_previously_lost_region"));
        }
        else C.Reasons.Add(TEXT("no_legal_resource_region"));
    }
    else if (Action == ESoulStrategyAction::AttackVisibleArmy)
    {
        FSoulVisibleArmy Best;
        bool bFound = false;
        int32 BestScore = MIN_int32;
        for (const FSoulVisibleArmy& Army : Snapshot.VisibleEnemyArmies)
        {
            if (!Army.bAdjacent || Army.FactionId == Snapshot.ActingFactionId) continue;
            if (Snapshot.Strength * 100 < Army.Strength * 65) continue;
            const int32 Edge = FMath::Clamp((Snapshot.Strength - Army.Strength) / 10, -300, 300);
            const int32 Rival = FSoulMemoryRules::RivalBias(Snapshot.Commander, Army.CommanderId, Snapshot.Turn);
            const int32 Score = 700 + 500 + 800 + Edge - FMath::Max(0, 700 - Snapshot.Readiness) + Rival;
            if (!bFound || Score > BestScore || (Score == BestScore && Army.Id.LexicalLess(Best.Id)))
            {
                Best = Army; BestScore = Score; bFound = true;
            }
        }
        C.bLegal = bFound;
        if (bFound)
        {
            C.TargetId = Best.Id;
            const int32 Edge = FMath::Clamp((Snapshot.Strength - Best.Strength) / 10, -300, 300);
            const int32 Rival = FSoulMemoryRules::RivalBias(Snapshot.Commander, Best.CommanderId, Snapshot.Turn);
            AddComponent(C, TEXT("urgency"), 700);
            AddComponent(C, TEXT("strategic_value"), 500);
            AddComponent(C, TEXT("feasibility"), 800 + Edge);
            AddComponent(C, TEXT("recovery_penalty"), -FMath::Max(0, 700 - Snapshot.Readiness));
            AddComponent(C, TEXT("rival_history"), Rival);
            C.Reasons.Add(TEXT("visible_enemy_army"));
            C.Reasons.Add(TEXT("force_ratio_acceptable"));
            if (Rival < 0) C.Reasons.Add(TEXT("remembered_major_defeat_against_rival"));
            if (Rival > 0) C.Reasons.Add(TEXT("confidence_from_prior_victory"));
        }
        else C.Reasons.Add(TEXT("no_acceptable_adjacent_enemy_army"));
    }
    else if (Action == ESoulStrategyAction::SeizeExposedRegion)
    {
        FSoulStrategicRegion Best;
        bool bFound = false;
        int32 BestScore = MIN_int32;
        for (const FSoulStrategicRegion& R : Snapshot.Regions)
        {
            if (!R.bAdjacent || !R.bExposed || R.bOccupiedByPlayer || R.OwnerFactionId == Snapshot.ActingFactionId) continue;
            const int32 Score = 900 + R.Value + R.Feasibility - R.TravelCost;
            if (!bFound || Score > BestScore || (Score == BestScore && R.Id.LexicalLess(Best.Id)))
            {
                Best = R; BestScore = Score; bFound = true;
            }
        }
        C.bLegal = bFound;
        if (bFound)
        {
            C.TargetId = Best.Id;
            AddComponent(C, TEXT("urgency"), 900);
            AddComponent(C, TEXT("strategic_value"), Best.Value);
            AddComponent(C, TEXT("feasibility"), Best.Feasibility);
            AddComponent(C, TEXT("travel_cost"), -Best.TravelCost);
            AddComponent(C, TEXT("continuity"), 100);
            C.Reasons.Append({TEXT("exposed_region"), TEXT("unopposed_capture"), TEXT("high_value_opportunity")});
        }
        else C.Reasons.Add(TEXT("no_legal_exposed_region"));
    }
    else if (Action == ESoulStrategyAction::SiegeSettlement)
    {
        FSoulStrategicRegion Best;
        bool bFound = false;
        int32 BestScore = MIN_int32;
        for (const FSoulStrategicRegion& R : Snapshot.Regions)
        {
            if (!R.bAdjacent || !R.bSettlement || R.OwnerFactionId == Snapshot.ActingFactionId || Snapshot.Readiness < 600) continue;
            const int32 ForceEdge = FMath::Clamp((Snapshot.Strength - R.GarrisonStrength) / 10, -400, 400);
            if (Snapshot.Strength * 100 < FMath::Max(1, R.GarrisonStrength) * 70) continue;
            const int32 Score = 600 + R.Value + R.Feasibility + ForceEdge - R.TravelCost;
            if (!bFound || Score > BestScore || (Score == BestScore && R.Id.LexicalLess(Best.Id)))
            {
                Best = R; BestScore = Score; bFound = true;
            }
        }
        C.bLegal = bFound;
        if (bFound)
        {
            C.TargetId = Best.Id;
            AddComponent(C, TEXT("urgency"), 600);
            AddComponent(C, TEXT("strategic_value"), Best.Value);
            AddComponent(C, TEXT("feasibility"), Best.Feasibility + FMath::Clamp((Snapshot.Strength - Best.GarrisonStrength) / 10, -400, 400));
            AddComponent(C, TEXT("travel_cost"), -Best.TravelCost);
            C.Reasons.Append({TEXT("adjacent_enemy_settlement"), TEXT("siege_force_ratio_acceptable")});
        }
        else C.Reasons.Add(TEXT("no_viable_siege_target"));
    }

    C.TotalScore = C.bLegal ? Sum(C.Components) : 0;
    return C;
}

FSoulStrategyDecision FSoulStrategyAI::Choose(const FSoulStrategySnapshot& Snapshot)
{
    FSoulStrategyDecision Decision;
    const ESoulStrategyAction Actions[] = {
        ESoulStrategyAction::Hold,
        ESoulStrategyAction::Recover,
        ESoulStrategyAction::DefendOwnedRegion,
        ESoulStrategyAction::CaptureResourceRegion,
        ESoulStrategyAction::AttackVisibleArmy,
        ESoulStrategyAction::SeizeExposedRegion,
        ESoulStrategyAction::SiegeSettlement
    };

    for (ESoulStrategyAction Action : Actions)
    {
        Decision.Candidates.Add(Evaluate(Action, Snapshot));
    }

    TArray<FSoulStrategyCandidate> Legal;
    for (const FSoulStrategyCandidate& C : Decision.Candidates)
    {
        if (C.bLegal) Legal.Add(C);
    }
    Legal.Sort([](const FSoulStrategyCandidate& A, const FSoulStrategyCandidate& B) { return BetterCandidate(A, B); });

    if (Legal.IsEmpty())
    {
        return Decision;
    }

    Decision.bOk = true;
    Decision.Chosen = Legal[0];
    for (int32 Index = 1; Index < FMath::Min(4, Legal.Num()); ++Index)
    {
        Decision.Alternatives.Add(Legal[Index]);
    }
    return Decision;
}
