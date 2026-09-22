#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "RBMagicLibrary.h"
#include "RBMagicSpellDefinition.h"
#include "SoulRealtimeBattleRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    URBMagicSpellDefinition* LoadProofSpell(const TCHAR* Path)
    {
        return LoadObject<URBMagicSpellDefinition>(nullptr, Path);
    }

    FRBMagicCastRequest MakeUnitRequest(
        const URBMagicSpellDefinition& Spell)
    {
        FRBMagicCastRequest Request;
        Request.CastId = FGuid::NewGuid();
        Request.SpellTag = Spell.SpellTag;
        Request.Caster.Domain = TEXT("Soul.Arena");
        Request.Caster.Id = FGuid::NewGuid();
        Request.Target.Entity.Domain = TEXT("Soul.Arena");
        Request.Target.Entity.Id = FGuid::NewGuid();
        return Request;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeRBMagicProofAssetsTest,
    "Soul.RealtimeBattle.Magic.LatestProofAssetsLoad",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeRBMagicProofAssetsTest::RunTest(const FString&)
{
    auto* Firebolt = LoadProofSpell(
        TEXT("/Game/Soul/Magic/Spells/"
             "DA_Soul_Firebolt.DA_Soul_Firebolt"));
    auto* Chain = LoadProofSpell(
        TEXT("/Game/Soul/Magic/Spells/"
             "DA_Soul_ChainLightning.DA_Soul_ChainLightning"));
    auto* Blizzard = LoadProofSpell(
        TEXT("/Game/Soul/Magic/Spells/"
             "DA_Soul_Blizzard.DA_Soul_Blizzard"));

    TestNotNull(TEXT("Firebolt proof spell loads"), Firebolt);
    TestNotNull(TEXT("Chain Lightning proof spell loads"), Chain);
    TestNotNull(TEXT("Blizzard proof spell loads"), Blizzard);

    UNiagaraSystem* FireFx = LoadObject<UNiagaraSystem>(
        nullptr, TEXT("/Game/MagicSpells/Fire/FX/"
                      "NS_Fireball.NS_Fireball"));
    TestNotNull(TEXT("qualified Firebolt provider VFX loads"), FireFx);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulRealtimeRBMagicIntentTest,
    "Soul.RealtimeBattle.Magic.FireboltBuildsVolcanicIntent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulRealtimeRBMagicIntentTest::RunTest(const FString&)
{
    auto* Firebolt = LoadProofSpell(
        TEXT("/Game/Soul/Magic/Spells/"
             "DA_Soul_Firebolt.DA_Soul_Firebolt"));
    if (!TestNotNull(TEXT("Firebolt loads"), Firebolt))
        return false;

    FRBMagicCastRequest Request = MakeUnitRequest(*Firebolt);
    TArray<FRBMagicEffectIntent> Intents;
    FString Error;
    const bool bBuilt = URBMagicLibrary::BuildEffectIntents(
        Firebolt, Request, Intents, Error);

    TestTrue(TEXT("RB Magic builds Firebolt effect intents"), bBuilt);
    TestTrue(TEXT("RB Magic returns Firebolt effects"), Intents.Num() > 0);
    TestTrue(TEXT("RB Magic build error is empty"), Error.IsEmpty());

    const FSoulTerrainMagicProfile Volcanic =
        FSoulRealtimeBattleRules::MakeVolcanicMagicProfile();
    const int32 Multiplier =
        Volcanic.MultiplierFor(*Firebolt->SchoolTag.ToString());
    TestEqual(TEXT("volcanic terrain empowers Firebolt"), Multiplier, 1100);

    bool bFoundPositiveMagnitude = false;
    for (const FRBMagicEffectIntent& Intent : Intents)
    {
        if (Intent.Effect.Magnitude <= 0.0f)
            continue;
        bFoundPositiveMagnitude = true;
        const float Scaled =
            Intent.Effect.Magnitude * Multiplier / 1000.0f;
        TestTrue(
            TEXT("volcanic multiplier increases positive Firebolt magnitude"),
            Scaled > Intent.Effect.Magnitude);
        break;
    }
    TestTrue(
        TEXT("Firebolt carries at least one positive-magnitude effect"),
        bFoundPositiveMagnitude);
    return true;
}

#endif
