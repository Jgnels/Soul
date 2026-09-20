#include "RBCombatContactNotify.h"
#include "RBCombatMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Animation/ActiveMontageInstanceScope.h"

namespace
{
int32 MontageId(const FAnimNotifyEventReference& Reference)
{
    const auto* Context = Reference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
    return Context ? Context->MontageInstanceID : INDEX_NONE;
}
URBCombatMeleeComponent* Find(USkeletalMeshComponent* Mesh)
{
    return Mesh && Mesh->GetOwner() ? Mesh->GetOwner()->FindComponentByClass<URBCombatMeleeComponent>() : nullptr;
}
}
void URBCombatContactNotify::NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration, const FAnimNotifyEventReference& Reference)
{ if (auto* Combat = Find(Mesh)) { Combat->OpenContact(MontageId(Reference)); } }
void URBCombatContactNotify::NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Seconds, const FAnimNotifyEventReference& Reference)
{ if (auto* Combat = Find(Mesh)) { Combat->SweepContact(MontageId(Reference)); } }
void URBCombatContactNotify::NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference)
{ if (auto* Combat = Find(Mesh)) { Combat->CloseContact(MontageId(Reference)); } }
