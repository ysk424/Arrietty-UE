using UnrealBuildTool;

public class ArriettyRuntime : ModuleRules
{
    public ArriettyRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "HeadMountedDisplay",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "ApplicationCore",
            "Projects",
            "Slate",
            "SlateCore"
        });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            bEnableExceptions = true;
            PublicSystemLibraries.Add("windowsapp.lib");
            PublicDefinitions.Add("ARRIETTY_WINDOWS_BLE=1");
        }
        else
        {
            PublicDefinitions.Add("ARRIETTY_WINDOWS_BLE=0");
        }
    }
}
