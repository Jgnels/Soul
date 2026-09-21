using UnrealBuildTool;

public class RBAIGroups : ModuleRules
{
    public RBAIGroups(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "RBAICore",
            "AITokenCore"
        });
    }
}

