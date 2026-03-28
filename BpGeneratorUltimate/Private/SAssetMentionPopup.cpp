// Copyright 2026, BlueprintsLab, All rights reserved
#include "SAssetMentionPopup.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "WidgetBlueprint.h"
#include "Animation/AnimBlueprint.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Materials/Material.h"
#include "Engine/DataTable.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#define LOCTEXT_NAMESPACE "SAssetMentionPopup"
void SAssetMentionPopup::Construct(const FArguments& InArgs)
{
	OnAssetSelected = InArgs._OnAssetSelected;
	OnPickerDismissed = InArgs._OnPickerDismissed;
	CurrentSearchQuery = InArgs._InitialSearchText;
	PopulateAssetInventory();
	ApplySearchFilter();
	const FLinearColor BorderColor = FLinearColor(0.1f, 0.7f, 0.9f);
	const FLinearColor BgColor = FLinearColor(0.08f, 0.08f, 0.1f);
	const FLinearColor TextColor = FLinearColor(0.9f, 0.9f, 0.9f);
	const FLinearColor HoverColor = FLinearColor(0.15f, 0.15f, 0.18f);
	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(BorderColor)
		.Padding(0)
		[
			SNew(SBorder)
			.BorderBackgroundColor(BgColor)
			.Padding(8.0f)
			[
				SNew(SBox)
				.MinDesiredWidth(320.0f)
				.MaxDesiredHeight(380.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 6.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AssetPickerTitle", "REFERENCED ASSETS"))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
						.ColorAndOpacity(FSlateColor(TextColor))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(AssetListView, SListView<TSharedPtr<FAssetRefItem>>)
						.ListItemsSource(&DisplayList)
						.OnGenerateRow(this, &SAssetMentionPopup::GenerateRowWidget)
						.OnSelectionChanged(this, &SAssetMentionPopup::OnListSelectionChanged)
						.OnMouseButtonDoubleClick(this, &SAssetMentionPopup::OnItemDoubleClicked)
						.SelectionMode(ESelectionMode::Single)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 6.0f, 0, 0)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AssetPickerHint", "↑↓ Navigate | Enter Select | Esc Close"))
						.Font(FAppStyle::GetFontStyle("SmallFont"))
						.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.9f))
					]
				]
			]
		]
	];
	if (DisplayList.Num() > 0)
	{
		AssetListView->SetSelection(DisplayList[0]);
	}
}
bool SAssetMentionPopup::HandleKeyDown(const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ClosePopup();
		return true;
	}
	else if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		ConfirmCurrentSelection();
		return true;
	}
	else if (InKeyEvent.GetKey() == EKeys::Up)
	{
		MoveSelectionUp();
		return true;
	}
	else if (InKeyEvent.GetKey() == EKeys::Down)
	{
		MoveSelectionDown();
		return true;
	}
	return false;
}
void SAssetMentionPopup::PopulateAssetInventory()
{
	MasterInventory.Empty();
	IndexBlueprintAssets();
	IndexSourceFiles();
	MasterInventory.Sort([](const TSharedPtr<FAssetRefItem>& A, const TSharedPtr<FAssetRefItem>& B)
	{
		if (A->GroupName != B->GroupName)
		{
			return A->GroupName < B->GroupName;
		}
		return A->Label < B->Label;
	});
}
void SAssetMentionPopup::IndexBlueprintAssets()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FTopLevelAssetPath> ClassPaths;
	ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
	ClassPaths.Add(UAnimBlueprint::StaticClass()->GetClassPathName());
	ClassPaths.Add(UBehaviorTree::StaticClass()->GetClassPathName());
	ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
	ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	for (const FTopLevelAssetPath& ClassPath : ClassPaths)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByClass(ClassPath, Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			FString PackagePath = Asset.PackagePath.ToString();
			if (!PackagePath.StartsWith(TEXT("/Game")))
			{
				continue;
			}
			EAssetRefType RefType = EAssetRefType::Blueprint;
			FString GroupLabel = TEXT("Blueprints");
			FString ClassName = ClassPath.ToString();
			if (ClassName.Contains(TEXT("WidgetBlueprint")))
			{
				RefType = EAssetRefType::WidgetBlueprint;
				GroupLabel = TEXT("Widget Blueprints");
			}
			else if (ClassName.Contains(TEXT("AnimBlueprint")))
			{
				RefType = EAssetRefType::AnimBlueprint;
				GroupLabel = TEXT("Animation Blueprints");
			}
			else if (ClassName.Contains(TEXT("BehaviorTree")))
			{
				RefType = EAssetRefType::BehaviorTree;
				GroupLabel = TEXT("Behavior Trees");
			}
			else if (ClassName.Contains(TEXT("Material")))
			{
				RefType = EAssetRefType::Material;
				GroupLabel = TEXT("Materials");
			}
			else if (ClassName.Contains(TEXT("DataTable")))
			{
				RefType = EAssetRefType::DataTable;
				GroupLabel = TEXT("Data Tables");
			}
			FString ShortPath = Asset.PackageName.ToString();
			MasterInventory.Add(MakeShared<FAssetRefItem>(
				Asset.AssetName.ToString(),
				ShortPath,
				GroupLabel,
				RefType
			));
		}
	}
}
void SAssetMentionPopup::IndexSourceFiles()
{
	FString SourceDir = FPaths::ProjectDir() / TEXT("Source");
	if (!FPaths::DirectoryExists(SourceDir))
	{
		return;
	}
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFilesRecursive(FoundFiles, *SourceDir, TEXT("*.h"), true, false);
	IFileManager::Get().FindFilesRecursive(FoundFiles, *SourceDir, TEXT("*.cpp"), true, false);
	for (const FString& FilePath : FoundFiles)
	{
		FString FileName = FPaths::GetCleanFilename(FilePath);
		FString Extension = FPaths::GetExtension(FilePath).ToLower();
		EAssetRefType RefType = Extension == TEXT("h") ? EAssetRefType::CppHeader : EAssetRefType::CppSource;
		FString GroupLabel = Extension == TEXT("h") ? TEXT("C++ Headers") : TEXT("C++ Sources");
		FString RelativePath = FilePath;
		FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());
		MasterInventory.Add(MakeShared<FAssetRefItem>(
			FileName,
			RelativePath,
			GroupLabel,
			RefType
		));
	}
}
void SAssetMentionPopup::ApplySearchFilter()
{
	DisplayList.Empty();
	if (CurrentSearchQuery.IsEmpty())
	{
		const int32 MaxItems = 100;
		for (int32 i = 0; i < FMath::Min(MasterInventory.Num(), MaxItems); ++i)
		{
			DisplayList.Add(MasterInventory[i]);
		}
	}
	else
	{
		FString SearchLower = CurrentSearchQuery.ToLower();
		for (const TSharedPtr<FAssetRefItem>& Item : MasterInventory)
		{
			if (Item->Label.ToLower().StartsWith(SearchLower))
			{
				DisplayList.Add(Item);
			}
		}
	}
	if (AssetListView.IsValid())
	{
		AssetListView->RequestListRefresh();
		if (DisplayList.Num() > 0)
		{
			AssetListView->SetSelection(DisplayList[0]);
		}
	}
}
void SAssetMentionPopup::UpdateSearchFilter(const FString& SearchText)
{
	CurrentSearchQuery = SearchText;
	ApplySearchFilter();
}
void SAssetMentionPopup::RebuildAssetList()
{
	PopulateAssetInventory();
	ApplySearchFilter();
}
void SAssetMentionPopup::MoveSelectionDown()
{
	if (DisplayList.Num() == 0)
	{
		return;
	}
	TArray<TSharedPtr<FAssetRefItem>> Selected;
	AssetListView->GetSelectedItems(Selected);
	int32 CurrentIdx = 0;
	if (Selected.Num() > 0)
	{
		CurrentIdx = DisplayList.IndexOfByKey(Selected[0]);
	}
	int32 NextIdx = FMath::Min(CurrentIdx + 1, DisplayList.Num() - 1);
	AssetListView->SetSelection(DisplayList[NextIdx]);
	AssetListView->RequestScrollIntoView(DisplayList[NextIdx]);
}
void SAssetMentionPopup::MoveSelectionUp()
{
	if (DisplayList.Num() == 0)
	{
		return;
	}
	TArray<TSharedPtr<FAssetRefItem>> Selected;
	AssetListView->GetSelectedItems(Selected);
	int32 CurrentIdx = 0;
	if (Selected.Num() > 0)
	{
		CurrentIdx = DisplayList.IndexOfByKey(Selected[0]);
	}
	int32 PrevIdx = FMath::Max(CurrentIdx - 1, 0);
	AssetListView->SetSelection(DisplayList[PrevIdx]);
	AssetListView->RequestScrollIntoView(DisplayList[PrevIdx]);
}
void SAssetMentionPopup::ConfirmCurrentSelection()
{
	TArray<TSharedPtr<FAssetRefItem>> Selected;
	AssetListView->GetSelectedItems(Selected);
	if (Selected.Num() > 0 && Selected[0].IsValid())
	{
		OnAssetSelected.ExecuteIfBound(*Selected[0]);
	}
}
TSharedRef<ITableRow> SAssetMentionPopup::GenerateRowWidget(TSharedPtr<FAssetRefItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FAssetRefItem>>, OwnerTable)
		.Padding(FMargin(8.0f, 4.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Label))
				.Font(FAppStyle::GetFontStyle("NormalFont"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->GroupName))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
				.ColorAndOpacity(FSlateColor(FStyleColors::AccentBlue))
			]
		];
}
void SAssetMentionPopup::OnListSelectionChanged(TSharedPtr<FAssetRefItem> Item, ESelectInfo::Type SelectInfo)
{
}
void SAssetMentionPopup::OnItemDoubleClicked(TSharedPtr<FAssetRefItem> Item)
{
	if (Item.IsValid())
	{
		OnAssetSelected.ExecuteIfBound(*Item);
	}
}
FReply SAssetMentionPopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ClosePopup();
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		ConfirmCurrentSelection();
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Up)
	{
		MoveSelectionUp();
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Down)
	{
		MoveSelectionDown();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
void SAssetMentionPopup::ClosePopup()
{
	OnPickerDismissed.ExecuteIfBound();
}
#undef LOCTEXT_NAMESPACE
