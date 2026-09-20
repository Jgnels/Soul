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

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    bool RefreshSettlementPresentation();

protected:
    virtual void BeginPlay() override;

private:
    class USoulSettlementStateSubsystem* ResolveState() const;
};
