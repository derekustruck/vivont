// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontPlugin.h"
#include "Engine/Engine.h"
#include "VivontLiveLinkSource.h"
#include "VivontPluginStyle.h"
#include "VivontCommands.h"
#include "ILiveLinkClient.h" // Required for AddSource/RemoveSource
#include "LiveLinkClientReference.h" // Required for GetLiveLinkClient
#include "VivontSettings.h"
#include "VivontLog.h"
#include "VivontAnimationManager.h"
#include "VivontAudioManager.h"
#include "SimpleOpenAIClient.h"       // Add the new OpenAI client
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#if WITH_EDITOR // Added editor check for LevelEditor include
#include "LevelEditor.h"
#endif // WITH_EDITOR
// #include "Interfaces/IMainFrameModule.h"
#include "WebSocketsModule.h"          // Add WebSockets module

#define LOCTEXT_NAMESPACE "FVivontPluginModule"


void FVivontPluginModule::StartupModule()
{
    // Make sure WebSockets module is loaded for the SimpleOpenAIClient
    FModuleManager::LoadModuleChecked<FWebSocketsModule>("WebSockets");

    // Register the settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Vivont",
            LOCTEXT("VivontSettingsName", "Vivont Plugin"),
            LOCTEXT("VivontSettingsDescription", "Configure the Vivont plugin settings."),
            GetMutableDefault<UVivontSettings>()
        );
    }
    UE_LOG(LogVivont, Log, TEXT("Vivont Plugin: Checking for VivontAudioSubsystem..."));
    if (GEngine)

    {
        // Check if subsystem exists without declaring a local variable
        // Note: The shadowing warning was here, but the local variable was already removed in a previous step.
        // This check is just for logging. The member variable is assigned below.
        if (GEngine->GetEngineSubsystem<UVivontAudioSubsystem>()) 
        {
            UE_LOG(LogVivont, Log, TEXT("Vivont Plugin: VivontAudioSubsystem is available"));
        }
        else
        {
            UE_LOG(LogVivont, Warning, TEXT("Vivont Plugin: VivontAudioSubsystem not found on startup"));
        }
    }
    // Get the audio subsystem
    if (!GEngine)
    {
        UE_LOG(LogVivont, Error, TEXT("GEngine is null, cannot get audio subsystem"));
    }
    else
    {
        AudioSubsystem = GEngine->GetEngineSubsystem<UVivontAudioSubsystem>();
        if (!AudioSubsystem)
        {
            UE_LOG(LogVivont, Error, TEXT("Failed to retrieve VivontAudioSubsystem"));
        }
        
        // Get the new subsystems (they'll be created if they don't exist)
        OpenAIClient = GEngine->GetEngineSubsystem<USimpleOpenAIClient>();
        if (!OpenAIClient)
        {
            UE_LOG(LogVivont, Error, TEXT("Failed to retrieve SimpleOpenAIClient subsystem"));
        }
        else
        {
            // Configure the OpenAI client if needed
            UVivontSettings* Settings = GetMutableDefault<UVivontSettings>();
            if (Settings && !Settings->OpenAIApiKey.IsEmpty())
            {
                OpenAIClient->SetAPIKey(Settings->OpenAIApiKey);
                UE_LOG(LogVivont, Log, TEXT("Configured SimpleOpenAIClient with API key from settings"));
            }
            
            UE_LOG(LogVivont, Log, TEXT("SimpleOpenAIClient subsystem initialized"));
        }

        // // RE-ADDED: Explicitly get the Animation Manager subsystem to ensure it initializes early.
        // VIVONT_LOG(Log, TEXT("StartupModule: Attempting to retrieve UVivontAnimationManager..."));
        // UVivontAnimationManager* AnimManager = GEngine->GetEngineSubsystem<UVivontAnimationManager>();
        // if (!AnimManager)
        // {
        //     VIVONT_ERROR("StartupModule", "Failed to retrieve UVivontAnimationManager subsystem!");
        // }
        // else
        // {
        //     VIVONT_LOG(Log, TEXT("StartupModule: Successfully retrieved UVivontAnimationManager subsystem (%p). Its Initialize should run."), AnimManager);
        // }

        // Removed getting AudioPlaybackManager subsystem
        // AudioPlaybackManager = GEngine->GetEngineSubsystem<UAudioPlaybackManager>();
        // if (!AudioPlaybackManager)
        // {
        //     UE_LOG(LogVivont, Error, TEXT("Failed to retrieve AudioPlaybackManager subsystem"));
        // }
        // else
        // {
        //     UE_LOG(LogVivont, Log, TEXT("AudioPlaybackManager subsystem initialized"));
        // }
    }

    // Register plugin style and commands
    FVivontPluginStyle::Initialize();
    FVivontPluginStyle::ReloadTextures();
    FVivontCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FVivontCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FVivontPluginModule::PluginButtonClicked),
        FCanExecuteAction());

    // Register menus only in editor builds
