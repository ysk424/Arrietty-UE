using UnrealBuildTool;

public class ArriettyWorldHost : ModuleRules
{
    public ArriettyWorldHost(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PrivateDependencyModuleNames.Add("Core");
    }
}
