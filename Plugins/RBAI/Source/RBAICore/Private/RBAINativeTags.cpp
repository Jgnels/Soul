#include "RBAINativeTags.h"

namespace RBAI::Tags
{
    UE_DEFINE_GAMEPLAY_TAG(Action_Idle, "RBAI.Action.Idle");
    UE_DEFINE_GAMEPLAY_TAG(Action_Flee, "RBAI.Action.Flee");
    UE_DEFINE_GAMEPLAY_TAG(Action_Pursue, "RBAI.Action.Pursue");
    UE_DEFINE_GAMEPLAY_TAG(Action_Attack, "RBAI.Action.Attack");
    UE_DEFINE_GAMEPLAY_TAG(Action_Investigate, "RBAI.Action.Investigate");
    UE_DEFINE_GAMEPLAY_TAG(Action_Feed, "RBAI.Action.Feed");
    UE_DEFINE_GAMEPLAY_TAG(Action_Rest, "RBAI.Action.Rest");
    UE_DEFINE_GAMEPLAY_TAG(Action_Guard, "RBAI.Action.Guard");
    UE_DEFINE_GAMEPLAY_TAG(Action_Flank, "RBAI.Action.Flank");
    UE_DEFINE_GAMEPLAY_TAG(Action_TakeCover, "RBAI.Action.TakeCover");

    UE_DEFINE_GAMEPLAY_TAG(Signal_Health, "RBAI.Signal.HealthNormalized");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Threat, "RBAI.Signal.Threat");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Distance, "RBAI.Signal.DistanceNormalized");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Sight, "RBAI.Signal.Sight");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Hearing, "RBAI.Signal.Hearing");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Hunger, "RBAI.Signal.Hunger");
    UE_DEFINE_GAMEPLAY_TAG(Signal_Fatigue, "RBAI.Signal.Fatigue");
    UE_DEFINE_GAMEPLAY_TAG(Signal_TerritoryDistance, "RBAI.Signal.TerritoryDistance");
    UE_DEFINE_GAMEPLAY_TAG(Signal_PackSupport, "RBAI.Signal.PackSupport");

    UE_DEFINE_GAMEPLAY_TAG(State_Threatened, "RBAI.State.Threatened");
    UE_DEFINE_GAMEPLAY_TAG(State_InCombat, "RBAI.State.InCombat");
    UE_DEFINE_GAMEPLAY_TAG(State_Creature, "RBAI.State.Creature");
    UE_DEFINE_GAMEPLAY_TAG(State_Boss, "RBAI.State.Boss");
    UE_DEFINE_GAMEPLAY_TAG(Context_Self, "RBAI.Context.Self");
    UE_DEFINE_GAMEPLAY_TAG(Context_Hostile, "RBAI.Context.Hostile");
    UE_DEFINE_GAMEPLAY_TAG(Context_Friendly, "RBAI.Context.Friendly");
    UE_DEFINE_GAMEPLAY_TAG(Context_Perceived, "RBAI.Context.Perceived");
}
