#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "SoulFounderPlaytestPlayerController.h"
#include "SoulTitanPilot.generated.h"

UCLASS()
class SOUL_API ASoulTitanPilotPawn : public ACharacter
{
    GENERATED_BODY()
public:
    ASoulTitanPilotPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    void LookYaw(float Value);
    void LookPitch(float Value);
};

/** Retains the Soul controller's RBSave keys; no Titan input or player classes. */
UCLASS()
class SOUL_API ASoulTitanPilotController : public ASoulFounderPlaytestPlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
};

/** Terrain traversal proof only. Campaign rules, saves and combat stay in Soul/RB. */
UCLASS()
class SOUL_API ASoulTitanPilotGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASoulTitanPilotGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    void Finish(bool Passed, const FString& Reason);
    float Elapsed = 0, PhaseTime = 0;
    int32 Phase = 0, Frames = 0;
    bool bProof = false, bDone = false;
    FVector MoveStart = FVector::ZeroVector;
    TArray<FVector> InteriorRoute;
    int32 RoutePoint = 1;
    FString Snapshot;
    double FrameTotal = 0, MaxFrame = 0;
};
