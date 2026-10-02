#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulZeroTimeCameraTest,
    "Soul.RealtimeBattle.Controls.ZeroTimeDeploymentCamera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulZeroTimeCameraTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    // CreateWorld already initializes its world and WorldSettings in UE 5.8.
    auto* PC = World->SpawnActor<ASoulRealtimeArenaPlayerController>();
    if (!PC->PlayerCameraManager)
    {
        PC->PlayerCameraManager = World->SpawnActor<APlayerCameraManager>();
        PC->PlayerCameraManager->InitializeFor(PC);
    }
    auto* Hero = World->SpawnActor<ACharacter>();
    auto* Camera = NewObject<UCameraComponent>(Hero);
    Hero->AddInstanceComponent(Camera);
    Camera->SetupAttachment(Hero->GetRootComponent());
    Camera->SetRelativeLocation(FVector(-600, 0, 380));
    Camera->SetRelativeRotation(FRotator(-30, 0, 0));
    Camera->RegisterComponent();
    Camera->SetActive(true);
    TestTrue(TEXT("Test hero has active camera"), Hero->HasActiveCameraComponent());
    PC->SetViewTarget(Hero);
    TestTrue(TEXT("Test view target is hero"), PC->GetViewTarget() == Hero);
    PC->UpdateCameraManager(0.0f);
    FVector Location;
    FRotator Rotation;
    PC->GetPlayerViewPoint(Location, Rotation);
    TestEqual(TEXT("Deployment did not advance world time"), World->GetTimeSeconds(), 0.0);
    TestTrue(TEXT("View uses camera instead of hero waist"),
        Location.Equals(Camera->GetComponentLocation(), 0.01));
    TestTrue(TEXT("View uses camera pitch"), Rotation.Equals(Camera->GetComponentRotation(), 0.01));
    Camera->SetRelativeLocation(FVector(0, 0, 85));
    Camera->SetRelativeRotation(FRotator(0, 65, 0));
    PC->UpdateCameraManager(0.0f);
    PC->GetPlayerViewPoint(Location, Rotation);
    TestTrue(TEXT("Camera can switch while world time remains zero"),
        Location.Equals(Camera->GetComponentLocation(), 0.01));
    TestTrue(TEXT("Camera can look while world time remains zero"),
        Rotation.Equals(Camera->GetComponentRotation(), 0.01));
    World->DestroyWorld(false);
    return true;
}
#endif

