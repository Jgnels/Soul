#include "RBActivatableScreen.h"
#include "Components/Widget.h"

URBActivatableScreen::URBActivatableScreen()
{
    bIsBackHandler = true;
    bAutoRestoreFocus = true;
    bSupportsActivationFocus = true;
}

void URBActivatableScreen::SetInitialFocusTarget(UWidget* InTarget)
{
    InitialFocusTarget = InTarget;
}

void URBActivatableScreen::SetModalMode(bool bModal)
{
    bIsModal = bModal;
}

UWidget* URBActivatableScreen::NativeGetDesiredFocusTarget() const
{
    if (IsValid(InitialFocusTarget) && InitialFocusTarget->GetIsEnabled())
    {
        return InitialFocusTarget;
    }
    return Super::NativeGetDesiredFocusTarget();
}
void URBActivatableScreen::RestoreInitialFocus()
{
    if (UWidget* Target = GetDesiredFocusTarget())
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            Target->SetUserFocus(PC);
        }
        Target->SetKeyboardFocus();
    }
}

bool URBActivatableScreen::NativeOnHandleBackAction()
{
    if (!bIsBackHandler)
    {
        return false;
    }
    if (!bDeactivateOnBack)
    {
        return true;
    }
    return Super::NativeOnHandleBackAction();
}
