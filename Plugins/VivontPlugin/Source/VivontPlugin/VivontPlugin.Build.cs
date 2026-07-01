// Copyright (c) 2025, Derek. All rights reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class VivontPlugin : ModuleRules
{
    public VivontPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        // For Unreal Engine 5.7
        bLegacyPublicIncludePaths = false;
        bUseUnity = false;
        
        PublicIncludePaths.AddRange(
            new string[] {
                // Add public include paths
                Path.Combine(ModuleDirectory, "Public"),
                Path.Combine(EngineDirectory, "Source/Runtime/Core/Public"),
                Path.Combine(EngineDirectory, "Source/Runtime/CoreUObject/Public"),
            }
        );
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // Add private include paths
                Path.Combine(ModuleDirectory, "Private"),
                Path.Combine(EngineDirectory, "Source/Runtime/Core/Private"),
                Path.Combine(EngineDirectory, "Source/Runtime/CoreUObject/Private"),
            }
        );
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "LiveLink",
                "LiveLinkInterface",
                "LiveLinkComponents",
                "HTTP",
                "Json",
                "JsonUtilities",
                "UMG",
                "Slate",
                "SlateCore",
                "Networking",
                "Sockets",
                "Serialization",
                "Projects",
                "AudioMixerCore",    // Core audio mixing
                "AudioCaptureCore",  // Core audio capture interfaces (needed for public headers)
                "DeveloperSettings", // For ISettingsModule (needed for public headers)
                "AudioExtensions",   // Added for EDynamicForceRealTimeDecompression
                "WebSockets",
                "ToolMenus",
                "Slate",
                "SlateCore",
                "UMG"         // Moved from Editor only - needed by VivontPlugin.cpp?
            }
        );
        
        // Required for menu extensions and editor UI
        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    // "ToolMenus", // Moved to main public dependencies
                    "EditorStyle",
                    "ToolWidgets",
                    "AssetTools",
                    "EditorFramework"
                }
            );
        }
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "LiveLinkAnimationCore",
                "AnimGraphRuntime",
                "LiveLink",
                "LiveLinkInterface",
                "Projects",
                "AnimationCore",
                "AudioCapture",      // Platform specific capture implementation
                "SignalProcessing",  // For DSP operations if needed
                "AudioMixer",       // Higher level audio mixer functionality
                "NNE"               // Neural Network Engine - in-process ONNX inference (GPU via NNERuntimeORTDml)
                // AudioMixerPlatform removed as it's handled implicitly
            }
        );
        
        // Add additional editor dependencies
        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "ApplicationCore",
                    "DesktopPlatform",
                    "ToolMenusEditor",
                    "WorkspaceMenuStructure",
                    "MainFrame",
                    "LevelEditor" // Added for LevelEditor.h include
                }
            );
        }
        
        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        // Define VIVONTPLUGIN_EXPORTS for conditional compilation
        PublicDefinitions.Add("VIVONTPLUGIN_EXPORTS=1");

        // Define UE 5.7 for conditional compilation
        PublicDefinitions.Add("UE_5_7=1");
        
    }
}
