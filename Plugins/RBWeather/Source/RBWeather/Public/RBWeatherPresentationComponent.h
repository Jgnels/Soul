#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBWeatherTypes.h"
#include "RBWeatherPresentationComponent.generated.h"

class UAudioComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class URBWeatherPlayerComponent;
class URBWeatherPresentationProfile;
class USoundBase;

/** Local-only bridge from RB Weather snapshots to built-in or project-supplied Niagara/audio art. */
UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBWEATHER_API URBWeatherPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBWeatherPresentationComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<URBWeatherPresentationProfile> Profile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<URBWeatherPlayerComponent> WeatherSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    FVector RelativeEffectLocation = FVector(0.0, 0.0, 250.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ClampMin="0.0"))
    float MinimumVisibleIntensity = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ClampMin="0.0"))
    float MinimumWindSpeedMps = 0.5f;

    UFUNCTION(BlueprintCallable, Category="RB Weather|Presentation")
    bool RefreshPresentationNow();

    UFUNCTION(BlueprintCallable, Category="RB Weather|Presentation")
    void StopPresentation();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> PrecipitationComponent;

    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> WindComponent;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WeatherAudioComponent;
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraSystem> ActivePrecipitationSystem;

    UPROPERTY(Transient)
    TObjectPtr<USoundBase> ActiveWeatherSound;

    UFUNCTION()
    void HandleWeatherUpdated(const FRBWeatherSnapshotAtLocation& Snapshot);

    UFUNCTION()
    void HandleLightning(int64 LightningSerial);

    void BindWeatherSource();
    void ApplySnapshot(const FRBWeatherSnapshotAtLocation& Snapshot);
    void EnsurePrecipitation(const FRBWeatherSnapshotAtLocation& Snapshot);
    void EnsureWind(const FRBWeatherSnapshotAtLocation& Snapshot);
    void EnsureAudio(const FRBWeatherSnapshotAtLocation& Snapshot);
    void ApplyNiagaraParameters(UNiagaraComponent* Component,
                                const FRBWeatherSnapshotAtLocation& Snapshot,
                                float Intensity) const;
    UNiagaraComponent* CreateAttachedNiagara(UNiagaraSystem* System, float Scale, bool bAutoDestroy);
};
