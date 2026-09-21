using UnrealBuildTool;

public class RBAICreatures : ModuleRules
{
    public RBAICreatures(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "RBAICore"
        });
    }
}