#if WITH_EDITOR
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FVivontPluginModule::RegisterMenus));
#endif // WITH_EDITOR

    // Create and register the LiveLink source owned by this module
    UE_LOG(LogVivont, Log, TEXT("Vivont Plugin: Creating and registering LiveLink source..."));
    LiveLinkSource = MakeShared<FVivontLiveLinkSource>(LOCTEXT("VivontLiveLinkSourceName", "Vivont Face"));

    // Get the LiveLink client and add the source
    FLiveLinkClientReference ClientRef; // Helper to get the client
    ILiveLinkClient* LiveLinkClient = ClientRef.GetClient();
    if (LiveLinkClient)
    {
        LiveLinkClient->AddSource(LiveLinkSource);
        UE_LOG(LogVivont, Log, TEXT("Vivont Plugin: LiveLink source registered with client."));
    }
    else
    {
        UE_LOG(LogVivont, Error, TEXT("Vivont Plugin: Failed to get LiveLink client. Source not registered."));
        LiveLinkSource.Reset(); // Don't keep the source if we can't register it
    }

    // Removed DelegateHandler creation, initialization, and delegate binding
}

// Removed FVivontPluginModule::HandleOpenAIAudio function implementation

bool FVivontPluginModule::HasAnimationDataForAudio(uint32 AudioId)
{
    // In a real implementation, you would check if animation data exists for this audio ID
    // This could involve checking a map of audio IDs to animation data timestamps
    // For now, we'll simulate this with a simple log and return false
    
    // For simplicity in this example, we'll always return false to trigger the warning log
    // In a real implementation, you might check if the LiveLink system has received 
    // corresponding blend shapes from your animation service
    UE_LOG(LogVivont, Verbose, TEXT("Checking for animation data for Audio ID: %u"), AudioId);
    
    // Could be enhanced to check a data structure like:
    // return AnimationDataCache.Contains(AudioId);
    
    return false; // Assuming no animation data exists yet
}

void FVivontPluginModule::ShutdownModule()
{
    // Unregister the settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "Vivont");
    }

    // Unregister menus only in editor builds
#if WITH_EDITOR
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
#endif // WITH_EDITOR

    // Removed DelegateHandler cleanup

    // Clear the OpenAI client reference (it will be cleaned up by the engine subsystem manager)
    OpenAIClient = nullptr;
    // AudioPlaybackManager = nullptr; // Removed nulling pointer

    // Next, stop audio receiver as it might access the LiveLink source
    if (bAudioReceiverActive && AudioSubsystem)
    {
        // AudioSubsystem->StopAudioReceiver(); // Removed call since StopAudioReceiver is obsolete.
        bAudioReceiverActive = false;
    }

    // Clean up the active source pointer if it's valid
    // Note: The actual source object's lifetime is managed by the LiveLink client / shared pointers elsewhere.
    // We just clear our weak reference. - No longer a weak reference
    // ActiveLiveLinkSource.Reset(); // Removed weak pointer reset

    // Remove the source we created from the LiveLink client
    if (LiveLinkSource.IsValid())
    {
        FLiveLinkClientReference ClientRef;
        ILiveLinkClient* LiveLinkClient = ClientRef.GetClient();
        if (LiveLinkClient)
        {
            LiveLinkClient->RemoveSource(LiveLinkSource);
            UE_LOG(LogVivont, Log, TEXT("Vivont Plugin: LiveLink source removed from client."));
        }
        else
        {
            UE_LOG(LogVivont, Warning, TEXT("Vivont Plugin: Failed to get LiveLink client during shutdown. Source might not be removed cleanly."));
        }
        LiveLinkSource.Reset(); // Release our shared pointer
    }

    // Shutdown style and commands last
    FVivontPluginStyle::Shutdown();
    FVivontCommands::Unregister();
} // Added missing closing brace for ShutdownModule

