#include "RBUIInputSubsystem.h"
#include "CommonInputSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/InputDeviceLibrary.h"

void URBUIInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UCommonInputSubsystem>();

    BoundCommonInput = UCommonInputSubsystem::Get(GetLocalPlayer());
    if (UCommonInputSubsystem* CommonInput = BoundCommonInput.Get())
    {
        CommonInput->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
        CommonInput->GetOnGamepadChangeDetected().AddUObject(this, &ThisClass::HandleGamepadTypeChanged);
    }

    BoundGameInstance = GetLocalPlayer() ? GetLocalPlayer()->GetGameInstance() : nullptr;
    if (UGameInstance* GameInstance = BoundGameInstance.Get())
    {
        GameInstance->OnInputDeviceConnectionChange.AddDynamic(this, &ThisClass::HandleInputDeviceConnectionChanged);
    }
}

void URBUIInputSubsystem::Deinitialize()
{
    if (UCommonInputSubsystem* CommonInput = BoundCommonInput.Get())
    {
        CommonInput->OnInputMethodChangedNative.RemoveAll(this);
        // Gamepad-change binding uses AddUObject (weak UObject binding). CommonInput owns
        // that delegate on its input preprocessor, which may already be released during
        // local-player teardown, so calling the accessor here is unsafe.
    }

    if (UGameInstance* GameInstance = BoundGameInstance.Get())
    {
        GameInstance->OnInputDeviceConnectionChange.RemoveDynamic(this, &ThisClass::HandleInputDeviceConnectionChanged);
    }

    BoundCommonInput = nullptr;
    BoundGameInstance = nullptr;
    Super::Deinitialize();
}

ECommonInputType URBUIInputSubsystem::GetCurrentInputType() const
{
    if (const UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(GetLocalPlayer()))
    {
        return CommonInput->GetCurrentInputType();
    }
    return ECommonInputType::MouseAndKeyboard;
}

bool URBUIInputSubsystem::IsGamepadActive() const
{
    return GetCurrentInputType() == ECommonInputType::Gamepad;
}

FName URBUIInputSubsystem::GetCurrentGamepadName() const
{
    if (const UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(GetLocalPlayer()))
    {
        return CommonInput->GetCurrentGamepadName();
    }
    return NAME_None;
}

bool URBUIInputSubsystem::IsPrimaryInputDeviceConnected() const
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer)
    {
        return false;
    }

    const FInputDeviceId DeviceId = UInputDeviceLibrary::GetPrimaryInputDeviceForUser(LocalPlayer->GetPlatformUserId());
    return UInputDeviceLibrary::GetInputDeviceConnectionState(DeviceId) == EInputDeviceConnectionState::Connected;
}

void URBUIInputSubsystem::SetAccessibilityPolicy(const FRBUIAccessibilityPolicy& InPolicy)
{
    FRBUIAccessibilityPolicy Sanitized = InPolicy;
    Sanitized.TextScale = FMath::Clamp(Sanitized.TextScale, 0.75f, 2.0f);

    const bool bChanged =
        !FMath::IsNearlyEqual(AccessibilityPolicy.TextScale, Sanitized.TextScale) ||
        AccessibilityPolicy.bReducedMotion != Sanitized.bReducedMotion ||
        AccessibilityPolicy.bVibrationEnabled != Sanitized.bVibrationEnabled ||
        AccessibilityPolicy.bHoldActionsUseToggle != Sanitized.bHoldActionsUseToggle;

    AccessibilityPolicy = Sanitized;
    if (bChanged)
    {
        OnAccessibilityPolicyChanged.Broadcast(AccessibilityPolicy);
    }
}

void URBUIInputSubsystem::HandleInputMethodChanged(ECommonInputType NewInputType)
{
    OnInputMethodChanged.Broadcast(NewInputType);
}

void URBUIInputSubsystem::HandleGamepadTypeChanged(FName NewGamepadName)
{
    OnGamepadTypeChanged.Broadcast(NewGamepadName);
}

void URBUIInputSubsystem::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer || PlatformUserId != LocalPlayer->GetPlatformUserId())
    {
        return;
    }

    OnInputDeviceConnectionChanged.Broadcast(NewState == EInputDeviceConnectionState::Connected, InputDeviceId.GetId());
}
