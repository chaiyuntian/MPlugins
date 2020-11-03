// Mans Isaksson 2020

using UnrealBuildTool;

public class ThumbnailGenerator : ModuleRules
{
    public ThumbnailGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
			"Slate",
            "RHI"
        });
    }
}
