#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "RBMagicLibrary.h"
#include "RBMagicSpellDefinition.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RBMagicTestSpell, "RBMagic.Test.Spell");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RBMagicTestEffect, "RBMagic.Test.Effect");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRBMagicBuildEffectIntentsTest,
    "RefinedBadger.Magic.Core.BuildEffectIntents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRBMagicBuildEffectIntentsTest::RunTest(const FString& Parameters)
{
    URBMagicSpellDefinition* Spell = NewObject<URBMagicSpellDefinition>();
    Spell->SpellTag = TAG_RBMagicTestSpell;
    Spell->TargetMode = ERBMagicTargetMode::Unit;

    FRBMagicEffectSpec Effect;
    Effect.EffectTag = TAG_RBMagicTestEffect;
    Effect.Magnitude = 25.0f;
    Spell->Effects.Add(Effect);

    FRBMagicCastRequest Request;
    Request.CastId = FGuid::NewGuid();
    Request.SpellTag = TAG_RBMagicTestSpell;
    Request.Caster.Domain = TEXT("Test");
    Request.Caster.Id = FGuid::NewGuid();
    Request.Target.Entity.Domain = TEXT("Test");
    Request.Target.Entity.Id = FGuid::NewGuid();

    TArray<FRBMagicEffectIntent> Intents;
    FString Error;
    const bool bBuilt = URBMagicLibrary::BuildEffectIntents(Spell, Request, Intents, Error);

    TestTrue(TEXT("Valid unit cast builds intents"), bBuilt);
    TestTrue(TEXT("Valid unit cast has no error"), Error.IsEmpty());
    TestEqual(TEXT("One effect produces one intent"), Intents.Num(), 1);

    if (Intents.Num() == 1)
    {
        TestEqual(TEXT("Intent carries cast id"), Intents[0].CastId, Request.CastId);
        TestTrue(TEXT("Intent carries spell tag"), Intents[0].SpellTag == TAG_RBMagicTestSpell.GetTag());
        TestTrue(TEXT("Intent carries effect tag"), Intents[0].Effect.EffectTag == TAG_RBMagicTestEffect.GetTag());
        TestEqual(TEXT("Intent preserves target"), Intents[0].Target.Entity.Id, Request.Target.Entity.Id);
    }

    Request.Target.Entity = FRBMagicEntityRef();
    Intents.Reset();
    Error.Reset();
    const bool bRejected = URBMagicLibrary::BuildEffectIntents(Spell, Request, Intents, Error);
    TestFalse(TEXT("Unit cast rejects missing target"), bRejected);
    TestTrue(TEXT("Rejected cast returns an error"), !Error.IsEmpty());

    Spell->TargetMode = ERBMagicTargetMode::Self;
    Request.Target = FRBMagicTarget();
    Intents.Reset();
    Error.Reset();
    const bool bSelfBuilt = URBMagicLibrary::BuildEffectIntents(Spell, Request, Intents, Error);

    TestTrue(TEXT("Self cast builds without an explicit target"), bSelfBuilt);
    TestEqual(TEXT("Self cast produces one intent"), Intents.Num(), 1);
    if (Intents.Num() == 1)
    {
        TestEqual(TEXT("Self cast normalizes target to caster"), Intents[0].Target.Entity.Id, Request.Caster.Id);
    }

    return true;
}

#endif
