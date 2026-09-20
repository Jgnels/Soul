using UnrealBuildTool;

public class RBUIInput : ModuleRules
{
    public RBUIInput(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore",
            "CommonUI", "CommonInput", "EnhancedInput", "DeveloperSettings", "GameplayTags"
        });
    }
}
