// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "Framework/Commands/Commands.h"
#include "BpGeneratorUltimateStyle.h"
class FBpGeneratorUltimateCommands : public TCommands<FBpGeneratorUltimateCommands>
{
public:
	FBpGeneratorUltimateCommands()
		: TCommands<FBpGeneratorUltimateCommands>(TEXT("BpGeneratorUltimate"), NSLOCTEXT("Contexts", "BpGeneratorUltimate", "BpGeneratorUltimate Plugin"), NAME_None, FBpGeneratorUltimateStyle::GetStyleSetName())
	{
	}
	virtual void RegisterCommands() override;
public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
