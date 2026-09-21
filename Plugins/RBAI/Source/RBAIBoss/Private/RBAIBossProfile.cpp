#include "RBAIBossProfile.h"

int32 URBAIBossProfile::FindPhaseForHealth(const float Health01) const
{
    const float Clamped = FMath::Clamp(Health01, 0.0f, 1.0f);
    int32 BestIndex = INDEX_NONE;
    float BestThreshold = 2.0f;

    for (int32 Index = 0; Index < Phases.Num(); ++Index)
    {
        const FRBAIBossPhaseSpec& Phase = Phases[Index];
        const float Threshold = FMath::Clamp(Phase.EnterAtOrBelowHealth, 0.0f, 1.0f);
        if (Phase.PhaseTag.IsValid() && !Phase.Actions.IsEmpty() &&
            Clamped <= Threshold && Threshold < BestThreshold)
        {
            BestThreshold = Threshold;
            BestIndex = Index;
        }
    }
    return BestIndex;
}
