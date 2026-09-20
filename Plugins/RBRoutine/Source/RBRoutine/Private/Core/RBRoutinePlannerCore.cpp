#include "Core/RBRoutinePlannerCore.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <unordered_map>

namespace RBRoutine::Core
{
    namespace
    {
        std::optional<double> AsNumber(const FactValue& Value)
        {
            if (const auto* IntValue = std::get_if<std::int64_t>(&Value))
            {
                return static_cast<double>(*IntValue);
            }
            if (const auto* DoubleValue = std::get_if<double>(&Value))
            {
                return *DoubleValue;
            }
            return std::nullopt;
        }

        bool ValuesEqual(const FactValue& Left, const FactValue& Right)
        {
            const auto LeftNumber = AsNumber(Left);
            const auto RightNumber = AsNumber(Right);
            if (LeftNumber && RightNumber)
            {
                return std::abs(*LeftNumber - *RightNumber) <= 1e-9;
            }
            return Left == Right;
        }

        std::string ValueKey(const FactValue& Value)
        {
            std::ostringstream Stream;
            Stream << std::setprecision(17);
            if (const auto* BoolValue = std::get_if<bool>(&Value))
            {
                Stream << "b:" << (*BoolValue ? '1' : '0');
            }
            else if (const auto* IntValue = std::get_if<std::int64_t>(&Value))
            {
                Stream << "i:" << *IntValue;
            }
            else if (const auto* DoubleValue = std::get_if<double>(&Value))
            {
                Stream << "d:" << *DoubleValue;
            }
            else
            {
                const auto& StringValue = std::get<std::string>(Value);
                Stream << "s:" << StringValue.size() << ':' << StringValue;
            }
            return Stream.str();
        }

        struct SearchNode
        {
            WorldState State;
            std::vector<std::string> Plan;
            double Cost = 0.0;
            std::size_t Depth = 0;
            std::uint64_t Serial = 0;
        };

        struct SearchNodeCompare
        {
            bool operator()(const SearchNode& Left, const SearchNode& Right) const
            {
                if (std::abs(Left.Cost - Right.Cost) > 1e-9)
                {
                    return Left.Cost > Right.Cost;
                }
                return Left.Serial > Right.Serial;
            }
        };
    }

    bool IsConditionSatisfied(const WorldState& State, const Condition& ConditionToTest)
    {
        const auto It = State.find(ConditionToTest.Key);
        if (It == State.end())
        {
            return false;
        }

        const FactValue& Actual = It->second;
        switch (ConditionToTest.Op)
        {
        case CompareOp::Equal:
            return ValuesEqual(Actual, ConditionToTest.Value);
        case CompareOp::NotEqual:
            return !ValuesEqual(Actual, ConditionToTest.Value);
        case CompareOp::Greater:
        case CompareOp::GreaterEqual:
        case CompareOp::Less:
        case CompareOp::LessEqual:
        {
            const auto Left = AsNumber(Actual);
            const auto Right = AsNumber(ConditionToTest.Value);
            if (!Left || !Right)
            {
                return false;
            }
            if (ConditionToTest.Op == CompareOp::Greater) return *Left > *Right;
            if (ConditionToTest.Op == CompareOp::GreaterEqual) return *Left >= *Right;
            if (ConditionToTest.Op == CompareOp::Less) return *Left < *Right;
            return *Left <= *Right;
        }
        }
        return false;
    }

    bool AreConditionsSatisfied(const WorldState& State, const std::vector<Condition>& Conditions)
    {
        return std::all_of(Conditions.begin(), Conditions.end(), [&State](const Condition& Item)
        {
            return IsConditionSatisfied(State, Item);
        });
    }

    WorldState ApplyEffects(const WorldState& State, const std::vector<Effect>& Effects)
    {
        WorldState Result = State;
        for (const Effect& Item : Effects)
        {
            Result[Item.Key] = Item.Value;
        }
        return Result;
    }

    std::string CanonicalStateKey(const WorldState& State)
    {
        std::vector<std::string> Keys;
        Keys.reserve(State.size());
        for (const auto& [Key, Value] : State)
        {
            (void)Value;
            Keys.push_back(Key);
        }
        std::sort(Keys.begin(), Keys.end());

        std::ostringstream Stream;
        for (const std::string& Key : Keys)
        {
            Stream << Key.size() << ':' << Key << '=' << ValueKey(State.at(Key)) << ';';
        }
        return Stream.str();
    }

