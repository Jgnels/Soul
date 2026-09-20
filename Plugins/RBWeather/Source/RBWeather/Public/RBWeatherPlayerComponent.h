#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBWeatherTypes.h"
#include "RBWeatherPlayerComponent.generated.h"

class UMaterialParameterCollection;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBLocalWeatherUpdatedSignature, const FRBWeatherSnapshotAtLocation&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBLocalWeatherChangedSignature, FName, PreviousWeather, FName, NewWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBLocalLightningSignature, int64, LightningSerial);

/** Owning-client weather listener/presentation bridge, analogous to a player weather manager. */
UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBWEATHER_API URBWeatherPlayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBWeatherPlayerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.02"))
    float QueryIntervalSeconds = 0.10f;

    /** Bounded local roof/overhang query. It affects presentation only, never climate state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter")
    bool bEnableAutomaticShelter = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter", meta=(ClampMin="0.05"))
    float ShelterQueryIntervalSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter", meta=(ClampMin="100.0"))
    float ShelterTraceHeightCm = 20000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter")
    TEnumAsByte<ECollisionChannel> ShelterTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter|Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double ShelterPrecipitationMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter|Effects", meta=(ClampMin="0.0", ClampMax="1.0"))
    double ShelterLensMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter|Effects", meta=(ClampMin="0.0", ClampMax="2.0"))
    double ShelterAudioMultiplier = 0.35;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shelter|Effects", meta=(ClampMin="0.0", ClampMax="2.0"))
    double ShelterWindMultiplier = 0.15;

    /** Defaults to the built-in RB Weather MPC; may be replaced or cleared by a project. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<UMaterialParameterCollection> WeatherMPC;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBLocalWeatherUpdatedSignature OnWeatherUpdated;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBLocalWeatherChangedSignature OnWeatherChanged;

    UPROPERTY(BlueprintAssignable, Category="Weather")
    FRBLocalLightningSignature OnLightning;

    UFUNCTION(BlueprintCallable, Category="RB Weather")
    bool RefreshWeatherNow();

    UFUNCTION(BlueprintCallable, Category="RB Weather|Shelter")
    bool RefreshShelterNow();

    UFUNCTION(BlueprintPure, Category="RB Weather")
    FRBWeatherSnapshotAtLocation GetCurrentSnapshot() const { return CurrentSnapshot; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(Transient)
    FRBWeatherSnapshotAtLocation CurrentSnapshot;

    bool bHasSnapshot = false;
    double TimeAccumulator = 0.0;
    double ShelterAccumulator = 0.0;
    bool bShelterInitialized = false;
    bool bCachedSheltered = false;
    FName LastLightningSourceId = NAME_None;
    int64 LastLightningSerial = -1;
    void ApplyMaterialParameters(const FRBWeatherSnapshotAtLocation& Snapshot) const;
    void ApplyShelterToSnapshot(FRBWeatherSnapshotAtLocation& Snapshot) const;
    bool IsRelevantLocalOwner() const;
};
