using UnrealBuildTool;

public class Soul : ModuleRules
{
    public Soul(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {"RBSaveCore", "Landscape"});
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "ProceduralMeshComponent", "InputCore", "SoulCore", "RBFoundation", "RBSave", "Json", "SoulRealtimeBattle"
        });

        // Runtime readers use ProjectDir()/Data and the Foundation plugin base dir.
        // Exact loose files preserve those paths without staging Data/ or Evidence/ wholesale.
        if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Type == TargetType.Game)
        {
            foreach (string File in new[]
            {
                "Data/soul_vertical_scenario_20260925.json",
                "Data/SettlementEnvironments/DwarfHoldRuntimeProof.json",
                "Data/SettlementEnvironments/HumanCapitalRuntimeProof.json",
                "Data/CampaignTerrainV2/FounderHeight.r16",
                "Data/CampaignTerrainV2/presentation.json",
                "Data/CampaignMesa/presentation.json",
                "Data/CampaignEvilCorridor/presentation.json",
                "Data/CampaignMesaLocal/MesaHeight.r16",
                "Data/soul_world_overmap_v1_20260922.json",
                "Data/soul_campaign_start_states_v1_20260922.json",
                "Data/soul_overmap_battle_handoff_v1_20260922.json",
                "Plugins/RBFoundation/StackManifest.json"
            })
            {
                RuntimeDependencies.Add("$(ProjectDir)/" + File, StagedFileType.NonUFS);
            }
            if (Target.Name == "SoulComposition")
            {
                // The isolated cook enables this content plugin. Its module is
                // editor-only, but the runtime mount still needs its descriptor.
                RuntimeDependencies.Add("$(EngineDir)/Plugins/Runtime/HDRIBackdrop/HDRIBackdrop.uplugin", StagedFileType.NonUFS);
                foreach (string File in new[]
                {
                    "Data/CampaignComposition/presentation.json",
                    "Data/CampaignComposition/RuntimeProof.json",
                    "Data/CampaignComposition/HumanRuntimeProof.json",
                    "Data/CampaignCompositionLocal/Composition_3500_r2.r16"
                })
                    RuntimeDependencies.Add("$(ProjectDir)/" + File, StagedFileType.NonUFS);
            }
        }
    }
}
