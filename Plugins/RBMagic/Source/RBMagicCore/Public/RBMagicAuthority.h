#pragma once

#include "CoreMinimal.h"
#include "RBMagicTypes.h"

class URBMagicSpellDefinition;

class RBMAGICCORE_API IRBMagicAuthority
{
public:
    virtual ~IRBMagicAuthority() = default;

    virtual bool CanCastMagic(
        const URBMagicSpellDefinition& Spell,
        const FRBMagicCastRequest& Request,
        FString& OutError) const = 0;

    // Host must validate and commit resource spend + gameplay effects atomically.
    // A false return must leave durable gameplay truth unchanged.
    virtual bool TryCommitMagicCast(
        const URBMagicSpellDefinition& Spell,
        const FRBMagicCastRequest& Request,
        TConstArrayView<FRBMagicEffectIntent> Effects,
        FString& OutError) = 0;
};
