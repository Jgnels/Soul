#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulSettlementPresentationController.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulSettlementPresentationController : public AActor
{
    GENERATED_BODY()

public:
    ASoulSettlementPresentationController();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    FName SettlementId;

    // Explicit donor convention. Convert only this loaded world instance when
    // Soul uses legacy luminance bounds; never change global renderer settings.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement|Presentation")
    bool bAuthoredEV100Exposure = false;

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    bool RefreshSettlementPresentation();

protected:
    virtual void BeginPlay() override;

private:
    void NormalizeAuthoredExposure();
    class USoulSettlementStateSubsystem* ResolveState() const;
};