    PlanResult BuildPlan(
        const WorldState& InitialState,
        const GoalSpec& Goal,
        const std::vector<ActionSpec>& Actions,
        const PlanOptions& Options,
        const DynamicCost& CostFunction)
    {
        PlanResult Result;
        Result.GoalId = Goal.Id;

        if (!AreConditionsSatisfied(InitialState, Goal.Eligibility))
        {
            Result.FailureReason = "goal_ineligible";
            return Result;
        }

        if (AreConditionsSatisfied(InitialState, Goal.Desired))
        {
            Result.Success = true;
            return Result;
        }

        std::vector<const ActionSpec*> OrderedActions;
        OrderedActions.reserve(Actions.size());
        for (const ActionSpec& Action : Actions)
        {
            OrderedActions.push_back(&Action);
        }
        std::sort(OrderedActions.begin(), OrderedActions.end(), [](const ActionSpec* Left, const ActionSpec* Right)
        {
            return Left->Id < Right->Id;
        });

        std::priority_queue<SearchNode, std::vector<SearchNode>, SearchNodeCompare> Open;
        std::unordered_map<std::string, double> BestCostByState;
        std::uint64_t Serial = 0;

        Open.push(SearchNode{InitialState, {}, 0.0, 0, Serial++});
        BestCostByState.emplace(CanonicalStateKey(InitialState), 0.0);

        while (!Open.empty())
        {
            SearchNode Node = Open.top();
            Open.pop();

            ++Result.ExpandedNodes;
            if (Result.ExpandedNodes > Options.MaxExpandedNodes)
            {
                Result.FailureReason = "expanded_node_budget_exceeded";
                return Result;
            }

            if (AreConditionsSatisfied(Node.State, Goal.Desired))
            {
                Result.Success = true;
                Result.ActionIds = std::move(Node.Plan);
                Result.TotalCost = Node.Cost;
                return Result;
            }

            if (Node.Depth >= Options.MaxDepth)
            {
                continue;
            }

            for (const ActionSpec* Action : OrderedActions)
            {
                if (!AreConditionsSatisfied(Node.State, Action->Preconditions))
                {
                    continue;
                }

                WorldState NextState = ApplyEffects(Node.State, Action->Effects);
                const std::string NextKey = CanonicalStateKey(NextState);
                if (NextKey == CanonicalStateKey(Node.State))
                {
                    continue;
                }

                double ActionCost = Action->BaseCost;
                if (CostFunction)
                {
                    ActionCost += CostFunction(*Action, Node.State);
                }
                if (!std::isfinite(ActionCost))
                {
                    continue;
                }
                ActionCost = std::max(0.001, ActionCost);

                const double NextCost = Node.Cost + ActionCost;
                const auto PreviousBest = BestCostByState.find(NextKey);
                if (PreviousBest != BestCostByState.end() && PreviousBest->second <= NextCost + 1e-9)
                {
                    continue;
                }

                BestCostByState[NextKey] = NextCost;
                std::vector<std::string> NextPlan = Node.Plan;
                NextPlan.push_back(Action->Id);
                Open.push(SearchNode{std::move(NextState), std::move(NextPlan), NextCost, Node.Depth + 1, Serial++});
            }
        }

        Result.FailureReason = "no_plan";
        return Result;
    }

