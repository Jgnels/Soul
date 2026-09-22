#include "RBPBILLibrary.h"

#include "Core/TCATSubsystem.h"
#include "Query/CPP/TCATQueryBuilder.h"
#include "Query/TCATQueryTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
    UTCATSubsystem* ResolveTCAT(UObject* WorldContextObject)
    {
        if (!GEngine || !WorldContextObject)
        {
            return nullptr;
        }

        UWorld* World = GEngine->GetWorldFromContextObject(
            WorldContextObject,
            EGetWorldErrorMode::ReturnNull);
        return World ? World->GetSubsystem<UTCATSubsystem>() : nullptr;
    }
}
FName URBPBILLibrary::GetChannelTag(ERBPBILChannel Channel)
{
    return FRBPBILLayers::ToTag(Channel);
}

bool URBPBILLibrary::QueryBestLocationImmediate(
    UObject* WorldContextObject,
    const FRBPBILQueryRequest& Request,
    FRBPBILQueryResult& OutResult)
{
    OutResult = FRBPBILQueryResult();

    UTCATSubsystem* TCAT = ResolveTCAT(WorldContextObject);
    if (!TCAT)
    {
        return false;
    }

    FTCATQueryBuilder Query = TCAT->MakeQuery(
        FRBPBILLayers::ToTag(Request.Channel));
    Query.From(Request.Origin)
        .SearchRadius(FMath::Max(0.0f, Request.SearchRadius))
        .ReachableOnly(Request.bReachableOnly)
        .IgnoreHeight(Request.bIgnoreHeight);

    if (Request.bFindHighest)
    {
        Query.FindHighest();
    }
    else
    {
        Query.FindLowest();
    }

    FTCATSingleResult Raw;
    if (!Query.RunImmediate(Raw))
    {
        return false;
    }

    OutResult.bSuccess = true;
    OutResult.WorldLocation = Raw.WorldPos;
    OutResult.Value = Raw.Value;
    return true;
}
bool URBPBILLibrary::SampleChannelImmediate(
    UObject* WorldContextObject,
    ERBPBILChannel Channel,
    FVector WorldLocation,
    float& OutValue)
{
    OutValue = 0.0f;

    UTCATSubsystem* TCAT = ResolveTCAT(WorldContextObject);
    if (!TCAT)
    {
        return false;
    }

    FTCATSingleResult Raw;
    const bool bFound = TCAT->MakeQuery(FRBPBILLayers::ToTag(Channel))
        .GetValueAt(WorldLocation)
        .RunImmediate(Raw);

    if (bFound)
    {
        OutValue = Raw.Value;
    }
    return bFound;
}
