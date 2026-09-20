#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RBFoundationTypes.h"
#include "RBFoundationInterfaces.generated.h"

UINTERFACE(BlueprintType)
class RBFOUNDATION_API URBFoundationTimeSource : public UInterface
{
    GENERATED_BODY()
};

class RBFOUNDATION_API IRBFoundationTimeSource
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Time")
    bool GetFoundationTimeState(FRBFoundationTimeState& OutTime, FString& OutError) const;
};

UINTERFACE(BlueprintType)
class RBFOUNDATION_API URBFoundationTimeConsumer : public UInterface
{
    GENERATED_BODY()
};

class RBFOUNDATION_API IRBFoundationTimeConsumer
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Time")
    bool ValidateFoundationTime(const FRBFoundationTimeState& NewTime,
        const FRBFoundationTimeState& PreviousTime, bool bHasPreviousTime,
        FString& OutError) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Time")
    bool ApplyFoundationTime(const FRBFoundationTimeState& NewTime,
        const FRBFoundationTimeState& PreviousTime, bool bHasPreviousTime,
        FString& OutError);
};

UINTERFACE(BlueprintType)
class RBFOUNDATION_API URBFoundationProductAdapter : public UInterface
{
    GENERATED_BODY()
};

class RBFOUNDATION_API IRBFoundationProductAdapter
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Adapter")
    bool GetFoundationProductDescriptor(FRBFoundationProductDescriptor& OutDescriptor, FString& OutError) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Adapter")
    bool FoundationAdapterStartup(FString& OutError);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Adapter")
    void FoundationAdapterShutdown();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Adapter")
    void ProbeFoundationAdapterHealth(TArray<FRBFoundationHealthIssue>& OutIssues) const;
};
UINTERFACE(BlueprintType)
class RBFOUNDATION_API URBFoundationEventConsumer : public UInterface
{
    GENERATED_BODY()
};

class RBFOUNDATION_API IRBFoundationEventConsumer
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Foundation|Events")
    bool ConsumeFoundationEvent(const FRBFoundationDomainEvent& Event,
        FRBFoundationEventConsumerResult& OutResult);
};
