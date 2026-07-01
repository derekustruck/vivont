// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontFaceGameMode.h"
#include "VivontLog.h"
#include "VivontPlugin.h"
#include "VivontAnimationManager.h"
#include "Engine.h"
#include "EngineGlobals.h"
#include "VivontWidget.h"
#include "Features/IModularFeatures.h"
#include "ILiveLinkClient.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h" // Added for GetGameInstance()

#define LOCTEXT_NAMESPACE "VivontFaceGameMode"

AVivontFaceGameMode::AVivontFaceGameMode()
    : AnimationManager(nullptr)
    , CurrentWidget(nullptr)
{
    // Enable tick for this game mode
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
}

void AVivontFaceGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    
    // Initialize the LiveLink source
    InitializeLiveLinkSource();
    
    // Get the animation manager (moved from GameInstance to Engine subsystem)
    AnimationManager = GEngine->GetEngineSubsystem<UVivontAnimationManager>();
    
    // Output some debug info
    UE_LOG(LogVivont, Log, TEXT("Vivont GameMode initialized"));
    if (AnimationManager)
    {
        UE_LOG(LogVivont, Log, TEXT("Animation Manager found"));
    }
    else
    {
        UE_LOG(LogVivont, Warning, TEXT("Animation Manager not found"));
    }
    
    if (LiveLinkSource.IsValid())
    {
        UE_LOG(LogVivont, Log, TEXT("LiveLink Source found"));
    }
    else
    {
        UE_LOG(LogVivont, Warning, TEXT("LiveLink Source not found"));
    }
}

void AVivontFaceGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    // Create and show the widget
    CreateAndShowWidget();
}

void AVivontFaceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    // Remove widget from viewport
    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }
}

void AVivontFaceGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    
    // The logic to update LiveLinkSource from here has been removed.
    // LiveLinkSource should be updated directly when blend shapes are received
    // by the UVivontAnimationManager.
}

UVivontWidget* AVivontFaceGameMode::CreateAndShowWidget()
{
    // Only create the widget if we have a valid class
    if (WidgetClass)
    {
        // Create the widget
        CurrentWidget = CreateWidget<UVivontWidget>(GetWorld(), WidgetClass);
        if (CurrentWidget)
        {
            // Add to viewport
            CurrentWidget->AddToViewport();
            
            // Set input mode to UI only
            APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
            if (PC)
            {
                PC->SetInputMode(FInputModeUIOnly());
                PC->bShowMouseCursor = true;
            }
            
            return CurrentWidget;
        }
    }
    
    return nullptr;
}

void AVivontFaceGameMode::InitializeLiveLinkSource()
{
    // Try to get the LiveLink source from the plugin
    FVivontPluginModule& PluginModule = FModuleManager::LoadModuleChecked<FVivontPluginModule>("VivontPlugin");
    LiveLinkSource = PluginModule.GetLiveLinkSource();
    
    // If we couldn't get it from the plugin, create a new one
    if (!LiveLinkSource.IsValid())
    {
        // Create a new LiveLink source
        LiveLinkSource = MakeShared<FVivontLiveLinkSource>(LOCTEXT("GameModeLiveLinkSource", "Vivont Game"));
        
        // Register the source with the LiveLink client if available
        if (IModularFeatures::Get().IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
        {
            ILiveLinkClient* LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
            if (LiveLinkClient)
            {
                LiveLinkClient->AddSource(LiveLinkSource);
                UE_LOG(LogVivont, Log, TEXT("Vivont LiveLink source registered from GameMode"));
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE
