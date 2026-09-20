#include "RBRoutineTypes.h"
#include "RBRoutineNativeTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_RBRoutine_Activity_Social, "RBRoutine.Activity.Social");
UE_DEFINE_GAMEPLAY_TAG(TAG_RBRoutine_Event_Perception_Saw, "RBRoutine.Event.Perception.Saw");
UE_DEFINE_GAMEPLAY_TAG(TAG_RBRoutine_Event_Perception_Heard, "RBRoutine.Event.Perception.Heard");
UE_DEFINE_GAMEPLAY_TAG(TAG_RBRoutine_Event_Perception_Threat, "RBRoutine.Event.Perception.Threat");
UE_DEFINE_GAMEPLAY_TAG(TAG_RBRoutine_Event_Perception_Lost, "RBRoutine.Event.Perception.Lost");

FRBRoutineFactValue FRBRoutineFactValue::FromBool(bool Value)
{
    FRBRoutineFactValue Result;
    Result.Type = ERBRoutineFactType::Bool;
    Result.BoolValue = Value;
    return Result;
}

FRBRoutineFactValue FRBRoutineFactValue::FromInteger(int64 Value)
{
    FRBRoutineFactValue Result;
    Result.Type = ERBRoutineFactType::Integer;
    Result.IntegerValue = Value;
    return Result;
}

FRBRoutineFactValue FRBRoutineFactValue::FromNumber(double Value)
{
    FRBRoutineFactValue Result;
    Result.Type = ERBRoutineFactType::Number;
    Result.NumberValue = Value;
    return Result;
}

FRBRoutineFactValue FRBRoutineFactValue::FromName(FName Value)
{
    FRBRoutineFactValue Result;
    Result.Type = ERBRoutineFactType::Name;
    Result.NameValue = Value;
    return Result;
}
