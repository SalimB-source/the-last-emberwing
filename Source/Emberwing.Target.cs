using UnrealBuildTool;
using System.Collections.Generic;

public class EmberwingTarget : TargetRules
{
    public EmberwingTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("Emberwing");
    }
}
