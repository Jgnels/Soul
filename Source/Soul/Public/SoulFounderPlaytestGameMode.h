#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SoulFounderPlaytestGameMode.generated.h"
UCLASS()
class SOUL_API ASoulFounderPlaytestGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ASoulFounderPlaytestGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float Seconds) override;
private:
    void TickVisualQualification(float Seconds);
    float VisualElapsed=0;
    int32 VisualStep=0;
    int32 VisualFrames=0;
    double VisualFrameSeconds=0;
    float VisualWorstFrame=0;
    FVector VisualDragStart=FVector::ZeroVector;
    bool bVisualQualification=false;
    FString CapturePrefix;
    UPROPERTY() TObjectPtr<class ASoulFounderPlaytestCampaignActor> Campaign;
    UPROPERTY() TObjectPtr<class USoulFounderPlaytestStateSubsystem> State;
    float Elapsed=0.0f;
    float ReturnHoldSeconds=0.0f;
    bool bRoundTripVerified=false;
    bool bRecoveryQualification=false,bRecoveryAttempted=false;
    bool bQualification=false,bStarted=false,bLoading=false,bDone=false;
    FString ExpectedSnapshot;
};
