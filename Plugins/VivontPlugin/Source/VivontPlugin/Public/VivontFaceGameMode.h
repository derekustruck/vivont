// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VivontAnimationManager.h"
#include "VivontLiveLinkSource.h"
#include "VivontWidget.h"
#include "VivontFaceGameMode.generated.h"

/**
 * Vivont Game Mode - Handles the Vivont session
 * This GameMode initializes the animation manager and LiveLink source
 */
UCLASS(BlueprintType, Blueprintable)
class VIVONTPLUGIN_API AVivontFaceGameMode : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    AVivontFaceGameMode();
    
    // Begin AGameModeBase interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    // End AGameModeBase interface
    
    // Tick function
    virtual void Tick(float DeltaSeconds) override;
    
    // Get the animation manager
    UFUNCTION(BlueprintCallable, Category = "Vivont")
    UVivontAnimationManager* GetAnimationManager() const { return AnimationManager; }
    
    // Get the widget class to spawn
    UFUNCTION(BlueprintCallable, Category = "Vivont")
    TSubclassOf<UVivontWidget> GetWidgetClass() const { return WidgetClass; }
    
    // Create and add the widget to the viewport
    UFUNCTION(BlueprintCallable, Category = "Vivont")
    UVivontWidget* CreateAndShowWidget();
    
protected:
    // The animation manager for this session
    UPROPERTY()
    UVivontAnimationManager* AnimationManager;
    
    // The widget to use for UI
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vivont")
    TSubclassOf<UVivontWidget> WidgetClass;
    
    // The current widget instance
    UPROPERTY()
    UVivontWidget* CurrentWidget;
    
    // The LiveLink source for this session
    TSharedPtr<FVivontLiveLinkSource> LiveLinkSource;
    
private:
    // Initialize the LiveLink source
    void InitializeLiveLinkSource();
};