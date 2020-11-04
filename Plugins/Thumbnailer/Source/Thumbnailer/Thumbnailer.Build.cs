// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Thumbnailer : ModuleRules
{
	public Thumbnailer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Private/ThumbnailerPluginPrivatePCH.h";
        bLegacyPublicIncludePaths = true;

        PublicDependencyModuleNames.AddRange(
            new string[]
                {
                    "Core",
                    "UnrealEd",
                    "ImageWrapper",
                    "AssetRegistry",
                    "PropertyEditor",
                    "EditorStyle",
                }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
                {
                    "Engine",
                    "Slate",
                    "SlateCore",
                    "Projects",
                    "LevelEditor",
                    "InputCore",
                    "CoreUObject",
                    "ImageWrapper",
                    "AssetRegistry",
                    "PropertyEditor",
                    "AdvancedPreviewScene",
                }
            );
    }
}
