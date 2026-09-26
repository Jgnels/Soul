using UnrealBuildTool;

public class Soul : ModuleRules
{
    public Soul(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("RBSaveCore");
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "SoulCore", "RBFoundation", "RBSave", "Json", "SoulRealtimeBattle"
        });
    }
}
