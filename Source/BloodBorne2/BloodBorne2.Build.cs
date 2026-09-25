using UnrealBuildTool;

public class BloodBorne2 : ModuleRules
{
    public BloodBorne2(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "Json", "JsonUtilities"
        });

        // Keep JSON editable at runtime and include it in packaged builds.
        RuntimeDependencies.Add("$(ProjectDir)/Content/Localization/en.json", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Content/Localization/it.json", StagedFileType.NonUFS);
    }
}
