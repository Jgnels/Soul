#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBUIInputTypes.h"
#include "RBInputPromptLibrary.generated.h"

UCLASS()
class RBUIINPUT_API URBInputPromptLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="RB UI|Glyphs")
    static ERBInputDeviceFamily ClassifyInputDevice(ECommonInputType InputType, FName GamepadName);

    UFUNCTION(BlueprintPure, Category="RB UI|Glyphs")
    static FName MakeGlyphId(ERBInputDeviceFamily DeviceFamily, FKey Key);

    UFUNCTION(BlueprintPure, Category="RB UI|Glyphs")
    static FRBInputGlyphDescriptor MakeGlyphDescriptor(ECommonInputType InputType, FName GamepadName, FName MappingName, FKey Key);
};
