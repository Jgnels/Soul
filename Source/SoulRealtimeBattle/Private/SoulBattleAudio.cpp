#include "SoulRealtimeBattleArena.h"
#include "RBCombatActions.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void ASoulRealtimeArenaGameMode::SetupBattleAudio()
{
    // Small, local derivative clips. Missing presentation never changes combat.
    static const TCHAR* Paths[] = {
        TEXT("/Game/Soul/Audio/Battle/SwordSwing1.SwordSwing1"),
        TEXT("/Game/Soul/Audio/Battle/SwordSwing2.SwordSwing2"),
        TEXT("/Game/Soul/Audio/Battle/SwordHit1.SwordHit1"),
        TEXT("/Game/Soul/Audio/Battle/SwordHit2.SwordHit2"),
        TEXT("/Game/Soul/Audio/Battle/BowRelease1.BowRelease1"),
        TEXT("/Game/Soul/Audio/Battle/BowRelease2.BowRelease2"),
        TEXT("/Game/Soul/Audio/Battle/ArrowHit.ArrowHit"),
        TEXT("/Game/Soul/Audio/Battle/Firebolt.Firebolt"),
        TEXT("/Game/Soul/Audio/Battle/Blizzard.Blizzard"),
        TEXT("/Game/Soul/Audio/Battle/MagicImpact.MagicImpact"),
        TEXT("/Game/Soul/Audio/Battle/TidalWard.TidalWard")
    };
    BattleSounds.Reset();
    for (const TCHAR* Path : Paths)
        BattleSounds.Add(LoadObject<USoundWave>(nullptr, Path, nullptr, LOAD_NoWarn));
    BattleAudioRequests.SetNumZeroed(UE_ARRAY_COUNT(Paths));
    TroopSoundConcurrency = NewObject<USoundConcurrency>(this);
    TroopSoundConcurrency->Concurrency.MaxCount = 8;
    TroopSoundConcurrency->Concurrency.bLimitToOwner = false;
    TroopSoundConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopFarthestThenOldest;
    TroopSoundConcurrency->Concurrency.VoiceStealReleaseTime = 0.05f;
    SpellSoundConcurrency = NewObject<USoundConcurrency>(this);
    SpellSoundConcurrency->Concurrency = TroopSoundConcurrency->Concurrency;
    SpellSoundConcurrency->Concurrency.MaxCount = 3;
    BattleSoundAttenuation = NewObject<USoundAttenuation>(this);
    auto& Settings = BattleSoundAttenuation->Attenuation;
    Settings.bAttenuate = true;
    Settings.bSpatialize = true;
    Settings.AttenuationShape = EAttenuationShape::Sphere;
    Settings.AttenuationShapeExtents = FVector(900.f, 0, 0);
    Settings.FalloffDistance = 6500.f;
    Settings.bEnableOcclusion = false; // No per-voice traces across dozens of combatants.
    UE_LOG(LogTemp, Display, TEXT("SOUL_BATTLE_AUDIO_READY: loaded=%d total=%d troopLimit=8 spellLimit=3"),
        BattleSounds.FilterByPredicate([](const auto& Sound){ return Sound != nullptr; }).Num(), BattleSounds.Num());
}

void ASoulRealtimeArenaGameMode::PlayBattleSound(ESoulBattleSound Sound, const FVector& Location, int32 Variation)
{
    const int32 Slot = static_cast<int32>(Sound);
    if (bFinished || bBattlePaused || Location.ContainsNaN() || !GetWorld() ||
        !BattleSounds.IsValidIndex(Slot) || !BattleSounds[Slot]) return;
    const bool Spell = Slot >= static_cast<int32>(ESoulBattleSound::Firebolt);
    ++BattleAudioRequests[Slot];
    // Own a shared concurrency group per category, not per actor or clip.
    // These are world sounds, so UE pauses/stops them with the battle world.
    auto* Audio = UGameplayStatics::SpawnSoundAtLocation(this, BattleSounds[Slot], Location,
        FRotator::ZeroRotator, Spell ? 0.55f : 0.45f,
        0.97f + 0.02f * (FMath::Abs(Variation % 4)), 0.f, BattleSoundAttenuation,
        Spell ? SpellSoundConcurrency.Get() : TroopSoundConcurrency.Get(), true);
    if (bControlDiagnostics && BattleAudioRequests[Slot] == 1)
        UE_LOG(LogTemp, Display, TEXT("SOUL_BATTLE_AUDIO_EVENT: clip=%s component=%d"),
            *BattleSounds[Slot]->GetName(), Audio != nullptr);
}

void ASoulRealtimeArenaGameMode::PresentBowLaunch(const FRBProjectileLaunch& Launch)
{
    // Called once when a successfully launched RB projectile first ticks.
    if (!Launch.IsValid() || Index(FRBHostIdentity::From(Launch.Source)) == INDEX_NONE) return;
    const int32 Variation = static_cast<int32>(Launch.ShotId.A & 3);
    PlayBattleSound((Variation & 1) ? ESoulBattleSound::BowRelease2 : ESoulBattleSound::BowRelease1,
        Launch.Position, Variation);
}
