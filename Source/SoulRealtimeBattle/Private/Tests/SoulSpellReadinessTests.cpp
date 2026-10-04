#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "RBMagicSpellDefinition.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSpellReadinessTest,
    "Soul.RealtimeBattle.Controls.SpellReadinessAndSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSpellReadinessTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("World"),World)) return false;
    auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    Context.OwningGameInstance=NewObject<UGameInstance>(GEngine);
    World->SetGameInstance(Context.OwningGameInstance);
    World->GetWorldSettings()->DefaultGameMode=ASoulRealtimeArenaGameMode::StaticClass();
    World->SetGameMode(FURL());
    auto* Host=World->GetAuthGameMode<ASoulRealtimeArenaGameMode>();
    if(!TestNotNull(TEXT("Authority installed"),Host))
    { GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false; }
    FSoulRealtimeArenaCombatant Unit;
    Unit.Id=FGuid::NewGuid();Unit.Health=100;Unit.bPlayerHero=true;
    Host->Combatants.Add(Unit);
    Host->PlayerHero=World->SpawnActor<ACharacter>();
    Host->Actors.Add(Host->PlayerHero);
    auto* Binding=NewObject<USoulRealtimeArenaBinding>(Host->PlayerHero);
    Host->PlayerHero->AddInstanceComponent(Binding);Binding->RegisterComponent();
    TestTrue(TEXT("Bind hero"),Binding->BindCombatant(Host->IdentityAt(0).Core()));
    auto* Spell=LoadObject<URBMagicSpellDefinition>(nullptr,
        TEXT("/Game/Soul/Magic/Spells/DA_Soul_Firebolt.DA_Soul_Firebolt"));
    if(!TestNotNull(TEXT("Existing Firebolt definition"),Spell))
    { GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false; }
    Host->BattleSpells.Add(Spell);
    const float Cost=Host->SpellManaCost(*Spell);
    Host->PlayerMana=Cost;Host->bBattlePaused=false;
    TestTrue(TEXT("Exact required mana enables targeting"),Host->SpellBlockReason(0).IsEmpty());
    Host->SelectPlayerSpell(0);
    TestEqual(TEXT("Select readied spell"),Host->SelectedSpellSlot,0);
    TestEqual(TEXT("Selection never spends mana"),Host->PlayerMana,Cost);
    Host->PlayerMana=Cost-1;
    Host->SelectPlayerSpell(0);
    TestEqual(TEXT("Unaffordable spell disarms old target"),Host->SelectedSpellSlot,INDEX_NONE);
    TestTrue(TEXT("Explains missing mana"),Host->Status.Contains(TEXT("mana")));
    Host->PlayerMana=Cost;
    Host->SpellCooldowns.Add(Spell->SpellTag.GetTagName(),1.1f);
    TestTrue(TEXT("Cooldown rounds up for UI"),Host->SpellButtonLabel(0).Contains(TEXT("2s")));
    Host->SelectPlayerSpell(0);
    TestEqual(TEXT("Cooldown prevents arming"),Host->SelectedSpellSlot,INDEX_NONE);
    Host->SpellCooldowns.Reset();Host->bBattlePaused=true;
    TestTrue(TEXT("Pause explains cast block"),Host->SpellBlockReason(0).Contains(TEXT("Resume")));
    Host->SelectPlayerSpell(0);
    TestEqual(TEXT("Paused target selection remains available"),Host->SelectedSpellSlot,0);
    TestEqual(TEXT("Paused selection has no cast"),Host->MagicCastCount(),0);
    Host->SelectPlayerSpell(-1);
    TestEqual(TEXT("Invalid slot cancels without selecting Firebolt"),Host->SelectedSpellSlot,INDEX_NONE);
    Host->HandleGamepadAction(TEXT("NextSpell"));
    TestEqual(TEXT("Controller selects first spell without casting"),Host->SelectedSpellSlot,0);
    TestTrue(TEXT("Controller explains pause"),Host->Status.Contains(TEXT("Resume")));
    Host->HandleGamepadAction(TEXT("Cast"));
    TestEqual(TEXT("Paused controller cast spends nothing"),Host->PlayerMana,Cost);
    Host->bBattlePaused=false;Host->PlayerMana=Cost-1;
    Host->HandleGamepadAction(TEXT("Cast"));
    TestTrue(TEXT("Controller explains insufficient mana"),Host->Status.Contains(TEXT("mana")));
    TestEqual(TEXT("Rejected controller cast does not commit"),Host->MagicCastCount(),0);
    Host->PlayerMana=100;
    Host->bBattlePaused=false;
    Host->HandleBattleAction(TEXT("Grimoire"));
    TestTrue(TEXT("Book opens"),Host->bSpellbookOpen);
    TestFalse(TEXT("Book never pauses battle"),Host->bBattlePaused);
    TestEqual(TEXT("Book clears stale targeting"),Host->SelectedSpellSlot,INDEX_NONE);
    Host->HandleBattleAction(TEXT("CloseGrimoire"));
    TestFalse(TEXT("Close returns to world"),Host->bSpellbookOpen);
    TestFalse(TEXT("Closing does not pause"),Host->bBattlePaused);
    Host->bBattlePaused=true;
    Host->HandleBattleAction(TEXT("Grimoire"));
    TestTrue(TEXT("Opening keeps manual pause"),Host->bBattlePaused);
    Host->HandleBattleAction(TEXT("PauseRule"));
    TestFalse(TEXT("Deployment can disable pause"),Host->bAllowTacticalPause);
    Host->BattleElapsed=1;
    Host->HandleBattleAction(TEXT("PauseRule"));
    TestFalse(TEXT("Pause rule locked after deployment"),Host->bAllowTacticalPause);
    Host->bBattlePaused=false;
    Host->ToggleBattlePause();
    TestFalse(TEXT("No-pause mode rejects tactical pause"),Host->bBattlePaused);
    Host->bAllowTacticalPause=true;
    const TCHAR* MoreSpells[]={TEXT("ChainLightning"),TEXT("Blizzard"),TEXT("TidalWard"),TEXT("Tailwind")};
    for(const TCHAR* Name:MoreSpells)
        Host->BattleSpells.Add(LoadObject<URBMagicSpellDefinition>(nullptr,
            *FString::Printf(TEXT("/Game/Soul/Magic/Spells/DA_Soul_%s.DA_Soul_%s"),Name,Name)));
    for(int32 Slot=0;Slot<5;++Slot)
    {
        Host->bBattlePaused=true;
        Host->bSpellbookOpen=true;
        Host->HandleBattleAction(FName(*FString::Printf(TEXT("BookSpell%d"),Slot)));
        TestEqual(TEXT("Every book entry arms correct spell"),Host->SelectedSpellSlot,Slot);
        TestFalse(TEXT("Selection closes book"),Host->bSpellbookOpen);
        TestTrue(TEXT("Selection preserves manual pause"),Host->bBattlePaused);
        TestEqual(TEXT("Selection spends no mana"),Host->PlayerMana,100.f);
        TestEqual(TEXT("No selected spell casts itself"),Host->MagicCastCount(),0);
    }
    Host->bBattlePaused=false;
    Host->HandleBattleAction(TEXT("Spell3"));
    TestEqual(TEXT("Quick ward also requires confirmation"),Host->SelectedSpellSlot,3);
    TestEqual(TEXT("Quick slot never auto-casts"),Host->MagicCastCount(),0);
    Host->PlayerMana=Cost;
    Host->Combatants[0].Health=0;
    Host->SelectPlayerSpell(0);
    TestTrue(TEXT("Fallen hero explained"),Host->Status.Contains(TEXT("fallen")));
    Host->Combatants[0].Health=100;Host->bFinished=true;
    TestTrue(TEXT("Resolution disables availability"),Host->SpellBlockReason(0).Contains(TEXT("resolved")));
    TestEqual(TEXT("All rejected selections preserve mana"),Host->PlayerMana,Cost);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return true;
}
#endif
