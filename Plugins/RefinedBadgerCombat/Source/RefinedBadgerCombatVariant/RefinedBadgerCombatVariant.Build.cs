using UnrealBuildTool;
public class RefinedBadgerCombatVariant : ModuleRules
{
    public RefinedBadgerCombatVariant(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "RefinedBadgerCombatCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "AIModule", "NavigationSystem" });
    }
}
