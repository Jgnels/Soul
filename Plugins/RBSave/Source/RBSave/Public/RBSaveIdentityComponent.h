#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RBSaveIdentityComponent.generated.h"

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBSAVE_API URBSaveIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URBSaveIdentityComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category="RB Save")
    FGuid PersistentId;

    UFUNCTION(BlueprintCallable, Category="RB Save")
    FGuid GetOrCreatePersistentId();

    UFUNCTION(BlueprintPure, Category="RB Save")
    bool HasPersistentId() const { return PersistentId.IsValid(); }
};
