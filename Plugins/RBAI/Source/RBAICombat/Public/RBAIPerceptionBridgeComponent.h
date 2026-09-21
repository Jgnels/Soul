#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "RBAITypes.h"
#include "RBAIPerceptionBridgeComponent.generated.h"

class UAIPerceptionComponent;
class URBAIBrainComponent;
class URBAIThreatComponent;

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAICOMBAT_API URBAIPerceptionBridgeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAIPerceptionBridgeComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Perception", meta=(ClampMin="1.0"))
    float DistanceNormalizationCm = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Perception", meta=(ClampMin="0.0"))
    float SightThreat = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RB AI|Perception", meta=(ClampMin="0.0"))
    float HearingThreat = 4.0f;

protected:
    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Target, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleTargetPerceptionForgotten(AActor* Target);

private:
    URBAIBrainComponent* ResolveBrain() const;
    URBAIThreatComponent* ResolveThreat() const;
    void RefreshThreatenedState();
    FName MakeContextId(const AActor* Target) const;

    UPROPERTY(Transient)
    TObjectPtr<UAIPerceptionComponent> BoundPerception;

    TMap<TWeakObjectPtr<AActor>, FRBAIContext> ContextByActor;
};