    PlanResult ChooseGoalAndPlan(
        const WorldState& InitialState,
        const std::vector<GoalSpec>& Goals,
        const std::vector<ActionSpec>& Actions,
        const PlanOptions& Options,
        const DynamicCost& CostFunction)
    {
        std::vector<const GoalSpec*> OrderedGoals;
        OrderedGoals.reserve(Goals.size());
        for (const GoalSpec& Goal : Goals)
        {
            if (AreConditionsSatisfied(InitialState, Goal.Eligibility) && !AreConditionsSatisfied(InitialState, Goal.Desired))
            {
                OrderedGoals.push_back(&Goal);
            }
        }

        std::sort(OrderedGoals.begin(), OrderedGoals.end(), [](const GoalSpec* Left, const GoalSpec* Right)
        {
            if (std::abs(Left->Priority - Right->Priority) > 1e-9)
            {
                return Left->Priority > Right->Priority;
            }
            return Left->Id < Right->Id;
        });

        std::size_t TotalExpanded = 0;
        for (const GoalSpec* Goal : OrderedGoals)
        {
            PlanResult Candidate = BuildPlan(InitialState, *Goal, Actions, Options, CostFunction);
            TotalExpanded += Candidate.ExpandedNodes;
            if (Candidate.Success)
            {
                Candidate.ExpandedNodes = TotalExpanded;
                return Candidate;
            }
        }

        PlanResult Failure;
        Failure.ExpandedNodes = TotalExpanded;
        Failure.FailureReason = OrderedGoals.empty() ? "no_eligible_goal" : "no_reachable_goal";
        return Failure;
    }

    bool IsScheduleEntryActive(const ScheduleEntry& Entry, std::int64_t DayIndex, int MinuteOfDay)
    {
        if (MinuteOfDay < 0 || MinuteOfDay >= 1440)
        {
            return false;
        }

        const int DayOfWeek = static_cast<int>((DayIndex % 7 + 7) % 7);
        if ((Entry.DayMask & (1u << DayOfWeek)) == 0)
        {
            return false;
        }

        const int Start = std::clamp(Entry.StartMinuteOfDay, 0, 1439);
        const int End = std::clamp(Entry.EndMinuteOfDay, 0, 1440);
        if (Start == End)
        {
            return true;
        }
        if (Start < End)
        {
            return MinuteOfDay >= Start && MinuteOfDay < End;
        }
        return MinuteOfDay >= Start || MinuteOfDay < End;
    }

    double GetScheduleBoost(const std::vector<ScheduleEntry>& Entries, std::int64_t DayIndex, int MinuteOfDay, const std::string& GoalId)
    {
        double Boost = 0.0;
        for (const ScheduleEntry& Entry : Entries)
        {
            if (Entry.PreferredGoalId == GoalId && IsScheduleEntryActive(Entry, DayIndex, MinuteOfDay))
            {
                Boost = std::max(Boost, Entry.PriorityBoost);
            }
        }
        return Boost;
    }

    bool RecordMemory(ContinuityState& State, const MemoryRecord& Record)
    {
        if (Record.EventId.empty())
        {
            return false;
        }
        const auto Duplicate = std::find_if(State.Memories.begin(), State.Memories.end(), [&Record](const MemoryRecord& Existing)
        {
            return Existing.EventId == Record.EventId;
        });
        if (Duplicate != State.Memories.end())
        {
            return false;
        }

        if (State.MemoryCapacity == 0)
        {
            return true;
        }

        State.Memories.push_back(Record);
        while (State.Memories.size() > State.MemoryCapacity)
        {
            auto Evict = std::min_element(State.Memories.begin(), State.Memories.end(), [](const MemoryRecord& Left, const MemoryRecord& Right)
            {
                if (std::abs(Left.Salience - Right.Salience) > 1e-9)
                {
                    return Left.Salience < Right.Salience;
                }
                return Left.WorldMinute < Right.WorldMinute;
            });
            State.Memories.erase(Evict);
        }
        return true;
    }

    void ApplyRelationshipDelta(
        ContinuityState& State,
        const std::string& OtherAgentId,
        double AffinityDelta,
        double TrustDelta,
        double FearDelta,
        std::int64_t WorldMinute)
    {
        if (OtherAgentId.empty())
        {
            return;
        }

        RelationshipState& Relationship = State.Relationships[OtherAgentId];
        Relationship.OtherAgentId = OtherAgentId;
        Relationship.Affinity = std::clamp(Relationship.Affinity + AffinityDelta, -1.0, 1.0);
        Relationship.Trust = std::clamp(Relationship.Trust + TrustDelta, -1.0, 1.0);
        Relationship.Fear = std::clamp(Relationship.Fear + FearDelta, 0.0, 1.0);
        Relationship.LastChangedWorldMinute = WorldMinute;
    }
}
