#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RBMagicTypes.h"
#include "RBMagicLibrary.generated.h"

class URBMagicSpellDefinition;

UCLASS()
class RBMAGICCORE_API URBMagicLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="RefinedBadger|Magic")
    static FGameplayTag RequestGameplayTag(FName TagName, bool bErrorIfNotFound = false);

    UFUNCTION(BlueprintCallable, Category="RefinedBadger|Magic")
    static bool BuildEffectIntents(
        const URBMagicSpellDefinition* Spell,
        const FRBMagicCastRequest& Request,
        TArray<FRBMagicEffectIntent>& OutEffects,
        FString& OutError);
};
