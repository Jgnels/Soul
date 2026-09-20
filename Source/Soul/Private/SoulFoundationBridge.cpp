#include "SoulFoundationBridge.h"

#include "RBFoundationSubsystem.h"
#include "Subsystems/SubsystemCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogSoulFoundation, Log, All);

void USoulFoundationBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<URBFoundationSubsystem>();

    Foundation = GetGameInstance() ? GetGameInstance()->GetSubsystem<URBFoundationSubsystem>() : nullptr;
    if (!Foundation)
    {
        UE_LOG(LogSoulFoundation, Error, TEXT("RB Foundation subsystem unavailable."));
        return;
    }

    FString Error;
    if (!Foundation->RegisterTimeSource(this, Error))
    {
        UE_LOG(LogSoulFoundation, Error, TEXT("Soul failed to register canonical time source: %s"), *Error);
    }

    const FName Domains[] = {
        TEXT("Soul.Campaign"),
        TEXT("Soul.TacticalSimulation"),
        TEXT("Soul.Regiment"),
        TEXT("Soul.CommanderMemory"),
        TEXT("Soul.StrategyAI"),
        TEXT("Soul.Siege"),
        TEXT("Soul.BattlefieldRecipe")
    };

    for (const FName Domain : Domains)
    {
        Error.Reset();
        if (!Foundation->RegisterAuthority(Domain, this, TEXT("0.1.0"), Error))
        {
            UE_LOG(LogSoulFoundation, Error, TEXT("Authority registration failed for %s: %s"), *Domain.ToString(), *Error);
            continue;
        }

        RegisteredAuthorities.Add(Domain);
        Error.Reset();
        if (!Foundation->RegisterCapability(Domain, Domain, this, TEXT("0.1.0"), true, Error))
        {
            UE_LOG(LogSoulFoundation, Error, TEXT("Capability registration failed for %s: %s"), *Domain.ToString(), *Error);
        }
    }

    Error.Reset();
    if (!Foundation->SynchronizeCanonicalTime(Error))
    {
        UE_LOG(LogSoulFoundation, Warning, TEXT("Initial canonical time synchronization failed: %s"), *Error);
    }
}

void USoulFoundationBridge::Deinitialize()
{
    if (Foundation)
    {
        Foundation->UnregisterCapabilitiesForOwner(this);
        for (const FName Domain : RegisteredAuthorities)
        {
            Foundation->UnregisterAuthority(Domain, this);
        }
        Foundation->UnregisterTimeSource(this);
    }

    RegisteredAuthorities.Reset();
    Foundation = nullptr;
    Super::Deinitialize();
}

bool USoulFoundationBridge::GetFoundationTimeState_Implementation(FRBFoundationTimeState& OutTime, FString& OutError) const
{
    OutTime.GameSeconds = static_cast<int64>(FMath::Max(0, CampaignDay - 1)) * 86400LL
        + static_cast<int64>(FMath::Clamp(CampaignMinuteOfDay, 0, 1439)) * 60LL;
    OutTime.DayIndex = FMath::Max(0, CampaignDay - 1);
    OutTime.MinuteOfDay = FMath::Clamp(CampaignMinuteOfDay, 0, 1439);
    OutTime.Season = CampaignSeason.IsNone() ? FName(TEXT("Default")) : CampaignSeason;
    OutTime.HourOfDay = static_cast<double>(OutTime.MinuteOfDay) / 60.0;
    OutError.Reset();
    return OutTime.IsValid(&OutError);
}

bool USoulFoundationBridge::SetCampaignTime(int32 DayIndex, int32 MinuteOfDay, FName Season, FString& OutError)
{
    CampaignDay = FMath::Max(1, DayIndex);
    CampaignMinuteOfDay = FMath::Clamp(MinuteOfDay, 0, 1439);
    CampaignSeason = Season.IsNone() ? FName(TEXT("Default")) : Season;

    if (!Foundation)
    {
        OutError = TEXT("RB Foundation subsystem unavailable.");
        return false;
    }
    return Foundation->SynchronizeCanonicalTime(OutError);
}
