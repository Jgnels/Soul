#pragma once

#include "CoreMinimal.h"
#include "Scene/TCATInfluenceVolume.h"
#include "RBPBILInfluenceVolume.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class ERBPBILRefreshPolicy : uint8
{
    Adaptive,
    CPU,
    GPU
};

UCLASS()
class RBPBIL_API ARBPBILInfluenceVolume : public ATCATInfluenceVolume
{
    GENERATED_BODY()

public:
    ARBPBILInfluenceVolume();

    UFUNCTION(CallInEditor, BlueprintCallable, Category="RB PBIL")
    void ApplyPBILPreset();

    /** Configure a deferred actor before FinishSpawning. Existing assets retain adaptive refresh. */
    UFUNCTION(BlueprintCallable, Category="RB PBIL")
    bool SetRefreshPolicy(ERBPBILRefreshPolicy Policy);

    /** Runtime domain without a BSP brush. Call before BeginPlay. */
    UFUNCTION(BlueprintCallable, Category="RB PBIL")
    bool ConfigureRuntimeBounds(FVector HalfExtents, float InCellSize = 150.0f);

    UFUNCTION(BlueprintPure, Category="RB PBIL")
    bool IsGPURefreshEnabled() const { return bRefreshWithGPU; }

    UFUNCTION(BlueprintPure, Category="RB PBIL")
    bool IsAdaptiveRefreshEnabled() const { return bAdaptivelySwitchRefreshMode; }

    UFUNCTION(BlueprintPure, Category="RB PBIL")
    int32 GetPBILChannelCount() const { return BaseLayerConfigs.Num(); }

private:
    UPROPERTY(Transient)
    TObjectPtr<UBoxComponent> RuntimeBounds;
};
