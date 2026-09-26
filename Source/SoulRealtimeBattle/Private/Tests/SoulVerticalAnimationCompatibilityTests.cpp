#include "Misc/AutomationTest.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoulVerticalKnightAnimationCompatibilityTest,
    "Soul.RealtimeBattle.Vertical.KnightUE4DwarfAnimationCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSoulVerticalKnightAnimationCompatibilityTest::RunTest(const FString&)
{
    const TCHAR* MeshPath = TEXT("/Game/Knights_Pack/Meshes/Knight_02/Mesh_UE4/Full/SK_Knight_02_Full_01.SK_Knight_02_Full_01");
    USkeletalMesh* KnightMesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
    if (!TestNotNull(TEXT("installed UE4 Knight mesh loads"), KnightMesh))
    {
        return false;
    }
    TestNotNull(TEXT("Knight mesh has a skeleton"), KnightMesh->GetSkeleton());
    const FReferenceSkeleton& MeshBones = KnightMesh->GetRefSkeleton();
    const TCHAR* Names[] = {
        TEXT("Anim_Warrior_Attack_1"),
        TEXT("Anim_Warrior_Dead_1"),
        TEXT("Anim_Warrior_Run"),
        TEXT("Anim_Warrior_Idle")
    };
    for (const TCHAR* Name : Names)
    {
        const FString AssetPath = FString::Printf(TEXT("/Game/Dwarf_Pack/Animations/1With_Weapon/%s.%s"), Name, Name);
        UAnimationAsset* Animation = LoadObject<UAnimationAsset>(nullptr, *AssetPath);
        if (!TestNotNull(FString::Printf(TEXT("existing %s loads"), Name), Animation))
        {
            continue;
        }
        const USkeleton* AnimationSkeleton = Animation->GetSkeleton();
        if (!TestNotNull(FString::Printf(TEXT("%s has a skeleton"), Name), AnimationSkeleton))
        {
            continue;
        }
        const bool bCompatible = AnimationSkeleton->IsCompatibleMesh(KnightMesh, true);
        AddInfo(FString::Printf(TEXT("SOUL_ANIMATION_COMPAT: animation=%s mesh=%s animationSkeleton=%s meshSkeleton=%s parentChainCheck=1 compatible=%d"),
            *AssetPath, MeshPath, *AnimationSkeleton->GetPathName(),
            KnightMesh->GetSkeleton() ? *KnightMesh->GetSkeleton()->GetPathName() : TEXT("None"), bCompatible));
        TestTrue(FString::Printf(TEXT("%s natively supports Knight UE4 mesh with parent chains checked"), Name), bCompatible);
        if (bCompatible)
        {
            continue;
        }
        const FReferenceSkeleton& AnimationBones = AnimationSkeleton->GetReferenceSkeleton();
        int32 Missing = 0;
        int32 DifferentParent = 0;
        for (int32 MeshIndex = 0; MeshIndex < MeshBones.GetNum(); ++MeshIndex)
        {
            const FName Bone = MeshBones.GetBoneName(MeshIndex);
            const int32 AnimationIndex = AnimationBones.FindBoneIndex(Bone);
            if (AnimationIndex == INDEX_NONE)
            {
                ++Missing;
                AddInfo(FString::Printf(TEXT("COMPAT_MISMATCH meshBone=%s absentFromAnimationSkeleton=1 meshParent=%s"),
                    *Bone.ToString(), MeshBones.GetParentIndex(MeshIndex) == INDEX_NONE ? TEXT("None") :
                    *MeshBones.GetBoneName(MeshBones.GetParentIndex(MeshIndex)).ToString()));
                continue;
            }
            const int32 MeshParentIndex = MeshBones.GetParentIndex(MeshIndex);
            const int32 AnimationParentIndex = AnimationBones.GetParentIndex(AnimationIndex);
            const FName MeshParent = MeshParentIndex == INDEX_NONE ? NAME_None : MeshBones.GetBoneName(MeshParentIndex);
            const FName AnimationParent = AnimationParentIndex == INDEX_NONE ? NAME_None : AnimationBones.GetBoneName(AnimationParentIndex);
            if (MeshParent != AnimationParent)
            {
                ++DifferentParent;
                AddInfo(FString::Printf(TEXT("COMPAT_MISMATCH bone=%s meshParent=%s animationParent=%s"),
                    *Bone.ToString(), *MeshParent.ToString(), *AnimationParent.ToString()));
            }
        }
        AddInfo(FString::Printf(TEXT("COMPAT_MISMATCH_SUMMARY meshBones=%d animationBones=%d meshBonesAbsent=%d differentParents=%d"),
            MeshBones.GetNum(), AnimationBones.GetNum(), Missing, DifferentParent));
    }
    // Asset reads only: no retargeting, skeleton mutation, or compatibility-list override.
    return true;
}

#endif
