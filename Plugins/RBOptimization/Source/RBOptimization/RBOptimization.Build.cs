using UnrealBuildTool;
public class RBOptimization : ModuleRules
{
    public RBOptimization(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NetCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "Json", "JsonUtilities" });
    }
}
