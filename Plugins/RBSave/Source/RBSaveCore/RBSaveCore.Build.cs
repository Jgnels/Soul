using UnrealBuildTool;

public class RBSaveCore : ModuleRules
{
    public RBSaveCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;
        PublicDependencyModuleNames.Add("Core");
        PublicDefinitions.Add("RBSAVE_UE=1");
    }
}
// Explicit module entry point lives in Private/RBSaveCoreModule.cpp.
