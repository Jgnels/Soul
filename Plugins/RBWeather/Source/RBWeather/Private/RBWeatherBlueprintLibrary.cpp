#include "RBWeatherBlueprintLibrary.h"

#include "Engine/World.h"

#include "Engine/Engine.h"
#include "RBWeatherDirector.h"
#include "RBWeatherSubsystem.h"

bool URBWeatherBlueprintLibrary::GetWeatherAtLocation(const UObject* WorldContextObject, FVector WorldLocation,
                                                      FRBWeatherSnapshotAtLocation& OutSnapshot)
{
    OutSnapshot = {};
    if (!GEngine || !WorldContextObject) return false;
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (!World) return false;
    if (const URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
    {
        return System->GetWeatherAtLocation(WorldLocation, OutSnapshot);
    }
    return false;
}

ARBWeatherDirector* URBWeatherBlueprintLibrary::GetWeatherDirector(const UObject* WorldContextObject)
{
    if (!GEngine || !WorldContextObject) return nullptr;
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (!World) return nullptr;
    if (const URBWeatherSubsystem* System = World->GetSubsystem<URBWeatherSubsystem>())
    {
        return System->GetDirector();
    }
    return nullptr;
}
