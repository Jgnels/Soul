#include "Core/RBAIUtilityCore.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace RBAI::Core
{
    double NormalizeSignal(double Value, double MinValue, double MaxValue)
    {
        if (!std::isfinite(Value))
        {
            return 0.0;
        }
        if (MaxValue <= MinValue)
        {
            return Value >= MaxValue ? 1.0 : 0.0;
        }
        return std::clamp((Value - MinValue) / (MaxValue - MinValue), 0.0, 1.0);
    }

    double EvaluateCurve(double X, CurveType Curve)
    {
        X = std::clamp(X, 0.0, 1.0);
        switch (Curve)
        {
        case CurveType::Linear: return X;
        case CurveType::InverseLinear: return 1.0 - X;
        case CurveType::Quadratic: return X * X;
        case CurveType::InverseQuadratic:
        {
            const double Y = 1.0 - X;
            return Y * Y;
        }
        case CurveType::Step: return X >= 0.5 ? 1.0 : 0.0;
        }
        return 0.0;
    }

    double EvaluateConsideration(const Consideration& Def, const SignalMap& Signals)
    {
        const auto It = Signals.find(Def.Signal);
        if (It == Signals.end())
        {
            return 0.0;
        }
        const double Normalized = NormalizeSignal(It->second, Def.MinValue, Def.MaxValue);
        const double Curved = EvaluateCurve(Normalized, Def.Curve);
        const double Exponent = std::max(0.001, Def.Exponent);
        const double Weight = std::max(0.0, Def.Weight);
        if (Weight <= 1e-9)
        {
            return 1.0;
        }
        return std::clamp(std::pow(Curved, Exponent * Weight), 0.0, 1.0);
    }

    double EvaluateAction(const Action& Def, const Context& Candidate,
        const SelectionOptions& Options)
    {
        double Score = std::max(0.0, Def.BaseScore);
        for (const Consideration& Item : Def.Considerations)
        {
            Score *= EvaluateConsideration(Item, Candidate.Signals);
            if (Score <= 0.0)
            {
                return 0.0;
            }
        }

        if (Def.Id == Options.CurrentActionId && Candidate.Id == Options.CurrentContextId)
        {
            Score += std::max(0.0, Def.InertiaBonus);
        }
        return Score;
    }

    SelectionResult ChooseAction(const std::vector<Action>& Actions,
        const std::vector<Context>& Contexts, const SelectionOptions& Options)
    {
        SelectionResult Result;
        std::set<int> Groups;
        for (const Action& Item : Actions)
        {
            Groups.insert(Item.PriorityGroup);
        }

        const std::vector<Context> FallbackContexts = Contexts.empty()
            ? std::vector<Context>{{"Self", {}}}
            : Contexts;

        for (const int Group : Groups)
        {
            SelectionResult GroupBest;
            GroupBest.PriorityGroup = Group;
            for (const Action& ActionDef : Actions)
            {
                if (ActionDef.PriorityGroup != Group)
                {
                    continue;
                }
                for (const Context& Candidate : FallbackContexts)
                {
                    if (Candidate.BlockedActionIds.contains(ActionDef.Id))
                    {
                        continue;
                    }
                    ++Result.EvaluatedPairs;
                    const double Score = EvaluateAction(ActionDef, Candidate, Options);
                    if (Score + 1e-12 < Options.MinimumScore)
                    {
                        continue;
                    }

                    const bool Better = !GroupBest.Success || Score > GroupBest.Score + 1e-12;
                    const bool Tie = GroupBest.Success && std::abs(Score - GroupBest.Score) <= 1e-12;
                    const bool Lexical = Tie && (ActionDef.Id < GroupBest.ActionId ||
                        (ActionDef.Id == GroupBest.ActionId && Candidate.Id < GroupBest.ContextId));
                    if (Better || Lexical)
                    {
                        GroupBest.Success = true;
                        GroupBest.ActionId = ActionDef.Id;
                        GroupBest.ContextId = Candidate.Id;
                        GroupBest.Score = Score;
                    }
                }
            }

            if (GroupBest.Success)
            {
                GroupBest.EvaluatedPairs = Result.EvaluatedPairs;
                return GroupBest;
            }
        }

        Result.Success = false;
        return Result;
    }
}

