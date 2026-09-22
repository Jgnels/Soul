using UnrealBuildTool;

public class RBPBIL : ModuleRules
{
    public RBPBIL(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "TCAT"
        });
    }
}
