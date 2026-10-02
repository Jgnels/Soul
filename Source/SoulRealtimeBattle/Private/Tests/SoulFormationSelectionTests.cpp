#include "Misc/AutomationTest.h"
#include "SoulRealtimeBattleArena.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulFormationSelectionTest,
    "Soul.RealtimeBattle.Controls.StableFormationSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulFormationSelectionTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Host=World->SpawnActor<ASoulRealtimeArenaGameMode>();
    for(int32 I=0;I<3;++I)
    {
        FSoulBattleFormationState State;
        State.GroupIndex=I; State.Side=0; State.InitialBodies=5;
        State.Kind=static_cast<ESoulBattleFormationKind>(I);
        Host->TacticalFormations.Add(State);
        FSoulRealtimeArenaCombatant Unit;
        Unit.GroupIndex=I;Unit.Side=0;Unit.Health=100;
        Host->Combatants.Add(Unit);
    }
    Host->SelectAlliedFormationSlot(1);
    TestEqual(TEXT("F2 selects missile group"),Host->SelectedAlliedFormation,1);
    Host->Combatants[0].Health=0;
    TestEqual(TEXT("Casualties preserve all formation cards"),Host->AlliedFormationCount(),3);
    TestTrue(TEXT("F1 remains defeated front line"),Host->AlliedFormationSummary(0).Contains(TEXT("DEFEATED")));
    Host->SelectAlliedFormationSlot(1);
    TestEqual(TEXT("F2 never slides to strike after frontline dies"),Host->SelectedAlliedFormation,1);
    Host->SelectAlliedFormationSlot(2);
    TestEqual(TEXT("F3 remains strike"),Host->SelectedAlliedFormation,2);
    Host->SelectAlliedFormationSlot(0);
    TestEqual(TEXT("Defeated card never selects another group"),Host->SelectedAlliedFormation,0);
    World->DestroyWorld(false);
    return true;
}
#endif
