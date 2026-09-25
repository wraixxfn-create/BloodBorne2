using UnrealBuildTool;
using System.Collections.Generic;

public class BloodBorne2EditorTarget : TargetRules
{
    public BloodBorne2EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("BloodBorne2");
    }
}
