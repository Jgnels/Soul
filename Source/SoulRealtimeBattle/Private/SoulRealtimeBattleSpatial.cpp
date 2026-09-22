#include "SoulRealtimeBattleSpatial.h"

namespace
{
    FSoulSpatialPreference Pref(
        ERBPBILChannel Channel, float Weight)
    {
        FSoulSpatialPreference Out;
        Out.Channel = Channel;
        Out.Weight = Weight;
        return Out;
    }
}

const FSoulSpatialPreference*
FSoulFormationSpatialPolicy::StrongestPreference() const
{
    const FSoulSpatialPreference* Best = nullptr;
    for (const FSoulSpatialPreference& Preference : Preferences)
    {
        if (!Best ||
            FMath::Abs(Preference.Weight) >
            FMath::Abs(Best->Weight))
        {
            Best = &Preference;
        }
    }
    return Best;
}
float FSoulFormationSpatialPolicy::WeightFor(
    ERBPBILChannel Channel) const
{
    for (const FSoulSpatialPreference& Preference : Preferences)
    {
        if (Preference.Channel == Channel)
        {
            return Preference.Weight;
        }
    }
    return 0.0f;
}

FSoulFormationSpatialPolicy
FSoulRealtimeBattleSpatial::MakePolicy(
    ESoulRealtimeFormationRole Role)
{
    FSoulFormationSpatialPolicy Policy;
    Policy.Preferences.Add(
        Pref(ERBPBILChannel::MagicHazard, -0.75f));
    switch (Role)
    {
        case ESoulRealtimeFormationRole::Shock:
            Policy.SearchRadius = 2200.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FlankOpportunity, 1.40f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Congestion, -0.80f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Objective, 0.65f));
            break;

        case ESoulRealtimeFormationRole::Ranged:
            Policy.SearchRadius = 2400.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Threat, -1.35f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::TerrainValue, 1.15f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Congestion, -0.90f));
            break;
        case ESoulRealtimeFormationRole::Support:
            Policy.SearchRadius = 1900.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FriendlySupport, 1.40f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Threat, -1.25f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Congestion, -0.70f));
            break;

        case ESoulRealtimeFormationRole::Apex:
            Policy.SearchRadius = 2100.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Objective, 1.20f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FlankOpportunity, 0.90f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Congestion, -0.15f));
            break;
        case ESoulRealtimeFormationRole::Hero:
            Policy.SearchRadius = 2000.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Objective, 1.10f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FriendlySupport, 0.85f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FlankOpportunity, 0.65f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Threat, -0.55f));
            break;

        case ESoulRealtimeFormationRole::Line:
        default:
            Policy.SearchRadius = 1800.0f;
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Objective, 1.05f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::FriendlySupport, 0.90f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Threat, -0.80f));
            Policy.Preferences.Add(Pref(
                ERBPBILChannel::Congestion, -0.55f));
            break;
    }
    return Policy;
}
FRBPBILQueryRequest
FSoulRealtimeBattleSpatial::MakePrimaryQuery(
    ESoulRealtimeFormationRole Role,
    const FVector& Origin)
{
    const FSoulFormationSpatialPolicy Policy =
        MakePolicy(Role);
    const FSoulSpatialPreference* Preference =
        Policy.StrongestPreference();

    FRBPBILQueryRequest Request;
    Request.Origin = Origin;
    Request.SearchRadius = Policy.SearchRadius;
    Request.bReachableOnly = Policy.bReachableOnly;

    if (Preference)
    {
        Request.Channel = Preference->Channel;
        Request.bFindHighest = Preference->Weight >= 0.0f;
    }
    return Request;
}
