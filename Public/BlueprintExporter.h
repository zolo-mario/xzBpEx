// Copyright (c) 2025 Infinito Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintExporterModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<class FExtender> OnExtendContentBrowser(const TArray<FAssetData>& SelectedAssets);
	void AddExportOption(class FMenuBuilder& MenuBuilder, const TArray<FAssetData>& SelectedAssets);
};
