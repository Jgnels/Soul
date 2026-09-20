#include "SoulSiegeObjectiveActor.h"

#include "Components/SceneComponent.h"

ASoulSiegeObjectiveActor::ASoulSiegeObjectiveActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
}
