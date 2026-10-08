using UnrealBuildTool;

// Explicit packaging target for the opt-in composition candidate. This uses
// the same gameplay modules; it does not promote the default map or save schema.
public class SoulCompositionTarget : TargetRules
{
    public SoulCompositionTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "SoulCore", "Soul", "SoulRealtimeBattle" });
    }
}
