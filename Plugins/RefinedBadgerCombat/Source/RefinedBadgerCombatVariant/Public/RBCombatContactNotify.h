#pragma once
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "RBCombatContactNotify.generated.h"

UCLASS(meta = (DisplayName = "RB Combat Contact"))
class REFINEDBADGERCOMBATVARIANT_API URBCombatContactNotify : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration, const FAnimNotifyEventReference& Reference) override;
    virtual void NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Seconds, const FAnimNotifyEventReference& Reference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& Reference) override;
};
