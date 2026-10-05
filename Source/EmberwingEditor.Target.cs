using UnrealBuildTool;
using System.Collections.Generic;

public class EmberwingEditorTarget : TargetRules
{
    public EmberwingEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("Emberwing");
    }
}
