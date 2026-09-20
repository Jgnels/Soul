#include "RBInputPromptLibrary.h"

namespace
{
const TCHAR* DeviceFamilyToken(const ERBInputDeviceFamily Family)
{
    switch (Family)
    {
        case ERBInputDeviceFamily::KeyboardMouse: return TEXT("KeyboardMouse");
        case ERBInputDeviceFamily::XboxStyle: return TEXT("XboxStyle");
        case ERBInputDeviceFamily::PlayStationStyle: return TEXT("PlayStationStyle");
        case ERBInputDeviceFamily::GenericGamepad: return TEXT("GenericGamepad");
        case ERBInputDeviceFamily::Touch: return TEXT("Touch");
        default: return TEXT("Unknown");
    }
}
}

ERBInputDeviceFamily URBInputPromptLibrary::ClassifyInputDevice(const ECommonInputType InputType, const FName GamepadName)
{
    if (InputType == ECommonInputType::MouseAndKeyboard)
    {
        return ERBInputDeviceFamily::KeyboardMouse;
    }
    if (InputType == ECommonInputType::Touch)
    {
        return ERBInputDeviceFamily::Touch;
    }
    if (InputType != ECommonInputType::Gamepad)
    {
        return ERBInputDeviceFamily::Unknown;
    }

    const FString Name = GamepadName.ToString().ToLower();
    if (Name.Contains(TEXT("dualshock")) || Name.Contains(TEXT("dualsense")) || Name.Contains(TEXT("playstation")) ||
        Name.Contains(TEXT("ps4")) || Name.Contains(TEXT("ps5")))
    {
        return ERBInputDeviceFamily::PlayStationStyle;
    }
    if (Name.Contains(TEXT("xbox")) || Name.Contains(TEXT("xinput")))
    {
        return ERBInputDeviceFamily::XboxStyle;
    }
    return ERBInputDeviceFamily::GenericGamepad;
}

FName URBInputPromptLibrary::MakeGlyphId(const ERBInputDeviceFamily DeviceFamily, const FKey Key)
{
    if (!Key.IsValid())
    {
        return NAME_None;
    }
    return FName(*FString::Printf(TEXT("RB.Glyph.%s.%s"), DeviceFamilyToken(DeviceFamily), *Key.GetFName().ToString()));
}

FRBInputGlyphDescriptor URBInputPromptLibrary::MakeGlyphDescriptor(const ECommonInputType InputType, const FName GamepadName, const FName MappingName, const FKey Key)
{
    FRBInputGlyphDescriptor Result;
    Result.InputType = InputType;
    Result.DeviceFamily = ClassifyInputDevice(InputType, GamepadName);
    Result.MappingName = MappingName;
    Result.Key = Key;
    Result.GlyphId = MakeGlyphId(Result.DeviceFamily, Key);
    Result.FallbackText = Key.IsValid() ? Key.GetDisplayName() : FText::GetEmpty();
    return Result;
}
