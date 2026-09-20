#pragma once

#include "CoreMinimal.h"
#include "RBSaveDomainProvider.h"
#include "RBOptimizationState.h"
#include "RBFoundationSaveProviders.generated.h"

class URBFoundationRBAdapterSubsystem;

USTRUCT()
struct RBFOUNDATIONADAPTERS_API FRBFoundationOptimizationGroupSnapshot
{
    GENERATED_BODY()

    UPROPERTY() FName GroupId;
    UPROPERTY() FRBOptSnapshot Snapshot;
};

USTRUCT()
struct RBFOUNDATIONADAPTERS_API FRBFoundationOptimizationSnapshot
{
    GENERATED_BODY()

    UPROPERTY() int32 SchemaVersion = 1;
    UPROPERTY() TArray<FRBFoundationOptimizationGroupSnapshot> Groups;
};

UCLASS(Abstract)
class RBFOUNDATIONADAPTERS_API URBFoundationSaveProviderBase : public UObject,
    public IRBSaveDomainProvider
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient)
    TObjectPtr<URBFoundationRBAdapterSubsystem> Adapter;
};

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationCoreSaveProvider final : public URBFoundationSaveProviderBase
{
    GENERATED_BODY()
public:
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& OutState, FString& OutError) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& State, FString& OutError) override;
};

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationItemEconomySaveProvider final : public URBFoundationSaveProviderBase
{
    GENERATED_BODY()
public:
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& OutState, FString& OutError) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& State, FString& OutError) override;
};

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationRoutineSaveProvider final : public URBFoundationSaveProviderBase
{
    GENERATED_BODY()
public:
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& OutState, FString& OutError) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& State, FString& OutError) override;
};

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationWeatherSaveProvider final : public URBFoundationSaveProviderBase
{
    GENERATED_BODY()
public:
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& OutState, FString& OutError) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& State, FString& OutError) override;
};

UCLASS()
class RBFOUNDATIONADAPTERS_API URBFoundationOptimizationSaveProvider final : public URBFoundationSaveProviderBase
{
    GENERATED_BODY()
public:
    virtual FName GetRBSaveDomainId_Implementation() const override;
    virtual int32 GetRBSaveSchemaVersion_Implementation() const override;
    virtual bool CaptureRBSaveDomain_Implementation(FRBSaveDomainState& OutState, FString& OutError) const override;
    virtual bool RestoreRBSaveDomain_Implementation(const FRBSaveDomainState& State, FString& OutError) override;
};
