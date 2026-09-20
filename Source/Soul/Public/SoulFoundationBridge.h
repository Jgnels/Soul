#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBFoundationInterfaces.h"
#include "SoulFoundationBridge.generated.h"

UCLASS()
class SOUL_API USoulFoundationBridge : public UGameInstanceSubsystem, public IRBFoundationTimeSource
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual bool GetFoundationTimeState_Implementation(FRBFoundationTimeState& OutTime, FString& OutError) const override;

    UFUNCTION(BlueprintCallable, Category="Soul|Time")
    bool SetCampaignTime(int32 DayIndex, int32 MinuteOfDay, FName Season, FString& OutError);

    UFUNCTION(BlueprintPure, Category="Soul|Time")
    int32 GetCampaignDay() const { return CampaignDay; }

    UFUNCTION(BlueprintPure, Category="Soul|Time")
    int32 GetMinuteOfDay() const { return CampaignMinuteOfDay; }

private:
    UPROPERTY(Transient)
    TObjectPtr<class URBFoundationSubsystem> Foundation;

    UPROPERTY(Transient)
    int32 CampaignDay = 1;

    UPROPERTY(Transient)
    int32 CampaignMinuteOfDay = 480;

    UPROPERTY(Transient)
    FName CampaignSeason = TEXT("Default");

    TArray<FName> RegisteredAuthorities;
};
