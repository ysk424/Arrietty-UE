using UnrealBuildTool;

public class ArriettyCesium : ModuleRules
{
    public ArriettyCesium(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "ArriettyRuntime",
            "CesiumRuntime"
        });

        // Cesium's Windows-facing dependencies can be included before Slate in
        // non-editor targets. Keep Win32 min/max macros out of public headers.
        PublicDefinitions.Add("NOMINMAX=1");
    }
}
