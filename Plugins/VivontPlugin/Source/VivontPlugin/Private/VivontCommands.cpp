// Copyright (c) 2025, Derek. All rights reserved.

#include "VivontCommands.h"

#define LOCTEXT_NAMESPACE "FVivontPluginModule"

void FVivontCommands::RegisterCommands()
{
    UI_COMMAND(OpenPluginWindow, "Vivont", "Open the Vivont Plugin Window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE