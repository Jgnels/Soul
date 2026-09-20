using UnrealBuildTool;

public class RBRoutine : ModuleRules
{
    public RBRoutine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags", "NetCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AIModule", "NavigationSystem"
        });
    }
}
