#include "SoulCampaignCamera.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "SoulCampaignWorldActor.h"
#include "SoulCampaignTerrain.h"
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
void ASoulCampaignCamera::BeginPlay() { Super::BeginPlay(); if(SoulCampaignTerrain::EvilCorridor())Distance=TargetDistance=4000.f; Tick(1.f); }
float ASoulCampaignCamera::GetMinimumDistance() const {return SoulCampaignTerrain::EvilCorridor()?900.f:MinDistance;}
float ASoulCampaignCamera::GetMaximumDistance() const {return SoulCampaignTerrain::Mesa()?9000.f:MaxDistance;}
FBox2D ASoulCampaignCamera::FocusRange(float ViewDistance,float ViewYaw,float FocusHeight) const
{
    const auto Bounds=SoulCampaignTerrain::FocusBounds()/SoulCampaignTerrain::Scale();
    if(!SoulCampaignTerrain::Mesa())return FBox2D(-Bounds,Bounds);
    const float Pitch=FMath::Lerp(38.f,48.f,FMath::Clamp((ViewDistance-MinDistance)/3500.f,0.f,1.f));
    const FRotator Rotation(-Pitch,ViewYaw,0);
    const FVector Forward=Rotation.Vector(),Right=FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y),Up=FRotationMatrix(Rotation).GetUnitAxis(EAxis::Z);
    const float Tan=FMath::Tan(FMath::DegreesToRadians(GetCameraComponent()->FieldOfView*.5f));
    int32 Width=1920,Height=1080;
    if(auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr)PC->GetViewportSize(Width,Height);
    const float Aspect=Width>0&&Height>0?static_cast<float>(Width)/Height:16.f/9.f;
    const FVector Position=-Forward*ViewDistance+FVector(0,0,FMath::Max(FocusHeight,0.f));
    FBox2D Footprint(ForceInit);
    for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})
    {
        const FVector Ray=Forward+Right*Tan*X+Up*Tan/Aspect*Y;
        const FVector Point=Position-Ray*(Position.Z/Ray.Z);
        Footprint+=FVector2D(Point.X,Point.Y);
    }
    // The water plane and Landscape end at 750 m. Keep 20 m of margin and
    // project to sea level so hills cannot expose a nearer map boundary.
    const float Limit=73000.f/SoulCampaignTerrain::Scale();
    return FBox2D(FVector2D(-Limit,-Limit)-Footprint.Min,FVector2D(Limit,Limit)-Footprint.Max);
}
bool ASoulCampaignCamera::ViewFitsTerrain() const
{
    const auto Range=FocusRange(Distance,Yaw,FocusPoint.Z);
    return Range.ExpandBy(1.f).IsInsideOrOn(FVector2D(FocusPoint.X,FocusPoint.Y));
}
void ASoulCampaignCamera::Zoom(float Steps)
{
    TargetDistance = FMath::Clamp(TargetDistance * FMath::Pow(.85f, Steps), GetMinimumDistance(), GetMaximumDistance());
}
void ASoulCampaignCamera::Pan(FVector2D Direction, float DeltaSeconds)
{
    const FRotator Flat(0,Yaw,0);
    const FVector Forward = Flat.Vector();
    const FVector Right = FRotationMatrix(Flat).GetUnitAxis(EAxis::Y);
    TargetFocus += (Forward * Direction.Y + Right * Direction.X) * FMath::Min(DeltaSeconds,.1f) * TargetDistance * .22f;
    const FVector2D Bounds=SoulCampaignTerrain::FocusBounds()/SoulCampaignTerrain::Scale();
    TargetFocus.X = FMath::Clamp(TargetFocus.X,-Bounds.X,Bounds.X);
    TargetFocus.Y = FMath::Clamp(TargetFocus.Y,-Bounds.Y,Bounds.Y);
}
void ASoulCampaignCamera::Orbit(float Direction, float DeltaSeconds)
{ TargetYaw = FMath::Clamp(TargetYaw + Direction * DeltaSeconds * 40.f, -135.f, -45.f); }
void ASoulCampaignCamera::Focus(FVector Location)
{
    TargetFocus = FVector(Location.X/SoulCampaignTerrain::Scale(),Location.Y/SoulCampaignTerrain::Scale(),Location.Z/SoulCampaignTerrain::Scale()+100);
    // Home/load must still center the company near the coast: zoom in enough
    // to contain that focus, instead of clamping the company out of the center.
    const float FocusMinimum=GetMinimumDistance();
    if(SoulCampaignTerrain::Mesa())while(TargetDistance>FocusMinimum
        &&!FocusRange(TargetDistance,TargetYaw,TargetFocus.Z).IsInsideOrOn(FVector2D(TargetFocus.X,TargetFocus.Y)))
        TargetDistance=FMath::Max(FocusMinimum,TargetDistance*.9f);
    // An edge settlement can remain outside the footprint even at minimum zoom.
    // Choose an allowed inward-facing angle before moving the focus off its anchor.
    if(SoulCampaignTerrain::EvilCorridor()&&!FocusRange(TargetDistance,TargetYaw,TargetFocus.Z).IsInsideOrOn(FVector2D(TargetFocus.X,TargetFocus.Y)))
        for(float Candidate:{-45.f,-90.f,-135.f})
            if(FocusRange(TargetDistance,Candidate,TargetFocus.Z).IsInsideOrOn(FVector2D(TargetFocus.X,TargetFocus.Y)))
            {TargetYaw=Candidate;break;}
}
FVector ASoulCampaignCamera::GetFocus() const {return FocusPoint*SoulCampaignTerrain::Scale();}
void ASoulCampaignCamera::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Pitch=SoulCampaignTerrain::Enabled()?FMath::Lerp(38.f,48.f,FMath::Clamp((Distance-MinDistance)/3500.f,0.f,1.f)):48.f;
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        FVector2D Input(0,0);
        Input.X = (PC->IsInputKeyDown(EKeys::D)?1.f:0.f) - (PC->IsInputKeyDown(EKeys::A)?1.f:0.f);
        Input.Y = (PC->IsInputKeyDown(EKeys::W)?1.f:0.f) - (PC->IsInputKeyDown(EKeys::S)?1.f:0.f);
        FVector2D Stick(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY));
        if(Stick.SizeSquared()>.04f)Input+=Stick;
        if (PC->IsInputKeyDown(EKeys::MiddleMouseButton))
        {
            float DX=0,DY=0; PC->GetInputMouseDelta(DX,DY);
            int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            const float UnitsPerPixel=2.f*Distance*FMath::Tan(FMath::DegreesToRadians(GetCameraComponent()->FieldOfView*.5f))/FMath::Max(Width,1);
            const FRotator Flat(0,Yaw,0);
            // Mouse deltas already represent displacement this frame: no delta-time factor.
            TargetFocus+=(-FRotationMatrix(Flat).GetUnitAxis(EAxis::Y)*DX+Flat.Vector()*DY/ FMath::Sin(FMath::DegreesToRadians(Pitch)))*UnitsPerPixel;
        }
        Pan(Input.GetClampedToMaxSize(2.f),DeltaSeconds);
        Orbit((PC->IsInputKeyDown(EKeys::E)||PC->IsInputKeyDown(EKeys::Gamepad_RightShoulder)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::Q)||PC->IsInputKeyDown(EKeys::Gamepad_LeftShoulder)?1.f:0.f),DeltaSeconds);
    }
    const float Scale=SoulCampaignTerrain::Scale();
    TargetFocus.Z=ASoulCampaignWorldActor::HeightAt(TargetFocus.X*Scale,TargetFocus.Y*Scale)/Scale+100.f;
    if(SoulCampaignTerrain::Mesa())
    {
        const auto Bounds=FocusRange(TargetDistance,TargetYaw,TargetFocus.Z);
        TargetFocus.X=FMath::Clamp(TargetFocus.X,Bounds.Min.X,Bounds.Max.X);
        TargetFocus.Y=FMath::Clamp(TargetFocus.Y,Bounds.Min.Y,Bounds.Max.Y);
    }
    FocusPoint = FMath::VInterpTo(FocusPoint,TargetFocus,DeltaSeconds,7.f);
    Distance = FMath::FInterpTo(Distance,TargetDistance,DeltaSeconds,7.f);
    Yaw = FMath::FInterpTo(Yaw,TargetYaw,DeltaSeconds,7.f);
    const FRotator View(-Pitch,Yaw,0);
    FVector Position = FocusPoint - View.Vector() * Distance;
    Position.Z = FMath::Max(Position.Z, ASoulCampaignWorldActor::HeightAt(Position.X*Scale,Position.Y*Scale)/Scale + 450.f);
    SetActorLocationAndRotation(Position*Scale,View);
}
