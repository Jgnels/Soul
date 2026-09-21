#include "RBAIGroupCoordinatorComponent.h"

#include "AITokenSystemLibrary.h"
#include "Component/AITokenHolderComponent.h"
#include "Component/AITokenSourceComponent.h"
#include "RBAIBrainComponent.h"
#include "RBAINativeTags.h"
#include "GameFramework/Actor.h"

URBAIGroupCoordinatorComponent::URBAIGroupCoordinatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBAIGroupCoordinatorComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        TokenSource = Owner->FindComponentByClass<UAITokenSourceComponent>();
    }
}

bool URBAIGroupCoordinatorComponent::RegisterMember(AActor* Member)
{
    if (!IsValid(Member))
    {
        return false;
    }
    PruneMembers();
    if (!Members.ContainsByPredicate([Member](const TWeakObjectPtr<AActor>& Item)
        { return Item.Get() == Member; }))
    {
        Members.Add(Member);
    }
    RefreshPackSupport();
    return true;
}

void URBAIGroupCoordinatorComponent::UnregisterMember(AActor* Member)
{
    Members.RemoveAll([Member](const TWeakObjectPtr<AActor>& Item)
        { return !Item.IsValid() || Item.Get() == Member; });
    RefreshPackSupport();
}

int32 URBAIGroupCoordinatorComponent::GetMemberCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AActor>& Item : Members)
    {
        Count += Item.IsValid() ? 1 : 0;
    }
    return Count;
}

void URBAIGroupCoordinatorComponent::PruneMembers()
{
    Members.RemoveAll([](const TWeakObjectPtr<AActor>& Item)
        { return !Item.IsValid(); });
}

void URBAIGroupCoordinatorComponent::RefreshPackSupport()
{
    PruneMembers();
    const float Support = FMath::Clamp((Members.Num() - 1) / 4.0f, 0.0f, 1.0f);
    for (const TWeakObjectPtr<AActor>& Item : Members)
    {
        if (AActor* Actor = Item.Get())
        {
            if (URBAIBrainComponent* Brain = Actor->FindComponentByClass<URBAIBrainComponent>())
            {
                Brain->SetSignal(RBAI::Tags::Signal_PackSupport, Support);
            }
        }
    }
}

bool URBAIGroupCoordinatorComponent::AcquireActionToken(AActor* Member, const FGameplayTag TokenTag)
{
    if (!TokenSource || !IsValid(Member) || !TokenTag.IsValid())
    {
        return false;
    }
    UAITokenHolderComponent* Holder = nullptr;
    return UAITokenSystemLibrary::GetAITokenHolderComponentFromActor(Member, Holder) &&
        UAITokenSystemLibrary::AcquireAITokenFromSourceComponent(Holder, TokenSource, TokenTag);
}

bool URBAIGroupCoordinatorComponent::ReleaseActionToken(AActor* Member)
{
    UAITokenHolderComponent* Holder = nullptr;
    return IsValid(Member) &&
        UAITokenSystemLibrary::GetAITokenHolderComponentFromActor(Member, Holder) &&
        UAITokenSystemLibrary::ReleaseAITokenToSourceComponent(Holder);
}

bool URBAIGroupCoordinatorComponent::LockActionToken(AActor* Member)
{
    UAITokenHolderComponent* Holder = nullptr;
    return IsValid(Member) &&
        UAITokenSystemLibrary::GetAITokenHolderComponentFromActor(Member, Holder) &&
        UAITokenSystemLibrary::LockHeldToken(Holder);
}

bool URBAIGroupCoordinatorComponent::UnlockActionToken(AActor* Member)
{
    UAITokenHolderComponent* Holder = nullptr;
    return IsValid(Member) &&
        UAITokenSystemLibrary::GetAITokenHolderComponentFromActor(Member, Holder) &&
        UAITokenSystemLibrary::UnlockHeldToken(Holder);
}
