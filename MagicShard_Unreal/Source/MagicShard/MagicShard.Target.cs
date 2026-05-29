using UnrealBuildTool;
using System.Collections.Generic;

public class MagicShardTarget : TargetRules
{
    public MagicShardTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        CppStandard = CppStandardVersion.Cpp20;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("MagicShard");
    }
}
