// Copyright 2026, BlueprintsLab, All rights reserved
#include "BpGeneratorUltimate.h"
#include "BpGeneratorUltimateStyle.h"
#include "BpGeneratorUltimateCommands.h"
#include "UIConfigManager.h"
#include "Misc/MessageDialog.h"
const TCHAR* GbB = TEXT("_pAL!v");
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
const TCHAR* GpPluginName = TEXT("BpGeneratorUltimate");
const TCHAR* GpPluginsDir = TEXT("Plugins");
const TCHAR* GpMarketplaceDir = TEXT("Marketplace");
#include "Engine/Blueprint.h"
#include "ToolMenus.h"
bool abc = true;
const TCHAR* GbC = TEXT("2_@");
const int32 GbD = 4;
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "GameFramework/Actor.h"
const TCHAR* GbA = TEXT("bldr_#k7F");
#include "SBpGeneratorUltimateWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Misc/ConfigCacheIni.h"
const int32 GpluginVariable = 24;
#include "WebBrowserModule.h"
#include "Engine/Engine.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
static const FName BpGeneratorUltimateTabName("BpGeneratorUltimate");
#define LOCTEXT_NAMESPACE "FBpGeneratorUltimateModule"
void FBpGeneratorUltimateModule::GenerateFunctionLists()
{
    auto GetFunctionsForClass = [](UClass* InClass, TArray<FString>& OutFunctions)
    {
        if (!InClass) return;
        for (TFieldIterator<UFunction> FuncIt(InClass, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
        {
            UFunction* Function = *FuncIt;
            if (Function->HasAnyFunctionFlags(FUNC_BlueprintCallable) &&
               !Function->HasAnyFunctionFlags(FUNC_BlueprintEvent | FUNC_Delegate) &&
               !Function->GetName().Contains("DEPRECATED"))
            {
                OutFunctions.Add(Function->GetName());
            }
        }
        OutFunctions.Sort();
    };
    TArray<FString> MathFuncs, StringFuncs, ActorFuncsList;
    GetFunctionsForClass(UKismetMathLibrary::StaticClass(), MathFuncs);
    KismetMathFunctions = FString::Join(MathFuncs, TEXT("\n- "));
    GetFunctionsForClass(UKismetStringLibrary::StaticClass(), StringFuncs);
    KismetStringFunctions = FString::Join(StringFuncs, TEXT("\n- "));
    GetFunctionsForClass(AActor::StaticClass(), ActorFuncsList);
    ActorFunctions = FString::Join(ActorFuncsList, TEXT("\n- "));
}
void FBpGeneratorUltimateModule::StartupModule()
{
    IWebBrowserModule::Get();
    TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(GpPluginName);
    if (ThisPlugin.IsValid())
    {
        const FString PluginBaseDir = ThisPlugin->GetBaseDir();
        const FString EngineDir = FPaths::EngineDir();
        const FString ExpectedMarketplacePath = FPaths::Combine(EngineDir, GpPluginsDir, GpMarketplaceDir);
        if (FPaths::IsUnderDirectory(PluginBaseDir, ExpectedMarketplacePath))
        {
            abc = true;
        }
    }
    if (!abc)
    {
        return;
    }
    FBpGeneratorUltimateStyle::Initialize();
    FBpGeneratorUltimateStyle::ReloadTextures();
    FBpGeneratorUltimateCommands::Register();
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FBpGeneratorUltimateModule::OnShowFirstTimeStartupDialog), 5.0f);
    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FBpGeneratorUltimateCommands::Get().PluginAction,
        FExecuteAction::CreateRaw(this, &FBpGeneratorUltimateModule::PluginButtonClicked),
        FCanExecuteAction());
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBpGeneratorUltimateModule::RegisterMenus));
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    ContentBrowserSelectionChangedHandle = ContentBrowserModule.GetOnAssetSelectionChanged().AddRaw(this, &FBpGeneratorUltimateModule::OnContentBrowserSelectionChanged);
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(BpGeneratorUltimateTabName, FOnSpawnTab::CreateRaw(this, &FBpGeneratorUltimateModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("BpGeneratorUltimateTabTitle", "Ultimate Engine Copilot"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    GenerateFunctionLists();
    FUIConfigManager::Get().Initialize();
}
void FBpGeneratorUltimateModule::ShutdownModule()
{
    if (!abc)
    {
        return;
    }
    FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
    if (FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
    {
        FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
        ContentBrowserModule.GetOnAssetSelectionChanged().Remove(ContentBrowserSelectionChangedHandle);
    }
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FBpGeneratorUltimateStyle::Shutdown();
    FBpGeneratorUltimateCommands::Unregister();
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BpGeneratorUltimateTabName);
}
void FBpGeneratorUltimateModule::OnContentBrowserSelectionChanged(const TArray<FAssetData>& SelectedAssets, bool bIsPrimaryBrowser)
{
    if (bIsPrimaryBrowser && SelectedAssets.Num() == 1)
    {
        UObject* SelectedObject = SelectedAssets[0].GetAsset();
        if (UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject))
        {
            TargetBlueprint = Blueprint;
            UE_LOG(LogTemp, Log, TEXT("Target Blueprint set to: %s"), *Blueprint->GetName());
            return;
        }
    }
    TargetBlueprint = nullptr;
}
void FBpGeneratorUltimateModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(BpGeneratorUltimateTabName);
}
TSharedRef<SDockTab> FBpGeneratorUltimateModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::MajorTab)
        [
            SNew(SBpGeneratorUltimateWidget)
        ];
}
void FBpGeneratorUltimateModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    const TArray<FName> WindowMenuNames = {
        "LevelEditor.MainMenu.Window",
        "BlueprintEditor.MainMenu.Window",
        "AnimationBlueprintEditor.MainMenu.Window",
        "MaterialEditor.MainMenu.Window",
        "BehaviorTreeEditor.MainMenu.Window",
        "WidgetBlueprintEditor.MainMenu.Window"
    };
    for (const FName& MenuName : WindowMenuNames)
    {
        if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName))
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FBpGeneratorUltimateCommands::Get().PluginAction, PluginCommands);
        }
    }
    {
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        if (ToolbarMenu)
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("UltimateBpGen_Tool");
            FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FBpGeneratorUltimateCommands::Get().PluginAction));
            Entry.SetCommandList(PluginCommands);
        }
    }
    const TArray<FName> ToolbarNames = {
        "BlueprintEditor.ToolBar",
        "WidgetBlueprintEditor.ToolBar",
        "AnimationBlueprintEditor.ToolBar",
        "MaterialEditor.ToolBar",
        "BehaviorTreeEditor.ToolBar",
        "MaterialInstanceEditor.ToolBar",
        "Persona.ToolBar"
    };
    for (const FName& ToolbarName : ToolbarNames)
    {
        if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(ToolbarName))
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Asset");
            FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FBpGeneratorUltimateCommands::Get().PluginAction));
            Entry.SetCommandList(PluginCommands);
        }
    }
}
bool FBpGeneratorUltimateModule::OnShowFirstTimeStartupDialog(float DeltaTime)
{
    if (!GEditor || !GEditor->GetEditorWorldContext().World())
    {
        return true;
    }
    bool bNeverShowAgain = false;
    GConfig->GetBool(TEXT("BpGeneratorUltimate"), TEXT("bNeverShowWelcomeDialog"), bNeverShowAgain, GEditorPerProjectIni);
    if (!bNeverShowAgain)
    {
        ShowWelcomePopup();
    }
    return false;
}
void FBpGeneratorUltimateModule::ShowWelcomePopup()
{
    TSharedPtr<bool> bNeverShowAgainPtr = MakeShared<bool>(false);
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("ULTIMATE BLUEPRINT GENERATOR - BluepintsLab(Roly Dev)")))
        .ClientSize(FVector2D(640, 500))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .HasCloseButton(true)
        .IsTopmostWindow(true)
        .FocusWhenFirstShown(true);
    TSharedRef<SVerticalBox> DialogContent = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(10).HAlign(HAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Thank you for buying the Ultimate Blueprint Generator")))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(10)
        [
            SNew(STextBlock).AutoWrapText(true)
                .Text(FText::FromString(TEXT("Welcome to your new AI Co-Pilot. This tool was built for one simple reason: to break down the barrier between your ideas and the editor. Whether you're learning the ropes or a seasoned pro looking to prototype faster, this is your creative partner. Ask it anything. Build anything. If you find it valuable, a rating on the marketplace means the world to a solo developer.")))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(10)
        [
            SNew(STextBlock).AutoWrapText(true)
                .Text(FText::FromString(TEXT("You are now part of the journey. Your feedback is the single most important part of this tool's evolution. Every suggestion and idea you share on our Discord helps shape the future of AI-driven development in Unreal. Let's build that future together.")))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 20)
        [
            SNew(STextBlock).AutoWrapText(true)
                .Text(FText::FromString(TEXT("The documentation is the fastest way to get started. If you have any questions or just want to share what you're building, join our community on Discord.")))
        ]
+ SVerticalBox::Slot().AutoHeight().Padding(10).HAlign(HAlign_Center)
[
    SNew(SUniformGridPanel).SlotPadding(FMargin(10.f))
    + SUniformGridPanel::Slot(0, 0)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("Open Documentation")))
        .OnClicked_Lambda([]() {
            FPlatformProcess::LaunchURL(TEXT("https://docs.google.com/document/d/1ljFb3e7eKGW0rpxkya_uOX2hfWpWOWFnbXqsMC2vIw4/edit?usp=sharing"), nullptr, nullptr);
            return FReply::Handled();
        })
    ]
    + SUniformGridPanel::Slot(1, 0)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("View Roadmap & Changelog")))
        .OnClicked_Lambda([]() {
            FPlatformProcess::LaunchURL(TEXT("https://trello.com/b/qmZ1t7vZ/ultimate-blueprint-generator"), nullptr, nullptr);
            return FReply::Handled();
        })
    ]
    + SUniformGridPanel::Slot(0, 1)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("Discord Server")))
        .OnClicked_Lambda([]() {
            FPlatformProcess::LaunchURL(TEXT("https://discord.gg/w65xMuxEhb"), nullptr, nullptr);
            return FReply::Handled();
        })
    ]
    + SUniformGridPanel::Slot(1, 1)
    [
        SNew(SButton)
        .Text(FText::FromString(TEXT("FAB Profile")))
        .OnClicked_Lambda([]() {
            FPlatformProcess::LaunchURL(TEXT("https://www.fab.com/sellers/BlueprintsLab"), nullptr, nullptr);
            return FReply::Handled();
        })
    ]
]
        + SVerticalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(20)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().VAlign(VAlign_Center).Padding(0, 0, 20, 0)
            [
                SNew(SCheckBox)
                .OnCheckStateChanged_Lambda([bNeverShowAgainPtr](ECheckBoxState NewState) {
                    *bNeverShowAgainPtr = (NewState == ECheckBoxState::Checked);
                })
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Never show again")))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Close")))
                .OnClicked_Lambda([DialogWindow, bNeverShowAgainPtr]() {
                    if (*bNeverShowAgainPtr)
                    {
                        GConfig->SetBool(TEXT("BpGeneratorUltimate"), TEXT("bNeverShowWelcomeDialog"), true, GEditorPerProjectIni);
                        GConfig->Flush(false, GEditorPerProjectIni);
                    }
                    DialogWindow->RequestDestroyWindow();
                    return FReply::Handled();
                })
            ]
        ];
    DialogWindow->SetContent(DialogContent);
    FSlateApplication::Get().AddWindow(DialogWindow);
}
#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FBpGeneratorUltimateModule, BpGeneratorUltimate)
