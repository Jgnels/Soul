using UnrealBuildTool;

public class SoulTarget : TargetRules
{
    public SoulTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "SoulCore", "Soul" });
    }
}
