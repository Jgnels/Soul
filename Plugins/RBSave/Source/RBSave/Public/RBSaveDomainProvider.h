#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RBSaveTypes.h"
#include "RBSaveDomainProvider.generated.h"

UINTERFACE(BlueprintType)
class RBSAVE_API URBSaveDomainProvider : public UInterface
{
    GENERATED_BODY()
};

class RBSAVE_API IRBSaveDomainProvider
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Save|Domain")
    FName GetRBSaveDomainId() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Save|Domain")
    int32 GetRBSaveSchemaVersion() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Save|Domain")
    bool CaptureRBSaveDomain(FRBSaveDomainState& OutState, FString& OutError) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="RB Save|Domain")
    bool RestoreRBSaveDomain(const FRBSaveDomainState& State, FString& OutError);
};