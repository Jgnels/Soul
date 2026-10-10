#include "SoulSettlementPresentationController.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "SoulFortificationSegmentActor.h"
#include "SoulSettlementBuildingActor.h"
#include "SoulSettlementStateSubsystem.h"

ASoulSettlementPresentationController::ASoulSettlementPresentationController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = .5f;
}

void ASoulSettlementPresentationController::BeginPlay()
{
    Super::BeginPlay();
    SetActorTickEnabled(bAuthoredEV100Exposure);
    NormalizeAuthoredExposure();
    RefreshSettlementPresentation();
}

void ASoulSettlementPresentationController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    NormalizeAuthoredExposure(); // Includes authored sublevels that load later.
}

void ASoulSettlementPresentationController::NormalizeAuthoredExposure()
{
    if (!bAuthoredEV100Exposure || !GetWorld()) return;
    const auto* Extended = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange"));
    if (Extended && Extended->GetInt() != 0) return;
    const auto* Attenuation = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptation.LensAttenuation"));
    const float LuminanceMax = .78f / FMath::Max(Attenuation ? Attenuation->GetFloat() : .78f, .01f);
    const FName AppliedTag(TEXT("Soul.Runtime.AuthoredExposureNormalized"));
    for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
    {
        if (It->ActorHasTag(AppliedTag)) continue;
        auto& Settings = It->Settings;
        if (Settings.bOverride_AutoExposureMinBrightness)
            Settings.AutoExposureMinBrightness = LuminanceMax * FMath::Pow(2.f, Settings.AutoExposureMinBrightness);
        if (Settings.bOverride_AutoExposureMaxBrightness)
            Settings.AutoExposureMaxBrightness = LuminanceMax * FMath::Pow(2.f, Settings.AutoExposureMaxBrightness);
        It->Tags.Add(AppliedTag);
        UE_LOG(LogTemp, Display, TEXT("SOUL_AUTHORED_EXPOSURE_NORMALIZED volume=%s min=%g max=%g"),
            *It->GetName(), Settings.AutoExposureMinBrightness, Settings.AutoExposureMaxBrightness);
    }
}

USoulSettlementStateSubsystem*
ASoulSettlementPresentationController::ResolveState() const
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance
        ? GameInstance->GetSubsystem<USoulSettlementStateSubsystem>()
        : nullptr;
}

bool ASoulSettlementPresentationController::RefreshSettlementPresentation()
{
    if (SettlementId.IsNone() || !GetWorld())
    {
        return false;
    }

    USoulSettlementStateSubsystem* State = ResolveState();
    if (!State || !State->HasSettlement(SettlementId))
    {
        return false;
    }

    for (TActorIterator<ASoulSettlementBuildingActor> It(GetWorld()); It; ++It)
    {
        ASoulSettlementBuildingActor* BuildingActor = *It;
        if (!BuildingActor || BuildingActor->SettlementId != SettlementId)
        {
            continue;
        }

        const FName Condition = State->GetBuildingConditionName(
            SettlementId, BuildingActor->BuildingId);
        const int32 Integrity = State->GetBuildingIntegrity(
            SettlementId, BuildingActor->BuildingId);

        if (Condition.IsNone())
        {
            BuildingActor->ApplyConditionName(TEXT("Unbuilt"));
        }
        else if (Integrity >= 0 && BuildingActor->MinimumBuildingLevel<=1)
        {
            BuildingActor->ApplyIntegrity(
                Integrity,
                Condition != TEXT("Unbuilt"),
                Condition == TEXT("Building") || Condition == TEXT("Repairing"));
        }
        else
        {
            BuildingActor->ApplyConditionName(Condition);
        }
    }

    const int32 WallIntegrity = State->GetWallIntegrity(SettlementId);
    for (TActorIterator<ASoulFortificationSegmentActor> It(GetWorld()); It; ++It)
    {
        ASoulFortificationSegmentActor* Segment = *It;
        if (!Segment || Segment->SettlementId != SettlementId)
        {
            continue;
        }

        const bool bPersistentlyBreached =
            !Segment->BreachScarId.IsNone()
            && State->HasSettlementScar(SettlementId, Segment->BreachScarId);

        Segment->ApplyWallState(
            bPersistentlyBreached ? 0 : FMath::Max(0, WallIntegrity),
            false);
    }

    return true;
}
