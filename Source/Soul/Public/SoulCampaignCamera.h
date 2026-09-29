#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "SoulCampaignCamera.generated.h"

UCLASS()
class SOUL_API ASoulCampaignCamera : public ACameraActor
{
    GENERATED_BODY()
public:
    ASoulCampaignCamera();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Zoom(float Steps);
    void Pan(FVector2D Direction, float DeltaSeconds);
    void Orbit(float Direction, float DeltaSeconds);
    void Focus(FVector Location);
    static constexpr float MaxFocusX = 3800.f;
    static constexpr float MaxFocusY = 2600.f;
    static constexpr float MinDistance = 1800.f;
    static constexpr float MaxDistance = 12000.f;
    float GetDistance() const { return Distance; }
    FVector GetFocus() const { return FocusPoint; }
private:
    FVector FocusPoint = FVector(-1000,-250,100), TargetFocus = FocusPoint;
    float Distance = 7500.f, TargetDistance = Distance;
    float Yaw = -90.f, TargetYaw = Yaw;
};
