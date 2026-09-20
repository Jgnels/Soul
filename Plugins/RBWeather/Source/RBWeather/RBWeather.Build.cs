using UnrealBuildTool;

public class RBWeather : ModuleRules
{
    public RBWeather(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Niagara"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });
    }
}
