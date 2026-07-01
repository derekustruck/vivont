// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontPluginStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateImageBrush.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FVivontPluginStyle::StyleInstance = nullptr;

void FVivontPluginStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FVivontPluginStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FVivontPluginStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("VivontPluginStyle"));
    return StyleSetName;
}

void FVivontPluginStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

const ISlateStyle& FVivontPluginStyle::Get()
{
    return *StyleInstance;
}

TSharedRef<FSlateStyleSet> FVivontPluginStyle::Create()
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("VivontPluginStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("VivontPlugin")->GetBaseDir() / TEXT("Resources"));

    const FVector2D Icon40x40(40.0f, 40.0f);
    FString ResourceDir = IPluginManager::Get().FindPlugin("VivontPlugin")->GetBaseDir() / TEXT("Resources");
    FString IconPath = ResourceDir / TEXT("Icon128.png");
    Style->Set("VivontPlugin.OpenPluginWindow", new FSlateImageBrush(FPaths::ConvertRelativePathToFull(IconPath), Icon40x40));

    return Style;
}

#undef RootToContentDir