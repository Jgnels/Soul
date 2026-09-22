#include "RBPBILTypes.h"

FName FRBPBILLayers::ToTag(ERBPBILChannel Channel)
{
    switch (Channel)
    {
    case ERBPBILChannel::Threat: return TEXT("PBIL.Threat");
    case ERBPBILChannel::Congestion: return TEXT("PBIL.Congestion");
    case ERBPBILChannel::Objective: return TEXT("PBIL.Objective");
    case ERBPBILChannel::MagicHazard: return TEXT("PBIL.MagicHazard");
    case ERBPBILChannel::Retreat: return TEXT("PBIL.Retreat");
    case ERBPBILChannel::FlankOpportunity: return TEXT("PBIL.Flank");
    case ERBPBILChannel::FriendlySupport: return TEXT("PBIL.FriendlySupport");
    case ERBPBILChannel::TerrainValue: return TEXT("PBIL.TerrainValue");
    default: return NAME_None;
    }
}

void FRBPBILLayers::GetAll(TArray<FName>& OutTags)
{
    OutTags.Reset();
    OutTags.Reserve(8);
    OutTags.Add(ToTag(ERBPBILChannel::Threat));
    OutTags.Add(ToTag(ERBPBILChannel::Congestion));
    OutTags.Add(ToTag(ERBPBILChannel::Objective));
    OutTags.Add(ToTag(ERBPBILChannel::MagicHazard));
    OutTags.Add(ToTag(ERBPBILChannel::Retreat));
    OutTags.Add(ToTag(ERBPBILChannel::FlankOpportunity));
    OutTags.Add(ToTag(ERBPBILChannel::FriendlySupport));
    OutTags.Add(ToTag(ERBPBILChannel::TerrainValue));
}
