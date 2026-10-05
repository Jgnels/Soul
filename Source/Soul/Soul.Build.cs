using UnrealBuildTool;

public class Soul : ModuleRules
{
    public Soul(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("RBSaveCore");
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "SoulCore", "RBFoundation", "RBSave", "Json", "SoulRealtimeBattle"
        });

        // Runtime readers use ProjectDir()/Data and the Foundation plugin base dir.
        // Exact loose files preserve those paths without staging Data/ or Evidence/ wholesale.
        if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Type == TargetType.Game)
        {
            foreach (string File in new[]
            {
                "Data/soul_vertical_scenario_20260925.json",
                "Data/soul_world_overmap_v1_20260922.json",
                "Data/soul_campaign_start_states_v1_20260922.json",
                "Data/soul_overmap_battle_handoff_v1_20260922.json",
                "Plugins/RBFoundation/StackManifest.json"
            })
            {
                RuntimeDependencies.Add("$(ProjectDir)/" + File, StagedFileType.NonUFS);
            }
        }
    }
}
