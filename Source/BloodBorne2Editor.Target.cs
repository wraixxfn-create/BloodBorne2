using UnrealBuildTool;
using System.Collections.Generic;

public class BloodBorne2EditorTarget : TargetRules
{
    public BloodBorne2EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        // Must match the settings the installed engine's UnrealEditor binaries were built with;
        // keep this identical to BloodBorne2.Target.cs. A mismatch on a shared property makes UBT
        // refuse to build:
        //   "BloodBorne2Editor modifies the values of properties: [ UndefinedIdentifierWarningLevel:
        //    Off != Error, UnreachableCodeWarningLevel: Off != Error, ... ] This is not allowed, as
        //    BloodBorne2Editor has build products in the shared area."
        // V4 (UE 5.3) leaves those warning levels Off; newer engines raise them to Error
        // (V6 in 5.7, V7 in 5.8). `Latest` tracks whatever engine the team is on.
        // Prefer an explicit pin once the team standardises on one release:
        //   5.3 -> V4, 5.4 -> V5, 5.7 -> V6, 5.8 -> V7.
        DefaultBuildSettings = BuildSettingsVersion.Latest;

        // Include order is resolved per module and is not part of the shared build environment, so
        // it can stay on the version this code was written against (UBT only prints an [Upgrade]
        // notice). Raise it to the engine's version - EngineIncludeOrderVersion.Unreal5_7 /
        // Unreal5_8 / Latest - when you are ready to deal with any IWYU fallout.
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

        ExtraModuleNames.Add("BloodBorne2");

        // Fallback if you must keep legacy build settings (e.g. BuildSettingsVersion.V4) on a newer
        // engine: uncomment to force this target's values onto the shared environment. Safe for
        // warning levels, which only affect the compiler command line of the project's own modules,
        // but it keeps the editor's "outdated BuildSettingsVersion" upgrade prompt around.
        //bOverrideBuildEnvironment = true;

        // Do NOT use BuildEnvironment = TargetBuildEnvironment.Unique here: that path is only valid
        // for engines compiled from source and would rebuild every engine module into the project's
        // Intermediate folder.
    }
}
