using UnrealBuildTool;

public class RBAICombat : ModuleRules
{
    public RBAICombat(ReadOnlyTargetRules Target) : base(Target)
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
            "AIModule"
        });
    }
}

