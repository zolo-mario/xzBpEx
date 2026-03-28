// Copyright 2026, BlueprintsLab, All rights reserved
#include "BpGeneratorUltimateCommands.h"
#define LOCTEXT_NAMESPACE "FBpGeneratorUltimateModule"
void FBpGeneratorUltimateCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "BpGeneratorUltimate", "Execute BpGeneratorUltimate action", EUserInterfaceActionType::Button, FInputChord());
}
#undef LOCTEXT_NAMESPACE
