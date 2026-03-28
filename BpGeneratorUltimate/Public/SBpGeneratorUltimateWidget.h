// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/STextComboBox.h"
#include "Dom/JsonObject.h"
#include "AssetReferenceTypes.h"
#include "ApiKeyManager.h"
#include "TextureGenManager.h"
#include "UIConfigManager.h"
class SBpGeneratorUltimateWidget;
class SMultiLineEditableTextBox;
class STextBlock;
class SWebBrowser;
class SWidgetSwitcher;
class SEditableTextBox;
class SSplitter;
class SAssetMentionPopup;
class SButton;
class SImage;
class SCheckBox;
enum class EApiProvider : uint8
{
	Gemini,
	OpenAI,
	Claude,
	DeepSeek,
	Custom
};
struct FApiSettings
{
	FString ApiKey;
	FString CustomBaseURL;
	FString CustomModelName;
	EApiProvider Provider = EApiProvider::Gemini;
};
enum class EThemePreset : uint8
{
	GoldenTurquoise,
	SlateProfessional,
	RoseGold,
	Emerald,
	Cyberpunk,
	SolarFlare,
	DeepOcean,
	LavenderDream,
	Custom
};
struct FBpGeneratorTheme
{
	FLinearColor PrimaryColor;
	FLinearColor SecondaryColor;
	FLinearColor TertiaryColor;
	FLinearColor MainBackgroundColor;
	FLinearColor PanelBackgroundColor;
	FLinearColor BorderBackgroundColor;
	FLinearColor PrimaryTextColor;
	FLinearColor SecondaryTextColor;
	FLinearColor HintTextColor;
	FLinearColor AnalystColor;
	FLinearColor ScannerColor;
	FLinearColor ArchitectColor;
	FLinearColor GradientTop;
	FLinearColor GradientBottom;
	bool bUseGradients;
	float BorderOpacity;
	float CornerRadius;
	EThemePreset Preset = EThemePreset::GoldenTurquoise;
	static FBpGeneratorTheme GetGoldenTurquoiseTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::GoldenTurquoise;
		Theme.PrimaryColor = FLinearColor(1.0f, 0.84f, 0.0f);
		Theme.SecondaryColor = FLinearColor(0.25f, 0.88f, 0.82f);
		Theme.TertiaryColor = FLinearColor(0.13f, 0.55f, 0.41f);
		Theme.MainBackgroundColor = FLinearColor(0.08f, 0.07f, 0.05f);
		Theme.PanelBackgroundColor = FLinearColor(0.12f, 0.10f, 0.08f);
		Theme.BorderBackgroundColor = FLinearColor(0.2f, 0.17f, 0.12f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.98f, 0.95f, 0.90f);
		Theme.SecondaryTextColor = FLinearColor(0.70f, 0.67f, 0.62f);
		Theme.HintTextColor = FLinearColor(0.50f, 0.47f, 0.42f);
		Theme.AnalystColor = FLinearColor(1.0f, 0.6f, 0.0f);
		Theme.ScannerColor = FLinearColor(0.0f, 0.5f, 1.0f);
		Theme.ArchitectColor = FLinearColor(0.0f, 0.7f, 0.3f);
		Theme.GradientTop = FLinearColor(0.15f, 0.12f, 0.08f);
		Theme.GradientBottom = FLinearColor(0.05f, 0.04f, 0.03f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.8f;
		Theme.CornerRadius = 4.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetSlateProfessionalTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::SlateProfessional;
		Theme.PrimaryColor = FLinearColor(0.4f, 0.5f, 0.6f);
		Theme.SecondaryColor = FLinearColor(0.3f, 0.35f, 0.4f);
		Theme.TertiaryColor = FLinearColor(0.2f, 0.25f, 0.3f);
		Theme.MainBackgroundColor = FLinearColor(0.1f, 0.1f, 0.12f);
		Theme.PanelBackgroundColor = FLinearColor(0.15f, 0.15f, 0.17f);
		Theme.BorderBackgroundColor = FLinearColor(0.2f, 0.2f, 0.22f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.95f, 0.95f, 0.97f);
		Theme.SecondaryTextColor = FLinearColor(0.65f, 0.65f, 0.7f);
		Theme.HintTextColor = FLinearColor(0.45f, 0.45f, 0.5f);
		Theme.AnalystColor = FLinearColor(0.4f, 0.5f, 0.6f);
		Theme.ScannerColor = FLinearColor(0.3f, 0.5f, 0.7f);
		Theme.ArchitectColor = FLinearColor(0.3f, 0.6f, 0.4f);
		Theme.GradientTop = FLinearColor(0.18f, 0.18f, 0.2f);
		Theme.GradientBottom = FLinearColor(0.08f, 0.08f, 0.1f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.7f;
		Theme.CornerRadius = 3.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetRoseGoldTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::RoseGold;
		Theme.PrimaryColor = FLinearColor(1.0f, 0.7f, 0.75f);
		Theme.SecondaryColor = FLinearColor(0.8f, 0.5f, 0.55f);
		Theme.TertiaryColor = FLinearColor(0.6f, 0.3f, 0.35f);
		Theme.MainBackgroundColor = FLinearColor(0.08f, 0.05f, 0.06f);
		Theme.PanelBackgroundColor = FLinearColor(0.12f, 0.08f, 0.1f);
		Theme.BorderBackgroundColor = FLinearColor(0.2f, 0.15f, 0.16f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.98f, 0.94f, 0.95f);
		Theme.SecondaryTextColor = FLinearColor(0.7f, 0.66f, 0.67f);
		Theme.HintTextColor = FLinearColor(0.5f, 0.46f, 0.47f);
		Theme.AnalystColor = FLinearColor(1.0f, 0.7f, 0.75f);
		Theme.ScannerColor = FLinearColor(0.7f, 0.5f, 0.8f);
		Theme.ArchitectColor = FLinearColor(0.8f, 0.6f, 0.5f);
		Theme.GradientTop = FLinearColor(0.15f, 0.1f, 0.12f);
		Theme.GradientBottom = FLinearColor(0.05f, 0.03f, 0.04f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.8f;
		Theme.CornerRadius = 5.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetEmeraldTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::Emerald;
		Theme.PrimaryColor = FLinearColor(0.2f, 0.8f, 0.5f);
		Theme.SecondaryColor = FLinearColor(0.15f, 0.6f, 0.4f);
		Theme.TertiaryColor = FLinearColor(0.1f, 0.4f, 0.3f);
		Theme.MainBackgroundColor = FLinearColor(0.05f, 0.08f, 0.06f);
		Theme.PanelBackgroundColor = FLinearColor(0.08f, 0.12f, 0.1f);
		Theme.BorderBackgroundColor = FLinearColor(0.12f, 0.18f, 0.14f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.92f, 0.97f, 0.94f);
		Theme.SecondaryTextColor = FLinearColor(0.62f, 0.72f, 0.67f);
		Theme.HintTextColor = FLinearColor(0.42f, 0.52f, 0.47f);
		Theme.AnalystColor = FLinearColor(0.2f, 0.8f, 0.5f);
		Theme.ScannerColor = FLinearColor(0.3f, 0.7f, 0.6f);
		Theme.ArchitectColor = FLinearColor(0.5f, 0.7f, 0.3f);
		Theme.GradientTop = FLinearColor(0.1f, 0.15f, 0.12f);
		Theme.GradientBottom = FLinearColor(0.04f, 0.06f, 0.05f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.75f;
		Theme.CornerRadius = 4.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetCyberpunkTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::Cyberpunk;
		Theme.PrimaryColor = FLinearColor(1.0f, 0.0f, 0.8f);
		Theme.SecondaryColor = FLinearColor(0.0f, 1.0f, 1.0f);
		Theme.TertiaryColor = FLinearColor(0.6f, 0.0f, 1.0f);
		Theme.MainBackgroundColor = FLinearColor(0.05f, 0.02f, 0.08f);
		Theme.PanelBackgroundColor = FLinearColor(0.1f, 0.05f, 0.12f);
		Theme.BorderBackgroundColor = FLinearColor(0.2f, 0.1f, 0.3f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.95f, 0.95f, 1.0f);
		Theme.SecondaryTextColor = FLinearColor(0.6f, 0.6f, 0.7f);
		Theme.HintTextColor = FLinearColor(0.4f, 0.4f, 0.5f);
		Theme.AnalystColor = FLinearColor(1.0f, 0.0f, 0.8f);
		Theme.ScannerColor = FLinearColor(0.0f, 1.0f, 1.0f);
		Theme.ArchitectColor = FLinearColor(0.5f, 1.0f, 0.3f);
		Theme.GradientTop = FLinearColor(0.15f, 0.05f, 0.2f);
		Theme.GradientBottom = FLinearColor(0.03f, 0.01f, 0.06f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.85f;
		Theme.CornerRadius = 3.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetSolarFlareTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::SolarFlare;
		Theme.PrimaryColor = FLinearColor(1.0f, 0.5f, 0.0f);
		Theme.SecondaryColor = FLinearColor(1.0f, 0.9f, 0.2f);
		Theme.TertiaryColor = FLinearColor(1.0f, 0.3f, 0.1f);
		Theme.MainBackgroundColor = FLinearColor(0.08f, 0.04f, 0.02f);
		Theme.PanelBackgroundColor = FLinearColor(0.12f, 0.06f, 0.03f);
		Theme.BorderBackgroundColor = FLinearColor(0.25f, 0.15f, 0.08f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(1.0f, 0.95f, 0.9f);
		Theme.SecondaryTextColor = FLinearColor(0.75f, 0.65f, 0.55f);
		Theme.HintTextColor = FLinearColor(0.55f, 0.45f, 0.35f);
		Theme.AnalystColor = FLinearColor(1.0f, 0.5f, 0.0f);
		Theme.ScannerColor = FLinearColor(1.0f, 0.7f, 0.1f);
		Theme.ArchitectColor = FLinearColor(1.0f, 0.3f, 0.5f);
		Theme.GradientTop = FLinearColor(0.2f, 0.1f, 0.05f);
		Theme.GradientBottom = FLinearColor(0.05f, 0.02f, 0.01f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.8f;
		Theme.CornerRadius = 4.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetDeepOceanTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::DeepOcean;
		Theme.PrimaryColor = FLinearColor(0.0f, 0.6f, 1.0f);
		Theme.SecondaryColor = FLinearColor(0.0f, 0.4f, 0.7f);
		Theme.TertiaryColor = FLinearColor(0.1f, 0.3f, 0.5f);
		Theme.MainBackgroundColor = FLinearColor(0.02f, 0.04f, 0.08f);
		Theme.PanelBackgroundColor = FLinearColor(0.04f, 0.06f, 0.12f);
		Theme.BorderBackgroundColor = FLinearColor(0.1f, 0.15f, 0.25f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.9f, 0.95f, 1.0f);
		Theme.SecondaryTextColor = FLinearColor(0.55f, 0.65f, 0.8f);
		Theme.HintTextColor = FLinearColor(0.35f, 0.45f, 0.6f);
		Theme.AnalystColor = FLinearColor(0.0f, 0.7f, 0.9f);
		Theme.ScannerColor = FLinearColor(0.2f, 0.5f, 1.0f);
		Theme.ArchitectColor = FLinearColor(0.0f, 0.8f, 0.7f);
		Theme.GradientTop = FLinearColor(0.08f, 0.12f, 0.18f);
		Theme.GradientBottom = FLinearColor(0.02f, 0.03f, 0.06f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.75f;
		Theme.CornerRadius = 4.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetLavenderDreamTheme()
	{
		FBpGeneratorTheme Theme;
		Theme.Preset = EThemePreset::LavenderDream;
		Theme.PrimaryColor = FLinearColor(0.7f, 0.5f, 1.0f);
		Theme.SecondaryColor = FLinearColor(0.9f, 0.7f, 1.0f);
		Theme.TertiaryColor = FLinearColor(0.5f, 0.3f, 0.8f);
		Theme.MainBackgroundColor = FLinearColor(0.06f, 0.04f, 0.1f);
		Theme.PanelBackgroundColor = FLinearColor(0.1f, 0.07f, 0.15f);
		Theme.BorderBackgroundColor = FLinearColor(0.18f, 0.12f, 0.25f, 0.8f);
		Theme.PrimaryTextColor = FLinearColor(0.98f, 0.95f, 1.0f);
		Theme.SecondaryTextColor = FLinearColor(0.7f, 0.65f, 0.8f);
		Theme.HintTextColor = FLinearColor(0.5f, 0.45f, 0.6f);
		Theme.AnalystColor = FLinearColor(0.8f, 0.6f, 1.0f);
		Theme.ScannerColor = FLinearColor(0.6f, 0.4f, 0.9f);
		Theme.ArchitectColor = FLinearColor(0.9f, 0.6f, 0.8f);
		Theme.GradientTop = FLinearColor(0.12f, 0.08f, 0.18f);
		Theme.GradientBottom = FLinearColor(0.04f, 0.02f, 0.08f);
		Theme.bUseGradients = true;
		Theme.BorderOpacity = 0.8f;
		Theme.CornerRadius = 5.0f;
		return Theme;
	}
	static FBpGeneratorTheme GetPreset(EThemePreset Preset)
	{
		switch (Preset)
		{
		case EThemePreset::SlateProfessional: return GetSlateProfessionalTheme();
		case EThemePreset::RoseGold: return GetRoseGoldTheme();
		case EThemePreset::Emerald: return GetEmeraldTheme();
		case EThemePreset::Cyberpunk: return GetCyberpunkTheme();
		case EThemePreset::SolarFlare: return GetSolarFlareTheme();
		case EThemePreset::DeepOcean: return GetDeepOceanTheme();
		case EThemePreset::LavenderDream: return GetLavenderDreamTheme();
		case EThemePreset::GoldenTurquoise:
		default: return GetGoldenTurquoiseTheme();
		}
	}
};
struct FConversationInfo
{
	FString ID;
	FString Title;
	FString LastUpdated;
	int32 TotalPromptTokens = 0;
	int32 TotalCompletionTokens = 0;
	int32 TotalTokens = 0;
};
enum class EAnalystView : uint8
{
	BlueprintArchitect,
	AssetAnalyst,
	ProjectScanner
};
struct FToolCallExtractionResult
{
	bool bHasToolCall = false;
	FString ToolName;
	TSharedPtr<FJsonObject> Arguments;
	FString ConversationalText;
};
struct FToolExecutionResult
{
	bool bSuccess = false;
	bool bIsPending = false;
	FString ResultJson;
	FString ErrorMessage;
};
struct FAttachedImage
{
	FString Name;
	FString MimeType;
	FString Base64Data;
	int32 Width = 0;
	int32 Height = 0;
};
struct FAttachedFileContext
{
	FString FileName;
	FString Content;
	int32 CharCount;
	bool bExpanded;
};
class SConversationListRow : public STableRow<TSharedPtr<FConversationInfo>>
{
public:
	SLATE_BEGIN_ARGS(SConversationListRow) {}
		SLATE_ARGUMENT(TSharedPtr<FConversationInfo>, ConversationInfo)
		SLATE_ARGUMENT(TWeakPtr<SBpGeneratorUltimateWidget>, ScribeWidget)
		SLATE_ARGUMENT(EAnalystView, ViewType)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void EnterEditMode();
private:
	void OnRenameTextCommitted(const FText& InText, ETextCommit::Type InCommitType);
	TSharedPtr<FConversationInfo> ConversationInfo;
	TWeakPtr<SBpGeneratorUltimateWidget> ScribeWidget;
	TSharedPtr<SWidgetSwitcher> WidgetSwitcher;
	TSharedPtr<SEditableTextBox> RenameTextBox;
	EAnalystView ViewType;
};
enum class EUserIntent : uint8
{
	CodeGeneration,
	AssetCreation,
	Model3DGeneration,
	TextureGeneration,
	Chat
};
class SBpGeneratorUltimateWidget : public SCompoundWidget
{
	friend class SConversationListRow;
public:
	SLATE_BEGIN_ARGS(SBpGeneratorUltimateWidget) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
	TSharedPtr<SWidget> OnGetContextMenuForChatList(EAnalystView ForView);
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
private:
	TSharedRef<SWidget> CreateSettingsWidget();
	TSharedRef<SWidget> CreateSlotConfigSection();
	void OnProviderChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void UpdateCustomFieldsVisibility();
	FReply OnShowSettingsClicked();
	FReply OnSaveSettingsClicked();
	void LoadSettings();
	void SaveSettings();
	void OnSlotProviderChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo, int32 SlotIndex);
	FReply OnSaveSlotClicked(int32 SlotIndex);
	FReply OnClearSlotClicked(int32 SlotIndex);
	void RefreshSlotUI();
	void UpdateSlotIndicators();
	void RebuildSettingsWidget();
	FReply OnSwitchToSlotClicked(int32 SlotIndex);
	bool HandleGlobalKeyPress(const FKeyEvent& InKeyEvent);
	TSharedRef<ITableRow> OnGenerateRowForChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo);
	FReply OnNewChatClicked();
	FReply OnSendClicked();
	FReply OnAddAssetContextClicked();
	FReply OnAddNodeContextClicked();
	FReply OnImportContextClicked();
	FReply OnInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	FReply OnImportAnalystContextClicked();
	FReply OnAddArchitectAssetContextClicked();
	FReply OnAddArchitectNodeContextClicked();
	FReply OnImportArchitectContextClicked();
	FReply OnAttachAnalystImageClicked();
	FReply OnAttachArchitectImageClicked();
	FReply OnAttachProjectImageClicked();
	FReply OnRemoveAnalystImageClicked(int32 ImageIndex);
	FReply OnRemoveArchitectImageClicked(int32 ImageIndex);
	FReply OnRemoveProjectImageClicked(int32 ImageIndex);
	void LoadAndAttachImage(const FString& ImagePath, TArray<FAttachedImage>& Attachments);
	void RefreshAnalystImagePreview();
	void RefreshArchitectImagePreview();
	void RefreshProjectImagePreview();
	bool TryPasteImageFromClipboard(TArray<FAttachedImage>& Attachments);
	void OnArchitectInputTextChanged(const FText& NewText);
	void ShowAssetPicker();
	void ShowAssetPickerForProject();
	void OnAssetRefSelected(const FAssetRefItem& SelectedItem);
	void OnAssetPickerDismissed();
	void SendChatRequest();
	void OnApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void RefreshChatHistoryView();
	void OnBrowserLoadError();
	bool OnBrowserLoadUrl(const FString& Method, const FString& Url, FString& Response);
	FString FindAssetPathByName(const FString& AssetName);
	FString LinkifyAssetNames(const FString& Text);
	void SaveManifest();
	void LoadManifest();
	void SaveChatHistory(const FString& ChatID);
	void LoadChatHistory(const FString& ChatID);
	void ExportConversationToFile(const FString& ChatID, const FString& Title, const FString& FileFormat, const TArray<TSharedPtr<FJsonValue>>& ConversationHistory);
	TSharedPtr<SWidgetSwitcher> MainSwitcher;
	TSharedPtr<SBox> SettingsContentBox;
	TSharedPtr<SBox> EntireSettingsBox;
	TSharedPtr<STextBlock> ActiveSlotTextBlock;
	TArray<TSharedPtr<SWidget>> SlotButtonWidgets;
	TSharedPtr<STextComboBox> ProviderComboBox;
	TSharedPtr<SEditableTextBox> ApiKeyInput;
	TSharedPtr<SBox> CustomBaseURLWrapper;
	TSharedPtr<SEditableTextBox> CustomBaseURLInput;
	TSharedPtr<SBox> CustomModelNameWrapper;
	TSharedPtr<SEditableTextBox> CustomModelNameInput;
	TSharedPtr<SListView<TSharedPtr<FConversationInfo>>> ChatListView;
	TSharedPtr<SWebBrowser> ChatHistoryBrowser;
	TSharedPtr<SMultiLineEditableTextBox> InputTextBox;
	FApiSettings CurrentSettings;
	TArray<TSharedPtr<FString>> ProviderOptions;
	TArray<TSharedPtr<FString>> SlotProviderOptions;
	TArray<TSharedPtr<FString>> GeminiModelOptions;
	TArray<TSharedPtr<FString>> OpenAIModelOptions;
	TArray<TSharedPtr<FString>> ClaudeModelOptions;
	TArray<TSharedPtr<FString>> ThemePresetOptions;
	TArray<TSharedPtr<FString>> LanguageOptions;
	TArray<TSharedPtr<FConversationInfo>> ConversationList;
	TArray<TSharedPtr<FJsonValue>> AnalystConversationHistory;
	FString ActiveChatID;
	FString PendingContext;
	bool bIsThinking = false;
	TArray<FAttachedImage> AnalystAttachedImages;
	TSharedPtr<SHorizontalBox> AnalystImagePreviewBox;
	TArray<FAttachedFileContext> AnalystAttachedFiles;
	TSharedPtr<STextBlock> AnalystTokenCounterText;
	TSharedPtr<SWidget> HowToUseOverlay;
	bool bIsShowingHowToUse = false;
	FReply OnHowToUseClicked();
	FReply OnCloseHowToUseClicked();
	TSharedRef<SWidget> CreateHowToUseWidget();
	TMap<FString, FString> CachedHowToTranslations;
	bool bIsTranslatingHowTo = false;
	int32 CurrentHowToViewIndex = 0;
	TSharedPtr<SWindow> TranslationResultWindow;
	FReply OnTranslateHowToClicked();
	void RequestHowToTranslation(const FString& TargetLanguage);
	void OnHowToTranslationComplete(const FString& TranslatedContent, const FString& TargetLanguage);
	FString GetHowToContentForTranslation() const;
	void ShowTranslatedContent(const FString& TranslatedContent, const FString& LanguageName);
	TSharedPtr<SWidget> CustomInstructionsOverlay;
	bool bIsShowingCustomInstructions = false;
	TSharedPtr<SMultiLineEditableTextBox> CustomInstructionsInput;
	FString CustomInstructions;
	FReply OnCustomInstructionsClicked();
	FReply OnCloseCustomInstructionsClicked();
	TSharedRef<SWidget> CreateCustomInstructionsWidget();
	TSharedPtr<SBox> GddOverlay;
	bool bIsShowingGddOverlay = false;
	TSharedPtr<SMultiLineEditableTextBox> GddInput;
	TSharedPtr<SListView<TSharedPtr<FGddFileEntry>>> GddListView;
	TSharedPtr<STextBlock> GddStatusText;
	TArray<TSharedPtr<FGddFileEntry>> GddFiles;
	FGuid SelectedGddFileId;
	bool bShowingGddHelp = false;
	FReply OnGddClicked();
	FReply OnCloseGddClicked();
	TSharedRef<SWidget> CreateGddWidget();
	FReply OnImportGddClicked();
	FReply OnSaveGddClicked();
	FReply OnDeleteGddClicked();
	void OnToggleGddEnabled(const FGuid& FileId);
	TSharedRef<ITableRow> OnGenerateRowForGddList(TSharedPtr<FGddFileEntry> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGddSelectionChanged(TSharedPtr<FGddFileEntry> InItem, ESelectInfo::Type SelectInfo);
	void SaveGddManifest();
	void LoadGddManifest();
	FString GetGddContentForAI() const;
	int32 GetGddTokenCount() const;
	TArray<TSharedPtr<FAiMemoryEntry>> AiMemories;
	TArray<TSharedPtr<FAiMemoryEntry>> SuggestedMemories;
	TSharedPtr<SBox> AiMemoryOverlay;
	bool bIsShowingAiMemoryOverlay = false;
	TSharedPtr<SListView<TSharedPtr<FAiMemoryEntry>>> MemoryListView;
	TSharedPtr<STextBlock> MemoryStatusText;
	TSharedPtr<SMultiLineEditableTextBox> MemoryInput;
	FGuid SelectedMemoryId;
	EAiMemoryCategory SelectedMemoryCategory;
	bool bShowingPendingMemories = false;
	int32 MemoryExtractionThreshold = 7;
	bool bShowingMemoryHelp = false;
	TArray<TSharedPtr<FString>> MemoryThresholdOptions;
	TArray<TSharedPtr<FString>> MemoryCategoryOptions;
	FReply OnAiMemoryClicked();
	FReply OnCloseAiMemoryClicked();
	TSharedRef<SWidget> CreateAiMemoryWidget();
	FReply OnAddMemoryClicked();
	FReply OnSaveMemoryClicked();
	FReply OnDeleteMemoryClicked();
	FReply OnApproveMemoryClicked();
	FReply OnRejectMemoryClicked();
	void OnToggleMemoryEnabled(const FGuid& MemoryId);
	TSharedRef<ITableRow> OnGenerateRowForMemoryList(TSharedPtr<FAiMemoryEntry> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnMemorySelectionChanged(TSharedPtr<FAiMemoryEntry> InItem, ESelectInfo::Type SelectInfo);
	void SaveAiMemoryManifest();
	void LoadAiMemoryManifest();
	FString GetAiMemoryContentForAI() const;
	int32 GetAiMemoryTokenCount() const;
	FString GetCategoryDisplayName(EAiMemoryCategory Category) const;
	FLinearColor GetCategoryColor(EAiMemoryCategory Category) const;
	void ExtractMemoriesFromConversation();
	FReply OnAnalyzePluginClicked();
	void AnalyzePluginSource(const FString& PluginPath);
	TSharedPtr<SWidgetSwitcher> MainViewSwitcher;
	TSharedPtr<SButton> ArchitectTabButton;
	TSharedPtr<SButton> AnalystTabButton;
	TSharedPtr<SButton> ScannerTabButton;
	TSharedPtr<SButton> ModelGenTabButton;
	TSharedPtr<SWebBrowser> ProjectChatBrowser;
	TSharedPtr<SMultiLineEditableTextBox> ProjectInputTextBox;
	TSharedPtr<STextBlock> ScanStatusTextBlock;
	TSharedPtr<SButton> ScanProjectButton;
	TArray<TSharedPtr<FJsonValue>> ProjectConversationHistory;
	bool bIsScanning = false;
	bool bIsProjectThinking = false;
	TArray<FAttachedImage> ProjectAttachedImages;
	TSharedPtr<SHorizontalBox> ProjectImagePreviewBox;
	FReply OnProjectViewSelected();
	FReply OnAnalystViewSelected();
	FReply OnTextureGenViewSelected();
	FReply OnScanProjectClicked();
	void HandleScanAndIndexProject(const TArray<FAssetData>& AssetDataList);
	void UpdateScanStatus(const FString& Message, bool bIsError = false);
	void CheckForExistingIndex();
	FReply OnProjectInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	void OnProjectInputTextChanged(const FText& NewText);
	FReply OnSendProjectQuestionClicked();
	void SendProjectChatRequest();
	void OnProjectApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void RefreshProjectChatView();
	TArray<TSharedPtr<FConversationInfo>> ProjectConversationList;
	TSharedPtr<SListView<TSharedPtr<FConversationInfo>>> ProjectChatListView;
	FString ActiveProjectChatID;
	void SaveProjectManifest();
	void LoadProjectManifest();
	void SaveProjectChatHistory(const FString& ChatID);
	void LoadProjectChatHistory(const FString& ChatID);
	TSharedRef<SWidget> CreateProjectScannerHowToUseWidget();
	TSharedRef<SWidget> CreateBlueprintArchitectHowToUseWidget();
	TSharedRef<SWidget> Create3DModelGenHowToUseWidget();
	TSharedRef<ITableRow> OnGenerateRowForProjectChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnProjectChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo);
	FReply OnNewProjectChatClicked();
	FReply OnGetPerformanceReportClicked();
	void SendPerformanceDataToAI(const FString& RawPerformanceData);
	FReply OnGetProjectOverviewClicked();
	void SendProjectOverviewToAI(const FString& OverviewData);
	TArray<TSharedPtr<FConversationInfo>> ArchitectConversationList;
	TSharedPtr<SListView<TSharedPtr<FConversationInfo>>> ArchitectChatListView;
	TArray<TSharedPtr<FJsonValue>> ArchitectConversationHistory;
	FString ActiveArchitectChatID;
	FString PendingArchitectContext;
	TSharedPtr<SWebBrowser> ArchitectChatBrowser;
	TSharedPtr<STextBlock> ArchitectTokenCounterText;
	TSharedPtr<SMultiLineEditableTextBox> ArchitectInputTextBox;
	TArray<FAttachedImage> ArchitectAttachedImages;
	TArray<FAttachedFileContext> ArchitectAttachedFiles;
	TSharedPtr<SHorizontalBox> ArchitectImagePreviewBox;
	TSharedPtr<SAssetMentionPopup> ArchitectAssetPickerPopup;
	TSharedPtr<SAssetMentionPopup> ProjectAssetPickerPopup;
	int32 AtSymbolPosition = INDEX_NONE;
	bool bAssetPickerOpen = false;
	int32 AssetPickerSourceView = 0;
	FString PreviousArchitectInputText;
	FString PreviousProjectInputText;
	bool bIsArchitectThinking = false;
	int32 LastArchitectUserMessageLength;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveArchitectHttpRequest;
	FString CachedStaticSystemPrompt;
	FString CachedPatternsSection;
	uint32 CachedStaticPromptHash = 0;
	bool bStaticPromptCacheValid = false;
	bool bPatternsCacheValid = false;
	bool bPatternsInjectedForSession = false;
	FString CachedProviderForPrompt;
	EAIInteractionMode CachedInteractionMode = EAIInteractionMode::AutoEdit;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveTextureGenRequest;
	bool bIsTextureGenerating = false;
	bool bHasGeneratedTexture = false;
	TSharedPtr<SEditableTextBox> TexturePromptInput;
	TSharedPtr<STextComboBox> AspectRatioComboBox;
	TSharedPtr<SButton> GenerateTextureButton;
	TSharedPtr<SButton> CancelTextureGenButton;
	TSharedPtr<STextBlock> TextureGenStatusText;
	TSharedPtr<SImage> TexturePreviewImage;
	TArray<TSharedPtr<FString>> AspectRatioOptions;
	FTextureGenResult LastTextureGenResult;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveModelGenRequest;
	bool bIsModelGenerating = false;
	FString CurrentModelTaskId;
	bool bIsPreviewMode = true;
	bool bMeshyApiKeyVisible = false;
	FString MeshyApiKey;
	enum class EMeshGenMode : uint8
	{
		TextTo3D,
		ImageTo3D
	};
	EMeshGenMode CurrentMeshGenMode = EMeshGenMode::TextTo3D;
	FAttachedImage SourceImageAttachment;
	bool bHasSourceImage = false;
	TSharedPtr<SMultiLineEditableTextBox> ModelPromptInput;
	TSharedPtr<SEditableTextBox> ModelNameInput;
	TSharedPtr<SEditableTextBox> ModelSavePathInput;
	TSharedPtr<SEditableTextBox> MeshyApiKeyInput;
	TSharedPtr<SButton> MeshyApiKeyVisibilityButton;
	TSharedPtr<SCheckBox> ModelAutoRefineCheckBox;
	TSharedPtr<SButton> GenerateModelButton;
	TSharedPtr<SButton> CancelModelGenButton;
	TSharedPtr<STextBlock> ModelGenStatusText;
	TArray<TSharedPtr<FString>> Model3DTypeOptions;
	TSharedPtr<STextComboBox> Model3DTypeComboBox;
	TSharedPtr<SCheckBox> ModelPreviewQualityCheckBox;
	TSharedPtr<SVerticalBox> MeshGenCustomFieldsWrapper;
	TSharedPtr<SEditableTextBox> MeshGenCustomEndpointInput;
	TSharedPtr<SEditableTextBox> MeshGenCustomModelNameInput;
	TSharedPtr<SEditableTextBox> MeshGenCustomFormatInput;
	TSharedPtr<SVerticalBox> ImageUploadSectionWrapper;
	TSharedPtr<SButton> BrowseSourceImageButton;
	TSharedPtr<SButton> RemoveSourceImageButton;
	TSharedPtr<SImage> SourceImagePreview;
	TSharedPtr<STextBlock> SourceImagePathText;
	TSharedPtr<FSlateDynamicImageBrush> SourceImageBrush;
	FReply OnToggleMeshyApiKeyVisibility();
	void OnModel3DTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnModelQualityChanged(ECheckBoxState NewState);
	void OnTextTo3DModeClicked(ECheckBoxState NewState);
	void OnImageTo3DModeClicked(ECheckBoxState NewState);
	FReply OnBrowseSourceImageClicked();
	FReply OnRemoveSourceImageClicked();
	void RefreshSourceImagePreview();
	struct FPBRMaterialGenState
	{
		FString MaterialName;
		FString Description;
		FString SavePath;
		bool bIsMetallic = false;
		bool bGenerateAO = true;
		int32 CurrentStep = 0;
		int32 TotalSteps = 5;
		FString BaseColorPath;
		FString NormalPath;
		FString RoughnessPath;
		FString MetallicPath;
		FString AOPath;
		bool bIsGenerating = false;
	};
	FPBRMaterialGenState ActivePBRMaterialGen;
	struct FSlotUIWidgets
	{
		TSharedPtr<SEditableTextBox> NameInput;
		TSharedPtr<STextComboBox> ProviderComboBox;
		TSharedPtr<SEditableTextBox> ApiKeyInput;
		TSharedPtr<SBox> CustomFieldsWrapper;
		TSharedPtr<SEditableTextBox> CustomBaseURLInput;
		TSharedPtr<SEditableTextBox> CustomModelNameInput;
		TSharedPtr<STextBlock> StatusText;
		TSharedPtr<SBox> GeminiModelWrapper;
		TSharedPtr<STextComboBox> GeminiModelComboBox;
		TSharedPtr<SBox> OpenAIModelWrapper;
		TSharedPtr<STextComboBox> OpenAIModelComboBox;
		TSharedPtr<SBox> ClaudeModelWrapper;
		TSharedPtr<STextComboBox> ClaudeModelComboBox;
		TSharedPtr<SEditableTextBox> TextureGenEndpointInput;
		TSharedPtr<SEditableTextBox> TextureGenModelInput;
		TSharedPtr<SCheckBox> TextureGenModeCheckBox;
	};
	FSlotUIWidgets SlotWidgets[MAX_API_KEY_SLOTS];
	TSharedPtr<SButton> SlotButtons[MAX_API_KEY_SLOTS];
	int32 CurrentlyEditingSlotIndex;
	EAIInteractionMode ArchitectInteractionMode = EAIInteractionMode::AutoEdit;
	FReply OnArchitectViewSelected();
	FReply OnNewArchitectChatClicked();
	FReply OnArchitectInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	FReply OnSendArchitectMessageClicked();
	FReply OnStopClicked();
	void OnInteractionModeChanged(EAIInteractionMode NewMode);
	void OnJustChatModeClicked(ECheckBoxState NewState);
	void OnAskBeforeEditModeClicked(ECheckBoxState NewState);
	void OnAutoEditModeClicked(ECheckBoxState NewState);
	bool IsReadOnlyTool(const FString& ToolName) const;
	TSharedRef<ITableRow> OnGenerateRowForArchitectChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnArchitectChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo);
	void SaveArchitectManifest();
	void LoadArchitectManifest();
	void SaveArchitectChatHistory(const FString& ChatID);
	void LoadArchitectChatHistory(const FString& ChatID);
	void RefreshArchitectChatView();
	void SendArchitectChatRequest();
	FString GenerateArchitectSystemPrompt();
	EUserIntent ClassifyUserIntentWithAI(const FString& UserMessage);
	bool SendIntentClassificationRequest(const FString& UserMessage, EUserIntent& OutIntent);
	FString GetCachedStaticSystemPrompt();
	FString GetCachedPatternsSection();
	FString GenerateStaticSystemPromptInternal();
	FString GeneratePatternsSectionInternal();
	void InjectPatternsAsSystemMessage();
	bool ShouldInvalidateStaticCache() const;
	void InvalidatePromptCaches();
	int32 EstimateFullRequestTokens(const TArray<TSharedPtr<FJsonValue>>& Messages);
	void OnArchitectApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	FReply OnGenerateTextureClicked();
	FReply OnCancelTextureGenClicked();
	void OnTextureGenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void ProcessPBRMaterialGenerationStep();
	void OnPBRTextureGenerated(const FTextureGenResult& Result);
	void CreatePBRMaterialFromGeneratedTextures();
	void OnAspectRatioChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void UpdateTextureGenStatus(const FText& Status, const FLinearColor& Color = FLinearColor::White);
	TSharedRef<SWidget> CreateTextureGenWidget();
	FReply OnGenerateModelClicked();
	FReply OnCancelModelGenClicked();
	FReply OnModelGenViewSelected();
	void UpdateModelGenStatus(const FText& Status, const FLinearColor& Color = FLinearColor::White);
	void UpdateModelGenButtons();
	void UpdateConversationTokens(const FString& ChatID, int32 PromptTokens, int32 CompletionTokens = 0);
	int32 EstimateConversationTokens(const TArray<TSharedPtr<FJsonValue>>& ConversationHistory);
	TSharedRef<SWidget> Create3DModelGenWidget();
	int32 CurrentToolCallDepth = 0;
	static constexpr int32 MaxToolCallDepth = 40;
	bool bJustExecutedCodeGenerationTool = false;
	void ResetCodeGenerationToolFlag() { bJustExecutedCodeGenerationTool = false; }
	TSharedPtr<FJsonObject> CachedApiCheatsheet;
	FBpGeneratorTheme CurrentTheme;
	void ApplyTheme();
	void LoadThemeSettings();
	void SaveThemeSettings();
	TSharedPtr<SBorder> CreateThemedBorder(TSharedPtr<SWidget> Content, const FMargin& Padding = FMargin(8));
	TSharedPtr<SButton> CreateThemedButton(FText Text, FOnClicked OnClicked, const FLinearColor& OverrideColor = FLinearColor::Transparent);
	TSharedPtr<STextBlock> CreateThemedTextBlock(FText Text, bool bIsPrimary = true);
	void OnThemePresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnLanguageChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	static FString LanguageNameToCode(const FString& LanguageName);
	FToolCallExtractionResult ExtractToolCallFromResponse(const FString& AiResponse);
	int32 FindMatchingClosingBrace(const FString& Haystack, int32 StartIndex);
	bool TryParseToolCallJson(const FString& JsonString, FToolCallExtractionResult& OutResult);
	void ProcessArchitectResponse(const FString& AiMessage);
	void ContinueConversationWithToolResult(const FString& ToolName, const FToolExecutionResult& Result);
	FToolExecutionResult DispatchToolCall(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments);
	FToolExecutionResult ExecuteTool_ClassifyIntent(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_FindAssetByName(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GenerateBlueprintLogic(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetCurrentFolder(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ListAssetsInFolder(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateBlueprint(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddComponent(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddVariable(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteVariable(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CategorizeVariables(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetAssetSummary(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_SpawnActors(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetAllSceneActors(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateMaterial(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GeneratePBRMaterial(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GenerateTexture(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateMaterialFromTextures(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateModel3D(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_RefineModel3D(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ImageToModel3D(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_SetActorMaterials(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetApiContext(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetSelectedAssets(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetSelectedNodes(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateStruct(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateEnum(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetDataAssetDetails(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_EditComponentProperty(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteComponent(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_SelectActors(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateProjectFolder(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_MoveAsset(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_MoveAssets(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_EditDataAssetDefaults(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetDetailedBlueprintSummary(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetMaterialGraphSummary(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_InsertCode(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteNodes(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetProjectRootPath(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddWidgetToUserWidget(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_EditWidgetProperty(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteWidget(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddWidgetsToLayout(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateWidgetFromLayout(const TSharedPtr<FJsonObject>& Args);
	void FlattenWidgetLayout(const TSharedPtr<FJsonObject>& WidgetDef, const FString& ParentName, TArray<TSharedPtr<FJsonObject>>& OutFlatList);
	FToolExecutionResult ExecuteTool_GetBehaviorTreeSummary(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_GetBatchWidgetProperties(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteUnusedVariables(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_DeleteFunction(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_UndoLastGenerated(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ListPlugins(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_FindPlugin(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ReadCppFile(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_WriteCppFile(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ListPluginFiles(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_ScanDirectory(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_SelectFolder(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CheckProjectSource(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateDataTable(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateInputAction(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_CreateInputMappingContext(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddInputMapping(const TSharedPtr<FJsonObject>& Args);
	FToolExecutionResult ExecuteTool_AddDataTableRows(const TSharedPtr<FJsonObject>& Args);
	FString GetContentBrowserCurrentFolder() const;
	FReply OnToggleSidebarClicked();
	void SetSidebarVisible(bool bVisible);
	EVisibility GetLoadingIndicatorVisibility() const;
	bool IsAnyViewThinking() const;
	TSharedRef<SWidget> CreateBurgerMenuButton();
	TSharedRef<SWidget> CreateLoadingIndicator();
	bool bSidebarVisible = true;
	TSharedPtr<SSplitter> ArchitectSplitter;
	TSharedPtr<SSplitter> AnalystSplitter;
	TSharedPtr<SSplitter> ProjectSplitter;
	TSharedPtr<STextBlock> LoadingIndicatorText;
	double LoadingAnimationTime = 0.0f;
};
class BPGENERATORULTIMATE_API FBpSummarizerr
{
public:
	static FString Summarize(UBlueprint* Blueprint);
};
