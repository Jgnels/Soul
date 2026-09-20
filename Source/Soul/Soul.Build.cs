using UnrealBuildTool;

public class Soul : ModuleRules
{
    public Soul(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "SoulCore", "RBFoundation", "RBSave", "Json"
        });
    }
}
