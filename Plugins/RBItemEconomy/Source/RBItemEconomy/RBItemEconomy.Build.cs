using UnrealBuildTool;
public class RBItemEconomy : ModuleRules
{
    public RBItemEconomy(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true; // Core rejects staged candidates with internal exceptions.
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
