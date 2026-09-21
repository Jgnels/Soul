#include "RBAICombatExecutionBridgeComponent.h"

#include "RBAIBrainComponent.h"
#include "RBAINativeTags.h"
#include "GameFramework/Actor.h"

URBAICombatExecutionBridgeComponent::URBAICombatExecutionBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URBAICombatExecutionBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* Owner = GetOwner())
    {
        Brain = Owner->FindComponentByClass<URBAIBrainComponent>();
    }
    if (Brain)
    {
        Brain->OnActionRequested.AddDynamic(this,
            &URBAICombatExecutionBridgeComponent::HandleActionRequested);
    }
}

void URBAICombatExecutionBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Brain)
    {
        Brain->OnActionRequested.RemoveDynamic(this,
            &URBAICombatExecutionBridgeComponent::HandleActionRequested);
    }
    Super::EndPlay(EndPlayReason);
}

ERBAICombatRequestType URBAICombatExecutionBridgeComponent::MapAction(const FGameplayTag ActionTag)
{
    using namespace RBAI::Tags;
    if (ActionTag == Action_Attack) return ERBAICombatRequestType::Attack;
    if (ActionTag == Action_Pursue) return ERBAICombatRequestType::Pursue;
    if (ActionTag == Action_Flee) return ERBAICombatRequestType::Flee;
    if (ActionTag == Action_Guard) return ERBAICombatRequestType::Guard;
    if (ActionTag == Action_Flank) return ERBAICombatRequestType::Flank;
    if (ActionTag == Action_TakeCover) return ERBAICombatRequestType::TakeCover;
    if (ActionTag == Action_Investigate) return ERBAICombatRequestType::Investigate;
    return ERBAICombatRequestType::None;
}

void URBAICombatExecutionBridgeComponent::HandleActionRequested(
    const FGameplayTag ActionTag, AActor* ContextActor, const FName ContextId)
{
    const ERBAICombatRequestType Type = MapAction(ActionTag);
    if (Type == ERBAICombatRequestType::None || !Brain || !GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    PendingRequest.RequestId = FGuid::NewGuid();
    PendingRequest.Type = Type;
    PendingRequest.ActionTag = ActionTag;
    PendingRequest.TargetActor = ContextActor;
    PendingRequest.ContextId = ContextId;
    PendingRequest.DecisionRevision = Brain->Decision.Revision;
    OnCombatRequest.Broadcast(PendingRequest);
}

bool URBAICombatExecutionBridgeComponent::ResolveRequest(
    const FGuid RequestId, const bool bSucceeded)
{
    if (!Brain || !GetOwner() || !GetOwner()->HasAuthority() ||
        !RequestId.IsValid() || PendingRequest.RequestId != RequestId)
    {
        return false;
    }

    const FGuid ResolvedId = PendingRequest.RequestId;
    PendingRequest = FRBAICombatRequest{};
    Brain->NotifyActionFinished(bSucceeded);
    OnCombatRequestResolved.Broadcast(ResolvedId, bSucceeded);
    return true;
}
