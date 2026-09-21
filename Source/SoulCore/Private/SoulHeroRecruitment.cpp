#include "SoulHeroRecruitment.h"

namespace
{
    bool IsInAvailabilityWindow(
        const FSoulHeroCandidate& Candidate,
        int32 CurrentDay)
    {
        if (CurrentDay < Candidate.AvailableFromDay)
        {
            return false;
        }
        return Candidate.LastAvailableDay <= 0
            || CurrentDay <= Candidate.LastAvailableDay;
    }

    bool IsVenueOperational(
        const FSoulSettlementState& Settlement,
        FName VenueBuildingId,
        int32 MinimumIntegrityPermille)
    {
        return VenueBuildingId.IsNone()
            || FSoulSettlementRules::IsOperational(
                Settlement,
                VenueBuildingId,
                MinimumIntegrityPermille);
    }
}

bool FSoulHeroRecruitmentRules::OfferCandidate(
    FSoulHeroRecruitmentState& State,
    const FSoulHeroCandidate& Candidate)
{
    if (Candidate.HeroId.IsNone()
        || State.Candidates.Contains(Candidate.HeroId)
        || State.RecruitedHeroIds.Contains(Candidate.HeroId))
    {
        return false;
    }

    State.Candidates.Add(Candidate.HeroId, Candidate);
    return true;
}

void FSoulHeroRecruitmentRules::AdvanceDay(
    FSoulHeroRecruitmentState& State,
    int32 CurrentDay)
{
    for (TPair<FName, FSoulHeroCandidate>& Pair : State.Candidates)
    {
        FSoulHeroCandidate& Candidate = Pair.Value;
        if (Candidate.State != ESoulHeroCandidateState::Available)
        {
            continue;
        }
        if (Candidate.LastAvailableDay > 0
            && CurrentDay > Candidate.LastAvailableDay)
        {
            Candidate.State = ESoulHeroCandidateState::Departed;
        }
    }
}

TArray<FName> FSoulHeroRecruitmentRules::AvailableAtVenue(
    const FSoulHeroRecruitmentState& State,
    const FSoulSettlementState& Settlement,
    FName VenueBuildingId,
    int32 CurrentDay,
    int32 Reputation,
    int32 MinimumVenueIntegrityPermille)
{
    TArray<FName> Result;
    if (!IsVenueOperational(
            Settlement,
            VenueBuildingId,
            MinimumVenueIntegrityPermille))
    {
        return Result;
    }

    for (const TPair<FName, FSoulHeroCandidate>& Pair : State.Candidates)
    {
        const FSoulHeroCandidate& Candidate = Pair.Value;
        if (Candidate.State != ESoulHeroCandidateState::Available
            || Candidate.VenueBuildingId != VenueBuildingId
            || Reputation < Candidate.MinimumReputation
            || !IsInAvailabilityWindow(Candidate, CurrentDay))
        {
            continue;
        }
        Result.Add(Candidate.HeroId);
    }

    Result.Sort(FNameLexicalLess());
    return Result;
}

bool FSoulHeroRecruitmentRules::CanRecruit(
    const FSoulHeroRecruitmentState& State,
    const FSoulCampaignEconomy& Economy,
    const FSoulSettlementState& Settlement,
    FName HeroId,
    int32 CurrentDay,
    int32 Reputation,
    int32 MinimumVenueIntegrityPermille)
{
    const FSoulHeroCandidate* Candidate = State.Candidates.Find(HeroId);
    if (!Candidate
        || Candidate->State != ESoulHeroCandidateState::Available
        || State.RecruitedHeroIds.Contains(HeroId)
        || Reputation < Candidate->MinimumReputation
        || !IsInAvailabilityWindow(*Candidate, CurrentDay)
        || !IsVenueOperational(
            Settlement,
            Candidate->VenueBuildingId,
            MinimumVenueIntegrityPermille))
    {
        return false;
    }

    return FSoulCampaignRules::CanAfford(
        Economy,
        Candidate->RecruitmentCost,
        1);
}

bool FSoulHeroRecruitmentRules::Recruit(
    FSoulHeroRecruitmentState& State,
    FSoulCampaignEconomy& Economy,
    const FSoulSettlementState& Settlement,
    FName HeroId,
    int32 CurrentDay,
    int32 Reputation,
    FSoulHeroState& OutHero,
    int32 MinimumVenueIntegrityPermille)
{
    if (!CanRecruit(
            State,
            Economy,
            Settlement,
            HeroId,
            CurrentDay,
            Reputation,
            MinimumVenueIntegrityPermille))
    {
        return false;
    }

    FSoulHeroCandidate& Candidate = State.Candidates.FindChecked(HeroId);
    for (const TPair<FName, int32>& Pair : Candidate.RecruitmentCost)
    {
        Economy.Resources.FindOrAdd(Pair.Key) -= FMath::Max(0, Pair.Value);
    }

    Candidate.State = ESoulHeroCandidateState::Recruited;
    State.RecruitedHeroIds.Add(HeroId);

    OutHero = FSoulHeroState();
    OutHero.HeroId = HeroId;
    return true;
}
