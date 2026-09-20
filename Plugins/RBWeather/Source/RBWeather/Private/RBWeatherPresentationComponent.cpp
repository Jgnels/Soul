#include "RBWeatherPresentationComponent.h"

#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "RBWeatherPlayerComponent.h"
#include "RBWeatherPresentationProfile.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

URBWeatherPresentationComponent::URBWeatherPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);

    static ConstructorHelpers::FObjectFinder<URBWeatherPresentationProfile> DefaultProfile(
        TEXT("/RBWeather/Presentation/DA_RBWeather_DefaultPresentation.DA_RBWeather_DefaultPresentation"));
    if (DefaultProfile.Succeeded())
    {
        Profile = DefaultProfile.Object;
    }
}

void URBWeatherPresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    BindWeatherSource();
    RefreshPresentationNow();
}

void URBWeatherPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (WeatherSource)
    {
        WeatherSource->OnWeatherUpdated.RemoveDynamic(this, &URBWeatherPresentationComponent::HandleWeatherUpdated);
        WeatherSource->OnLightning.RemoveDynamic(this, &URBWeatherPresentationComponent::HandleLightning);
    }
    StopPresentation();
    Super::EndPlay(EndPlayReason);
}
void URBWeatherPresentationComponent::BindWeatherSource()
{
    AActor* OwnerActor = GetOwner();
    if (!WeatherSource && OwnerActor)
    {
        WeatherSource = OwnerActor->FindComponentByClass<URBWeatherPlayerComponent>();
    }
    if (!WeatherSource) return;

    WeatherSource->OnWeatherUpdated.RemoveDynamic(this, &URBWeatherPresentationComponent::HandleWeatherUpdated);
    WeatherSource->OnLightning.RemoveDynamic(this, &URBWeatherPresentationComponent::HandleLightning);
    WeatherSource->OnWeatherUpdated.AddDynamic(this, &URBWeatherPresentationComponent::HandleWeatherUpdated);
    WeatherSource->OnLightning.AddDynamic(this, &URBWeatherPresentationComponent::HandleLightning);
}

bool URBWeatherPresentationComponent::RefreshPresentationNow()
{
    if (!WeatherSource) BindWeatherSource();
    if (!WeatherSource || !Profile) return false;
    if (!WeatherSource->RefreshWeatherNow()) return false;
    ApplySnapshot(WeatherSource->GetCurrentSnapshot());
    return true;
}

void URBWeatherPresentationComponent::HandleWeatherUpdated(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    ApplySnapshot(Snapshot);
}
void URBWeatherPresentationComponent::ApplySnapshot(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    if (!Profile) return;
    EnsurePrecipitation(Snapshot);
    EnsureWind(Snapshot);
    EnsureAudio(Snapshot);
}

