using UnrealBuildTool;

public class SoulEditorTarget : TargetRules
{
    public SoulEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "SoulCore", "Soul" });
    }
}
