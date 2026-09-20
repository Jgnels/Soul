#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RBSaveSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="RB Save"))
class RBSAVE_API URBSaveSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    URBSaveSettings();

    UPROPERTY(Config, EditAnywhere, Category="Backend")
    FName PreferredWorldBackend;

    UPROPERTY(Config, EditAnywhere, Category="Backend")
    bool bAllowNativeFallback = true;

    UPROPERTY(Config, EditAnywhere, Category="Safety")
    bool bRequireDomainProviders = false;

    UPROPERTY(Config, EditAnywhere, Category="Safety", meta=(ClampMin="1", ClampMax="1000"))
    int32 MaxGenerationScan = 1000;
};
