#include "RBAICreatureArchetype.h"

#include "RBAINativeTags.h"

namespace
{
    FRBAIConsiderationDefinition MakeConsideration(
        const FGameplayTag Signal, const float Min, const float Max,
        const ERBAIResponseCurve Curve, const float Weight = 1.0f)
    {
        FRBAIConsiderationDefinition Out;
        Out.SignalTag = Signal;
        Out.MinValue = Min;
        Out.MaxValue = Max;
        Out.Curve = Curve;
        Out.Weight = Weight;
        return Out;
    }

    FRBAIActionDefinition MakeAction(const FGameplayTag ActionTag, const int32 Group,
        const FGameplayTag RequiredContext)
    {
        FRBAIActionDefinition Out;
        Out.ActionTag = ActionTag;
        Out.PriorityGroup = Group;
        Out.BaseScore = 1.0f;
        Out.InertiaBonus = 0.08f;
        Out.bInterruptible = true;
        Out.RequiredTags.AddTag(RBAI::Tags::State_Creature);
        Out.RequiredTags.AddTag(RequiredContext);
        return Out;
    }
}

TArray<FRBAIActionDefinition> URBAICreatureArchetype::BuildStandardActions() const
{
    using namespace RBAI::Tags;
    TArray<FRBAIActionDefinition> Out;

    FRBAIActionDefinition Flee = MakeAction(Action_Flee, 0, Context_Hostile);
    Flee.Considerations.Add(MakeConsideration(Signal_Health, 0.0f,
        FMath::Max(FleeHealthThreshold, 0.01f), ERBAIResponseCurve::InverseLinear));
    Flee.Considerations.Add(MakeConsideration(Signal_Threat, 0.05f, 1.0f,
        ERBAIResponseCurve::Linear));
    Flee.Considerations.Add(MakeConsideration(Signal_PackSupport, 0.0f, 1.0f,
        ERBAIResponseCurve::InverseLinear, PackConfidenceWeight));
    Out.Add(Flee);

    FRBAIActionDefinition Attack = MakeAction(Action_Attack, 1, Context_Hostile);
    Attack.Considerations.Add(MakeConsideration(Signal_Health, AttackHealthFloor, 1.0f,
        ERBAIResponseCurve::Linear));
    Attack.Considerations.Add(MakeConsideration(Signal_Threat, 0.05f, 1.0f,
        ERBAIResponseCurve::Linear));
    Attack.Considerations.Add(MakeConsideration(Signal_Distance, 0.0f, 1.0f,
        ERBAIResponseCurve::InverseQuadratic));
    Out.Add(Attack);

    FRBAIActionDefinition Pursue = MakeAction(Action_Pursue, 1, Context_Hostile);
    Pursue.Considerations.Add(MakeConsideration(Signal_Health, AttackHealthFloor, 1.0f,
        ERBAIResponseCurve::Linear));
    Pursue.Considerations.Add(MakeConsideration(Signal_Threat, 0.05f, 1.0f,
        ERBAIResponseCurve::Linear));
    Pursue.Considerations.Add(MakeConsideration(Signal_Distance, 0.0f, 1.0f,
        ERBAIResponseCurve::Quadratic));
    Out.Add(Pursue);

    FRBAIActionDefinition Feed = MakeAction(Action_Feed, 2, Context_Self);
    Feed.Considerations.Add(MakeConsideration(Signal_Hunger, FeedHungerThreshold, 1.0f,
        ERBAIResponseCurve::Linear));
    Out.Add(Feed);

    FRBAIActionDefinition Rest = MakeAction(Action_Rest, 2, Context_Self);
    Rest.Considerations.Add(MakeConsideration(Signal_Fatigue, RestFatigueThreshold, 1.0f,
        ERBAIResponseCurve::Linear));
    Out.Add(Rest);

    FRBAIActionDefinition Idle = MakeAction(Action_Idle, 3, Context_Self);
    Idle.BaseScore = 0.25f;
    Idle.InertiaBonus = 0.15f;
    Out.Add(Idle);
    return Out;
}