void FVivontPluginModule::PluginButtonClicked()
{
    // Open a dialog to show plugin info
    FText DialogText = FText::Format(
        LOCTEXT("PluginButtonDialogText", "Vivont Plugin\n\nVersion: {0}\nLiveLink Source Status: {1}\nOpenAI API connected: {2}"),
        FText::FromString(TEXT("1.0")),
        LiveLinkSource.IsValid() ? LiveLinkSource->GetSourceStatus() : LOCTEXT("SourceStatusNotAvailable", "Not Available"),
        OpenAIClient && OpenAIClient->IsConnected() ? LOCTEXT("Connected", "Yes") : LOCTEXT("NotConnected", "No")
    );
    FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

void FVivontPluginModule::RegisterMenus()
{
#if WITH_EDITOR // Ensure menu registration only happens in editor
    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);

    // Add entry to main editor menu
    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("VivontSection");
        Section.AddMenuEntry(
            "OpenVivontWindow",
            LOCTEXT("OpenVivontWindow", "Vivont"),
            LOCTEXT("OpenVivontWindowTooltip", "Opens the Vivont plugin window"),
            FSlateIcon(FVivontPluginStyle::GetStyleSetName(), "VivontPlugin.OpenPluginWindow"),
            FUIAction(
                FExecuteAction::CreateRaw(this, &FVivontPluginModule::PluginButtonClicked),
                FCanExecuteAction()
            )
        );
    }

    // Add entry to toolbar
    if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar"))
    {
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Vivont");
        Section.AddEntry(
            FToolMenuEntry::InitToolBarButton(
                "OpenVivontWindow",
                FUIAction(
                    FExecuteAction::CreateRaw(this, &FVivontPluginModule::PluginButtonClicked),
                    FCanExecuteAction()
                ),
                LOCTEXT("OpenVivontWindowToolbar", "Vivont"),
                LOCTEXT("OpenVivontWindowToolbarTooltip", "Opens the Vivont plugin window"),
                FSlateIcon(FVivontPluginStyle::GetStyleSetName(), "VivontPlugin.OpenPluginWindow")
            )
        );
    }
#endif // WITH_EDITOR
}

TSharedPtr<FVivontLiveLinkSource> FVivontPluginModule::GetLiveLinkSource() const
{
    // Return the source owned by the module
    return LiveLinkSource;
}

// Removed SetActiveLiveLinkSource function implementation

USimpleOpenAIClient* FVivontPluginModule::GetOpenAIClient() const
{
    return OpenAIClient;
}

// Removed GetAudioPlaybackManager function implementation

// Console command implementation
static FAutoConsoleCommand DiagnoseLiveLinkCommand(
    TEXT("Vivont.DiagnoseLiveLink"),
    TEXT("Outputs diagnostic information about the Vivont LiveLink connection"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        // Get the plugin module
        FVivontPluginModule& PluginModule = FModuleManager::LoadModuleChecked<FVivontPluginModule>("VivontPlugin");
        
        // Get the LiveLink source managed by the module
        TSharedPtr<FVivontLiveLinkSource> Source = PluginModule.GetLiveLinkSource();
        
        // if (Source.IsValid())
        // {
        //     // Call the diagnostic function
        //     Source->DiagnoseLiveLinkConnection();
        //     UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink diagnostics requested via console command."));
        // }
        // else
        // {
        //     UE_LOG(LogVivont, Error, TEXT("Vivont LiveLink source not available via module. Cannot run diagnostics."));
        // }
    })
);

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVivontPluginModule, VivontPlugin)
