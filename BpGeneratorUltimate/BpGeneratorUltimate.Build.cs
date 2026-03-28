// Copyright 2026, BlueprintsLab, All rights reserved

using UnrealBuildTool;

public class BpGeneratorUltimate : ModuleRules
{
	public BpGeneratorUltimate(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"HTTP",
       				"Json",
			        "JsonUtilities",
                    "EditorScriptingUtilities"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"HTTP",
				"Json",
				"JsonUtilities",
				"Kismet",
				"AssetRegistry",
				"KismetCompiler",
				"BlueprintGraph", 
				"GraphEditor", 
				"ApplicationCore",
                "UMG",
                "UMGEditor",
                "Blutility",
                "Sockets",
                "Networking",
				"ContentBrowser",
                "ContentBrowserData",
                "MaterialEditor",
                "AssetTools",
                "Foliage",
				"FoliageEdit",
                "GraphEditor",
                "AIModule",
                "AIGraph",
				"BehaviorTreeEditor",
                "DesktopPlatform",
                "WebBrowser",
                "WebBrowserWidget",
                "EnhancedInput"
            }
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
