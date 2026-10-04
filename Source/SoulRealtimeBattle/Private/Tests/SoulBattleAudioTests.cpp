#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattleAudioTest,
    "Soul.RealtimeBattle.Presentation.BoundedAudioAcceptedEvents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattleAudioTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("World"), World)) return false;
    auto* Host = World->SpawnActor<ASoulRealtimeArenaGameMode>();
    if (!TestNotNull(TEXT("Battle host"), Host)) { World->DestroyWorld(false); return false; }
    Host->SetupBattleAudio();
    TestEqual(TEXT("Bounded sound palette"), Host->BattleSounds.Num(), 11);
    for (const auto& Sound : Host->BattleSounds)
        if (TestNotNull(TEXT("Imported clip"), Sound.Get()))
        {
            TestTrue(TEXT("Short finite clip"), Sound->GetDuration()>0 && Sound->GetDuration()<8);
            TestEqual(TEXT("Spatial mono clip"), Sound->NumChannels, 1);
        }
    TestEqual(TEXT("Shared troop voice cap"), Host->TroopSoundConcurrency->Concurrency.MaxCount, 8);
    TestEqual(TEXT("Separate spell voice cap"), Host->SpellSoundConcurrency->Concurrency.MaxCount, 3);
    TestFalse(TEXT("Cap is shared across soldiers"), Host->TroopSoundConcurrency->Concurrency.bLimitToOwner);
    TestTrue(TEXT("Attenuated world audio"), Host->BattleSoundAttenuation->Attenuation.bAttenuate);
    FSoulRealtimeArenaCombatant A,B;
    A.Id=FGuid::NewGuid(); B.Id=FGuid::NewGuid(); B.Side=1;
    Host->Combatants={A,B};
    FRBHostHit Hit;
    Hit.ContactId=FGuid::NewGuid();Hit.Attacker=Host->IdentityAt(0);Hit.Victim=Host->IdentityAt(1);
    Hit.bHasExactImpact=true;Hit.AcceptedDamage=1;Hit.Weapon.Category=TEXT("Bow");
    FString Error;
    TestTrue(TEXT("Accepted arrow contact"),Host->CommitHit(Hit,Error));
    const int32 Arrow=static_cast<int32>(ESoulBattleSound::ArrowHit);
    TestEqual(TEXT("Accepted hit queues one audio event"),Host->BattleAudioRequests[Arrow],1);
    TestFalse(TEXT("Replay rejected"),Host->CommitHit(Hit,Error));
    Hit.ContactId=FGuid::NewGuid();Host->Combatants[1].Side=0;
    TestFalse(TEXT("Allied hit rejected"),Host->CommitHit(Hit,Error));
    TestEqual(TEXT("Invalid hits stay silent"),Host->BattleAudioRequests[Arrow],1);
    Host->Combatants[1].Side=1;Host->bBattlePaused=true;
    TestFalse(TEXT("Paused contact rejected"),Host->CommitHit(Hit,Error));
    Host->PlayBattleSound(ESoulBattleSound::Firebolt,FVector::ZeroVector);
    TestEqual(TEXT("Paused cast silent"),Host->BattleAudioRequests[7],0);
    Host->bBattlePaused=false;Host->bFinished=true;
    Host->PlayBattleSound(ESoulBattleSound::Firebolt,FVector::ZeroVector);
    TestEqual(TEXT("Resolved battle silent"),Host->BattleAudioRequests[7],0);
    World->DestroyWorld(false);
    return true;
}
#endif
