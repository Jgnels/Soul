#include "SoulCampaignCamera.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "SoulCampaignWorldActor.h"
#include "SoulFounderPlaytestCampaignActor.h"
#include "EngineUtils.h"
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
void ASoulCampaignCamera::BeginPlay() { Super::BeginPlay(); if(SoulCampaignTerrain::Composition())Distance=TargetDistance=9000.f;else if(SoulCampaignTerrain::EvilCorridor())Distance=TargetDistance=3200.f; Tick(1.f); }
float ASoulCampaignCamera::GetMinimumDistance() const {return SoulCampaignTerrain::Composition()||SoulCampaignTerrain::EvilCorridor()?900.f:MinDistance;}
float ASoulCampaignCamera::GetMaximumDistance() const {return SoulCampaignTerrain::Composition()?22000.f:SoulCampaignTerrain::Mesa()?9000.f:MaxDistance;}
float ASoulCampaignCamera::ViewPitch(float ViewDistance) const
{
    if(SoulCampaignTerrain::Composition())return FMath::Clamp(InspectionPitchOffset+(ViewDistance<=18000.f?
        FMath::Lerp(40.f,78.f,FMath::Clamp((ViewDistance-1800.f)/50000.f,0.f,1.f)):
        FMath::Lerp(52.312f,55.f,FMath::Clamp((ViewDistance-18000.f)/202000.f,0.f,1.f))),35.f,75.f);
    return SoulCampaignTerrain::Enabled()?FMath::Lerp(38.f,48.f,FMath::Clamp((ViewDistance-MinDistance)/3500.f,0.f,1.f)):48.f;
}
FBox2D ASoulCampaignCamera::RenderBounds(float ViewDistance) const
{
    return SoulCampaignTerrain::TerrainBounds().ExpandBy(-2000);
}
FBox2D ASoulCampaignCamera::FocusRange(float ViewDistance,float ViewYaw,float FocusHeight) const
{
    const auto Bounds=SoulCampaignTerrain::FocusBounds()/SoulCampaignTerrain::Scale();
    if(!SoulCampaignTerrain::Mesa()&&!SoulCampaignTerrain::Composition())return FBox2D(-Bounds,Bounds);
    const float Pitch=ViewPitch(ViewDistance);
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
    const auto Terrain=RenderBounds(ViewDistance);
    const float Scale=SoulCampaignTerrain::Scale();
    FBox2D Range(Terrain.Min/Scale-Footprint.Min,Terrain.Max/Scale-Footprint.Max);
    if(SoulCampaignTerrain::Composition())
    {
        Range.Min.X=FMath::Max(Range.Min.X,-Bounds.X);Range.Min.Y=FMath::Max(Range.Min.Y,-Bounds.Y);
        Range.Max.X=FMath::Min(Range.Max.X,Bounds.X);Range.Max.Y=FMath::Min(Range.Max.Y,Bounds.Y);
    }
    if(Range.Min.X>Range.Max.X||Range.Min.Y>Range.Max.Y)return FBox2D(ForceInit);
    return Range;
}
bool ASoulCampaignCamera::ViewFitsTerrain() const
{
    const auto Range=FocusRange(Distance,Yaw,FocusPoint.Z);
    if(!Range.bIsValid||!Range.ExpandBy(1.f).IsInsideOrOn(FVector2D(FocusPoint.X,FocusPoint.Y)))return false;
    if(!SoulCampaignTerrain::Mesa()&&!SoulCampaignTerrain::Composition())return true;
    const auto Bounds=RenderBounds(Distance).ExpandBy(10);
    const FRotationMatrix Rotation(GetActorRotation());
    const FVector Forward=Rotation.GetUnitAxis(EAxis::X),Right=Rotation.GetUnitAxis(EAxis::Y),Up=Rotation.GetUnitAxis(EAxis::Z),Position=GetActorLocation();
    const float Tan=FMath::Tan(FMath::DegreesToRadians(GetCameraComponent()->FieldOfView*.5f));
    int32 Width=1920,Height=1080;if(auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr)PC->GetViewportSize(Width,Height);
    const float Aspect=Width>0&&Height>0?static_cast<float>(Width)/Height:16.f/9.f;
    for(float X:{-1.f,1.f})for(float Y:{-1.f,1.f})
    {
        const FVector Ray=Forward+Right*Tan*X+Up*Tan/Aspect*Y;
        if(Ray.Z>=0)return false;
        const FVector Point=Position-Ray*(Position.Z/Ray.Z);
        if(!Bounds.IsInsideOrOn(FVector2D(Point.X,Point.Y)))return false;
    }
    return true;
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
{ TargetYaw = SoulCampaignTerrain::Composition()?FRotator::NormalizeAxis(TargetYaw + Direction * DeltaSeconds * 40.f):FMath::Clamp(TargetYaw + Direction * DeltaSeconds * 40.f, -135.f, -45.f); }
void ASoulCampaignCamera::Focus(FVector Location)
{
    TargetFocus = FVector(Location.X/SoulCampaignTerrain::Scale(),Location.Y/SoulCampaignTerrain::Scale(),Location.Z/SoulCampaignTerrain::Scale()+100);
    // Home/load must still center the company near the coast: zoom in enough
    // to contain that focus, instead of clamping the company out of the center.
    const float FocusMinimum=GetMinimumDistance();
    if(SoulCampaignTerrain::Mesa()||SoulCampaignTerrain::Composition())while(TargetDistance>FocusMinimum
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
    const float Pitch=ViewPitch(Distance);
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        if(SoulCampaignTerrain::Composition())
        {
            InspectionPitchOffset=FMath::Clamp(InspectionPitchOffset+((PC->IsInputKeyDown(EKeys::PageUp)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::PageDown)?1.f:0.f))*25.f*FMath::Min(DeltaSeconds,.1f),-15.f,30.f);
            if(PC->WasInputKeyJustPressed(EKeys::F))for(TActorIterator<ASoulFounderPlaytestCampaignActor> It(GetWorld());It;++It)
                if(const FVector* P=ASoulCampaignWorldActor::Locations().Find(It->GetSelectedRegion())){Focus(*P);break;}
        }
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
    if(SoulCampaignTerrain::Mesa()||SoulCampaignTerrain::Composition())
    {
        const auto Bounds=FocusRange(TargetDistance,TargetYaw,TargetFocus.Z);
        if(Bounds.bIsValid)
        {
            TargetFocus.X=FMath::Clamp(TargetFocus.X,Bounds.Min.X,Bounds.Max.X);
            TargetFocus.Y=FMath::Clamp(TargetFocus.Y,Bounds.Min.Y,Bounds.Max.Y);
        }
    }
    FocusPoint = FMath::VInterpTo(FocusPoint,TargetFocus,DeltaSeconds,7.f);
    Distance = FMath::FInterpTo(Distance,TargetDistance,DeltaSeconds,7.f);
    Yaw = FMath::RInterpTo(FRotator(0,Yaw,0),FRotator(0,TargetYaw,0),DeltaSeconds,7.f).Yaw;
    const FRotator View(-Pitch,Yaw,0);
    FVector Position = FocusPoint - View.Vector() * Distance;
    Position.Z = FMath::Max(Position.Z, ASoulCampaignWorldActor::HeightAt(Position.X*Scale,Position.Y*Scale)/Scale + 450.f);
    SetActorLocationAndRotation(Position*Scale,View);
}
