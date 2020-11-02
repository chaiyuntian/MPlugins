// Copyright 2020 Phyronnaz

using System.IO;
using UnrealBuildTool;

public class VoxelExtension : ModuleRules
{
    public VoxelExtension(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;
        bLegacyPublicIncludePaths = false;


        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UMG",
                "Slate",
                "SlateCore",
                "Voxel"
            }
        );

       PrivateDependencyModuleNames.AddRange(new string[]
            {
                "ImageWrapper",
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "Voxel"
            });
    }
}
