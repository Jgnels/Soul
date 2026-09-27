#include "SoulRealtimeBattleRules.h"
#include <cstdlib>
#include <iostream>

static void Check(bool Condition, const char* Detail)
{
    if (!Condition) { std::cerr << "FAIL: " << Detail << '\n'; std::exit(1); }
}
static FSoulRealtimeFormation Formation(const char* Id, const char* Side, int Count, int Cap)
{
    FSoulRealtimeFormation F;
    F.FormationId = Id; F.SideId = Side; F.StrategicCount = Count; F.MaxActiveRepresentations = Cap;
    return F;
}
int main()
{
    FSoulRealtimeBattleState Small;
    Small.MaxActivePerSide = 35;
    Small.Formations.Add(Formation("line", "P", 20, 4));
    FSoulRealtimeBattleRules::InitializeDeployment(Small);
    Check(Small.Formations[0].ActiveCount == 4, "small army exceeds formation cap");
    Check(Small.Formations[0].ReserveCount == 16, "small army loses reserves");
    Check(!FSoulRealtimeBattleRules::ShouldReinforce(Small, "P"), "full formations report ready");
    Check(FSoulRealtimeBattleRules::BuildAndApplyWave(Small, "P").TotalBodies() == 0, "full formation admits wave");
    FSoulRealtimeBattleRules::ApplyCasualties(Small, "line", 1);
    Check(FSoulRealtimeBattleRules::ShouldReinforce(Small, "P"), "vacant slot does not report ready");
    Check(FSoulRealtimeBattleRules::BuildAndApplyWave(Small, "P").TotalBodies() == 1, "vacant formation slot not refilled");
    Small.bReinforcementsEnabled = false;
    FSoulRealtimeBattleRules::InitializeDeployment(Small);
    Check(Small.Formations[0].ActiveCount == 19 && Small.Formations[0].ReserveCount == 0,
        "explicit lab all-at-once mode changed");

    for (int Cap : {15, 25, 35}) for (int Pool : {1, 14, 15, 30, 35, 70, 1000, 10000})
    {
        FSoulRealtimeBattleState B;
        B.MaxActivePerSide = Cap; B.MaxWaveSize = 4;
        B.Formations.Add(Formation("P", "P", Pool, Cap));
        B.Formations.Add(Formation("E", "E", Pool, Cap));
        FSoulRealtimeBattleRules::InitializeDeployment(B);
        Check(FSoulRealtimeBattleRules::ActiveBodies(B, "E") == std::min(Cap, Pool), "initial deployment count");
        int Lost = 0;
        for (int Step = 0; Step < Pool; ++Step)
        {
            Lost += FSoulRealtimeBattleRules::ApplyCasualties(B, "E", 1);
            const int BeforeActive = FSoulRealtimeBattleRules::ActiveBodies(B, "E");
            const int BeforeReserve = FSoulRealtimeBattleRules::ReserveBodies(B, "E");
            const auto Preview = FSoulRealtimeBattleRules::PreviewWave(B, "E");
            Check(FSoulRealtimeBattleRules::PreviewWave(B, "E").TotalBodies() == Preview.TotalBodies(), "repeated preview differs");
            Check(FSoulRealtimeBattleRules::ActiveBodies(B, "E") == BeforeActive &&
                FSoulRealtimeBattleRules::ReserveBodies(B, "E") == BeforeReserve, "preview mutates live ledger");
            {
                auto Discarded = B;
                Check(FSoulRealtimeBattleRules::BuildAndApplyWave(Discarded, "E").TotalBodies() == Preview.TotalBodies(),
                    "discarded candidate differs from preview");
            }
            const auto Wave = FSoulRealtimeBattleRules::BuildAndApplyWave(B, "E");
            const int Active = FSoulRealtimeBattleRules::ActiveBodies(B, "E");
            const int Reserve = FSoulRealtimeBattleRules::ReserveBodies(B, "E");
            Check(Wave.TotalBodies() <= 4, "oversized wave");
            Check(Wave.TotalBodies() == Preview.TotalBodies(), "displayed and applied waves differ");
            Check(BeforeReserve - Reserve == Wave.TotalBodies(), "wave reserve debit mismatch");
            Check(Active <= Cap, "active cap exceeded");
            Check(Lost + Active + Reserve == Pool, "casualty conservation");
            Check(B.Formations[1].StrategicCount == Active + Reserve, "formation ledger conservation");
        }
        Check(Lost == Pool && !FSoulRealtimeBattleRules::HasLivingForce(B, "E"), "stranded reserve");
        Check(FSoulRealtimeBattleRules::ActiveBodies(B, "P") + FSoulRealtimeBattleRules::ReserveBodies(B, "P") == Pool,
            "opposing force changed");
        Check(FSoulRealtimeBattleRules::BuildAndApplyWave(B, "E").TotalBodies() == 0, "ghost final wave");
    }

    FSoulRealtimeBattleState Normalized;
    Normalized.MaxActivePerSide = 0; Normalized.MaxWaveSize = 0;
    Normalized.Formations.Add(Formation("line", "P", 2, 1));
    FSoulRealtimeBattleRules::InitializeDeployment(Normalized);
    Check(Normalized.Formations[0].ActiveCount == 1, "normalized initial cap");
    FSoulRealtimeBattleRules::ApplyCasualties(Normalized, "line", 1);
    Check(FSoulRealtimeBattleRules::BuildAndApplyWave(Normalized, "P").TotalBodies() == 1,
        "normalized wave cap strands reserves");

    FSoulRealtimeBattleState Ordered;
    Ordered.MaxActivePerSide = 15;
    Ordered.Formations.Add(Formation("z", "P", 20, 6));
    Ordered.Formations.Add(Formation("a", "P", 20, 6));
    Ordered.Formations.Add(Formation("hero", "P", 1, 1));
    Ordered.Formations[2].Role = ESoulRealtimeFormationRole::Hero;
    auto Reversed = Ordered;
    std::reverse(Reversed.Formations.begin(), Reversed.Formations.end());
    FSoulRealtimeBattleRules::InitializeDeployment(Ordered);
    FSoulRealtimeBattleRules::InitializeDeployment(Reversed);
    for (const auto& A : Ordered.Formations) for (const auto& B : Reversed.Formations)
        if (A.FormationId == B.FormationId)
            Check(A.ActiveCount == B.ActiveCount && A.ReserveCount == B.ReserveCount, "input order changes deployment");
    std::cout << "PASS: production reinforcement rules; 24 pool/cap cases, preview/retry conservation, small formation caps, lab mode, stable ordering\n";
}
