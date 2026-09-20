using UnrealBuildTool;

public class RBFoundationAdapters : ModuleRules
{
    public RBFoundationAdapters(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "RBFoundation",
            "RBSave", "RBOptimization"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Json", "JsonUtilities", "GameplayTags",
            "RBItemEconomy", "RBRoutine", "RBWeather",
            "RefinedBadgerCombatCore"
        });
    }
}
