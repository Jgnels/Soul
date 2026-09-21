#include "Misc/AutomationTest.h"
#include "SoulHeroRecruitment.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulHeroRecruitmentVenueTest,
    "Soul.Core.HeroRecruitment.PhysicalVenueGatesHiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulHeroRecruitmentVenueTest::RunTest(const FString&)
{
    const FName TavernId = TEXT("human.tavern");
    const FName HeroId = TEXT("hero_wanderer");

    FSoulSettlementState Town;
    FSoulBuildingState Tavern;
    Tavern.Id = TavernId;
    Tavern.Level = 1;
    Tavern.IntegrityPermille = 1000;
    Tavern.Condition = ESoulBuildingCondition::Intact;
    Town.Buildings.Add(Tavern.Id, Tavern);

    FSoulCampaignEconomy Economy;
    Economy.Day = 5;
    Economy.Resources.Add(TEXT("gold"), 1000);

    FSoulHeroRecruitmentState Recruitment;
    FSoulHeroCandidate Candidate;
    Candidate.HeroId = HeroId;
    Candidate.VenueBuildingId = TavernId;
    Candidate.RecruitmentCost.Add(TEXT("gold"), 400);
    Candidate.MinimumReputation = 10;
    Candidate.AvailableFromDay = 3;
    Candidate.LastAvailableDay = 8;

    TestTrue(TEXT("candidate can be offered once"),
        FSoulHeroRecruitmentRules::OfferCandidate(Recruitment, Candidate));
    TestFalse(TEXT("candidate cannot be duplicated"),
        FSoulHeroRecruitmentRules::OfferCandidate(Recruitment, Candidate));

    TestEqual(TEXT("candidate appears physically at operational venue"),
        FSoulHeroRecruitmentRules::AvailableAtVenue(
            Recruitment, Town, TavernId, 5, 10).Num(), 1);

    TestFalse(TEXT("insufficient reputation blocks recruitment"),
        FSoulHeroRecruitmentRules::CanRecruit(
            Recruitment, Economy, Town, HeroId, 5, 9));

    FSoulSettlementRules::DamageBuilding(
        Town, TavernId, 1000, TEXT("tavern_burned"));
    TestEqual(TEXT("ruined venue has no present candidates"),
        FSoulHeroRecruitmentRules::AvailableAtVenue(
            Recruitment, Town, TavernId, 5, 10).Num(), 0);
    TestFalse(TEXT("ruined venue blocks hiring"),
        FSoulHeroRecruitmentRules::CanRecruit(
            Recruitment, Economy, Town, HeroId, 5, 10));

    TestTrue(TEXT("venue repair succeeds"),
        FSoulSettlementRules::RepairBuilding(Town, TavernId, 1000));

    FSoulHeroState Hero;
    TestTrue(TEXT("repaired venue allows in-world candidate recruitment"),
        FSoulHeroRecruitmentRules::Recruit(
            Recruitment, Economy, Town, HeroId, 5, 10, Hero));
    TestTrue(TEXT("recruited hero id is preserved"), Hero.HeroId == HeroId);
    TestEqual(TEXT("recruitment cost is atomic"), Economy.Resources[TEXT("gold")], 600);
    TestEqual(TEXT("recruited hero leaves venue"),
        FSoulHeroRecruitmentRules::AvailableAtVenue(
            Recruitment, Town, TavernId, 5, 10).Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulHeroRecruitmentExpiryTest,
    "Soul.Core.HeroRecruitment.CandidatesCanDepart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulHeroRecruitmentExpiryTest::RunTest(const FString&)
{
    FSoulHeroRecruitmentState Recruitment;
    FSoulHeroCandidate Candidate;
    Candidate.HeroId = TEXT("hero_transient");
    Candidate.VenueBuildingId = TEXT("human.tavern");
    Candidate.AvailableFromDay = 1;
    Candidate.LastAvailableDay = 4;

    TestTrue(TEXT("candidate offered"),
        FSoulHeroRecruitmentRules::OfferCandidate(Recruitment, Candidate));
    FSoulHeroRecruitmentRules::AdvanceDay(Recruitment, 5);

    const FSoulHeroCandidate* Stored =
        Recruitment.Candidates.Find(Candidate.HeroId);
    TestTrue(TEXT("expired candidate remains history but is departed"),
        Stored && Stored->State == ESoulHeroCandidateState::Departed);
    return true;
}

#endif
