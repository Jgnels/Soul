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
    bool bAlphaBattleTravel = false;
    double AlphaNextActionTime = 0;
    void TickFourFactionAlphaQualification(float Seconds);
    void TickHeartlandQualification(float Seconds);
    double AlphaProofNextTime = 0;
    void TickCompositionTraversal(float Seconds);
    void TickSixFactionQualification(float Seconds);
    void TickControlledBattleQualification(float Seconds);
    void TickVisualQualification(float Seconds);
    void TickSettlementDevelopmentQualification(float Seconds);
    bool bSettlementDevelopmentQualification=false;
    double DevelopmentQualificationStart=0,DevelopmentQualificationNextStep=0;
    int32 DevelopmentInitialGold=0,DevelopmentInitialDay=0,DevelopmentGoldIncome=0;
    uint32 DevelopmentExpectedLoadRevision=0,DevelopmentExpectedStateRevision=0;
    FString DevelopmentSettlementSnapshot;
    TArray<uint8> DevelopmentSaveBefore;
    bool bDevelopmentObservedPersistenceBusy=false;
    float VisualElapsed=0;
    int32 VisualStep=0;
    int32 VisualFrames=0;
    int32 BenchmarkSampleSeconds=60;
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
