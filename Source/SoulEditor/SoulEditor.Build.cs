using UnrealBuildTool;

public class SoulEditor : ModuleRules
{
    public SoulEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "AssetRegistry", "Json" });
    }
}
