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
private:
    UPROPERTY() TObjectPtr<USoulFounderPlaytestStateSubsystem> State;
    FString LastFailure;
    bool bCameraReady = false;
    float CameraWaitSeconds = 0;
    uint32 ObservedDevelopmentRevision = MAX_uint32, ObservedLoadRevision = MAX_uint32;
    void RefreshPresentation();
};
