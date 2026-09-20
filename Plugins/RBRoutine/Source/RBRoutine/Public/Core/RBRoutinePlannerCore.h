#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace RBRoutine::Core
{
    using FactValue = std::variant<bool, std::int64_t, double, std::string>;
    using WorldState = std::unordered_map<std::string, FactValue>;

    enum class CompareOp : std::uint8_t
    {
        Equal,
        NotEqual,
        Greater,
        GreaterEqual,
        Less,
        LessEqual
    };

    struct Condition
    {
        std::string Key;
        CompareOp Op = CompareOp::Equal;
        FactValue Value = false;
    };

    struct Effect
    {
        std::string Key;
        FactValue Value = false;
    };

    struct ActionSpec
    {
        std::string Id;
        std::vector<Condition> Preconditions;
        std::vector<Effect> Effects;
        double BaseCost = 1.0;
        bool Interruptible = true;
    };

    struct GoalSpec
    {
        std::string Id;
        std::vector<Condition> Eligibility;
        std::vector<Condition> Desired;
        double Priority = 0.0;
    };

    struct PlanOptions
    {
        std::size_t MaxDepth = 8;
        std::size_t MaxExpandedNodes = 4096;
    };

    struct PlanResult
    {
        bool Success = false;
        std::string GoalId;
        std::vector<std::string> ActionIds;
        double TotalCost = 0.0;
        std::size_t ExpandedNodes = 0;
        std::string FailureReason;
    };

    using DynamicCost = std::function<double(const ActionSpec&, const WorldState&)>;

    bool IsConditionSatisfied(const WorldState& State, const Condition& ConditionToTest);
    bool AreConditionsSatisfied(const WorldState& State, const std::vector<Condition>& Conditions);
    WorldState ApplyEffects(const WorldState& State, const std::vector<Effect>& Effects);
    std::string CanonicalStateKey(const WorldState& State);

    PlanResult BuildPlan(
        const WorldState& InitialState,
        const GoalSpec& Goal,
        const std::vector<ActionSpec>& Actions,
        const PlanOptions& Options = {},
        const DynamicCost& CostFunction = {});

    PlanResult ChooseGoalAndPlan(
        const WorldState& InitialState,
        const std::vector<GoalSpec>& Goals,
        const std::vector<ActionSpec>& Actions,
        const PlanOptions& Options = {},
        const DynamicCost& CostFunction = {});

    struct ScheduleEntry
    {
        std::string Id;
        std::uint8_t DayMask = 0x7F;
        int StartMinuteOfDay = 0;
        int EndMinuteOfDay = 1440;
        std::string PreferredGoalId;
        double PriorityBoost = 0.0;
    };

    bool IsScheduleEntryActive(const ScheduleEntry& Entry, std::int64_t DayIndex, int MinuteOfDay);
    double GetScheduleBoost(const std::vector<ScheduleEntry>& Entries, std::int64_t DayIndex, int MinuteOfDay, const std::string& GoalId);

    struct MemoryRecord
    {
        std::string EventId;
        std::string EventType;
        std::string OtherAgentId;
        double Salience = 0.0;
        double Valence = 0.0;
        std::int64_t WorldMinute = 0;
    };

    struct RelationshipState
    {
        std::string OtherAgentId;
        double Affinity = 0.0;
        double Trust = 0.0;
        double Fear = 0.0;
        std::int64_t LastChangedWorldMinute = 0;
    };

    struct ContinuityState
    {
        std::string AgentId;
        std::size_t MemoryCapacity = 128;
        std::vector<MemoryRecord> Memories;
        std::unordered_map<std::string, RelationshipState> Relationships;
    };

    bool RecordMemory(ContinuityState& State, const MemoryRecord& Record);
    void ApplyRelationshipDelta(ContinuityState& State, const std::string& OtherAgentId, double AffinityDelta, double TrustDelta, double FearDelta, std::int64_t WorldMinute);
}
