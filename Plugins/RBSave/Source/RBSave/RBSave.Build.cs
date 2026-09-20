using UnrealBuildTool;

public class RBSave : ModuleRules
{
    public RBSave(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings" });
        PrivateDependencyModuleNames.AddRange(new[] { "RBSaveCore" });
    }
}
