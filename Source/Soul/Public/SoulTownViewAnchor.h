#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulTownViewAnchor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulTownViewAnchor : public AActor
{
    GENERATED_BODY()

public:
    ASoulTownViewAnchor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Town View")
    FName SettlementId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Soul|Town View")
    TObjectPtr<class UCameraComponent> Camera;

    UFUNCTION(BlueprintCallable, Category="Soul|Town View")
    bool ActivateForPlayer(
        class APlayerController* PlayerController,
        float BlendTime = 0.35f);
};
