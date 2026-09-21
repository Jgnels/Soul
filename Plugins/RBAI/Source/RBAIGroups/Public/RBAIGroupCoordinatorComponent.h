#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "RBAIGroupCoordinatorComponent.generated.h"

class UAITokenSourceComponent;

UCLASS(ClassGroup=(RefinedBadger), meta=(BlueprintSpawnableComponent))
class RBAIGROUPS_API URBAIGroupCoordinatorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URBAIGroupCoordinatorComponent();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    bool RegisterMember(AActor* Member);

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    void UnregisterMember(AActor* Member);

    UFUNCTION(BlueprintPure, Category="RB AI|Groups")
    int32 GetMemberCount() const;

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    bool AcquireActionToken(AActor* Member, FGameplayTag TokenTag);

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    bool ReleaseActionToken(AActor* Member);

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    bool LockActionToken(AActor* Member);

    UFUNCTION(BlueprintCallable, Category="RB AI|Groups")
    bool UnlockActionToken(AActor* Member);

private:
    void PruneMembers();
    void RefreshPackSupport();

    UPROPERTY(Transient)
    TObjectPtr<UAITokenSourceComponent> TokenSource;

    TArray<TWeakObjectPtr<AActor>> Members;
};