UNiagaraComponent* URBWeatherPresentationComponent::CreateAttachedNiagara(
    UNiagaraSystem* System, float Scale, bool bAutoDestroy)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !System) return nullptr;
    UNiagaraComponent* Component = NewObject<UNiagaraComponent>(OwnerActor);
    if (!Component) return nullptr;

    Component->SetAsset(System);
    Component->SetAutoActivate(false);
    Component->SetAutoDestroy(bAutoDestroy);
    if (USceneComponent* Root = OwnerActor->GetRootComponent())
    {
        Component->SetupAttachment(Root);
        Component->SetRelativeLocation(RelativeEffectLocation);
    }
    Component->SetRelativeScale3D(FVector(FMath::Max(0.0f, Scale)));
    Component->RegisterComponent();
    Component->Activate(true);
    return Component;
}
void URBWeatherPresentationComponent::EnsurePrecipitation(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    const FRBWeatherPresentationSlot* Slot = Profile ? Profile->ResolveSlot(Snapshot) : nullptr;
    const float Intensity = static_cast<float>(FMath::Clamp(
        Snapshot.Environment.PrecipitationIntensity * Snapshot.PrecipitationVisualMultiplier, 0.0, 1.0));
    UNiagaraSystem* Desired = Slot ? Slot->NiagaraSystem.LoadSynchronous() : nullptr;
    if (Intensity < MinimumVisibleIntensity) Desired = nullptr;

    if (Desired != ActivePrecipitationSystem)
    {
        if (PrecipitationComponent)
        {
            PrecipitationComponent->Deactivate();
            PrecipitationComponent->DestroyComponent();
            PrecipitationComponent = nullptr;
        }
        ActivePrecipitationSystem = Desired;
        if (Desired)
        {
            PrecipitationComponent = CreateAttachedNiagara(Desired, Slot ? Slot->VisualScale : 1.0f, false);
        }
    }
    if (PrecipitationComponent)
    {
        ApplyNiagaraParameters(PrecipitationComponent, Snapshot, Intensity);
    }
}
void URBWeatherPresentationComponent::EnsureWind(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    const float EffectiveWind = static_cast<float>(FMath::Max(
        0.0, Snapshot.Environment.WindSpeedMps * Snapshot.LocalWindMultiplier));
    UNiagaraSystem* Desired = nullptr;
    if (Profile && EffectiveWind >= MinimumWindSpeedMps)
    {
        Desired = Profile->WindSystem.LoadSynchronous();
    }
    if (!Desired && WindComponent)
    {
        WindComponent->Deactivate();
        WindComponent->DestroyComponent();
        WindComponent = nullptr;
    }
    if (Desired && (!WindComponent || WindComponent->GetAsset() != Desired))
    {
        if (WindComponent) WindComponent->DestroyComponent();
        WindComponent = CreateAttachedNiagara(Desired, 1.0f, false);
    }
    if (WindComponent)
    {
        const float WindIntensity = FMath::Clamp(EffectiveWind / 20.0f, 0.0f, 1.0f);
        ApplyNiagaraParameters(WindComponent, Snapshot, WindIntensity);
    }
}
void URBWeatherPresentationComponent::EnsureAudio(const FRBWeatherSnapshotAtLocation& Snapshot)
{
    const FRBWeatherPresentationSlot* Slot = Profile ? Profile->ResolveSlot(Snapshot) : nullptr;
    USoundBase* Desired = Slot ? Slot->LoopingSound.LoadSynchronous() : nullptr;
    const float Volume = Slot ? static_cast<float>(FMath::Clamp(
        Snapshot.Environment.AudioIntensity * Snapshot.WeatherAudioMultiplier * Slot->AudioVolumeScale,
        0.0, 2.0)) : 0.0f;

    if (Desired != ActiveWeatherSound)
    {
        if (WeatherAudioComponent)
        {
            WeatherAudioComponent->Stop();
            WeatherAudioComponent->DestroyComponent();
            WeatherAudioComponent = nullptr;
        }
        ActiveWeatherSound = Desired;
        if (Desired && GetOwner())
        {
            WeatherAudioComponent = NewObject<UAudioComponent>(GetOwner());
            WeatherAudioComponent->SetSound(Desired);
            WeatherAudioComponent->SetAutoActivate(false);
            if (USceneComponent* Root = GetOwner()->GetRootComponent()) WeatherAudioComponent->SetupAttachment(Root);
            WeatherAudioComponent->RegisterComponent();
            WeatherAudioComponent->SetVolumeMultiplier(Volume);
            WeatherAudioComponent->Play();
        }
    }
    if (WeatherAudioComponent) WeatherAudioComponent->SetVolumeMultiplier(Volume);
}
void URBWeatherPresentationComponent::ApplyNiagaraParameters(
    UNiagaraComponent* Component, const FRBWeatherSnapshotAtLocation& Snapshot, float Intensity) const
{
    if (!Component || !Profile) return;
    const double WindScale = FMath::Max(0.0, Snapshot.LocalWindMultiplier);
    const double Speed = Snapshot.Environment.WindSpeedMps * WindScale;
    const double Gust = Snapshot.Environment.WindGustMps * WindScale;
    const double Radians = FMath::DegreesToRadians(Snapshot.Environment.WindDirectionDegrees);
    const FVector WindVelocity(FMath::Cos(Radians) * Speed, FMath::Sin(Radians) * Speed, 0.0);

    if (!Profile->IntensityParameter.IsNone()) Component->SetVariableFloat(Profile->IntensityParameter, Intensity);
    if (!Profile->WindVelocityParameter.IsNone()) Component->SetVariableVec3(Profile->WindVelocityParameter, WindVelocity);
    if (!Profile->WindSpeedParameter.IsNone()) Component->SetVariableFloat(Profile->WindSpeedParameter, static_cast<float>(Speed));
    if (!Profile->WindGustParameter.IsNone()) Component->SetVariableFloat(Profile->WindGustParameter, static_cast<float>(Gust));
    if (!Profile->ShelterParameter.IsNone()) Component->SetVariableFloat(Profile->ShelterParameter, Snapshot.bSheltered ? 1.0f : 0.0f);
    if (!Profile->TemperatureParameter.IsNone()) Component->SetVariableFloat(Profile->TemperatureParameter, static_cast<float>(Snapshot.LocalTemperatureC));
    if (!Profile->HumidityParameter.IsNone()) Component->SetVariableFloat(Profile->HumidityParameter, static_cast<float>(Snapshot.Environment.Humidity));
}
void URBWeatherPresentationComponent::HandleLightning(int64 LightningSerial)
{
    (void)LightningSerial;
    if (!Profile || !GetOwner()) return;
    const FRBWeatherSnapshotAtLocation Snapshot = WeatherSource ? WeatherSource->GetCurrentSnapshot() : FRBWeatherSnapshotAtLocation{};

    if (UNiagaraSystem* Lightning = Profile->LightningSystem.LoadSynchronous())
    {
        if (UNiagaraComponent* Component = CreateAttachedNiagara(Lightning, 1.0f, true))
        {
            ApplyNiagaraParameters(Component, Snapshot, 1.0f);
        }
    }
    if (USoundBase* Sound = Profile->LightningSound.LoadSynchronous())
    {
        UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
    }
}

void URBWeatherPresentationComponent::StopPresentation()
{
    if (PrecipitationComponent)
    {
        PrecipitationComponent->Deactivate();
        PrecipitationComponent->DestroyComponent();
    }
    if (WindComponent)
    {
        WindComponent->Deactivate();
        WindComponent->DestroyComponent();
    }
    if (WeatherAudioComponent)
    {
        WeatherAudioComponent->Stop();
        WeatherAudioComponent->DestroyComponent();
    }
    PrecipitationComponent = nullptr;
    WindComponent = nullptr;
    WeatherAudioComponent = nullptr;
    ActivePrecipitationSystem = nullptr;
    ActiveWeatherSound = nullptr;
}