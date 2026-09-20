#pragma once

#include "CoreMinimal.h"
#include "RBFoundationInterfaces.h"
#include "RBFoundationDeclarativeAdapter.generated.h"

UCLASS(BlueprintType)
class RBFOUNDATION_API URBFoundationDeclarativeProductAdapter : public UObject,
    public IRBFoundationProductAdapter
{
    GENERATED_BODY()
public:
    bool Configure(const FRBFoundationProductDescriptor& InDescriptor, FString& OutError);

    virtual bool GetFoundationProductDescriptor_Implementation(
        FRBFoundationProductDescriptor& OutDescriptor, FString& OutError) const override;
    virtual bool FoundationAdapterStartup_Implementation(FString& OutError) override;
    virtual void FoundationAdapterShutdown_Implementation() override;
    virtual void ProbeFoundationAdapterHealth_Implementation(
        TArray<FRBFoundationHealthIssue>& OutIssues) const override;

private:
    UPROPERTY(Transient)
    FRBFoundationProductDescriptor Descriptor;
    bool bConfigured = false;
};
