using UnrealBuildTool;

public class SoulRealtimeBattle : ModuleRules
{
    public SoulRealtimeBattle(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AIModule",
            "InputCore",
            "SoulCore",
            "RefinedBadgerCombatCore",
            "RefinedBadgerCombatVariant",
            "RBAICore",
            "RBAICombat",
            "RBAICreatures",
            "RBAIGroups",
            "RBPBIL",
            "RBMagicCore",
            "RBMagicPresentation",
            "Niagara"
        });
    }
}
