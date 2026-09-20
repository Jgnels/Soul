#include "RBWeatherPresentationProfile.h"

const FRBWeatherPresentationSlot* URBWeatherPresentationProfile::ResolveSlot(
    const FRBWeatherSnapshotAtLocation& Snapshot) const
{
    for (const FRBWeatherPresentationOverride& Override : WeatherOverrides)
    {
        if (!Override.WeatherId.IsNone() && Override.WeatherId == Snapshot.CurrentWeather)
        {
            return &Override.Slot;
        }
    }

    switch (Snapshot.Environment.PrecipitationType)
    {
    case ERBWeatherPrecipitation::Rain: return &Rain;
    case ERBWeatherPrecipitation::Snow: return &Snow;
    case ERBWeatherPrecipitation::Sleet: return &Sleet;
    case ERBWeatherPrecipitation::Hail: return &Hail;
    case ERBWeatherPrecipitation::Dust: return &Dust;
    case ERBWeatherPrecipitation::None:
    default:
        return nullptr;
    }
}
