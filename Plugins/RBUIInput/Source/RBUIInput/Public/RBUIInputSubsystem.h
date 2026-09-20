#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "RBUIInputTypes.h"
#include "RBUIInputSubsystem.generated.h"

class UCommonInputSubsystem;
class UGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBInputMethodChanged, ECommonInputType, InputType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBGamepadTypeChanged, FName, GamepadName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRBInputDeviceConnectionChanged, bool, bConnected, int32, DeviceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRBAccessibilityPolicyChanged, const FRBUIAccessibilityPolicy&, Policy);

UCLASS()
class RBUIINPUT_API URBUIInputSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(BlueprintAssignable, Category="RB UI|Input")
    FRBInputMethodChanged OnInputMethodChanged;

    UPROPERTY(BlueprintAssignable, Category="RB UI|Input")
    FRBGamepadTypeChanged OnGamepadTypeChanged;

    UPROPERTY(BlueprintAssignable, Category="RB UI|Input")
    FRBInputDeviceConnectionChanged OnInputDeviceConnectionChanged;

    UPROPERTY(BlueprintAssignable, Category="RB UI|Accessibility")
    FRBAccessibilityPolicyChanged OnAccessibilityPolicyChanged;

    UFUNCTION(BlueprintPure, Category="RB UI|Input")
    ECommonInputType GetCurrentInputType() const;

    UFUNCTION(BlueprintPure, Category="RB UI|Input")
    bool IsGamepadActive() const;

    UFUNCTION(BlueprintPure, Category="RB UI|Input")
    FName GetCurrentGamepadName() const;

    UFUNCTION(BlueprintPure, Category="RB UI|Input")
    bool IsPrimaryInputDeviceConnected() const;

    UFUNCTION(BlueprintCallable, Category="RB UI|Accessibility")
    void SetAccessibilityPolicy(const FRBUIAccessibilityPolicy& InPolicy);

    UFUNCTION(BlueprintPure, Category="RB UI|Accessibility")
    FRBUIAccessibilityPolicy GetAccessibilityPolicy() const { return AccessibilityPolicy; }

private:
    void HandleInputMethodChanged(ECommonInputType NewInputType);
    void HandleGamepadTypeChanged(FName NewGamepadName);

    UFUNCTION()
    void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

    UPROPERTY(Transient)
    TObjectPtr<UCommonInputSubsystem> BoundCommonInput = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UGameInstance> BoundGameInstance = nullptr;

    FRBUIAccessibilityPolicy AccessibilityPolicy;
};
