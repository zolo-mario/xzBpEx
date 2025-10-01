// Copyright (c) 2025 Infinito Studio. All Rights Reserved.

#include "BlueprintExporter.h"
#include "Modules/ModuleManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine/Blueprint.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Tools/ExporterTool.h"

#define LOCTEXT_NAMESPACE "FBlueprintExporterModule"

void FBlueprintExporterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<FContentBrowserMenuExtender_SelectedAssets>& Extenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	Extenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FBlueprintExporterModule::OnExtendContentBrowser));
}

void FBlueprintExporterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

TSharedRef<FExtender> FBlueprintExporterModule::OnExtendContentBrowser(const TArray<FAssetData>& SelectedAssets)
{
    TSharedRef<FExtender> Extender = MakeShareable(new FExtender());

    Extender->AddMenuExtension(
        "GetAssetActions",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder)
            {
                MenuBuilder.AddMenuEntry(
                    FText::FromString("Export Blueprint to Text"),
                    FText::FromString("Save this Blueprint as readable text"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
                        {
                            for (const FAssetData& Asset : SelectedAssets)
                            {
                                if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
                                {
                                    FExporterTool::ExportBlueprintToText(Blueprint);
                                }
                            }
                        }))
                );
            })
    );


    return Extender;
}

void FBlueprintExporterModule::AddExportOption(FMenuBuilder& MenuBuilder, const TArray<FAssetData>& SelectedAssets)
{
    MenuBuilder.AddMenuEntry(
        FText::FromString("Export Blueprint to Text"),
        FText::FromString("Save a readable version of this Blueprint to a .txt file"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateLambda([SelectedAssets]()
            {
                for (const FAssetData& Asset : SelectedAssets)
                {
                    if (UBlueprint* BP = Cast<UBlueprint>(Asset.GetAsset()))
                    {
                        FExporterTool::ExportBlueprintToText(BP);
                    }
                }
            }))
    );
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintExporterModule, BlueprintExporter)