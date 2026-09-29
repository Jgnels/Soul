using UnrealBuildTool;

public class SoulEditorTarget : TargetRules
{
    public SoulEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "SoulCore", "Soul", "SoulRealtimeBattle" });
        // Opt-in machine build workaround; normal target defaults remain intact.
        if (System.Environment.GetEnvironmentVariable("SOUL_NO_PCH_COMPAT") == "1")
        {
            bOverrideBuildEnvironment = true;
            AdditionalCompilerArguments = "/FI" + System.IO.Path.Combine(ProjectFile.Directory.FullName, "Tools", "SoulNoPchCompatibility.h");
        }
    }
}
