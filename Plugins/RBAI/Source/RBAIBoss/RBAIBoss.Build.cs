using UnrealBuildTool;

public class RBAIBoss : ModuleRules
{
    public RBAIBoss(ReadOnlyTargetRules Target) : base(Target)
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

