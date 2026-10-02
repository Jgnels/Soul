#include "Misc/AutomationTest.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulElephantBodyAnimationTest,
    "Soul.RealtimeBattle.Vertical.FullElephantAnimationCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulElephantBodyAnimationTest::RunTest(const FString&)
{
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/AfricanAnimalsPack/Elephant/Meshes/SK_Elephant.SK_Elephant"));
    if (!TestNotNull(TEXT("Full elephant body loads"), Mesh)) return false;
    const TCHAR* Clips[] = {TEXT("ANIM_Elephant_IdleBreathe"), TEXT("ANIM_Elephant_Run")};
    for (const TCHAR* Clip : Clips)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/AfricanAnimalsPack/Elephant/Animations/%s.%s"), Clip, Clip);
        auto* Animation = LoadObject<UAnimationAsset>(nullptr, *Path);
        if (!TestNotNull(Path, Animation)) continue;
        const auto* Skeleton = Animation->GetSkeleton();
        if (!TestNotNull(TEXT("Animation skeleton exists"), Skeleton)) continue;
        TestTrue(FString::Printf(TEXT("%s matches full body and parent chains"), Clip),
            Skeleton->IsCompatibleMesh(Mesh, true));
    }
    AddInfo(FString::Printf(TEXT("Full body bounds origin=%s extent=%s"),
        *Mesh->GetBounds().Origin.ToCompactString(),
        *Mesh->GetBounds().BoxExtent.ToCompactString()));
    return true;
}
#endif
