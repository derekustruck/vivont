// Copyright (c) 2025, Derek. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Templates/SharedPointer.h"
#include "VivontPluginStyle.h"

class FVivontCommands : public TCommands<FVivontCommands>
{
public:
    FVivontCommands()
        : TCommands<FVivontCommands>(
              TEXT("VivontPlugin"),
              NSLOCTEXT("Contexts", "VivontPlugin", "Vivont Plugin"),
              NAME_None,
              FVivontPluginStyle::GetStyleSetName())
    {
    }

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenPluginWindow;
};