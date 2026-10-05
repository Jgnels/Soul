#include "Misc/AutomationTest.h"
#include "SoulBattleSpellCue.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulSpellCueGroundTest,
    "Soul.RealtimeBattle.Magic.GroundCueMatchesTargetPlane",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulSpellCueGroundTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("World"),World)) return false;
    auto* Cue=World->SpawnActor<ASoulBattleSpellCue>();
    const FVector Ground(320,-150,400);
    Cue->Blizzard(Ground,250,4);
    TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes(Cue);
    bool FoundRim=false;
    for(auto* Mesh:Meshes)
    {
        if(Mesh->GetFName()!=TEXT("SpellRibbons")) continue;
        FoundRim=true;
        TestEqual(TEXT("Frost boundary uses a bounded instance count"),Mesh->GetInstanceCount(),48);
        for(int32 I=0;I<Mesh->GetInstanceCount();++I)
        {
            FTransform Transform;
            TestTrue(TEXT("Boundary transform readable"),Mesh->GetInstanceTransform(I,Transform,true));
            TestTrue(TEXT("Frost boundary remains above ground target"),Transform.GetLocation().Z>Ground.Z);
            TestTrue(TEXT("Boundary matches the actual spell radius"),
                FMath::IsNearlyEqual(FVector::Dist2D(Transform.GetLocation(),Ground),250.0,2.0));
        }
    }
    TestTrue(TEXT("Frost boundary component exists"),FoundRim);
    World->DestroyWorld(false);
    return true;
}
#endif
