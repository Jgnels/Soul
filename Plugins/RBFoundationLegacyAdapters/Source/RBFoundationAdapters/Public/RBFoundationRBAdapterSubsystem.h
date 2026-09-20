#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RBFoundationInterfaces.h"
#include "RBFoundationRBAdapterSubsystem.generated.h"

class ARBOptimizationGroup;
class ARBWeatherDirector;
class URBFoundationSubsystem;
class URBItemEconomySubsystem;
class URBSaveSubsystem;
class URBRoutineWorldSubsystem;

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationRBAdapterSubsystem : public UGameInstanceSubsystem,
    public IRBFoundationTimeConsumer
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="RB Foundation|RB Stack")
    bool VerifyFrozenStack(FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category="RB Foundation|RB Stack")
    bool RegisterWeatherDirector(ARBWeatherDirector* Director, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|RB Stack")
    void UnregisterWeatherDirector(ARBWeatherDirector* Director);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|RB Stack")
    bool RegisterOptimizationGroup(FName GroupId, ARBOptimizationGroup* Group, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB Foundation|RB Stack")
    void UnregisterOptimizationGroup(FName GroupId, ARBOptimizationGroup* Group);

    UFUNCTION(BlueprintPure, Category="RB Foundation|RB Stack")
    ARBOptimizationGroup* GetOptimizationGroup(FName GroupId) const;

    UFUNCTION(BlueprintCallable, Category="RB Foundation|Transactions")
    FRBFoundationOperationResult CommitRoutineProductionAward(FGuid CorrelationId,
        FName AgentId, int64 DestinationInventoryId, FName DefinitionId,
        int64 Quantity, int32 Quality = 10000);

    UFUNCTION(BlueprintPure, Category="RB Foundation|Diagnostics")
    FString GetInitializationError() const { return InitializationError; }

    virtual bool ValidateFoundationTime_Implementation(const FRBFoundationTimeState& NewTime,
        const FRBFoundationTimeState& PreviousTime, bool bHasPreviousTime,
        FString& OutError) const override;

    virtual bool ApplyFoundationTime_Implementation(const FRBFoundationTimeState& NewTime,
        const FRBFoundationTimeState& PreviousTime, bool bHasPreviousTime,
        FString& OutError) override;

    URBFoundationSubsystem* GetFoundation() const;
    URBSaveSubsystem* GetSave() const;
    URBItemEconomySubsystem* GetItemEconomy() const;
    URBRoutineWorldSubsystem* GetRoutine() const;
    ARBWeatherDirector* GetWeatherDirector() const { return WeatherDirector.Get(); }
    const TMap<FName, TWeakObjectPtr<ARBOptimizationGroup>>& GetOptimizationGroups() const
        { return OptimizationGroups; }
    void ResetTimeBridgeAfterRestore();

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> SaveProviders;

    TWeakObjectPtr<ARBWeatherDirector> WeatherDirector;
    TMap<FName, TWeakObjectPtr<ARBOptimizationGroup>> OptimizationGroups;
    FString InitializationError;
    int64 LastWeatherGameSeconds = 0;
    bool bWeatherTimeApplied = false;

    bool RegisterStackAuthorities(FString& OutError);
    bool RegisterSaveProviders(FString& OutError);
    void UnregisterSaveProviders();
};
