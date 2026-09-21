#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RBAI::Core
{
    enum class CurveType
    {
        Linear,
        InverseLinear,
        Quadratic,
        InverseQuadratic,
        Step
    };

    struct Consideration
    {
        std::string Signal;
        double MinValue = 0.0;
        double MaxValue = 1.0;
        CurveType Curve = CurveType::Linear;
        double Exponent = 1.0;
        double Weight = 1.0;
    };
    using SignalMap = std::unordered_map<std::string, double>;

    struct Context
    {
        std::string Id;
        SignalMap Signals;
        std::unordered_set<std::string> BlockedActionIds;
    };

    struct Action
    {
        std::string Id;
        int PriorityGroup = 0;
        double BaseScore = 1.0;
        double InertiaBonus = 0.0;
        bool Interruptible = true;
        std::vector<Consideration> Considerations;
    };

    struct SelectionOptions
    {
        std::string CurrentActionId;
        std::string CurrentContextId;
        double MinimumScore = 1e-6;
    };

    struct SelectionResult
    {
        bool Success = false;
        std::string ActionId;
        std::string ContextId;
        double Score = 0.0;
        std::size_t EvaluatedPairs = 0;
        int PriorityGroup = 0;
    };

    RBAICORE_API double NormalizeSignal(double Value, double MinValue, double MaxValue);
    RBAICORE_API double EvaluateCurve(double NormalizedValue, CurveType Curve);
    RBAICORE_API double EvaluateConsideration(const Consideration& Definition, const SignalMap& Signals);
    RBAICORE_API double EvaluateAction(const Action& Definition, const Context& Candidate,
        const SelectionOptions& Options);

    RBAICORE_API SelectionResult ChooseAction(
        const std::vector<Action>& Actions,
        const std::vector<Context>& Contexts,
        const SelectionOptions& Options = {});
}

