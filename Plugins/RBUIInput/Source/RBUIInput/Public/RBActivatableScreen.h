#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RBActivatableScreen.generated.h"

class UWidget;

UCLASS(Abstract, Blueprintable)
class RBUIINPUT_API URBActivatableScreen : public UCommonActivatableWidget
{
    GENERATED_BODY()
public:
    URBActivatableScreen();

    UFUNCTION(BlueprintCallable, Category="RB UI|Focus")
    void SetInitialFocusTarget(UWidget* InTarget);

    UFUNCTION(BlueprintCallable, Category="RB UI|Focus")
    void RestoreInitialFocus();

    UFUNCTION(BlueprintCallable, Category="RB UI|Navigation")
    void SetModalMode(bool bModal);

protected:
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual bool NativeOnHandleBackAction() override;

    UPROPERTY(BlueprintReadWrite, meta=(BindWidgetOptional), Category="RB UI|Focus")
    TObjectPtr<UWidget> InitialFocusTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB UI|Navigation")
    bool bDeactivateOnBack = true;
};
