// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h" // Already present
// #include "ISettingsModule.h" // Compiler couldn't find this, trying full path
#include "Developer/Settings/Public/ISettingsModule.h" // Trying full path
#include "UObject/GCObject.h"

// Forward declare before include
class ILiveLinkSource; 
#include "ILiveLinkSource.h" 

class FToolBarBuilder;
class FMenuBuilder;
class FVivontLiveLinkSource;
class UVivontAudioSubsystem;
class USimpleOpenAIClient;
class UVivontAnimationManager; // Removed forward declaration
// class UVivontDelegateHandler; // Removed forward declaration

class FVivontPluginModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    /** This function will be bound to Command. */
    void PluginButtonClicked();
    
    /** Get the LiveLink source managed by this module */
    TSharedPtr<FVivontLiveLinkSource> GetLiveLinkSource() const;

    /** Get the OpenAI client */
    USimpleOpenAIClient* GetOpenAIClient() const;

    // Removed GetAudioPlaybackManager function declaration

    // Removed HandleOpenAIAudio function declaration

    /**
     * Check if animation data exists for the given audio
     * @param AudioId The unique identifier for the audio data
     * @return True if matching animation data exists
     */
    bool HasAnimationDataForAudio(uint32 AudioId);
    
private:
    void RegisterMenus();

private:
    TSharedPtr<class FUICommandList> PluginCommands;
    TSharedPtr<FVivontLiveLinkSource> LiveLinkSource; // Module now owns the source
    
    /** Audio subsystem reference */
    UVivontAudioSubsystem* AudioSubsystem;
    
    /** OpenAI client reference */
    USimpleOpenAIClient* OpenAIClient;

    // Removed unused AudioPlaybackManager member variable

    /** Flag to track if audio receiver is active */
    bool bAudioReceiverActive;

    // Removed DelegateHandler member variable
};
