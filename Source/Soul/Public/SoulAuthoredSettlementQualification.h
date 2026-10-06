#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "InputCoreTypes.h"
#include "SoulAuthoredSettlementQualification.generated.h"

// Explicit isolated-run observer/input driver. It owns no gameplay or save state.
// GameInstance lifetime lets one test follow the existing visit and battle travel.
UCLASS()
class SOUL_API USoulAuthoredSettlementQualification : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return !IsTemplate() && !bDone; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(SoulAuthoredSettlementQualification, STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
private:
    bool Check(bool Condition, const FString& Label);
    bool Snapshots(FString& Campaign, FString& Settlement);
    bool Remember(const TCHAR* Label);
    bool Matches();
    bool Presentation(bool bBuilt, bool bFullEnvironment);
    void Key(FKey Key);
    void Capture(const TCHAR* Label);
    bool CaptureWithoutHUD(const TCHAR* Label, double Now);
    void Next(double Delay = 4.0);
    bool MeasureView(const TCHAR* Label);
    bool AwaitVisualAssets(double Now);
    void TickHumanEnvironmentSurvey(double Now);
    int32 Step = 0;
    int32 LastLoggedStep = -1;
    bool bDone = false;
    bool bVisitPanelOpenedAfterLoad = false;
    bool bBattleWarmCapture = false;
    bool bReturnedBattleViewWarmed = false;
    bool bCleanCapturePending = false, bSavedHUDVisible = true;
    bool bCityGPUProfileQueued = false;
    bool bHumanStreamingReady = false;
    int32 SavedMaterialDrawEvents = 0;
    double Started = 0, NextTime = 0, StepStarted = 0;
    FString Prefix, ExpectedCampaign, ExpectedSettlement, PreBattleSettlement;
    FString SavePath;
    TArray<uint8> SaveBefore;
    TArray<FString> Captures;
    uint32 ExpectedLoadRevision = 0;
    int32 InitialGold = 0;
    FName Encounter;
    double MeasureStarted = 0, PreviousFrame = 0, FrameMs = 0;
    double AssetsQuietSince = 0, NextAssetWaitLog = 0;
    FString MeasureStartedUTC;
    TArray<double> FrameSamples;
};
