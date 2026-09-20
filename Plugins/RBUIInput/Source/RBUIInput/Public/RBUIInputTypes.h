#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "InputCoreTypes.h"
#include "RBUIInputTypes.generated.h"

UENUM(BlueprintType)
enum class ERBInputDeviceFamily : uint8
{
    KeyboardMouse,
    XboxStyle,
    PlayStationStyle,
    GenericGamepad,
    Touch,
    Unknown
};

UENUM(BlueprintType)
enum class ERBInputBindingSlot : uint8
{
    Primary,
    Secondary,
    Tertiary,
    Quaternary
};

UENUM(BlueprintType)
enum class ERBRemapStatus : uint8
{
    Success,
    Cancelled,
    InvalidRequest,
    UserSettingsUnavailable,
    Conflict,
    EngineRejected
};

USTRUCT(BlueprintType)
struct RBUIINPUT_API FRBInputGlyphDescriptor
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    ECommonInputType InputType = ECommonInputType::MouseAndKeyboard;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    ERBInputDeviceFamily DeviceFamily = ERBInputDeviceFamily::Unknown;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    FName MappingName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    FKey Key;

    // Stable project-facing identifier. RB UI intentionally does not bundle trademarked platform glyph art.
    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    FName GlyphId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Glyphs")
    FText FallbackText;
};

USTRUCT(BlueprintType)
struct RBUIINPUT_API FRBRemapResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Remap")
    ERBRemapStatus Status = ERBRemapStatus::InvalidRequest;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Remap")
    FName MappingName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Remap")
    FKey NewKey;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Remap")
    TArray<FName> ConflictingMappings;

    UPROPERTY(BlueprintReadOnly, Category="RB UI|Remap")
    FString Error;
};

USTRUCT(BlueprintType)
struct RBUIINPUT_API FRBUIAccessibilityPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB UI|Accessibility", meta=(ClampMin="0.75", ClampMax="2.0"))
    float TextScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB UI|Accessibility")
    bool bReducedMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB UI|Accessibility")
    bool bVibrationEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB UI|Accessibility")
    bool bHoldActionsUseToggle = false;
};
