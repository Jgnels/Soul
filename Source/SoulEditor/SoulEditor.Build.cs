using UnrealBuildTool;

public class SoulEditor : ModuleRules
{
    public SoulEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Source/Runtime/AssetRegistry/Internal"));
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "AssetRegistry", "Json", "Soul", "UnrealEd", "NavigationSystem", "MessageLog" });
    }
}
