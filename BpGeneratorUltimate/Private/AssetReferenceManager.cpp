// Copyright 2026, BlueprintsLab, All rights reserved
#include "AssetReferenceManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/EditorEngine.h"
#include "Blueprint/UserWidgetBlueprint.h"
#include "Engine/Blueprint.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Engine/DataTable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BpGraphDescriber.h"
#include "BtGraphDescriber.h"
#include "MaterialGraphDescriber.h"
#include "EditorAssetLibrary.h"
#include "SBpGeneratorUltimateWidget.h"
FAssetReferenceManager& FAssetReferenceManager::Get()
{
	static FAssetReferenceManager Instance;
	return Instance;
}
FAssetReferenceManager::FAssetReferenceManager()
{
}
void FAssetReferenceManager::LinkAsset(const FString& AssetPath, const FString& DisplayLabel, EAssetRefType Type)
{
	FLinkedAsset NewLink;
	NewLink.LinkId = FGuid::NewGuid();
	NewLink.DisplayLabel = DisplayLabel;
	NewLink.FullAssetPath = AssetPath;
	NewLink.Category = Type;
	NewLink.SummaryText = GenerateAssetSummary(AssetPath, Type);
	LinkedAssets.Add(NewLink);
	NotifyChange();
}
FString FAssetReferenceManager::GenerateAssetSummary(const FString& AssetPath, EAssetRefType Type)
{
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!LoadedAsset)
	{
		return FString::Printf(TEXT("Failed to load asset at: %s"), *AssetPath);
	}
	FString Summary;
	if (Type == EAssetRefType::Blueprint || Type == EAssetRefType::WidgetBlueprint || Type == EAssetRefType::AnimBlueprint)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset))
		{
			Summary = FBpSummarizerr::Summarize(Blueprint);
		}
	}
	else if (Type == EAssetRefType::BehaviorTree)
	{
		if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(LoadedAsset))
		{
			FBtGraphDescriber Describer;
			Summary = Describer.Describe(BehaviorTree);
		}
	}
	else if (Type == EAssetRefType::Material)
	{
		if (UMaterial* Material = Cast<UMaterial>(LoadedAsset))
		{
			FMaterialGraphDescriber Describer;
			Summary = Describer.Describe(Material);
		}
	}
	else if (Type == EAssetRefType::MaterialInstance)
	{
		if (UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(LoadedAsset))
		{
			Summary = FString::Printf(TEXT("Material Instance: %s\nParent: %s\n"),
				*MaterialInstance->GetName(),
				MaterialInstance->Parent ? *MaterialInstance->Parent->GetName() : TEXT("None"));
		}
	}
	else if (Type == EAssetRefType::DataTable)
	{
		if (UDataTable* DataTable = Cast<UDataTable>(LoadedAsset))
		{
			Summary = FString::Printf(TEXT("Data Table: %s\nRow Struct: %s\nRow Count: %d\n"),
				*DataTable->GetName(),
				DataTable->RowStruct ? *DataTable->RowStruct->GetName() : TEXT("None"),
				DataTable->GetRowMap().Num());
		}
	}
	if (Summary.IsEmpty())
	{
		Summary = FString::Printf(TEXT("Asset: %s (Type: %s)\n"), *LoadedAsset->GetName(), *LoadedAsset->GetClass()->GetName());
	}
	return Summary;
}
void FAssetReferenceManager::UnlinkAsset(const FGuid& LinkId)
{
	LinkedAssets.RemoveAll([&](const FLinkedAsset& Link) { return Link.LinkId == LinkId; });
	NotifyChange();
}
void FAssetReferenceManager::UnlinkAll()
{
	LinkedAssets.Empty();
	NotifyChange();
}
TArray<FLinkedAsset> FAssetReferenceManager::GetLinkedAssets() const
{
	return LinkedAssets;
}
FString FAssetReferenceManager::GetSummaryForAI() const
{
	if (LinkedAssets.Num() == 0)
	{
		return FString();
	}
	FString Summary = TEXT("=== REFERENCED ASSETS ===\n\n");
	for (const FLinkedAsset& Link : LinkedAssets)
	{
		FString TypeName;
		switch (Link.Category)
		{
		case EAssetRefType::Blueprint: TypeName = TEXT("Blueprint"); break;
		case EAssetRefType::WidgetBlueprint: TypeName = TEXT("Widget Blueprint"); break;
		case EAssetRefType::AnimBlueprint: TypeName = TEXT("Anim Blueprint"); break;
		case EAssetRefType::BehaviorTree: TypeName = TEXT("Behavior Tree"); break;
		case EAssetRefType::Material: TypeName = TEXT("Material"); break;
		case EAssetRefType::MaterialInstance: TypeName = TEXT("Material Instance"); break;
		case EAssetRefType::DataTable: TypeName = TEXT("Data Table"); break;
		case EAssetRefType::Struct: TypeName = TEXT("Struct"); break;
		case EAssetRefType::Enum: TypeName = TEXT("Enum"); break;
		default: TypeName = TEXT("Asset"); break;
		}
		Summary += FString::Printf(TEXT("### [%s] %s\nPath: %s\n\n%s\n\n"),
			*TypeName, *Link.DisplayLabel, *Link.FullAssetPath, *Link.SummaryText);
	}
	return Summary;
}
FString FAssetReferenceManager::FormatAsMarkdown() const
{
	if (LinkedAssets.Num() == 0)
	{
		return FString();
	}
	FString Markdown = TEXT("## Referenced Assets\n\n");
	for (const FLinkedAsset& Link : LinkedAssets)
	{
		Markdown += FString::Printf(TEXT("- **%s** (`%s`)\n"), *Link.DisplayLabel, *Link.FullAssetPath);
	}
	return Markdown;
}
void FAssetReferenceManager::NotifyChange()
{
	OnLinksChanged.Broadcast();
}
