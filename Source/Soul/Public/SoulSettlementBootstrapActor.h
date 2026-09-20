#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoulSettlementBootstrapActor.generated.h"

UCLASS(Blueprintable)
class SOUL_API ASoulSettlementBootstrapActor : public AActor
{
    GENERATED_BODY()

public:
    ASoulSettlementBootstrapActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    TObjectPtr<class USoulSettlementScenarioData> Scenario;

    // Never overwrite a settlement restored by RB Save unless explicitly enabled for a test map.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Soul|Settlement")
    bool bOnlyCreateIfMissing = true;

    UFUNCTION(BlueprintCallable, Category="Soul|Settlement")
    bool ApplyScenario();

protected:
    virtual void BeginPlay() override;
};
