#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoulBattlePresentationAssets,
    "Soul.RealtimeBattle.Vertical.CombatPresentationAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoulBattlePresentationAssets::RunTest(const FString&)
{
    const auto Check = [this](const TCHAR* MeshPath, const FString& Folder,
        const TArray<FString>& Names)
    {
        auto* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
        if (!TestNotNull(MeshPath, Mesh)) return;
        for (const FString& Name : Names)
        {
            const FString Path = Folder / (Name + TEXT(".") + Name);
            auto* Clip = LoadObject<UAnimSequence>(nullptr, *Path);
            if (!TestNotNull(Path, Clip)) continue;
            if (!TestNotNull(TEXT("Animation skeleton"), Clip->GetSkeleton())) continue;
            TestTrue(Path + TEXT(" compatible with ") + Mesh->GetName(),
                Clip->GetSkeleton()->IsCompatibleMesh(Mesh, true));
            FTransform Start, Finish;
            Clip->GetBoneTransform(Start, FSkeletonPoseBoneIndex(0), FAnimExtractContext(0.0), true);
            Clip->GetBoneTransform(Finish, FSkeletonPoseBoneIndex(0), FAnimExtractContext(Clip->GetPlayLength()), true);
            AddInfo(FString::Printf(TEXT("CLIP %s duration=%.2f rootStart=%s rootEnd=%s"),
                *Name, Clip->GetPlayLength(), *Start.GetLocation().ToCompactString(), *Finish.GetLocation().ToCompactString()));
        }
    };
    const FString Warrior = TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon");
    const TArray<FString> WarriorClips = {TEXT("Anim_Warrior_Idle"), TEXT("Anim_Warrior_Run"),
        TEXT("Anim_Warrior_Attack_1"), TEXT("Anim_Warrior_Dead_1")};
    Check(TEXT("/Game/Knights_Pack/Meshes/Knight_03/Mesh_UE4/Full/SK_Knight_03_Full_01.SK_Knight_03_Full_01"), Warrior, WarriorClips);
    Check(TEXT("/Game/Knights_Pack/Meshes/Knight_04/Mesh_UE4/Full_Mesh/SK_Knight_04_Full_01.SK_Knight_04_Full_01"), Warrior, WarriorClips);
    const FString Sparrow = TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Animations");
    const TArray<FString> BowClips = {TEXT("idle"), TEXT("Jog_Fwd"),
        TEXT("RMB_Drawback"), TEXT("RMB_Fire"), TEXT("Death_Fwd")};
    Check(TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Meshes/Sparrow.Sparrow"), Sparrow, BowClips);
    Check(TEXT("/Game/ParagonSparrow/Characters/Heroes/Sparrow/Skins/Raven/Meshes/Sparrow_Raven.Sparrow_Raven"), Sparrow, BowClips);
    Check(TEXT("/Game/AfricanAnimalsPack/Elephant/Meshes/SK_Elephant.SK_Elephant"),
        TEXT("/Game/AfricanAnimalsPack/Elephant/Animations"),
        {TEXT("ANIM_Elephant_Run"), TEXT("ANIM_Elephant_TusksAttack1"), TEXT("ANIM_Elephant_Death")});
    Check(TEXT("/Game/Kraken/Meshes/KRAKEN.KRAKEN"), TEXT("/Game/Kraken/Animations"),
        {TEXT("KRAKEN_walk"), TEXT("KRAKEN_sweepAttack"), TEXT("KRAKEN_death")});
    return true;
}
#endif

