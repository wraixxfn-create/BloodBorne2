using UnrealBuildTool;
using System.Collections.Generic;

public class BloodBorne2Target : TargetRules
{
    public BloodBorne2Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        // Build settings must produce the same values as the engine this project is opened with.
        // Game/editor targets share build products with the precompiled UnrealGame/UnrealEditor
        // binaries, so UBT aborts on any difference in a [RequiresUniqueBuildEnvironment] property:
        //   "BloodBorne2 modifies the values of properties: [ UndefinedIdentifierWarningLevel:
        //    Off != Error, UnreachableCodeWarningLevel: Off != Error, ... ]"
        // V4 (UE 5.3) leaves those four warning levels Off; newer engines raise them to Error
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
