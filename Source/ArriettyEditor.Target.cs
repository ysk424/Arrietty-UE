using UnrealBuildTool;
using System.Collections.Generic;

public class ArriettyEditorTarget : TargetRules
{
    public ArriettyEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        CppStandard = CppStandardVersion.Cpp20;
        WindowsPlatform.Compiler = WindowsCompiler.VisualStudio2026;
        WindowsPlatform.CompilerVersion = "Latest";
        WindowsPlatform.bUseCPPWinRT = true;
        WindowsPlatform.WindowsSdkVersion = "10.0.26100.0";
        ExtraModuleNames.Add("ArriettyWorldHost");
    }
}
