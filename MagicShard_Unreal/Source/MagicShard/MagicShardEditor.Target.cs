using UnrealBuildTool;
using System.Collections.Generic;

public class MagicShardEditorTarget : TargetRules
{
    public MagicShardEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        CppStandard = CppStandardVersion.Cpp20;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MagicShard");
    }
}
