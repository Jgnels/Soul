using UnrealBuildTool;
public class RefinedBadgerCombatCore : ModuleRules
{
    public RefinedBadgerCombatCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PublicDependencyModuleNames.Add("Core");
    }
}
