#include "SoulTownViewAnchor.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

ASoulTownViewAnchor::ASoulTownViewAnchor()
{
    PrimaryActorTick.bCanEverTick = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("TownViewCamera"));
    SetRootComponent(Camera);
    Camera->SetFieldOfView(45.0f);
}

bool ASoulTownViewAnchor::ActivateForPlayer(
    APlayerController* PlayerController,
    float BlendTime)
{
    if (!PlayerController)
    {
        return false;
    }

    PlayerController->SetViewTargetWithBlend(
        this,
        FMath::Max(0.0f, BlendTime));
    return true;
}
