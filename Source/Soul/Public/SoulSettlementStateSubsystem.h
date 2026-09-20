#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBSaveDomainProvider.h"
#include "SoulSettlement.h"
#include "SoulSiegeAftermath.h"
#include "SoulSettlementStateSubsystem.generated.h"

UCLASS()
class SOUL_API USoulSettlementStateSubsystem
    : public UGameInstanceSubsystem
    , public IRBSaveDomainProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(
        FRBSaveDomainState& OutState,
        FString& OutError) const override;

    virtual bool RestoreRBSaveDomain_Implementation(
        const FRBSaveDomainState& State,
        FString& OutError) override;

    FSoulSettlementState& FindOrAddSettlement(FName SettlementId);
    FSoulSettlementState* FindSettlement(FName SettlementId);
    const FSoulSettlementState* FindSettlement(FName SettlementId) const;

    bool ApplySiegeAftermath(
        FName SettlementId,
        const FSoulSiegeAftermath& Aftermath);

    UFUNCTION(BlueprintPure, Category="Soul|Settlement")
    bool HasSettlement(FName SettlementId) const;

    UFUNCTION(BlueprintPure, Category="Soul|Settlement")
    int32 GetWallIntegrity(FName SettlementId) const;

    UFUNCTION(BlueprintPure, Category="Soul|Settlement")
    FName GetBuildingConditionName(
        FName SettlementId,
        FName BuildingId) const;

private:
    TMap<FName, FSoulSettlementState> Settlements;
    TWeakObjectPtr<class URBSaveSubsystem> SaveSubsystem;

    bool SerializeToJson(FString& OutJson, FString& OutError) const;
    bool RestoreFromJson(const FString& Json, FString& OutError);
};
