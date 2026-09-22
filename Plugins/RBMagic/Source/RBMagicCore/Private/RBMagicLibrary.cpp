#include "RBMagicLibrary.h"
#include "RBMagicSpellDefinition.h"
#include "GameplayTagsManager.h"

FGameplayTag URBMagicLibrary::RequestGameplayTag(FName TagName, bool bErrorIfNotFound)
{
    if (TagName.IsNone())
    {
        return FGameplayTag();
    }

    return UGameplayTagsManager::Get().RequestGameplayTag(TagName, bErrorIfNotFound);
}

bool URBMagicLibrary::BuildEffectIntents(
    const URBMagicSpellDefinition* Spell,
    const FRBMagicCastRequest& Request,
    TArray<FRBMagicEffectIntent>& OutEffects,
    FString& OutError)
{
    OutEffects.Reset();
    OutError.Reset();

    if (!Spell)
    {
        OutError = TEXT("Spell definition is null.");
        return false;
    }

    if (!Spell->SpellTag.IsValid() || Request.SpellTag != Spell->SpellTag)
    {
        OutError = TEXT("Cast request does not match a valid spell tag.");
        return false;
    }

    if (!Request.CastId.IsValid() || !Request.Caster.IsValid())
    {
        OutError = TEXT("Cast request is missing a valid cast id or caster.");
        return false;
    }

    FRBMagicTarget NormalizedTarget = Request.Target;

    switch (Spell->TargetMode)
    {
    case ERBMagicTargetMode::Self:
        NormalizedTarget.Entity = Request.Caster;
        break;
    case ERBMagicTargetMode::Unit:
        if (!NormalizedTarget.Entity.IsValid())
        {
            OutError = TEXT("Unit-targeted spell requires a valid target entity.");
            return false;
        }
        break;

    case ERBMagicTargetMode::Ground:
        if (NormalizedTarget.WorldLocation.ContainsNaN())
        {
            OutError = TEXT("Ground target contains invalid coordinates.");
            return false;
        }
        break;
    case ERBMagicTargetMode::Direction:
        if (NormalizedTarget.Direction.ContainsNaN() || NormalizedTarget.Direction.IsNearlyZero())
        {
            OutError = TEXT("Directional spell requires a valid direction.");
            return false;
        }
        NormalizedTarget.Direction.Normalize();
        break;
    case ERBMagicTargetMode::Global:
        break;
    default:
        OutError = TEXT("Unsupported target mode.");
        return false;
    }

    for (const FRBMagicEffectSpec& Effect : Spell->Effects)
    {
        if (!Effect.EffectTag.IsValid())
        {
            OutError = TEXT("Spell contains an effect without a valid effect tag.");
            OutEffects.Reset();
            return false;
        }

        FRBMagicEffectIntent& Intent = OutEffects.AddDefaulted_GetRef();
        Intent.CastId = Request.CastId;
        Intent.SpellTag = Spell->SpellTag;
        Intent.Caster = Request.Caster;
        Intent.Target = NormalizedTarget;
        Intent.Effect = Effect;
    }

    return true;
}
