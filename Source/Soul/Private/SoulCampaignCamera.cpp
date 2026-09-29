#include "SoulCampaignCamera.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "SoulCampaignWorldActor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

ASoulCampaignCamera::ASoulCampaignCamera()
{
    PrimaryActorTick.bCanEverTick = true;
    auto* Camera = GetCameraComponent();
    Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
    Camera->SetFieldOfView(52.f);
    Camera->SetAspectRatio(16.f / 9.f);
    Camera->SetConstraintAspectRatio(false);
    Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
    Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
    Camera->PostProcessSettings.AutoExposureMinBrightness = 1.f;
    Camera->PostProcessSettings.AutoExposureMaxBrightness = 1.f;
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias = 0.f;
    Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
    Camera->PostProcessSettings.MotionBlurAmount = 0.f;
    Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
    Camera->PostProcessSettings.VignetteIntensity = .22f;
}
void ASoulCampaignCamera::BeginPlay() { Super::BeginPlay(); Tick(1.f); }
void ASoulCampaignCamera::Zoom(float Steps)
{
    TargetDistance = FMath::Clamp(TargetDistance * FMath::Pow(.85f, Steps), MinDistance, MaxDistance);
}
void ASoulCampaignCamera::Pan(FVector2D Direction, float DeltaSeconds)
{
    const FRotator Flat(0,Yaw,0);
    const FVector Forward = Flat.Vector();
    const FVector Right = FRotationMatrix(Flat).GetUnitAxis(EAxis::Y);
    TargetFocus += (Forward * Direction.Y + Right * Direction.X) * FMath::Min(DeltaSeconds,.1f) * TargetDistance * .22f;
    TargetFocus.X = FMath::Clamp(TargetFocus.X,-MaxFocusX,MaxFocusX);
    TargetFocus.Y = FMath::Clamp(TargetFocus.Y,-MaxFocusY,MaxFocusY);
}
void ASoulCampaignCamera::Orbit(float Direction, float DeltaSeconds)
{ TargetYaw = FMath::Clamp(TargetYaw + Direction * DeltaSeconds * 40.f, -135.f, -45.f); }
void ASoulCampaignCamera::Focus(FVector Location)
{ TargetFocus = FVector(Location.X,Location.Y,100); }
void ASoulCampaignCamera::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        FVector2D Input(0,0);
        Input.X = (PC->IsInputKeyDown(EKeys::D)?1.f:0.f) - (PC->IsInputKeyDown(EKeys::A)?1.f:0.f);
        Input.Y = (PC->IsInputKeyDown(EKeys::W)?1.f:0.f) - (PC->IsInputKeyDown(EKeys::S)?1.f:0.f);
        if (PC->IsInputKeyDown(EKeys::MiddleMouseButton))
        {
            float DX=0,DY=0; PC->GetInputMouseDelta(DX,DY);
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            const float UnitsPerPixel=2.f*Distance*FMath::Tan(FMath::DegreesToRadians(GetCameraComponent()->FieldOfView*.5f))/FMath::Max(Width,1);
            const FRotator Flat(0,Yaw,0);
            // Mouse deltas already represent displacement this frame: no delta-time factor.
            TargetFocus+=(-FRotationMatrix(Flat).GetUnitAxis(EAxis::Y)*DX+Flat.Vector()*DY/ FMath::Sin(FMath::DegreesToRadians(48.f)))*UnitsPerPixel;
        }
        Pan(Input.GetClampedToMaxSize(2.f),DeltaSeconds);
        Orbit((PC->IsInputKeyDown(EKeys::E)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::Q)?1.f:0.f),DeltaSeconds);
    }
    TargetFocus.Z=ASoulCampaignWorldActor::HeightAt(TargetFocus.X,TargetFocus.Y)+100.f;
    FocusPoint = FMath::VInterpTo(FocusPoint,TargetFocus,DeltaSeconds,7.f);
    Distance = FMath::FInterpTo(Distance,TargetDistance,DeltaSeconds,7.f);
    Yaw = FMath::FInterpTo(Yaw,TargetYaw,DeltaSeconds,7.f);
    const FRotator View(-48.f,Yaw,0);
    FVector Position = FocusPoint - View.Vector() * Distance;
    Position.Z = FMath::Max(Position.Z, ASoulCampaignWorldActor::HeightAt(Position.X,Position.Y) + 450.f);
    SetActorLocationAndRotation(Position,View);
}
