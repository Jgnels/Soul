#include "SoulBattlefieldLayoutActor.h"

#include "Components/SceneComponent.h"

ASoulBattlefieldLayoutActor::ASoulBattlefieldLayoutActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    AttackerDeploymentRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("AttackerDeployment"));
    AttackerDeploymentRoot->SetupAttachment(SceneRoot);

    DefenderDeploymentRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("DefenderDeployment"));
    DefenderDeploymentRoot->SetupAttachment(SceneRoot);

    LandmarkRoot =
        CreateDefaultSubobject<USceneComponent>(TEXT("Landmark"));
    LandmarkRoot->SetupAttachment(SceneRoot);
}
