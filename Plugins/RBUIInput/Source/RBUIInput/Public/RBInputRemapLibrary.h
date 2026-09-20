#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBUIInputTypes.h"
#include "RBInputRemapLibrary.generated.h"

class ULocalPlayer;

UCLASS()
class RBUIINPUT_API URBInputRemapLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="RB UI|Remap")
    static bool IsRemappingAvailable(ULocalPlayer* LocalPlayer);

    UFUNCTION(BlueprintCallable, Category="RB UI|Remap")
    static FRBRemapResult PreviewRemap(ULocalPlayer* LocalPlayer, FName MappingName, FKey NewKey);

    UFUNCTION(BlueprintCallable, Category="RB UI|Remap")
    static bool ApplyRemap(ULocalPlayer* LocalPlayer, FName MappingName, FKey NewKey, ERBInputBindingSlot Slot, bool bRejectConflicts, bool bPersist, FRBRemapResult& OutResult);

    UFUNCTION(BlueprintCallable, Category="RB UI|Remap")
    static bool ResetMappingToDefault(ULocalPlayer* LocalPlayer, FName MappingName, bool bPersist, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="RB UI|Remap")
    static bool ResetAllMappingsToDefault(ULocalPlayer* LocalPlayer, bool bPersist, FString& OutError);

    UFUNCTION(BlueprintPure, Category="RB UI|Remap")
    static TArray<FKey> GetCurrentKeysForMapping(ULocalPlayer* LocalPlayer, FName MappingName);
};
