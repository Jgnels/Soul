using UnrealBuildTool;

public class RBAICore : ModuleRules
{
    public RBAICore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "AIModule", "NavigationSystem", "NetCore"
        });
    }
}
