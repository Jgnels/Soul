#pragma once

#include "CoreMinimal.h"
#include "SoulCampaign.h"
#include "SoulHero.h"
#include "SoulSettlement.h"

enum class ESoulHeroCandidateState : uint8
{
    Available,
    Recruited,
    Departed
};

struct FSoulHeroCandidate
{
    FName HeroId;
    ESoulHeroKind Kind = ESoulHeroKind::Hero;
    FName VenueBuildingId;
    TMap<FName, int32> RecruitmentCost;
    int32 MinimumReputation = 0;
    int32 AvailableFromDay = 1;
    // <= 0 means the candidate does not expire.
    int32 LastAvailableDay = 0;
    ESoulHeroCandidateState State = ESoulHeroCandidateState::Available;
};

struct FSoulHeroRecruitmentState
{
    TMap<FName, FSoulHeroCandidate> Candidates;
    TSet<FName> RecruitedHeroIds;
};

class SOULCORE_API FSoulHeroRecruitmentRules
{
public:
    static bool OfferCandidate(
        FSoulHeroRecruitmentState& State,
        const FSoulHeroCandidate& Candidate);

    static void AdvanceDay(
        FSoulHeroRecruitmentState& State,
        int32 CurrentDay);

    static TArray<FName> AvailableAtVenue(
        const FSoulHeroRecruitmentState& State,
        const FSoulSettlementState& Settlement,
        FName VenueBuildingId,
        int32 CurrentDay,
        int32 Reputation,
        int32 MinimumVenueIntegrityPermille = 500);

    static bool CanRecruit(
        const FSoulHeroRecruitmentState& State,
        const FSoulCampaignEconomy& Economy,
        const FSoulSettlementState& Settlement,
        FName HeroId,
        int32 CurrentDay,
        int32 Reputation,
        int32 MinimumVenueIntegrityPermille = 500);

    static bool Recruit(
        FSoulHeroRecruitmentState& State,
        FSoulCampaignEconomy& Economy,
        const FSoulSettlementState& Settlement,
        FName HeroId,
        int32 CurrentDay,
        int32 Reputation,
        FSoulHeroState& OutHero,
        int32 MinimumVenueIntegrityPermille = 500);
};
