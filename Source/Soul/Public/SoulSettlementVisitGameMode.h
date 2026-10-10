#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulSettlementVisitGameMode.generated.h"

class USoulFounderPlaytestStateSubsystem;

// A view of the existing campaign/settlement authorities, with no separate game state.
UCLASS()
class SOUL_API ASoulSettlementVisitGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASoulSettlementVisitGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Seconds) override;
    static bool CanVisit(const USoulFounderPlaytestStateSubsystem* State, FString& OutError);
    void HandleAction(FName Action);
    USoulFounderPlaytestStateSubsystem* GetState() const { return State.Get(); }
    bool IsVisitReady() const { return bCameraReady && LastFailure.IsEmpty(); }
    FString LastMessage;
    bool bManagePanel = false;
    bool IsWalking() const { return Walker!=nullptr; }
private:
    UPROPERTY() TObjectPtr<USoulFounderPlaytestStateSubsystem> State;
    FString LastFailure;
    bool bCameraReady = false;
    bool bReturnRequested = false;
    float ReturnWaitSeconds=0;
    bool IsStreamingReady() const;
    float CameraWaitSeconds = 0;
    uint32 ObservedDevelopmentRevision = MAX_uint32, ObservedLoadRevision = MAX_uint32;
    void RefreshPresentation();
    bool StartWalking();
    void TickWalking(float Seconds);
    UPROPERTY() TObjectPtr<class ACharacter> Walker;
    UPROPERTY() TObjectPtr<class UAnimationAsset> WalkIdle;
    UPROPERTY() TObjectPtr<class UAnimationAsset> WalkJog;
    bool bWalkingAnimation=false;
    float WalkingProofTime=0;FVector WalkingProofOrigin=FVector::ZeroVector;
};
