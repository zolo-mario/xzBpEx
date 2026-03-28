// Copyright 2026, BlueprintsLab, All rights reserved
#include "SBpGeneratorUltimateWidget.h"
#include "UIConfigManager.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "ToolMenus.h"
#include "Misc/Base64.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "BpGraphDescriber.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "K2Node_Event.h"
#include "EdGraphSchema_K2.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "BlueprintEditor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SWebBrowser.h"
#include "Interfaces/IPluginManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Internationalization/Regex.h"
#include "WebBrowserModule.h"
#include "MaterialGraphDescriber.h"
#include "MaterialNodeDescriber.h"
#include "BtGraphDescriber.h"
#include "Materials/Material.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "UObject/Interface.h"
#include "Kismet2/StructureEditorUtils.h"
#include "IMaterialEditor.h"
#include "BpGeneratorCompiler.h"
#include "BpGeneratorUltimate.h"
#include "BehaviorTreeEditor.h"
#include "Editor/UMGEditor/Public/WidgetBlueprintEditor.h"
#include "MaterialGraph/MaterialGraphNode.h"
#if WITH_EDITOR
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture.h"
#endif
#include <Editor/MaterialEditor/Private/MaterialEditor.h>
#include "Components/ContentWidget.h"
#include "UObject/TextProperty.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "SAssetMentionPopup.h"
#include "AssetReferenceManager.h"
#include "ApiKeyManager.h"
#include "TextureGenManager.h"
#include "MeshAssetManager.h"
#include "Widgets/Images/SImage.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMisc.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_MAC
#include "Mac/MacApplication.h"
#include "Cocoa/Cocoa.h"
#include <AppKit/AppKit.h>
#endif
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SOverlay.h"
#include "Async/Async.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/SavePackage.h"
#include "Engine/World.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialFactoryNew.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "AssetToolsModule.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Framework/Text/TextLayout.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelSlot.h"
#include "UObject/Package.h"
class SConversationListRow;
DEFINE_LOG_CATEGORY_STATIC(LogBpGeneratorUltimate, Log, All);
FString AssembleTextFormat(const TArray<uint8>& PackedData, const FString& ValidationKey)
{
	FString AssembledString;
	TArray<uint8> KeyBytes;
	FTCHARToUTF8 Converter(*ValidationKey);
	KeyBytes.Append((uint8*)Converter.Get(), Converter.Length());
	if (KeyBytes.Num() == 0) return FString();
	for (int32 i = 0; i < PackedData.Num(); ++i)
	{
		AssembledString += (TCHAR)(PackedData[i] ^ KeyBytes[i % KeyBytes.Num()]);
	}
	return AssembledString;
}
const TCHAR* const _obf_E = TEXT("LastKnownState");
template<int32 BufferSize>
static void DescribeWidgetHierarchy(UWidget* Widget, TStringBuilder<BufferSize>& Report, int32 Indent)
{
	if (!Widget) return;
	Report.Append(FString::ChrN(Indent * 2, ' '));
	Report.Appendf(TEXT("- %s (Class: %s)\n"), *Widget->GetFName().ToString(), *Widget->GetClass()->GetName());
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			DescribeWidgetHierarchy(Panel->GetChildAt(i), Report, Indent + 1);
		}
	}
}
static FString PinTypeToString(const FEdGraphPinType& PinType)
{
	if (PinType.PinSubCategoryObject.IsValid())
	{
		return PinType.PinSubCategoryObject->GetName();
	}
	return PinType.PinCategory.ToString();
}
static FLinearColor ParseColor(const FString& ColorStr);
FString FBpSummarizerr::Summarize(UBlueprint* Blueprint)
{
	if (!Blueprint) return TEXT("Invalid Blueprint provided.");
	TStringBuilder<8192> Report;
	TArray<FString> PerformanceIssues;
	bool bTickIsUsed = false;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(Blueprint));
	Report.Appendf(TEXT("--- BLUEPRINT SUMMARY: %s ---\n\n"), *Blueprint->GetName());
	Report.Append(TEXT("--- ASSET ANALYSIS ---\n"));
	TArray<FAssetIdentifier> Referencers;
	AssetRegistryModule.Get().GetReferencers(AssetData.PackageName, Referencers);
	TArray<FAssetIdentifier> Dependencies;
	AssetRegistryModule.Get().GetDependencies(AssetData.PackageName, Dependencies);
	const FString FilePath = FPackageName::LongPackageNameToFilename(AssetData.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
	const int64 FileSize = IFileManager::Get().FileSize(*FilePath);
	Report.Appendf(TEXT("On-Disk Size: %.2f KB\n"), FileSize / 1024.0f);
	Report.Appendf(TEXT("Referenced By (Referencers): %d other assets\n"), Referencers.Num());
	Report.Appendf(TEXT("References (Dependencies): %d other assets\n"), Dependencies.Num());
	Report.Append(TEXT("Hard Asset References:\n"));
	int32 HardRefCount = 0;
	for (const FAssetIdentifier& Dep : Dependencies)
	{
		if (Dep.IsPackage() && Dep.PackageName != AssetData.PackageName && !Dep.PackageName.ToString().StartsWith(TEXT("/Script")))
		{
			Report.Appendf(TEXT("  - %s\n"), *Dep.PackageName.ToString());
			HardRefCount++;
		}
	}
	if (HardRefCount == 0) Report.Append(TEXT("  - None\n"));
	Report.Append(TEXT("\n"));
	Report.Appendf(TEXT("Parent Class: %s\n"), Blueprint->ParentClass ? *Blueprint->ParentClass->GetName() : TEXT("None (Invalid)"));
	if (Blueprint->ImplementedInterfaces.Num() > 0)
	{
		Report.Append(TEXT("Implemented Interfaces:\n"));
		for (const auto& Interface : Blueprint->ImplementedInterfaces)
		{
			if (Interface.Interface) Report.Appendf(TEXT("  - %s\n"), *Interface.Interface->GetName());
		}
	}
	Report.Append(TEXT("\n"));
	if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (WidgetBP->WidgetTree && WidgetBP->WidgetTree->RootWidget)
		{
			Report.Append(TEXT("--- WIDGET HIERARCHY ---\n"));
			DescribeWidgetHierarchy(WidgetBP->WidgetTree->RootWidget, Report, 0);
			Report.Append(TEXT("\n"));
		}
	}
	if (Blueprint->SimpleConstructionScript && Blueprint->SimpleConstructionScript->GetAllNodes().Num() > 0)
	{
		Report.Append(TEXT("--- COMPONENTS ---\n"));
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass) Report.Appendf(TEXT("- %s (Name: %s)\n"), *Node->ComponentClass->GetName(), *Node->GetVariableName().ToString());
		}
		Report.Append(TEXT("\n"));
	}
	if (Blueprint->NewVariables.Num() > 0)
	{
		Report.Append(TEXT("--- VARIABLES ---\n"));
		for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
		{
			FString TypeStr = PinTypeToString(VarDesc.VarType);
			FString MetaData;
			if ((VarDesc.PropertyFlags & CPF_Edit) != 0) MetaData += TEXT("(Instance Editable) ");
			if ((VarDesc.PropertyFlags & CPF_ExposeOnSpawn) != 0) MetaData += TEXT("(Expose on Spawn) ");
			Report.Appendf(TEXT("- %s %s %s\n"), *TypeStr, *VarDesc.VarName.ToString(), *MetaData);
		}
		Report.Append(TEXT("\n"));
	}
	TArray<UEdGraph*> GraphsToAnalyze;
	GraphsToAnalyze.Append(Blueprint->UbergraphPages);
	GraphsToAnalyze.Append(Blueprint->FunctionGraphs);
	FBpGraphDescriber GraphDescriber;
	for (UEdGraph* Graph : GraphsToAnalyze)
	{
		if (!Graph) continue;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				if (EventNode->EventReference.GetMemberName() == FName(TEXT("Tick")))
				{
					UEdGraphPin* ThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
					if (ThenPin && ThenPin->LinkedTo.Num() > 0)
					{
						bTickIsUsed = true;
						int32 TickNodeCount = 0;
						TSet<UEdGraphNode*> VisitedNodes;
						TArray<UEdGraphNode*> NodesToCheck;
						NodesToCheck.Add(EventNode);
						while (NodesToCheck.Num() > 0)
						{
							UEdGraphNode* CurrentNode = NodesToCheck.Pop();
							if (!CurrentNode || VisitedNodes.Contains(CurrentNode)) continue;
							VisitedNodes.Add(CurrentNode);
							TickNodeCount++;
							FString NodeName = CurrentNode->GetClass()->GetName();
							if (NodeName.Contains(TEXT("Cast")) || NodeName.Contains(TEXT("CastTo")))
							{
								PerformanceIssues.AddUnique(TEXT("Cast<> in Event Tick - cache the reference instead"));
							}
							if (NodeName.Contains(TEXT("GetAllActorsOfClass")))
							{
								PerformanceIssues.AddUnique(TEXT("GetAllActorsOfClass in Event Tick - extremely expensive, use interfaces or tags"));
							}
							if (NodeName.Contains(TEXT("SpawnActor")) || NodeName.Contains(TEXT("Spawn")))
							{
								PerformanceIssues.AddUnique(TEXT("SpawnActor in Event Tick - object pooling recommended"));
							}
							if (NodeName.Contains(TEXT("BuildString")) || NodeName.Contains(TEXT("Concat")))
							{
								PerformanceIssues.AddUnique(TEXT("String operations in Event Tick - causes GC pressure"));
							}
							for (UEdGraphPin* Pin : CurrentNode->Pins)
							{
								if (Pin->Direction == EGPD_Output)
								{
									for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
									{
										if (LinkedPin && LinkedPin->GetOwningNode())
										{
											NodesToCheck.Add(LinkedPin->GetOwningNode());
										}
									}
								}
							}
						}
						if (TickNodeCount > 20)
						{
							PerformanceIssues.AddUnique(FString::Printf(TEXT("Event Tick has %d nodes - consider complexity reduction"), TickNodeCount));
						}
						break;
					}
				}
			}
		}
		if (bTickIsUsed) break;
		Report.Appendf(TEXT("--- Analyzing Graph: %s ---\n"), *Graph->GetName());
		TSet<UObject*> AllNodes;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node) AllNodes.Add(Node);
		}
		Report.Append(GraphDescriber.Describe(AllNodes));
		Report.Append(TEXT("\n"));
	}
	if (bTickIsUsed && PerformanceIssues.Num() == 0)
	{
		PerformanceIssues.AddUnique(TEXT("Uses Event Tick - review for optimization opportunities"));
	}
	if (PerformanceIssues.Num() > 0)
	{
		Report.Append(TEXT("\n--- POTENTIAL PERFORMANCE ISSUES ---\n"));
		for (const FString& Issue : PerformanceIssues)
		{
			Report.Appendf(TEXT("! %s\n"), *Issue);
		}
	}
	int32 TotalNodeCount = 0;
	int32 GraphCount = Blueprint->UbergraphPages.Num() + Blueprint->FunctionGraphs.Num();
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph) TotalNodeCount += Graph->Nodes.Num();
	}
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph) TotalNodeCount += Graph->Nodes.Num();
	}
	int32 ComplexityScore = TotalNodeCount * FMath::Max(1, GraphCount);
	Report.Appendf(TEXT("\n--- COMPLEXITY ---\n"));
	Report.Appendf(TEXT("Total Nodes: %d\n"), TotalNodeCount);
	Report.Appendf(TEXT("Total Graphs: %d\n"), GraphCount);
	Report.Appendf(TEXT("Complexity Score: %d\n"), ComplexityScore);
	return FString(Report);
}
const FString POS = TEXT("Y1YZY41FO2520KBS");
static const FName CodeExplainerTabName("CodeExplainer");
#define LOCTEXT_NAMESPACE "SBpGeneratorUltimateWidget"
void SBpGeneratorUltimateWidget::OnBrowserLoadError()
{
	UE_LOG(LogBpGeneratorUltimate, Error, TEXT("SWebBrowser failed to load its content. This is often a first-run timing issue."));
	const FString ErrorHtml = TEXT(
		"<!DOCTYPE html><html><head><style>"
		"body { font-family: sans-serif; background: #2a2a2a; color: #e0e0e0; padding: 20px; line-height: 1.6; }"
		"h3 { color: #ffa07a; }"
		"</style></head><body>"
		"<h3>Content Failed to Load</h3>"
		"<p>The browser component could not render the AI's response.</p>"
		"<p>This can sometimes happen when the plugin is first opened. Please try sending your message again or restarting the chat.</p>"
		"</body></html>"
	);
	if (ChatHistoryBrowser.IsValid())
	{
		ChatHistoryBrowser->LoadString(ErrorHtml, TEXT("about:error"));
	}
}
bool SBpGeneratorUltimateWidget::OnBrowserLoadUrl(const FString& Method, const FString& Url, FString& Response)
{
	if (Url.StartsWith(TEXT("ue://diagram-popout?data=")))
	{
		FString EncodedData = Url.Mid(25);
		FString DiagramData = FGenericPlatformHttp::UrlDecode(EncodedData);
		FString DiagramHtml = FString::Printf(TEXT(
			"<!DOCTYPE html><html><head><style>"
			"body{background:#1e1e1e;margin:0;font-family:'Segoe UI',Arial,sans-serif;overflow:hidden;height:100vh;}"
			"#toolbar{position:fixed;top:10px;left:10px;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:8px 12px;border-radius:4px;border:1px solid #404040;z-index:100;}"
			"#toolbar span{color:#888;font-size:11px;}"
			"#toolbar kbd{background:#444;color:#ccc;padding:2px 6px;border-radius:3px;font-size:10px;border:1px solid #555;}"
			"#zoom-pct{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;border-left:1px solid #404040;padding-left:8px;}"
			"button{background:#444;color:#d4d4d4;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;}"
			"button:hover{background:#555;}"
			"#container{position:absolute;transform-origin:0 0;cursor:grab;width:100%%;height:100%%;display:flex;justify-content:center;align-items:center;}"
			"#container.dragging{cursor:grabbing;}"
			"#container svg{max-width:95vw;max-height:90vh;}"
			"</style></head><body>"
			"<div id='toolbar'>"
			"<span><kbd>Ctrl</kbd>+Scroll zoom | <kbd>Drag</kbd> pan</span>"
			"<span id='zoom-pct'>100%%</span>"
			"<button onclick='resetView()'>Reset</button>"
			"</div>"
			"<div id='container'>%s</div>"
			"<script>"
			"var zoom=100,panX=0,panY=0,isDragging=false,startX,startY;"
			"var container=document.getElementById('container');"
			"function updateView(){container.style.transform='translate('+panX+'px,'+panY+'px) scale('+zoom/100+')';document.getElementById('zoom-pct').textContent=zoom+'%%';}"
			"function resetView(){zoom=100;panX=0;panY=0;updateView();}"
			"document.addEventListener('wheel',function(e){if(e.ctrlKey){e.preventDefault();zoom+=e.deltaY>0?-10:10;zoom=Math.max(20,Math.min(500,zoom));updateView();}},{passive:false});"
			"container.addEventListener('mousedown',function(e){if(e.button===0){isDragging=true;container.classList.add('dragging');startX=e.clientX-panX;startY=e.clientY-panY;e.preventDefault();}});"
			"document.addEventListener('mousemove',function(e){if(isDragging){panX=e.clientX-startX;panY=e.clientY-startY;updateView();}});"
			"document.addEventListener('mouseup',function(){isDragging=false;container.classList.remove('dragging');});"
			"</script></body></html>"
		), *DiagramData);
		TSharedRef<SWindow> DiagramWindow = SNew(SWindow)
			.Title(FText::FromString(TEXT("Diagram Viewer")))
			.ClientSize(FVector2D(1024, 768))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			.AutoCenter(EAutoCenter::PreferredWorkArea);
		TSharedPtr<SWebBrowser> DiagramBrowser;
		DiagramWindow->SetContent(
			SAssignNew(DiagramBrowser, SWebBrowser)
			.SupportsTransparency(false)
			.ShowControls(false)
			.ShowAddressBar(false)
		);
		if (DiagramBrowser.IsValid())
		{
			DiagramBrowser->LoadString(DiagramHtml, TEXT("about:diagram"));
		}
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(DiagramWindow, ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(DiagramWindow);
		}
		Response = TEXT(
			"<!DOCTYPE html><html><head><script>"
			"if(typeof closeModal==='function')closeModal();"
			"setTimeout(function(){history.back();},10);"
			"</script></head><body style='background:#1e1e1e'></body></html>"
		);
		return true;
	}
	if (Url.StartsWith(TEXT("ue://asset?name=")))
	{
		FString AssetName = Url.Mid(16);
		AssetName = FGenericPlatformHttp::UrlDecode(AssetName);
		FString AssetPath = FindAssetPathByName(AssetName);
		if (!AssetPath.IsEmpty() && UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			FAssetData AssetData = UEditorAssetLibrary::FindAssetData(AssetPath);
			FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			CBModule.Get().SyncBrowserToAssets({ AssetData }, true);
		}
		Response = FString::Printf(TEXT(
			"<!DOCTYPE html><html><head><style>"
			"body{background:#1e1e1e;color:#4ec9b0;font-family:'Segoe UI',Arial,sans-serif;padding:20px;text-align:center;}"
			"</style></head><body>"
			"<p>Selected: %s</p>"
			"<p style='color:#888;font-size:12px;'>Content Browser synced</p>"
			"<script>setTimeout(function(){history.back();},50);</script>"
			"</body></html>"
		), *AssetName);
		return true;
	}
	return false;
}
FString SBpGeneratorUltimateWidget::FindAssetPathByName(const FString& AssetName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& Registry = AssetRegistryModule.Get();
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*FString("/Game")));
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetName.ToString() == AssetName)
		{
			return Asset.GetSoftObjectPath().ToString();
		}
	}
	return FString();
}
FString SBpGeneratorUltimateWidget::LinkifyAssetNames(const FString& Text)
{
	TMap<FString, FString> CodeBlockRanges;
	const FRegexPattern CodeBlockPattern(TEXT("(`[^`]*`|```[\\s\\S]*?```)"));
	FRegexMatcher CodeBlockMatcher(CodeBlockPattern, Text);
	while (CodeBlockMatcher.FindNext())
	{
		int32 MatchStart = CodeBlockMatcher.GetMatchBeginning();
		int32 MatchEnd = CodeBlockMatcher.GetMatchEnding();
		FString Placeholder = FString::Printf(TEXT("___CODEBLOCK_%d___"), MatchStart);
		CodeBlockRanges.Add(Placeholder, Text.Mid(MatchStart, MatchEnd - MatchStart));
	}
	FString TextWithoutCode = Text;
	for (auto& Pair : CodeBlockRanges)
	{
		TextWithoutCode = TextWithoutCode.Replace(*Pair.Value, *Pair.Key);
	}
	static TArray<FString> AssetPrefixes = {
		TEXT("BP_"), TEXT("WBP_"), TEXT("ABP_"), TEXT("SM_"), TEXT("SK_"),
		TEXT("M_"), TEXT("MI_"), TEXT("T_"), TEXT("DT_"), TEXT("BT_")
	};
	TMap<FString, FString> AssetLinks;
	for (const FString& Prefix : AssetPrefixes)
	{
		const FRegexPattern AssetPattern(*FString::Printf(TEXT("\\b(%s[A-Za-z0-9_]+)\\b"), *Prefix));
		FRegexMatcher Matcher(AssetPattern, TextWithoutCode);
		while (Matcher.FindNext())
		{
			FString AssetName = Matcher.GetCaptureGroup(1);
			if (!AssetLinks.Contains(AssetName))
			{
				FString AssetPath = FindAssetPathByName(AssetName);
				if (!AssetPath.IsEmpty())
				{
					FString EncodedPath = FGenericPlatformHttp::UrlEncode(AssetPath);
					FString Link = FString::Printf(
						TEXT("<a href=\"ue://asset?path=%s\" class=\"asset-link\">%s</a>"),
						*EncodedPath, *AssetName
					);
					AssetLinks.Add(AssetName, Link);
				}
			}
		}
	}
	for (const auto& Pair : AssetLinks)
	{
		TextWithoutCode = TextWithoutCode.Replace(*Pair.Key, *Pair.Value);
	}
	for (const auto& Pair : CodeBlockRanges)
	{
		TextWithoutCode = TextWithoutCode.Replace(*Pair.Key, *Pair.Value);
	}
	return TextWithoutCode;
}
const int32 _obf_A = 8763829;
FString GenerateHtmlForMarkdown(const FString& InMarkdown)
{
	static FString MarkedJs;
	static FString GraphreJs;
	static FString NomnomlJs;
	if (MarkedJs.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BpGeneratorUltimate"));
		if (Plugin.IsValid())
		{
			FString MarkedJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("marked.min.js"));
			FFileHelper::LoadFileToString(MarkedJs, *MarkedJsPath);
			FString GraphreJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("graphre.min.js"));
			FFileHelper::LoadFileToString(GraphreJs, *GraphreJsPath);
			FString NomnomlJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("nomnoml.min.js"));
			FFileHelper::LoadFileToString(NomnomlJs, *NomnomlJsPath);
		}
	}
	const FString Css = TEXT(
		"body { font: 14px 'Segoe UI', Arial, sans-serif; margin: 16px; line-height: 1.6; color: #d4d4d4; background: #1e1e1e; }"
		"h1, h2, h3, h4, h5, h6 { margin-top: 24px; margin-bottom: 16px; font-weight: 600; line-height: 1.25; color: #e0e0e0; border-bottom: 1px solid #404040; padding-bottom: 0.3em; }"
		"code { background: #2d2d30; color: #ce9178; padding: 0.2em 0.4em; border-radius: 3px; font-family: 'Consolas', 'Courier New', monospace; font-size: 85%; }"
		"pre { background: #2d2d30; padding: 16px; border-radius: 6px; overflow: auto; }"
		"pre code { background: none; padding: 0; color: #d4d4d4; }"
		"blockquote { padding: 0 1em; color: #858585; border-left: 0.25em solid #404040; margin: 0; }"
		"a { color: #4fc3f7; text-decoration: none; }"
		"strong, b { font-weight: bold; }"
		"li { margin-bottom: 0.5em; }"
		"table { border-collapse: collapse; margin: 16px 0; width: 100%; }"
		"th, td { border: 1px solid #404040; padding: 8px 12px; text-align: left; }"
		"th { background: #2d2d30; color: #e0e0e0; font-weight: 600; }"
		"tr:nth-child(even) { background: #252526; }"
		"tr:hover { background: #363636; }"
	);
	FString EscapedMarkdown = InMarkdown;
	EscapedMarkdown.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	EscapedMarkdown.ReplaceInline(TEXT("`"), TEXT("\\`"));
	EscapedMarkdown.ReplaceInline(TEXT("$"), TEXT("\\$"));
	const FString FullHtml = FString::Printf(TEXT(
		"<!DOCTYPE html><html><head><meta charset='utf-8'><style>%s</style>"
		"<script>%s</script><script>%s</script><script>%s</script>"
		"<script>"
		"function createPieChart(dataStr){"
		"var colors=['#4fc3f7','#81c784','#ffb74d','#f06292','#ba68c8','#4db6ac','#ffd54f','#90a4ae','#e57373','#64b5f6'];"
		"var parts=dataStr.split(',').map(function(p){var s=p.trim().split(/\\s+/);return{label:s[0],value:parseInt(s[1])||0};});"
		"var total=parts.reduce(function(s,p){return s+p.value;},0);"
		"if(total===0)return'<p>No data</p>';"
		"var cx=100,cy=100,r=80;"
		"var svg='<svg width=\"300\" height=\"250\" style=\"margin:16px auto;display:block;\">';"
		"var angle=-Math.PI/2;"
		"parts.forEach(function(p,i){"
		"var pct=p.value/total;"
		"var sweep=pct*2*Math.PI;"
		"var x1=cx+r*Math.cos(angle),y1=cy+r*Math.sin(angle);"
		"angle+=sweep;"
		"var x2=cx+r*Math.cos(angle),y2=cy+r*Math.sin(angle);"
		"var large=sweep>Math.PI?1:0;"
		"svg+='<path d=\"M'+cx+','+cy+' L'+x1+','+y1+' A'+r+','+r+' 0 '+large+',1 '+x2+','+y2+' Z\" fill=\"'+colors[i%%colors.length]+'\"/>';"
		"});"
		"svg+='<circle cx=\"'+cx+'\" cy=\"'+cy+'\" r=\"35\" fill=\"#1e1e1e\"/>';"
		"svg+='<text x=\"'+cx+'\" y=\"'+(cy+5)+'\" text-anchor=\"middle\" fill=\"#d4d4d4\" font-size=\"12\">'+total+' total</text></svg>';"
		"svg+='<div style=\"display:flex;flex-wrap:wrap;gap:8px;justify-content:center;margin:8px 0;\">';"
		"parts.forEach(function(p,i){"
		"var pct=Math.round(p.value/total*100);"
		"svg+='<span style=\"display:flex;align-items:center;gap:4px;font-size:12px;\"><span style=\"width:12px;height:12px;background:'+colors[i%%colors.length]+';border-radius:2px;\"></span>'+p.label+' ('+pct+'%%)</span>';"
		"});"
		"return svg+'</div>';"
		"}"
		"function parseMarkdown(str){"
		"var html=marked.parse(str);"
		"html=html.replace(/<pre><code class=\"language-chart\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){return createPieChart(c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&'));});"
		"html=html.replace(/<pre><code class=\"language-flow\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){var d=c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&').trim();d=d.replace(/\\(([^)]*)\\)/g,'-$1-');var styled='#stroke: #b0b0b0\\n#lineWidth: 2\\n#fill: #2d2d30\\n#fontSize: 14\\n'+d;try{if(typeof nomnoml==='undefined')return'<pre style=\"color:#f44\">Flow error: nomnoml not loaded</pre>';return nomnoml.renderSvg(styled);}catch(e){return'<details><summary style=\"color:#f44;cursor:pointer\">Flow error: '+e.message+'</summary><pre style=\"background:#2d2d30;padding:8px\">'+d.substring(0,200)+'</pre></details>';}});"
		"html=html.replace(/\\[([^\\]]+)\\]\\(ue:\\/\\/asset\\?name=([^)]+)\\)/g,'<a href=\"ue://asset?name=$2\" class=\"asset-link\">$1</a>');"
		"return html;"
		"}"
		"</script></head>"
		"<body><div id='content'></div><script>document.getElementById('content').innerHTML = parseMarkdown(`%s`);</script></body></html>"
	), *Css, *MarkedJs, *GraphreJs, *NomnomlJs, *EscapedMarkdown);
	return FullHtml;
}
const TCHAR* const _obf_D = TEXT("Editor.Stats");
const TArray<uint8> EngineStart = { 24, 120, 35, 59, 10, 77, 115, 13, 61, 125, 1, 122, 3, 15, 18, 106, 23, 75, 32, 19, 30, 25, 105, 17, 59, 87, 90, 112, 87, 1, 51, 96, 16, 102, 12, 35, 40, 99, 90 };
void SBpGeneratorUltimateWidget::Construct(const FArguments& InArgs)
{
	FUIConfigManager::Get().RefreshConfig();
	ProviderOptions.Add(MakeShared<FString>(TEXT("Google Gemini")));
	ProviderOptions.Add(MakeShared<FString>(TEXT("OpenAI")));
	ProviderOptions.Add(MakeShared<FString>(TEXT("Anthropic Claude")));
	ProviderOptions.Add(MakeShared<FString>(TEXT("DeepSeek")));
	ProviderOptions.Add(MakeShared<FString>(TEXT("Custom API Provider")));
	SlotProviderOptions.Add(MakeShared<FString>(TEXT("Gemini")));
	SlotProviderOptions.Add(MakeShared<FString>(TEXT("OpenAI")));
	SlotProviderOptions.Add(MakeShared<FString>(TEXT("Claude")));
	SlotProviderOptions.Add(MakeShared<FString>(TEXT("DeepSeek")));
	SlotProviderOptions.Add(MakeShared<FString>(TEXT("Custom")));
	GeminiModelOptions.Add(MakeShared<FString>(TEXT("gemini-2.5-flash")));
	GeminiModelOptions.Add(MakeShared<FString>(TEXT("gemini-2.5-pro")));
	GeminiModelOptions.Add(MakeShared<FString>(TEXT("gemini-3-flash-preview")));
	GeminiModelOptions.Add(MakeShared<FString>(TEXT("gemini-3-pro-preview")));
	GeminiModelOptions.Add(MakeShared<FString>(TEXT("gemini-3.1-pro-preview")));
	OpenAIModelOptions.Add(MakeShared<FString>(TEXT("gpt-5.2-2025-12-11")));
	OpenAIModelOptions.Add(MakeShared<FString>(TEXT("gpt-5-2025-08-07")));
	OpenAIModelOptions.Add(MakeShared<FString>(TEXT("gpt-5-mini-2025-08-07")));
	OpenAIModelOptions.Add(MakeShared<FString>(TEXT("gpt-5-nano-2025-08-07")));
	OpenAIModelOptions.Add(MakeShared<FString>(TEXT("gpt-4.1-mini-2025-04-14")));
	ClaudeModelOptions.Add(MakeShared<FString>(TEXT("claude-opus-4-6")));
	ClaudeModelOptions.Add(MakeShared<FString>(TEXT("claude-sonnet-4-6")));
	ClaudeModelOptions.Add(MakeShared<FString>(TEXT("claude-haiku-4-5-20251001")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("1:1")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("16:9")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("9:16")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("4:3")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("3:4")));
	AspectRatioOptions.Add(MakeShared<FString>(TEXT("21:9")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("1")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("3")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("5")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("7")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("10")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("15")));
	MemoryThresholdOptions.Add(MakeShared<FString>(TEXT("20")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Project Info")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Recent Work")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Preferences")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Patterns")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Asset Relations")));
	MemoryCategoryOptions.Add(MakeShared<FString>(TEXT("Decisions")));
	CurrentlyEditingSlotIndex = FApiKeyManager::Get().GetActiveSlotIndex();
	LoadSettings();
	LoadThemeSettings();
	if (GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), MeshyApiKey, GEditorPerProjectIni) && MeshyApiKey.IsEmpty())
	{
		MeshyApiKey = TEXT("");
	}
	ChildSlot
		[
			SAssignNew(MainSwitcher, SWidgetSwitcher).WidgetIndex(0)
				+ SWidgetSwitcher::Slot()
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(5)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2)
								[
									CreateBurgerMenuButton()
								]
								+ SHorizontalBox::Slot().FillWidth(1.0).Padding(2)
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(1.0).Padding(1)
										[
											SNew(SBorder)
												.BorderImage_Lambda([this]() -> const FSlateBrush* {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 0
														? FCoreStyle::Get().GetBrush("WhiteBrush") : nullptr;
												})
												.BorderBackgroundColor_Lambda([this]() -> FSlateColor {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 0
														? FSlateColor(FLinearColor(1, 1, 1, 0.8f)) : FSlateColor(FLinearColor::Transparent);
												})
												.Padding(2)
												[
													SAssignNew(ArchitectTabButton, SButton)
														.Text(UIConfig::GetTextAttr(TEXT("architect_label"), TEXT("Blueprint Architect")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnArchitectViewSelected)
														.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor {
															return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 0
																? CurrentTheme.ArchitectColor : CurrentTheme.ArchitectColor.CopyWithNewOpacity(0.2f);
														})
														.ForegroundColor(FSlateColor(FLinearColor::White))
												]
										]
										+ SHorizontalBox::Slot().FillWidth(1.0).Padding(1)
										[
											SNew(SBorder)
												.Visibility(EVisibility::Collapsed)
												.BorderImage_Lambda([this]() -> const FSlateBrush* {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 1
														? FCoreStyle::Get().GetBrush("WhiteBrush") : nullptr;
												})
												.BorderBackgroundColor_Lambda([this]() -> FSlateColor {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 1
														? FSlateColor(FLinearColor(1, 1, 1, 0.8f)) : FSlateColor(FLinearColor::Transparent);
												})
												.Padding(2)
												[
													SAssignNew(AnalystTabButton, SButton)
														.Text(UIConfig::GetTextAttr(TEXT("analyst_label"), TEXT("Asset Analyst")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnAnalystViewSelected)
														.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor {
															return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 1
																? CurrentTheme.AnalystColor : CurrentTheme.AnalystColor.CopyWithNewOpacity(0.2f);
														})
														.ForegroundColor(FSlateColor(FLinearColor::White))
												]
										]
										+ SHorizontalBox::Slot().FillWidth(1.0).Padding(1)
										[
											SNew(SBorder)
												.BorderImage_Lambda([this]() -> const FSlateBrush* {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 2
														? FCoreStyle::Get().GetBrush("WhiteBrush") : nullptr;
												})
												.BorderBackgroundColor_Lambda([this]() -> FSlateColor {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 2
														? FSlateColor(FLinearColor(1, 1, 1, 0.8f)) : FSlateColor(FLinearColor::Transparent);
												})
												.Padding(2)
												[
													SAssignNew(ScannerTabButton, SButton)
														.Text(UIConfig::GetTextAttr(TEXT("scanner_label"), TEXT("Project Scanner")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnProjectViewSelected)
														.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor {
															return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 2
																? CurrentTheme.ScannerColor : CurrentTheme.ScannerColor.CopyWithNewOpacity(0.2f);
														})
														.ForegroundColor(FSlateColor(FLinearColor::White))
												]
										]
										+ SHorizontalBox::Slot().FillWidth(1.0).Padding(1)
										[
											SNew(SBorder)
												.BorderImage_Lambda([this]() -> const FSlateBrush* {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 3
														? FCoreStyle::Get().GetBrush("WhiteBrush") : nullptr;
												})
												.BorderBackgroundColor_Lambda([this]() -> FSlateColor {
													return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 3
														? FSlateColor(FLinearColor(1, 1, 1, 0.8f)) : FSlateColor(FLinearColor::Transparent);
												})
												.Padding(2)
												[
													SAssignNew(ModelGenTabButton, SButton)
														.Text(UIConfig::GetTextAttr(TEXT("modelgen_label"), TEXT("3D Model Gen")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnModelGenViewSelected)
														.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor {
															return MainViewSwitcher.IsValid() && MainViewSwitcher->GetActiveWidgetIndex() == 3
																? CurrentTheme.TertiaryColor : CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.2f);
														})
														.ForegroundColor(FSlateColor(FLinearColor::White))
												]
										]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(8, 2, 2, 2)
								[
									SNew(SBox)
										.Visibility_Lambda([this]() -> EVisibility { return GetLoadingIndicatorVisibility(); })
										[
											CreateLoadingIndicator()
										]
								]
						]
					+ SVerticalBox::Slot().AutoHeight().Padding(10.f, 5.f, 10.f, 5.f)
						[
							SNew(SBorder)
								.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
								.Padding(10.f)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("full_feature_warning"), TEXT("NOTICE: Click on 'How To Use' to see every tab's purpose. The Blueprint Arhitect tab is the one that's able to Generate Blueprints, Create Actors and so on. I recommend checking the documentation.")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor))
										.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
										.Justification(ETextJustify::Center)
										.AutoWrapText(true)
								]
						]
					+ SVerticalBox::Slot().FillHeight(1.0f)
						[
							SNew(SOverlay)
								+ SOverlay::Slot()
								[
									SAssignNew(MainViewSwitcher, SWidgetSwitcher).WidgetIndex(0)
										+ SWidgetSwitcher::Slot()
										[
											SAssignNew(ArchitectSplitter, SSplitter).Orientation(EOrientation::Orient_Horizontal)
												+ SSplitter::Slot().Value(0.25f).MinSize(0.0f)
												[
													SNew(SVerticalBox)
														.Visibility_Lambda([this]() { return bSidebarVisible ? EVisibility::Visible : EVisibility::Collapsed; })
														+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SButton)
																.HAlign(HAlign_Center)
																.Text(UIConfig::GetTextAttr(TEXT("new_architect_chat"), TEXT("New Architect Chat")))
																.OnClicked(this, &SBpGeneratorUltimateWidget::OnNewArchitectChatClicked)
																.ButtonColorAndOpacity(FSlateColor(CurrentTheme.ArchitectColor.CopyWithNewOpacity(0.5f)))
																.ForegroundColor(FSlateColor(FLinearColor::White))
														]
														+ SVerticalBox::Slot().FillHeight(1.0f)
														[
															SNew(SBorder)
																.Padding(3)
																.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
																.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
																[
																	SAssignNew(ArchitectChatListView, SListView<TSharedPtr<FConversationInfo>>)
																		.ListItemsSource(&ArchitectConversationList)
																		.OnGenerateRow(this, &SBpGeneratorUltimateWidget::OnGenerateRowForArchitectChatList)
																		.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnArchitectChatSelectionChanged)
																		.SelectionMode(ESelectionMode::Single)
																]
														]
												]
											+ SSplitter::Slot().Value(0.75f)
												[
													SNew(SOverlay)
														+SOverlay::Slot()
														[
															SNew(SVerticalBox)
																+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
																[
																	SAssignNew(ArchitectChatBrowser, SWebBrowser)
																		.SupportsTransparency(false)
																		.ShowControls(false)
																		.ShowAddressBar(false)
																		.OnLoadUrl(this, &SBpGeneratorUltimateWidget::OnBrowserLoadUrl)
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 10, 0))
																		[
																			SNew(STextBlock)
																				.Text(UIConfig::GetTextAttr(TEXT("mode_label"), TEXT("Mode:")))
																				.TextStyle(FAppStyle::Get(), "NormalText")
																				.ColorAndOpacity(FLinearColor::Gray)
																		]
																		+ SHorizontalBox::Slot().AutoWidth()
																		[
																			SNew(SCheckBox)
																				.IsChecked_Lambda([this]() { return ArchitectInteractionMode == EAIInteractionMode::JustChat ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
																				.OnCheckStateChanged(FOnCheckStateChanged::CreateSP(this, &SBpGeneratorUltimateWidget::OnJustChatModeClicked))
																				.Style(FAppStyle::Get(), "RadioButton")
																			[
																				SNew(STextBlock)
																					.Text(UIConfig::GetTextAttr(TEXT("mode_just_chat"), TEXT("Just Chat")))
																					.TextStyle(FAppStyle::Get(), "NormalText")
																					.ColorAndOpacity(FLinearColor::White)
																			]
																		]
																		+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
																		[
																			SNew(SCheckBox)
																				.IsChecked_Lambda([this]() { return ArchitectInteractionMode == EAIInteractionMode::AskBeforeEdit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
																				.OnCheckStateChanged(FOnCheckStateChanged::CreateSP(this, &SBpGeneratorUltimateWidget::OnAskBeforeEditModeClicked))
																				.Style(FAppStyle::Get(), "RadioButton")
																			[
																				SNew(STextBlock)
																					.Text(UIConfig::GetTextAttr(TEXT("mode_ask_before_edit"), TEXT("Ask Before Edit")))
																					.TextStyle(FAppStyle::Get(), "NormalText")
																					.ColorAndOpacity(FLinearColor::White)
																			]
																		]
																		+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
																		[
																			SNew(SCheckBox)
																				.IsChecked_Lambda([this]() { return ArchitectInteractionMode == EAIInteractionMode::AutoEdit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
																				.OnCheckStateChanged(FOnCheckStateChanged::CreateSP(this, &SBpGeneratorUltimateWidget::OnAutoEditModeClicked))
																				.Style(FAppStyle::Get(), "RadioButton")
																			[
																				SNew(STextBlock)
																					.Text(UIConfig::GetTextAttr(TEXT("mode_auto_edit"), TEXT("Auto Edit")))
																					.TextStyle(FAppStyle::Get(), "NormalText")
																					.ColorAndOpacity(FLinearColor::White)
																			]
																		]
																]
																+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0, 0, 10, 0))
																[
																	SAssignNew(ArchitectTokenCounterText, STextBlock)
																		.Text(LOCTEXT("TokenCounter", "Tokens: 0"))
																		.TextStyle(FAppStyle::Get(), "NormalText")
																		.ColorAndOpacity(FLinearColor::Gray)
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().AutoWidth()
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("import_file_button"), TEXT("Import File")))
																				.ToolTipText(UIConfig::GetText(TEXT("import_file_tooltip"), TEXT("Import text/markdown files as context")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnImportArchitectContextClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("analyze_plugin_button"), TEXT("Analyze Plugin")))
																				.ToolTipText(UIConfig::GetText(TEXT("analyze_plugin_tooltip"), TEXT("Select and analyze a marketplace plugin's C++ source code")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAnalyzePluginClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.6f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SScrollBox)
																	+ SScrollBox::Slot() [
																		SNew(SBox).MaxDesiredHeight(200.0f) [
																			SAssignNew(ArchitectInputTextBox, SMultiLineEditableTextBox)
																				.HintText(UIConfig::GetText(TEXT("architect_input_hint"), TEXT("e.g., 'Create a function that moves the actor forward...' or type @ to reference an asset")))
																				.AutoWrapText(true)
																				.OnKeyDownHandler(this, &SBpGeneratorUltimateWidget::OnArchitectInputKeyDown)
																				.OnTextChanged(this, &SBpGeneratorUltimateWidget::OnArchitectInputTextChanged)
																		]
																	]
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SBox)
																		.HeightOverride(32)
																		.Visibility_Lambda([this]() {
																			return ArchitectAttachedImages.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
																		})
																		[
																			SNew(SScrollBox)
																				.Orientation(Orient_Horizontal)
																				+ SScrollBox::Slot()
																				[
																					SAssignNew(ArchitectImagePreviewBox, SHorizontalBox)
																				]
																		]
																]
																+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(5)
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("attach_image_button"), TEXT("📎 Image")))
																				.ToolTipText(UIConfig::GetText(TEXT("attach_image_tooltip"), TEXT("Attach an image or screenshot")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAttachArchitectImageClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.5f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().AutoWidth()
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("send_button"), TEXT("Send")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnSendArchitectMessageClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.ArchitectColor.CopyWithNewOpacity(0.7f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																				.Visibility_Lambda([this]() { return bIsArchitectThinking ? EVisibility::Collapsed : EVisibility::Visible; })
																		]
																		+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("stop_button"), TEXT("Stop")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnStopClicked)
																				.ButtonColorAndOpacity(FSlateColor(FLinearColor::Red))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																				.Visibility_Lambda([this]() { return bIsArchitectThinking ? EVisibility::Visible : EVisibility::Collapsed; })
																		]
																]
														]
														+ SOverlay::Slot()
															.HAlign(HAlign_Left)
															.VAlign(VAlign_Bottom)
															.Padding(FMargin(0, 0, 0, 80))
														[
															SNew(SBox)
																.Visibility_Lambda([this]() { return bAssetPickerOpen && AssetPickerSourceView == 0 ? EVisibility::Visible : EVisibility::Collapsed; })
															[
																SAssignNew(ArchitectAssetPickerPopup, SAssetMentionPopup)
																	.OnAssetSelected(FOnAssetRefSelected::CreateSP(this, &SBpGeneratorUltimateWidget::OnAssetRefSelected))
																	.OnPickerDismissed(FSimpleDelegate::CreateSP(this, &SBpGeneratorUltimateWidget::OnAssetPickerDismissed))
															]
														]
													]
										]
									+ SWidgetSwitcher::Slot()
										[
											SAssignNew(AnalystSplitter, SSplitter).Orientation(EOrientation::Orient_Horizontal)
												+ SSplitter::Slot().Value(0.25f).MinSize(0.0f)
												[
													SNew(SVerticalBox)
														.Visibility_Lambda([this]() { return bSidebarVisible ? EVisibility::Visible : EVisibility::Collapsed; })
														+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SButton)
																.HAlign(HAlign_Center)
																.Text(UIConfig::GetTextAttr(TEXT("new_chat_button"), TEXT("New Chat")))
																.OnClicked(this, &SBpGeneratorUltimateWidget::OnNewChatClicked)
																.ButtonColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.5f)))
																.ForegroundColor(FSlateColor(FLinearColor::White))
														]
														+ SVerticalBox::Slot().FillHeight(1.0f)
														[
															SNew(SBorder)
																.Padding(3)
																.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
																.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
																[
																	SAssignNew(ChatListView, SListView<TSharedPtr<FConversationInfo>>)
																		.ListItemsSource(&ConversationList)
																		.OnGenerateRow(this, &SBpGeneratorUltimateWidget::OnGenerateRowForChatList)
																		.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnChatSelectionChanged)
																		.SelectionMode(ESelectionMode::Single)
																]
														]
												]
											+ SSplitter::Slot().Value(0.75f)
												[
													SNew(SVerticalBox)
														+ SVerticalBox::Slot().FillHeight(1.0f).Padding(5)
														[
															SAssignNew(ChatHistoryBrowser, SWebBrowser)
																.SupportsTransparency(false)
																.ShowControls(false)
																.ShowAddressBar(false)
																.OnLoadError(this, &SBpGeneratorUltimateWidget::OnBrowserLoadError)
																.OnLoadUrl(this, &SBpGeneratorUltimateWidget::OnBrowserLoadUrl)
														]
														+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(FMargin(0, 0, 10, 0))
														[
															SAssignNew(AnalystTokenCounterText, STextBlock)
																.Text(LOCTEXT("AnalystTokenCounter", "Tokens: 0"))
																.TextStyle(FAppStyle::Get(), "NormalText")
																.ColorAndOpacity(FLinearColor::Gray)
														]
													+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SScrollBox) + SScrollBox::Slot()
																[
																	SNew(SBox).MaxDesiredHeight(200.0f)
																		[
																			SAssignNew(InputTextBox, SMultiLineEditableTextBox)
																				.HintText(UIConfig::GetText(TEXT("chat_input_hint"), TEXT("Ask a question about the asset...")))
																				.AutoWrapText(true)
																				.OnKeyDownHandler(this, &SBpGeneratorUltimateWidget::OnInputKeyDown)
																		]
																]
														]
														+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SBox)
																.HeightOverride(32)
																.Visibility_Lambda([this]() {
																	return AnalystAttachedImages.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
																})
																[
																	SNew(SScrollBox)
																		.Orientation(Orient_Horizontal)
																		+ SScrollBox::Slot()
																		[
																			SAssignNew(AnalystImagePreviewBox, SHorizontalBox)
																		]
																]
														]
													+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SVerticalBox)
																+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 2, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("add_asset_context_button"), TEXT("Add Full Asset Context")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAddAssetContextClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.4f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0, 0, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("add_node_context_button"), TEXT("Add Selected Nodes Context")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAddNodeContextClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.4f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																]
															+ SVerticalBox::Slot().AutoHeight()
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 2, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("attach_image_button"), TEXT("📎 Image")))
																				.ToolTipText(UIConfig::GetText(TEXT("attach_image_tooltip"), TEXT("Attach an image or screenshot")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAttachAnalystImageClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.5f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 2, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("import_file_button"), TEXT("Import File...")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnImportAnalystContextClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.5f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0, 0, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("send_button"), TEXT("Send")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnSendClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.7f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																]
														]
												]
										]
									+ SWidgetSwitcher::Slot()
										[
											SAssignNew(ProjectSplitter, SSplitter).Orientation(EOrientation::Orient_Horizontal)
												+ SSplitter::Slot().Value(0.25f).MinSize(0.0f)
												[
													SNew(SVerticalBox)
														.Visibility_Lambda([this]() { return bSidebarVisible ? EVisibility::Visible : EVisibility::Collapsed; })
														+ SVerticalBox::Slot().AutoHeight().Padding(5)
														[
															SNew(SButton)
																.HAlign(HAlign_Center)
																.Text(UIConfig::GetTextAttr(TEXT("new_project_chat"), TEXT("New Project Chat")))
																.OnClicked(this, &SBpGeneratorUltimateWidget::OnNewProjectChatClicked)
																.ButtonColorAndOpacity(FSlateColor(CurrentTheme.ScannerColor.CopyWithNewOpacity(0.5f)))
																.ForegroundColor(FSlateColor(FLinearColor::White))
														]
														+ SVerticalBox::Slot().FillHeight(1.0f)
														[
															SNew(SBorder)
																.Padding(3)
																.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
																.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
																[
																	SAssignNew(ProjectChatListView, SListView<TSharedPtr<FConversationInfo>>)
																		.ListItemsSource(&ProjectConversationList)
																		.OnGenerateRow(this, &SBpGeneratorUltimateWidget::OnGenerateRowForProjectChatList)
																		.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnProjectChatSelectionChanged)
																		.SelectionMode(ESelectionMode::Single)
																]
														]
												]
											+ SSplitter::Slot().Value(0.75f)
												[
													SNew(SOverlay)
														+SOverlay::Slot()
														[
															SNew(SVerticalBox)
																+ SVerticalBox::Slot().AutoHeight().Padding(10)
																[
																	SNew(SBorder)
																		.Padding(8)
																		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
																		.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
																		[
																			SNew(SHorizontalBox)
																				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
																				[
																					SAssignNew(ScanStatusTextBlock, STextBlock)
																						.Text(UIConfig::GetTextAttr(TEXT("scan_status_idle"), TEXT("Project index is not loaded. Scan the project to begin.")))
																						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
																				]
																				+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
																				[
																					SAssignNew(ScanProjectButton, SButton)
																						.Text(UIConfig::GetTextAttr(TEXT("scan_project_button"), TEXT("Scan and Index Project")))
																						.OnClicked(this, &SBpGeneratorUltimateWidget::OnScanProjectClicked)
																						.ButtonColorAndOpacity(FSlateColor(CurrentTheme.ScannerColor.CopyWithNewOpacity(0.6f)))
																						.ForegroundColor(FSlateColor(FLinearColor::White))
																				]
																				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
																				[
																					SNew(SButton)
																						.Text(UIConfig::GetTextAttr(TEXT("performance_report_button"), TEXT("Get Performance Report")))
																						.OnClicked(this, &SBpGeneratorUltimateWidget::OnGetPerformanceReportClicked)
																						.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.5f)))
																						.ForegroundColor(FSlateColor(FLinearColor::White))
																				]
																				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
																				[
																					SNew(SButton)
																						.Text(UIConfig::GetTextAttr(TEXT("project_overview_button"), TEXT("Get Project Overview")))
																						.OnClicked(this, &SBpGeneratorUltimateWidget::OnGetProjectOverviewClicked)
																						.ButtonColorAndOpacity(FSlateColor(CurrentTheme.ScannerColor.CopyWithNewOpacity(0.5f)))
																						.ForegroundColor(FSlateColor(FLinearColor::White))
																				]
																		]
																]
															+ SVerticalBox::Slot().FillHeight(1.0f)
																[
																	SAssignNew(ProjectChatBrowser, SWebBrowser)
																		.SupportsTransparency(false)
																		.ShowControls(false)
																		.ShowAddressBar(false)
																		.OnLoadUrl(this, &SBpGeneratorUltimateWidget::OnBrowserLoadUrl)
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SScrollBox) + SScrollBox::Slot()
																		[
																			SNew(SBox).MaxDesiredHeight(200.0f)
																				[
																					SAssignNew(ProjectInputTextBox, SMultiLineEditableTextBox)
																						.HintText(UIConfig::GetText(TEXT("project_input_hint"), TEXT("Ask a question about the project (e.g., 'where is player health handled?') or type @ to reference an asset...")))
																						.AutoWrapText(true)
																						.OnKeyDownHandler(this, &SBpGeneratorUltimateWidget::OnProjectInputKeyDown)
																						.OnTextChanged(this, &SBpGeneratorUltimateWidget::OnProjectInputTextChanged)
																				]
																		]
																]
																+ SVerticalBox::Slot().AutoHeight().Padding(5)
																[
																	SNew(SBox)
																		.HeightOverride(32)
																		.Visibility_Lambda([this]() {
																			return ProjectAttachedImages.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
																		})
																		[
																			SNew(SScrollBox)
																				.Orientation(Orient_Horizontal)
																				+ SScrollBox::Slot()
																				[
																					SAssignNew(ProjectImagePreviewBox, SHorizontalBox)
																				]
																		]
																]
															+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(5)
																[
																	SNew(SHorizontalBox)
																		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("attach_image_button"), TEXT("📎 Image")))
																				.ToolTipText(UIConfig::GetText(TEXT("attach_image_tooltip"), TEXT("Attach an image or screenshot")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnAttachProjectImageClicked)
																				.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.5f)))
																				.ForegroundColor(FSlateColor(FLinearColor::White))
																		]
																		+ SHorizontalBox::Slot().AutoWidth()
																		[
																			SNew(SButton)
																				.Text(UIConfig::GetTextAttr(TEXT("send_button"), TEXT("Send")))
																				.OnClicked(this, &SBpGeneratorUltimateWidget::OnSendProjectQuestionClicked)
																		]
																]
														]
														+ SOverlay::Slot()
															.HAlign(HAlign_Left)
															.VAlign(VAlign_Bottom)
															.Padding(FMargin(0, 0, 0, 80))
														[
															SNew(SBox)
																.Visibility_Lambda([this]() { return bAssetPickerOpen && AssetPickerSourceView == 1 ? EVisibility::Visible : EVisibility::Collapsed; })
															[
																SAssignNew(ProjectAssetPickerPopup, SAssetMentionPopup)
																	.OnAssetSelected(FOnAssetRefSelected::CreateSP(this, &SBpGeneratorUltimateWidget::OnAssetRefSelected))
																	.OnPickerDismissed(FSimpleDelegate::CreateSP(this, &SBpGeneratorUltimateWidget::OnAssetPickerDismissed))
															]
														]
												]
										]
									+ SWidgetSwitcher::Slot()
										[
											Create3DModelGenWidget()
										]
								]
							+ SOverlay::Slot()
								[
									SNew(SBox)
										.Visibility_Lambda([this] { return bIsShowingHowToUse ? EVisibility::Visible : EVisibility::Collapsed; })
										[
											SNew(SOverlay)
												+ SOverlay::Slot()
												[
													SNew(SWidgetSwitcher)
														.WidgetIndex_Lambda([this]() {
															int32 ActiveIndex = MainViewSwitcher->GetActiveWidgetIndex();
															if (ActiveIndex == 0) return 2;
															if (ActiveIndex == 1) return 0;
															if (ActiveIndex == 2) return 1;
															if (ActiveIndex == 3) return 3;
															return 0;
															})
														+ SWidgetSwitcher::Slot()[CreateHowToUseWidget()]
														+ SWidgetSwitcher::Slot()[CreateProjectScannerHowToUseWidget()]
														+ SWidgetSwitcher::Slot()[CreateBlueprintArchitectHowToUseWidget()]
														+ SWidgetSwitcher::Slot()[Create3DModelGenHowToUseWidget()]
												]
											+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(5)
												[
													SNew(SHorizontalBox)
														+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
														[
															SNew(SButton)
																.Text(UIConfig::GetTextAttr(TEXT("translate_button"), TEXT("Translate")))
																.OnClicked(this, &SBpGeneratorUltimateWidget::OnTranslateHowToClicked)
																.IsEnabled_Lambda([this]() { return !bIsTranslatingHowTo; })
														]
														+ SHorizontalBox::Slot().AutoWidth()
														[
															SNew(SButton)
																.Text(FText::FromString(TEXT("X")))
																.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseHowToUseClicked)
														]
												]
										]
								]
							+ SOverlay::Slot()
								[
									SNew(SBox)
										.Visibility_Lambda([this] { return bIsShowingCustomInstructions ? EVisibility::Visible : EVisibility::Collapsed; })
										[
											SNew(SOverlay)
												+ SOverlay::Slot()
												[
													SNew(SBorder)
														.Padding(FMargin(0, 50, 0, 0))
														.VAlign(VAlign_Center)
														.HAlign(HAlign_Center)
														[
															SNew(SBox)
																.WidthOverride(600)
																.HeightOverride(500)
																[
																	CreateCustomInstructionsWidget()
																]
														]
												]
											+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(5)
												[
													SNew(SButton)
														.Text(FText::FromString(TEXT("X")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseCustomInstructionsClicked)
												]
										]
								]
							+ SOverlay::Slot()
								[
									SAssignNew(GddOverlay, SBox)
										.Visibility_Lambda([this] { return bIsShowingGddOverlay ? EVisibility::Visible : EVisibility::Collapsed; })
										[
											SNew(SOverlay)
												+ SOverlay::Slot()
												[
													SNew(SBorder)
														.Padding(FMargin(0, 50, 0, 0))
														.VAlign(VAlign_Center)
														.HAlign(HAlign_Center)
														[
															SNew(SBox)
																.WidthOverride(700)
																.HeightOverride(600)
														]
												]
											+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(5)
												[
													SNew(SButton)
														.Text(FText::FromString(TEXT("X")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseGddClicked)
												]
										]
								]
							+ SOverlay::Slot()
								[
									SAssignNew(AiMemoryOverlay, SBox)
										.Visibility_Lambda([this] { return bIsShowingAiMemoryOverlay ? EVisibility::Visible : EVisibility::Collapsed; })
										[
											SNew(SOverlay)
												+ SOverlay::Slot()
												[
													SNew(SBorder)
														.Padding(FMargin(0, 50, 0, 0))
														.VAlign(VAlign_Center)
														.HAlign(HAlign_Center)
														[
															SNew(SBox)
																.WidthOverride(700)
																.HeightOverride(600)
														]
												]
											+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(5)
												[
													SNew(SButton)
														.Text(FText::FromString(TEXT("X")))
														.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseAiMemoryClicked)
												]
										]
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(5, 2, 5, 5)
						[
							SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								.Padding(5)
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(SButton).Text(UIConfig::GetTextAttr(TEXT("settings_button"), TEXT("API Settings"))).OnClicked(this, &SBpGeneratorUltimateWidget::OnShowSettingsClicked)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton).Text(UIConfig::GetTextAttr(TEXT("how_to_use_button"), TEXT("How to Use"))).OnClicked(this, &SBpGeneratorUltimateWidget::OnHowToUseClicked)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton)
											.Text(UIConfig::GetTextAttr(TEXT("instructions_button"), TEXT("Instructions")))
											.ToolTipText(UIConfig::GetText(TEXT("instructions_tooltip"), TEXT("Set custom instructions for the AI")))
											.OnClicked(this, &SBpGeneratorUltimateWidget::OnCustomInstructionsClicked)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton)
											.Text(UIConfig::GetTextAttr(TEXT("gdd_button"), TEXT("Game Design Docs")))
											.ToolTipText(UIConfig::GetText(TEXT("gdd_tooltip"), TEXT("Manage Game Design Documents")))
											.OnClicked(this, &SBpGeneratorUltimateWidget::OnGddClicked)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton)
											.Text(UIConfig::GetTextAttr(TEXT("ai_memory_button"), TEXT("AI Memory")))
											.ToolTipText(UIConfig::GetText(TEXT("ai_memory_tooltip"), TEXT("Manage AI learned memories")))
											.OnClicked(this, &SBpGeneratorUltimateWidget::OnAiMemoryClicked)
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f)
									[
										SNew(SSpacer)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton).Text(UIConfig::GetTextAttr(TEXT("discord_button"), TEXT("Join Discord")))
											.OnClicked_Lambda([]() { FPlatformProcess::LaunchURL(*UIConfig::GetURL(TEXT("discord_url"), TEXT("https://discord.gg/w65xMuxEhb")), nullptr, nullptr); return FReply::Handled(); })
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
									[
										SNew(SButton).Text(UIConfig::GetTextAttr(TEXT("forum_button"), TEXT("Join Exclusive Forum")))
											.OnClicked_Lambda([]() { FPlatformProcess::LaunchURL(*UIConfig::GetURL(TEXT("forum_url"), TEXT("https://www.gamedevcore.com/community")), nullptr, nullptr); return FReply::Handled(); })
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(SButton).Text(UIConfig::GetTextAttr(TEXT("docs_button"), TEXT("-> DOCUMENTATION <-")))
											.OnClicked_Lambda([]() { FPlatformProcess::LaunchURL(*UIConfig::GetURL(TEXT("docs_url"), TEXT("https://www.gamedevcore.com/docs/bpgenerator-ultimate")), nullptr, nullptr); return FReply::Handled(); })
									]
							]
						]
					]
				+ SWidgetSwitcher::Slot()
					[
						SAssignNew(EntireSettingsBox, SBox)
						[
							CreateSettingsWidget()
						]
					]
			];
	LoadManifest();
	LoadProjectManifest();
	LoadArchitectManifest();
	LoadGddManifest();
	LoadAiMemoryManifest();
	CheckForExistingIndex();
	RefreshProjectChatView();
}
FReply SBpGeneratorUltimateWidget::OnGetPerformanceReportClicked()
{
	const FString IndexFilePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	if (!FPaths::FileExists(IndexFilePath))
	{
		UpdateScanStatus(TEXT("Error: Project Index file not found. Please scan the project first."), true);
		return FReply::Handled();
	}
	FString IndexFileContent;
	if (!FFileHelper::LoadFileToString(IndexFileContent, *IndexFilePath))
	{
		UpdateScanStatus(TEXT("Error: Failed to read the Project Index file."), true);
		return FReply::Handled();
	}
	TSharedPtr<FJsonObject> ProjectIndexObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(IndexFileContent);
	if (!FJsonSerializer::Deserialize(Reader, ProjectIndexObject) || !ProjectIndexObject.IsValid())
	{
		UpdateScanStatus(TEXT("Error: Project Index file is corrupted or not valid JSON."), true);
		return FReply::Handled();
	}
	TStringBuilder<8192> IssuesBuilder;
	const FString PerformanceTag = TEXT("--- POTENTIAL PERFORMANCE ISSUES ---");
	bool bIssuesFound = false;
	for (const auto& Pair : ProjectIndexObject->Values)
	{
		FString AssetPath = Pair.Key;
		FString AssetSummary = Pair.Value->AsString();
		int32 TagIndex = AssetSummary.Find(PerformanceTag, ESearchCase::CaseSensitive);
		if (TagIndex != INDEX_NONE)
		{
			bIssuesFound = true;
			FString IssuesSection = AssetSummary.RightChop(TagIndex + PerformanceTag.Len()).TrimStart();
			IssuesBuilder.Appendf(TEXT("Blueprint: %s\nIssues:\n%s\n\n"), *AssetPath, *IssuesSection);
		}
	}
	FString FinalIssuesString;
	if (!bIssuesFound)
	{
		FinalIssuesString = TEXT("Scan complete. No Blueprints with common performance issues (like Event Tick usage) were found in the index.");
		if (ActiveProjectChatID.IsEmpty())
		{
			OnNewProjectChatClicked();
		}
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), FinalIssuesString);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		SaveProjectChatHistory(ActiveProjectChatID);
		RefreshProjectChatView();
	}
	else
	{
		FinalIssuesString = IssuesBuilder.ToString();
		SendPerformanceDataToAI(FinalIssuesString);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnGetProjectOverviewClicked()
{
	const FString IndexFilePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	if (!FPaths::FileExists(IndexFilePath))
	{
		UpdateScanStatus(TEXT("Error: Project Index file not found. Please scan the project first."), true);
		return FReply::Handled();
	}
	FString IndexFileContent;
	if (!FFileHelper::LoadFileToString(IndexFileContent, *IndexFilePath))
	{
		UpdateScanStatus(TEXT("Error: Failed to read the Project Index file."), true);
		return FReply::Handled();
	}
	TSharedPtr<FJsonObject> ProjectIndexObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(IndexFileContent);
	if (!FJsonSerializer::Deserialize(Reader, ProjectIndexObject) || !ProjectIndexObject.IsValid())
	{
		UpdateScanStatus(TEXT("Error: Project Index file is corrupted or not valid JSON."), true);
		return FReply::Handled();
	}
	TStringBuilder<16384> OverviewBuilder;
	int32 WidgetCount = 0, ActorCount = 0, AnimBPCount = 0, BTCount = 0;
	int32 EnumCount = 0, StructCount = 0, InterfaceCount = 0, DataAssetCount = 0, DataTableCount = 0, MaterialCount = 0;
	double TempValue = 0.0;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_widget_count"), TempValue)) WidgetCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_actor_count"), TempValue)) ActorCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_anim_bp_count"), TempValue)) AnimBPCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_behavior_tree_count"), TempValue)) BTCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_enum_count"), TempValue)) EnumCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_struct_count"), TempValue)) StructCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_interface_count"), TempValue)) InterfaceCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_data_asset_count"), TempValue)) DataAssetCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_data_table_count"), TempValue)) DataTableCount = (int32)TempValue;
	if (ProjectIndexObject->TryGetNumberField(TEXT("_material_count"), TempValue)) MaterialCount = (int32)TempValue;
	OverviewBuilder.Append(TEXT("=== ASSET DISTRIBUTION ===\n"));
	OverviewBuilder.Appendf(TEXT("Widgets: %d\n"), WidgetCount);
	OverviewBuilder.Appendf(TEXT("Actors: %d\n"), ActorCount);
	OverviewBuilder.Appendf(TEXT("Anim BPs: %d\n"), AnimBPCount);
	OverviewBuilder.Appendf(TEXT("Behavior Trees: %d\n"), BTCount);
	OverviewBuilder.Appendf(TEXT("Enums: %d\n"), EnumCount);
	OverviewBuilder.Appendf(TEXT("Structs: %d\n"), StructCount);
	OverviewBuilder.Appendf(TEXT("Interfaces: %d\n"), InterfaceCount);
	OverviewBuilder.Appendf(TEXT("Data Assets: %d\n"), DataAssetCount);
	OverviewBuilder.Appendf(TEXT("Data Tables: %d\n"), DataTableCount);
	OverviewBuilder.Appendf(TEXT("Materials: %d\n"), MaterialCount);
	OverviewBuilder.Append(TEXT("\n"));
	TArray<TPair<FString, int32>> ComplexityScores;
	for (const auto& Pair : ProjectIndexObject->Values)
	{
		if (Pair.Key.StartsWith(TEXT("_"))) continue;
		FString Summary = Pair.Value->AsString();
		FString ComplexityTag = TEXT("Complexity Score: ");
		int32 TagIndex = Summary.Find(ComplexityTag, ESearchCase::CaseSensitive);
		if (TagIndex != INDEX_NONE)
		{
			FString AfterTag = Summary.RightChop(TagIndex + ComplexityTag.Len());
			int32 EndIndex = AfterTag.Find(TEXT("\n"));
			if (EndIndex == INDEX_NONE) EndIndex = AfterTag.Len();
			FString ScoreStr = AfterTag.Left(EndIndex).TrimStartAndEnd();
			int32 Score = FCString::Atoi(*ScoreStr);
			if (Score > 0)
			{
				ComplexityScores.Add(TPair<FString, int32>(Pair.Key, Score));
			}
		}
	}
	ComplexityScores.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
		return A.Value > B.Value;
	});
	OverviewBuilder.Append(TEXT("=== COMPLEXITY LEADERBOARD (Top 10) ===\n"));
	int32 Rank = 1;
	for (const auto& Entry : ComplexityScores)
	{
		if (Rank > 10) break;
		FString AssetName = Entry.Key;
		int32 LastSlash = AssetName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (LastSlash != INDEX_NONE)
		{
			AssetName = AssetName.RightChop(LastSlash + 1);
		}
		int32 DotIndex = AssetName.Find(TEXT("."));
		if (DotIndex != INDEX_NONE)
		{
			AssetName = AssetName.Left(DotIndex);
		}
		OverviewBuilder.Appendf(TEXT("%d. %s (Score: %d)\n"), Rank, *AssetName, Entry.Value);
		Rank++;
	}
	if (ComplexityScores.Num() == 0)
	{
		OverviewBuilder.Append(TEXT("No complexity data found. Re-scan the project to get complexity scores.\n"));
	}
	OverviewBuilder.Append(TEXT("\n"));
	OverviewBuilder.Append(TEXT("=== MATERIAL ANALYSIS ===\n"));
	int32 MaterialsWithIssues = 0;
	for (const auto& Pair : ProjectIndexObject->Values)
	{
		if (Pair.Key.StartsWith(TEXT("_"))) continue;
		FString Summary = Pair.Value->AsString();
		if (Summary.Contains(TEXT("MATERIAL SUMMARY")))
		{
			if (Summary.Contains(TEXT("Issues: ")) && !Summary.Contains(TEXT("Issues: None")))
			{
				MaterialsWithIssues++;
				FString AssetName = Pair.Key;
				int32 LastSlash = AssetName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (LastSlash != INDEX_NONE) AssetName = AssetName.RightChop(LastSlash + 1);
				int32 DotIndex = AssetName.Find(TEXT("."));
				if (DotIndex != INDEX_NONE) AssetName = AssetName.Left(DotIndex);
				OverviewBuilder.Appendf(TEXT("- %s has issues\n"), *AssetName);
			}
		}
	}
	if (MaterialsWithIssues == 0)
	{
		OverviewBuilder.Append(TEXT("No materials with performance issues found.\n"));
	}
	SendProjectOverviewToAI(OverviewBuilder.ToString());
	return FReply::Handled();
}
const int32 _obf_B = 8675309;
void SBpGeneratorUltimateWidget::SendProjectOverviewToAI(const FString& OverviewData)
{
	if (ActiveProjectChatID.IsEmpty())
	{
		OnNewProjectChatClicked();
	}
	FString UserPlaceholder = TEXT("Generating project overview...");
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), UserPlaceholder);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	bIsProjectThinking = true;
	RefreshProjectChatView();
	FString FinalPrompt = FString::Printf(TEXT(
		"You are an expert Unreal Engine project analyst. Analyze the following project statistics and provide a helpful overview.\n\n"
		"=== CHART CAPABILITY ===\n"
		"You CAN generate pie charts that will be automatically rendered as visual SVG graphics.\n"
		"Use this exact format:\n"
		"```chart\n"
		"Category1 count1, Category2 count2, Category3 count3\n"
		"```\n"
		"Example: Widgets 15, Actors 23, Materials 8\n"
		"The chart will render automatically - you just provide the data.\n\n"
		"=== INSTRUCTIONS ===\n"
		"1. YOU MUST start with a pie chart showing asset distribution using the chart format above\n"
		"2. Use markdown tables for the complexity leaderboard\n"
		"3. Identify any patterns or concerns\n"
		"4. Provide actionable recommendations\n"
		"5. Be concise and helpful\n\n"
		"=== PROJECT DATA ===\n%s\n=== END DATA ==="
	), *OverviewData);
	TArray<TSharedPtr<FJsonValue>> RequestPayloadHistory;
	TSharedPtr<FJsonObject> UserTurn = MakeShareable(new FJsonObject);
	UserTurn->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> TurnParts;
	TSharedPtr<FJsonObject> TurnPart = MakeShareable(new FJsonObject);
	TurnPart->SetStringField(TEXT("text"), FinalPrompt);
	TurnParts.Add(MakeShareable(new FJsonValueObject(TurnPart)));
	UserTurn->SetArrayField(TEXT("parts"), TurnParts);
	RequestPayloadHistory.Add(MakeShareable(new FJsonValueObject(UserTurn)));
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	bool bIsLimitedKey = false;
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		bIsLimitedKey = true;
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		int32 DecryptedLimit = _obf_A ^ _obf_B;
		if (CurrentCharacters >= DecryptedLimit)
		{
			FString LimitMessage = TEXT("You have reached the token limit for the built-in API key. Please go to 'API Settings' and enter your own API key to continue using the Project Scanner.");
			TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
			ModelContent->SetStringField(TEXT("role"), TEXT("model"));
			TArray<TSharedPtr<FJsonValue>> ModelParts;
			TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
			ModelPartText->SetStringField(TEXT("text"), LimitMessage);
			ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
			ModelContent->SetArrayField(TEXT("parts"), ModelParts);
			ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
			bIsProjectThinking = false;
			SaveProjectChatHistory(ActiveProjectChatID);
			RefreshProjectChatView();
			return;
		}
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		FString ErrorMessage = FString::Printf(TEXT("No API key configured for %s. Please go to 'API Settings' and enter your API key."), *ProviderStr);
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMessage);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsProjectThinking = false;
		SaveProjectChatHistory(ActiveProjectChatID);
		RefreshProjectChatView();
		return;
	}
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	if (ProviderStr == TEXT("Gemini"))
	{
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		HttpRequest->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		UE_LOG(LogTemp, Log, TEXT("BP Gen: Gemini API Request with model: %s"), *GeminiModel);
		TArray<TSharedPtr<FJsonValue>> ContentsArray;
		TSharedPtr<FJsonObject> UserTurnObj = MakeShareable(new FJsonObject);
		UserTurnObj->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> UserPartsArr;
		TSharedPtr<FJsonObject> UserPartObj = MakeShareable(new FJsonObject);
		UserPartObj->SetStringField(TEXT("text"), FinalPrompt);
		UserPartsArr.Add(MakeShareable(new FJsonValueObject(UserPartObj)));
		UserTurnObj->SetArrayField(TEXT("parts"), UserPartsArr);
		ContentsArray.Add(MakeShareable(new FJsonValueObject(UserTurnObj)));
		JsonPayload->SetArrayField(TEXT("contents"), ContentsArray);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", FApiKeyManager::Get().GetActiveOpenAIModel());
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField("role", "user");
		UserMessage->SetStringField("content", FinalPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		HttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		HttpRequest->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), FApiKeyManager::Get().GetActiveClaudeModel());
		JsonPayload->SetNumberField(TEXT("max_tokens"), 4096);
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> ContentArray;
		TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
		TextPart->SetStringField("type", "text");
		TextPart->SetStringField("text", FinalPrompt);
		ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
		UserMessage->SetArrayField("content", ContentArray);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		HttpRequest->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", "deepseek-chat");
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField("role", "user");
		UserMessage->SetStringField("content", FinalPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("Custom") && !BaseURL.IsEmpty())
	{
		HttpRequest->SetURL(BaseURL);
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", ModelName);
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField("role", "user");
		UserMessage->SetStringField("content", FinalPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->OnProcessRequestComplete().BindLambda([this, bIsLimitedKey](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
	{
		AsyncTask(ENamedThreads::GameThread, [this, Response, bSuccess, bIsLimitedKey]()
		{
			bIsProjectThinking = false;
			if (bSuccess && Response.IsValid())
			{
				FString ResponseStr = Response->GetContentAsString();
				TSharedPtr<FJsonObject> JsonResponse;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
				if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
				{
					FString AiText;
					if (JsonResponse->HasField(TEXT("candidates")))
					{
						const TArray<TSharedPtr<FJsonValue>>* Candidates;
						if (JsonResponse->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
						{
							TSharedPtr<FJsonObject> Candidate = (*Candidates)[0]->AsObject();
							if (Candidate->HasField(TEXT("content")))
							{
								TSharedPtr<FJsonObject> Content = Candidate->GetObjectField(TEXT("content"));
								const TArray<TSharedPtr<FJsonValue>>* Parts;
								if (Content->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
								{
									AiText = (*Parts)[0]->AsObject()->GetStringField(TEXT("text"));
								}
							}
						}
					}
					else if (JsonResponse->HasField(TEXT("choices")))
					{
						const TArray<TSharedPtr<FJsonValue>>* Choices;
						if (JsonResponse->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
						{
							TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
							if (Choice->HasField(TEXT("message")))
							{
								AiText = Choice->GetObjectField(TEXT("message"))->GetStringField(TEXT("content"));
							}
						}
					}
					else if (JsonResponse->HasField(TEXT("content")))
					{
						const TArray<TSharedPtr<FJsonValue>>* Content;
						if (JsonResponse->TryGetArrayField(TEXT("content"), Content) && Content->Num() > 0)
						{
							AiText = (*Content)[0]->AsObject()->GetStringField(TEXT("text"));
						}
					}
					if (!AiText.IsEmpty())
					{
						if (bIsLimitedKey)
						{
							int32 ResponseChars = AiText.Len();
							FString EncryptedUsageString;
							int32 CurrentCharacters = 0;
							if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
							{
								const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
								CurrentCharacters = EncryptedValue ^ _obf_B;
							}
							CurrentCharacters += ResponseChars;
							const int32 EncryptedNew = CurrentCharacters ^ _obf_B;
							GConfig->SetString(_obf_D, _obf_E, *FString::FromInt(EncryptedNew), GEditorPerProjectIni);
							GConfig->Flush(false, GEditorPerProjectIni);
						}
						TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
						ModelContent->SetStringField(TEXT("role"), TEXT("model"));
						TArray<TSharedPtr<FJsonValue>> ModelParts;
						TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
						ModelPartText->SetStringField(TEXT("text"), AiText);
						ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
						ModelContent->SetArrayField(TEXT("parts"), ModelParts);
						ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
					}
				}
			}
			SaveProjectChatHistory(ActiveProjectChatID);
			RefreshProjectChatView();
		});
	});
	HttpRequest->ProcessRequest();
}
void SBpGeneratorUltimateWidget::SendPerformanceDataToAI(const FString& RawPerformanceData)
{
	if (ActiveProjectChatID.IsEmpty())
	{
		OnNewProjectChatClicked();
	}
	FString UserPlaceholder = TEXT("Generating a comprehensive performance report from the findings...");
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), UserPlaceholder);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	bIsProjectThinking = true;
	RefreshProjectChatView();
	FString FinalPrompt = FString::Printf(
		TEXT("You are an expert Unreal Engine performance analyst. Analyze the following performance issues found in a UE5 project.\n\n"
		"=== INSTRUCTIONS ===\n"
		"1. Group issues by category (Event Tick abuse, Memory, etc.)\n"
		"2. For each issue, provide:\n"
		"   - SEVERITY: CRITICAL (noticeable FPS impact), WARNING (scale issues), or INFO (best practice)\n"
		"   - SPECIFIC FIX: Exact steps or code changes needed\n"
		"   - IMPACT: Estimated FPS improvement if fixed\n"
		"3. Be CONCISE - no fluff, just actionable info\n"
		"4. Use Markdown tables for summary\n\n"
		"=== CLICKABLE ASSET LINKS (MANDATORY) ===\n"
		"When mentioning ANY blueprint/asset, use this format: `[AssetName](ue://asset?name=AssetName)`\n"
		"Example: `[WB_StatusCombatText](ue://asset?name=WB_StatusCombatText)` NOT the full path\n"
		"Extract just the asset name from paths like /Game/Folder/BP_MyActor.BP_MyActor -> BP_MyActor\n\n"
		"=== UE DOCUMENTATION REFERENCES ===\n"
		"For each issue category, include a 'Learn more:' link:\n"
		"- Tick issues: [Tick Optimization](https://dev.epicgames.com/documentation/en-us/unreal-engine/optimizing-the-tick-event-in-unreal-engine)\n"
		"- Actor iteration: [Actor Iterators](https://dev.epicgames.com/documentation/en-us/unreal-engine/iterators-in-unreal-engine)\n"
		"- Memory/GC: [Memory & Performance](https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-and-performance-in-unreal-engine)\n\n"
		"=== COMMON UE5 PERFORMANCE FIXES ===\n"
		"- Cast<> in Tick: Cache reference in BeginPlay, validate with IsValid\n"
		"- GetAllActorsOfClass: Use interfaces, tags, or single manager actor\n"
		"- SpawnActor in Tick: Use object pooling\n"
		"- String ops in Tick: Pre-build strings, use FName\n"
		"- Heavy Tick logic: Use timers, delegates, or coroutines\n\n"
		"=== RAW PERFORMANCE DATA ===\n%s\n=== END DATA ==="),
		*RawPerformanceData
	);
	TArray<TSharedPtr<FJsonValue>> RequestPayloadHistory;
	TSharedPtr<FJsonObject> UserTurn = MakeShareable(new FJsonObject);
	UserTurn->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> TurnParts;
	TSharedPtr<FJsonObject> TurnPart = MakeShareable(new FJsonObject);
	TurnPart->SetStringField(TEXT("text"), FinalPrompt);
	TurnParts.Add(MakeShareable(new FJsonValueObject(TurnPart)));
	UserTurn->SetArrayField(TEXT("parts"), TurnParts);
	RequestPayloadHistory.Add(MakeShareable(new FJsonValueObject(UserTurn)));
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Using Slot %d, Provider=%s, Model=%s, URL=%s"),
		FApiKeyManager::Get().GetActiveSlotIndex(), *ProviderStr, *ModelName, *BaseURL);
	bool bIsLimitedKey = false;
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		bIsLimitedKey = true;
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		int32 DecryptedLimit = _obf_A ^ _obf_B;
		if (CurrentCharacters >= DecryptedLimit)
		{
			FString LimitMessage = TEXT("You have reached the token limit for the built-in API key. Please go to 'API Settings' and enter your own API key to continue using the Project Scanner.");
			TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
			ModelContent->SetStringField(TEXT("role"), TEXT("model"));
			TArray<TSharedPtr<FJsonValue>> ModelParts;
			TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
			ModelPartText->SetStringField(TEXT("text"), LimitMessage);
			ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
			ModelContent->SetArrayField(TEXT("parts"), ModelParts);
			if (ProjectConversationHistory.Num() > 0)
			{
				ProjectConversationHistory.Pop();
			}
			ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
			bIsProjectThinking = false;
			RefreshProjectChatView();
			SaveProjectChatHistory(ActiveProjectChatID);
			return;
		}
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		FString ErrorMessage = FString::Printf(TEXT("No API key configured for %s. Please go to 'API Settings' and enter your API key."), *ProviderStr);
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMessage);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		if (ProjectConversationHistory.Num() > 0)
		{
			ProjectConversationHistory.Pop();
		}
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsProjectThinking = false;
		RefreshProjectChatView();
		SaveProjectChatHistory(ActiveProjectChatID);
		return;
	}
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	if (ProviderStr == TEXT("Custom"))
	{
		FString CustomURL = BaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty())
		{
			CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		}
		HttpRequest->SetURL(CustomURL);
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		FString CustomModel = ModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty())
		{
			CustomModel = TEXT("gpt-3.5-turbo");
		}
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		TArray<TSharedPtr<FJsonValue>> OpenAiMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", TEXT("You are an expert Unreal Engine performance analyst. I have scanned a project and found the following assets with potential performance issues. Your task is to analyze this raw data and present it back to me as a clean, human-readable, and actionable report. Use Markdown for formatting. Group similar issues together if it makes sense, and provide a brief, high-level summary at the end about the general performance health based on these findings."));
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField("role", "user");
		UserMessage->SetStringField("content", RawPerformanceData);
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), OpenAiMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Custom API Request - URL=%s, Model=%s"), *CustomURL, *CustomModel);
	}
	else if (ProviderStr == TEXT("Gemini"))
	{
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		HttpRequest->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		JsonPayload->SetArrayField(TEXT("contents"), RequestPayloadHistory);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Gemini API Request with model: %s"), *GeminiModel);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", FApiKeyManager::Get().GetActiveOpenAIModel());
		TArray<TSharedPtr<FJsonValue>> OpenAiMessages;
		const TSharedPtr<FJsonObject>& GeminiMessage = RequestPayloadHistory[0]->AsObject();
		TSharedPtr<FJsonObject> OpenAiMessage = MakeShareable(new FJsonObject);
		OpenAiMessage->SetStringField(TEXT("role"), TEXT("user"));
		OpenAiMessage->SetStringField(TEXT("content"), GeminiMessage->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text")));
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(OpenAiMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), OpenAiMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: OpenAI API Request"));
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		HttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		HttpRequest->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), FApiKeyManager::Get().GetActiveClaudeModel());
		JsonPayload->SetNumberField(TEXT("max_tokens"), 4096);
		TArray<TSharedPtr<FJsonValue>> ClaudeMessages;
		const TSharedPtr<FJsonObject>& GeminiMessage = RequestPayloadHistory[0]->AsObject();
		TSharedPtr<FJsonObject> ClaudeMessage = MakeShareable(new FJsonObject);
		ClaudeMessage->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> ContentArray;
		TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
		TextPart->SetStringField(TEXT("type"), TEXT("text"));
		TextPart->SetStringField(TEXT("text"), GeminiMessage->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text")));
		ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
		ClaudeMessage->SetArrayField("content", ContentArray);
		ClaudeMessages.Add(MakeShareable(new FJsonValueObject(ClaudeMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), ClaudeMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Claude API Request"));
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		HttpRequest->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), TEXT("deepseek-chat"));
		TArray<TSharedPtr<FJsonValue>> DeepSeekMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", TEXT("You are an expert Unreal Engine performance analyst. I have scanned a project and found the following assets with potential performance issues. Your task is to analyze this raw data and present it back to me as a clean, human-readable, and actionable report."));
		DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		const TSharedPtr<FJsonObject>& GeminiMessage = RequestPayloadHistory[0]->AsObject();
		TSharedPtr<FJsonObject> DeepSeekMessage = MakeShareable(new FJsonObject);
		DeepSeekMessage->SetStringField(TEXT("role"), TEXT("user"));
		DeepSeekMessage->SetStringField(TEXT("content"), GeminiMessage->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text")));
		DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(DeepSeekMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), DeepSeekMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: DeepSeek API Request"));
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->OnProcessRequestComplete().BindSP(this, &SBpGeneratorUltimateWidget::OnProjectApiResponseReceived);
	HttpRequest->ProcessRequest();
}
FReply SBpGeneratorUltimateWidget::OnProjectInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGlobalKeyPress(InKeyEvent))
	{
		return FReply::Handled();
	}
	if (bAssetPickerOpen && ProjectAssetPickerPopup.IsValid())
	{
		if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Enter || InKeyEvent.GetKey() == EKeys::Up || InKeyEvent.GetKey() == EKeys::Down)
		{
			if (ProjectAssetPickerPopup->HandleKeyDown(InKeyEvent))
			{
				return FReply::Handled();
			}
		}
	}
	if (InKeyEvent.GetKey() == EKeys::Enter && !InKeyEvent.IsShiftDown())
	{
		OnSendProjectQuestionClicked();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::V && InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsAltDown())
	{
		if (TryPasteImageFromClipboard(ProjectAttachedImages))
		{
			RefreshProjectImagePreview();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}
FReply SBpGeneratorUltimateWidget::OnAnalystViewSelected()
{
	if (MainViewSwitcher.IsValid())
	{
		MainViewSwitcher->SetActiveWidgetIndex(1);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnProjectViewSelected()
{
	if (MainViewSwitcher.IsValid())
	{
		MainViewSwitcher->SetActiveWidgetIndex(2);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnModelGenViewSelected()
{
	if (MainViewSwitcher.IsValid())
	{
		MainViewSwitcher->SetActiveWidgetIndex(3);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnHowToUseClicked()
{
	bIsShowingHowToUse = true;
	if (MainViewSwitcher.IsValid())
	{
		int32 ActiveIndex = MainViewSwitcher->GetActiveWidgetIndex();
		if (ActiveIndex == 0) CurrentHowToViewIndex = 2;
		else if (ActiveIndex == 1) CurrentHowToViewIndex = 0;
		else if (ActiveIndex == 2) CurrentHowToViewIndex = 1;
		else if (ActiveIndex == 3) CurrentHowToViewIndex = 3;
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCloseHowToUseClicked()
{
	bIsShowingHowToUse = false;
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnTranslateHowToClicked()
{
	if (bIsTranslatingHowTo)
	{
		return FReply::Handled();
	}
	if (MainViewSwitcher.IsValid())
	{
		int32 ActiveIndex = MainViewSwitcher->GetActiveWidgetIndex();
		if (ActiveIndex == 0) CurrentHowToViewIndex = 2;
		else if (ActiveIndex == 1) CurrentHowToViewIndex = 0;
		else if (ActiveIndex == 2) CurrentHowToViewIndex = 1;
		else if (ActiveIndex == 3) CurrentHowToViewIndex = 3;
	}
	FString TargetLanguage = FUIConfigManager::Get().GetLanguage();
	if (TargetLanguage == TEXT("en"))
	{
		FNotificationInfo Info(FText::FromString(TEXT("Content is already in English")));
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
	}
	FString CacheKey = FString::Printf(TEXT("%d_%s"), CurrentHowToViewIndex, *TargetLanguage);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Translate: Using cache key '%s' (ViewIndex=%d)"), *CacheKey, CurrentHowToViewIndex);
	if (FString* CachedTranslation = CachedHowToTranslations.Find(CacheKey))
	{
		FString LanguageName = TEXT("English");
		if (TargetLanguage == TEXT("es")) LanguageName = TEXT("Spanish");
		else if (TargetLanguage == TEXT("fr")) LanguageName = TEXT("French");
		else if (TargetLanguage == TEXT("de")) LanguageName = TEXT("German");
		else if (TargetLanguage == TEXT("zh")) LanguageName = TEXT("Chinese");
		else if (TargetLanguage == TEXT("ja")) LanguageName = TEXT("Japanese");
		else if (TargetLanguage == TEXT("ru")) LanguageName = TEXT("Russian");
		else if (TargetLanguage == TEXT("pt")) LanguageName = TEXT("Portuguese");
		else if (TargetLanguage == TEXT("ko")) LanguageName = TEXT("Korean");
		else if (TargetLanguage == TEXT("it")) LanguageName = TEXT("Italian");
		else if (TargetLanguage == TEXT("ar")) LanguageName = TEXT("Arabic");
		else if (TargetLanguage == TEXT("nl")) LanguageName = TEXT("Dutch");
		else if (TargetLanguage == TEXT("tr")) LanguageName = TEXT("Turkish");
		ShowTranslatedContent(*CachedTranslation, LanguageName);
		return FReply::Handled();
	}
	RequestHowToTranslation(TargetLanguage);
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::RequestHowToTranslation(const FString& TargetLanguage)
{
	bIsTranslatingHowTo = true;
	FString ContentToTranslate = GetHowToContentForTranslation();
	if (ContentToTranslate.IsEmpty())
	{
		bIsTranslatingHowTo = false;
		return;
	}
	FString LanguageName = TEXT("English");
	if (TargetLanguage == TEXT("es")) LanguageName = TEXT("Spanish");
	else if (TargetLanguage == TEXT("fr")) LanguageName = TEXT("French");
	else if (TargetLanguage == TEXT("de")) LanguageName = TEXT("German");
	else if (TargetLanguage == TEXT("zh")) LanguageName = TEXT("Chinese");
	else if (TargetLanguage == TEXT("ja")) LanguageName = TEXT("Japanese");
	else if (TargetLanguage == TEXT("ru")) LanguageName = TEXT("Russian");
	else if (TargetLanguage == TEXT("pt")) LanguageName = TEXT("Portuguese");
	else if (TargetLanguage == TEXT("ko")) LanguageName = TEXT("Korean");
	else if (TargetLanguage == TEXT("it")) LanguageName = TEXT("Italian");
	else if (TargetLanguage == TEXT("ar")) LanguageName = TEXT("Arabic");
	else if (TargetLanguage == TEXT("nl")) LanguageName = TEXT("Dutch");
	else if (TargetLanguage == TEXT("tr")) LanguageName = TEXT("Turkish");
	FString SystemPrompt = FString::Printf(TEXT("You are a professional translator. Translate the following text to %s. Keep all formatting, line breaks, and structure exactly the same. Only output the translated text, nothing else."), *LanguageName);
	int32 ActiveSlot = FApiKeyManager::Get().GetActiveSlotIndex();
	FApiKeySlot SlotConfig = FApiKeyManager::Get().GetSlot(ActiveSlot);
	FString ApiKey = SlotConfig.ApiKey;
	FString BaseUrl = SlotConfig.CustomBaseURL;
	FString OriginalProvider = SlotConfig.Provider;
	FString Provider = SlotConfig.Provider.ToLower();
	FString Model;
	if (Provider == TEXT("gemini"))
	{
		Model = SlotConfig.GeminiModel;
	}
	else if (Provider == TEXT("openai"))
	{
		Model = SlotConfig.OpenAIModel;
	}
	else if (Provider == TEXT("claude"))
	{
		Model = SlotConfig.ClaudeModel;
	}
	else
	{
		Model = SlotConfig.CustomModelName;
	}
	if (ApiKey.IsEmpty())
	{
		if (Provider == TEXT("gemini"))
		{
			ApiKey = AssembleTextFormat(EngineStart, POS);
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Translation: Using built-in Gemini key"));
		}
		else
		{
			bIsTranslatingHowTo = false;
			FNotificationInfo Info(FText::FromString(TEXT("No API key configured. Please set up your API key in settings.")));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}
	}
	FString Endpoint;
	if (Provider == TEXT("openai") || Provider == TEXT("custom"))
	{
		if (BaseUrl.IsEmpty())
		{
			BaseUrl = TEXT("https://api.openai.com/v1");
		}
		Endpoint = BaseUrl + TEXT("/chat/completions");
		if (Model.IsEmpty())
		{
			Model = TEXT("gpt-4o-mini");
		}
	}
	else if (Provider == TEXT("claude"))
	{
		if (BaseUrl.IsEmpty())
		{
			BaseUrl = TEXT("https://api.anthropic.com/v1");
		}
		Endpoint = BaseUrl + TEXT("/messages");
		if (Model.IsEmpty())
		{
			Model = TEXT("claude-3-5-haiku-20241022");
		}
	}
	else if (Provider == TEXT("gemini"))
	{
		if (BaseUrl.IsEmpty())
		{
			BaseUrl = TEXT("https://generativelanguage.googleapis.com/v1beta");
		}
		Endpoint = FString::Printf(TEXT("%s/models/%s:generateContent?key=%s"), *BaseUrl, Model.IsEmpty() ? TEXT("gemini-2.0-flash") : *Model, *ApiKey);
		if (Model.IsEmpty())
		{
			Model = TEXT("gemini-2.0-flash");
		}
	}
	else
	{
		if (BaseUrl.IsEmpty())
		{
			BaseUrl = TEXT("https://api.openai.com/v1");
		}
		Endpoint = BaseUrl + TEXT("/chat/completions");
		if (Model.IsEmpty())
		{
			Model = TEXT("gpt-4o-mini");
		}
	}
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	if (Provider == TEXT("claude"))
	{
		RequestJson->SetStringField(TEXT("model"), Model);
		RequestJson->SetNumberField(TEXT("max_tokens"), 4096);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
		UserMsg->SetStringField(TEXT("role"), TEXT("user"));
		UserMsg->SetStringField(TEXT("content"), SystemPrompt + TEXT("\n\n") + ContentToTranslate);
		Messages.Add(MakeShareable(new FJsonValueObject(UserMsg)));
		RequestJson->SetArrayField(TEXT("messages"), Messages);
	}
	else if (Provider == TEXT("gemini"))
	{
		TSharedPtr<FJsonObject> Contents = MakeShareable(new FJsonObject);
		TArray<TSharedPtr<FJsonValue>> Parts;
		TSharedPtr<FJsonObject> Part = MakeShareable(new FJsonObject);
		Part->SetStringField(TEXT("text"), SystemPrompt + TEXT("\n\n") + ContentToTranslate);
		Parts.Add(MakeShareable(new FJsonValueObject(Part)));
		Contents->SetArrayField(TEXT("parts"), Parts);
		TArray<TSharedPtr<FJsonValue>> ContentsArray;
		ContentsArray.Add(MakeShareable(new FJsonValueObject(Contents)));
		RequestJson->SetArrayField(TEXT("contents"), ContentsArray);
	}
	else
	{
		RequestJson->SetStringField(TEXT("model"), Model);
		RequestJson->SetNumberField(TEXT("max_tokens"), 4096);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> SystemMsg = MakeShareable(new FJsonObject);
		SystemMsg->SetStringField(TEXT("role"), TEXT("system"));
		SystemMsg->SetStringField(TEXT("content"), SystemPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(SystemMsg)));
		TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
		UserMsg->SetStringField(TEXT("role"), TEXT("user"));
		UserMsg->SetStringField(TEXT("content"), ContentToTranslate);
		Messages.Add(MakeShareable(new FJsonValueObject(UserMsg)));
		RequestJson->SetArrayField(TEXT("messages"), Messages);
	}
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (Provider == TEXT("claude"))
	{
		Request->SetHeader(TEXT("x-api-key"), ApiKey);
		Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
	}
	else if (Provider == TEXT("gemini"))
	{
	}
	else
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}
	Request->SetContentAsString(RequestBody);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== Translation Request Debug ==="));
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Original Provider: %s"), *OriginalProvider);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Provider (lower): %s"), *Provider);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Endpoint: %s"), *Endpoint);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Model: %s"), *Model);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  GeminiModel from slot: %s"), *SlotConfig.GeminiModel);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Target Language: %s"), *TargetLanguage);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Request Body Length: %d"), RequestBody.Len());
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("================================"));
	FString CapturedLanguage = TargetLanguage;
	Request->OnProcessRequestComplete().BindLambda([this, CapturedLanguage](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bSuccess)
	{
		bIsTranslatingHowTo = false;
		if (!bSuccess || !Response.IsValid())
		{
			FNotificationInfo Info(FText::FromString(TEXT("Translation failed: Network error")));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}
		int32 ResponseCode = Response->GetResponseCode();
		FString ResponseContent = Response->GetContentAsString();
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== Translation Response Debug ==="));
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Response Code: %d"), ResponseCode);
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Response Content: %s"), *ResponseContent.Left(500));
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=================================="));
		if (ResponseCode != 200)
		{
			UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Translation failed with HTTP %d: %s"), ResponseCode, *ResponseContent);
			FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Translation failed: HTTP %d"), ResponseCode)));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}
		FString TranslatedText;
		TSharedPtr<FJsonObject> ResponseJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Choices;
			if (ResponseJson->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* ChoiceObj;
				if ((*Choices)[0]->TryGetObject(ChoiceObj))
				{
					const TSharedPtr<FJsonObject>* MessageObj;
					if ((*ChoiceObj)->TryGetObjectField(TEXT("message"), MessageObj))
					{
						(*MessageObj)->TryGetStringField(TEXT("content"), TranslatedText);
					}
				}
			}
			if (TranslatedText.IsEmpty())
			{
				const TArray<TSharedPtr<FJsonValue>>* Content;
				if (ResponseJson->TryGetArrayField(TEXT("content"), Content) && Content->Num() > 0)
				{
					const TSharedPtr<FJsonObject>* ContentObj;
					if ((*Content)[0]->TryGetObject(ContentObj))
					{
						(*ContentObj)->TryGetStringField(TEXT("text"), TranslatedText);
					}
				}
			}
			if (TranslatedText.IsEmpty())
			{
				const TArray<TSharedPtr<FJsonValue>>* Candidates;
				if (ResponseJson->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
				{
					const TSharedPtr<FJsonObject>* CandidateObj;
					if ((*Candidates)[0]->TryGetObject(CandidateObj))
					{
						const TSharedPtr<FJsonObject>* ContentObj;
						if ((*CandidateObj)->TryGetObjectField(TEXT("content"), ContentObj))
						{
							const TArray<TSharedPtr<FJsonValue>>* Parts;
							if ((*ContentObj)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
							{
								const TSharedPtr<FJsonObject>* PartObj;
								if ((*Parts)[0]->TryGetObject(PartObj))
								{
									(*PartObj)->TryGetStringField(TEXT("text"), TranslatedText);
								}
							}
						}
					}
				}
			}
		}
		if (TranslatedText.IsEmpty())
		{
			FNotificationInfo Info(FText::FromString(TEXT("Translation failed: Could not parse response")));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}
		OnHowToTranslationComplete(TranslatedText, CapturedLanguage);
	});
	Request->ProcessRequest();
	FNotificationInfo Info(UIConfig::GetText(TEXT("translating_status"), TEXT("Translating...")));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}
void SBpGeneratorUltimateWidget::OnHowToTranslationComplete(const FString& TranslatedContent, const FString& TargetLanguage)
{
	FString CacheKey = FString::Printf(TEXT("%d_%s"), CurrentHowToViewIndex, *TargetLanguage);
	CachedHowToTranslations.Add(CacheKey, TranslatedContent);
	FString LanguageName = TEXT("English");
	if (TargetLanguage == TEXT("es")) LanguageName = TEXT("Spanish");
	else if (TargetLanguage == TEXT("fr")) LanguageName = TEXT("French");
	else if (TargetLanguage == TEXT("de")) LanguageName = TEXT("German");
	else if (TargetLanguage == TEXT("zh")) LanguageName = TEXT("Chinese");
	else if (TargetLanguage == TEXT("ja")) LanguageName = TEXT("Japanese");
	else if (TargetLanguage == TEXT("ru")) LanguageName = TEXT("Russian");
	else if (TargetLanguage == TEXT("pt")) LanguageName = TEXT("Portuguese");
	else if (TargetLanguage == TEXT("ko")) LanguageName = TEXT("Korean");
	else if (TargetLanguage == TEXT("it")) LanguageName = TEXT("Italian");
	else if (TargetLanguage == TEXT("ar")) LanguageName = TEXT("Arabic");
	else if (TargetLanguage == TEXT("nl")) LanguageName = TEXT("Dutch");
	else if (TargetLanguage == TEXT("tr")) LanguageName = TEXT("Turkish");
	ShowTranslatedContent(TranslatedContent, LanguageName);
}
void SBpGeneratorUltimateWidget::ShowTranslatedContent(const FString& TranslatedContent, const FString& LanguageName)
{
	if (TranslationResultWindow.IsValid())
	{
		TranslationResultWindow->RequestDestroyWindow();
		TranslationResultWindow.Reset();
	}
	TranslationResultWindow = SNew(SWindow)
		.Title(FText::FromString(FString::Printf(TEXT("How to Use - %s Translation"), *LanguageName)))
		.ClientSize(FVector2D(600, 500))
		.SupportsMaximize(true)
		.SupportsMinimize(false)
		[
			SNew(SBorder)
				.Padding(10)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0f)
						[
							SNew(SScrollBox)
								+ SScrollBox::Slot()
								[
									SNew(STextBlock)
										.Text(FText::FromString(TranslatedContent))
										.AutoWrapText(true)
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
								]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0).HAlign(HAlign_Right)
						[
							SNew(SButton)
								.Text(FText::FromString(TEXT("Close")))
								.OnClicked_Lambda([this]() {
									if (TranslationResultWindow.IsValid())
									{
										TranslationResultWindow->RequestDestroyWindow();
										TranslationResultWindow.Reset();
									}
									return FReply::Handled();
								})
						]
				]
		];
	FSlateApplication::Get().AddWindow(TranslationResultWindow.ToSharedRef());
}
FString SBpGeneratorUltimateWidget::GetHowToContentForTranslation() const
{
	FString Content;
	if (CurrentHowToViewIndex == 1)
	{
		Content = TEXT(
			"Project Scanner - How to Use\n\n"
			"The Project Scanner indexes your entire project so you can ask high-level questions about your codebase. It understands Blueprint relationships, asset dependencies, and project architecture.\n\n"
			"HOW IT WORKS\n"
			"1. Click 'Scan and Index Project' - This reads all Blueprints, Widgets, AnimBPs, Behavior Trees, Materials, Enums, Structs, and DataAssets\n\n"
			"2. Wait for completion - Large projects may take a minute\n\n"
			"3. Ask questions - The AI answers based on the scanned index\n\n"
			"EXAMPLE QUESTIONS\n"
			"- \"Where is the player's health variable managed?\"\n"
			"- \"Find all Blueprints that use timers\"\n"
			"- \"Give me a performance report of Blueprints using Event Tick\"\n"
			"- \"Which Behavior Tree handles enemy AI?\"\n"
			"- \"Show me all assets that reference BP_Player\"\n"
			"- \"Create a pie chart of asset types in my project\"\n\n"
			"VISUAL GRAPHS & DIAGRAMS\n"
			"- Generate pie charts to visualize asset distributions\n"
			"- Create flowcharts for architecture overviews\n"
			"- Click diagrams to view fullscreen or pop out to separate window\n"
			"- Zoom with CTRL+mouse wheel\n\n"
			"QUICK ACTIONS\n"
			"- Performance Report - Analyze Event Tick usage and expensive operations\n"
			"- Project Overview - Get a summary of your project structure\n\n"
			"TIPS\n"
			"- Use @ to reference specific assets in your questions\n"
			"- Re-scan after major changes to keep the index updated\n"
			"- The scanner works with all Blueprint types\n\n"
			"Note: The AI answers based ONLY on the scanned index. It cannot see real-time changes until you re-scan."
		);
	}
	else if (CurrentHowToViewIndex == 2)
	{
		Content = TEXT(
			"Blueprint Architect - How to Use\n\n"
			"BIG FEATURES\n\n"
			"Blueprints & Actors\n"
			"- Create new Blueprint classes and actors from scratch\n"
			"- Add variables, functions, and components\n"
			"- Implement game logic and interactions\n"
			"- Generate complete gameplay systems\n\n"
			"Widget/UI Creation\n"
			"- Design UMG widgets and HUD elements\n"
			"- Create menus, buttons, and input fields\n"
			"- Set up data binding and event handlers\n\n"
			"Materials & Textures\n"
			"- Generate PBR materials with texture maps\n"
			"- Create procedural materials\n"
			"- Import and configure textures\n\n"
			"3D Model Generation\n"
			"- Text-to-3D: Generate models from descriptions\n"
			"- Image-to-3D: Convert images to 3D models\n"
			"- Supports GLB, FBX, OBJ, USDZ formats\n\n"
			"Blueprint Analysis\n"
			"- Analyze existing blueprints\n"
			"- Debug and optimize code\n"
			"- Refactor and improve patterns\n\n"
			"Visual Graphs & Diagrams\n"
			"- Generate pie charts for data visualization\n"
			"- Create flowcharts for logic flows\n"
			"- View in modal or pop-out window\n\n"
			"HELPFUL FEATURES\n"
			"- @ Reference System: Type @ to reference assets for context\n"
			"- File Attachments: Import files as additional context\n"
			"- Image Attachments: Attach images for visual context\n"
			"- Custom Instructions: Set persistent AI behavior rules\n"
			"- Memory: Store important project context\n\n"
			"USAGE TIPS\n"
			"- Be specific about asset paths and names\n"
			"- Use @ to provide asset context\n"
			"- Describe desired behavior clearly\n"
			"- Break complex tasks into steps\n\n"
			"KEYBOARD SHORTCUTS\n"
			"- ALT+1-9: Switch API key slots\n"
			"- CTRL+wheel: Zoom in chat\n\n"
			"Note: The Architect creates real assets in your project. Always describe what you want clearly and specify paths when needed."
		);
	}
	else if (CurrentHowToViewIndex == 3)
	{
		Content = TEXT(
			"How to Use 3D Model Generation\n\n"
			"Text-to-3D Generation\n"
			"Generate 3D models directly from text descriptions using the Meshy API.\n\n"
			"Steps:\n"
			"1. Enter a detailed description of your model\n"
			"2. Choose quality: Preview (fast) or Full (detailed)\n"
			"3. Select output format: GLB, FBX, OBJ, or USDZ\n"
			"4. Click Generate and wait for completion\n"
			"5. Model auto-imports to your project\n\n"
			"Image-to-3D Conversion\n"
			"Convert 2D images into 3D models. Upload concept art or reference images to generate 3D assets.\n\n"
			"Steps:\n"
			"1. Attach an image using the image button\n"
			"2. Optionally add a text description for guidance\n"
			"3. Choose quality and format\n"
			"4. Click Generate\n\n"
			"Model Settings\n"
			"- Quality: Preview generates quickly, Full takes longer but produces better results\n"
			"- Format: GLB is recommended for UE5, FBX for compatibility\n"
			"- Auto-Refine: Automatically refine preview models\n\n"
			"AI Chat Tools\n"
			"- Ask questions about 3D modeling\n"
			"- Get prompts optimized for generation\n"
			"- Learn best practices\n\n"
			"TIPS & BEST PRACTICES\n"
			"- Be descriptive: Include shape, style, materials, colors\n"
			"- Specify scale: small, medium, large, life-size\n"
			"- Add style keywords: realistic, stylized, low-poly, cartoon\n"
			"- Reference existing designs: like a chair, like a sword, etc.\n\n"
			"EXAMPLE PROMPTS\n"
			"- A wooden treasure chest with gold trim\n"
			"- A futuristic sci-fi door with glowing panels\n"
			"- A simple wooden table, rustic style\n"
			"- A fantasy sword with ornate engravings\n\n"
			"Note: 3D generation requires a Meshy API key. Configure your API key in the settings panel. Generation typically takes 1-3 minutes for Preview mode."
		);
	}
	else
	{
		Content = TEXT("How to Use");
	}
	return Content;
}
FReply SBpGeneratorUltimateWidget::OnCustomInstructionsClicked()
{
	bIsShowingCustomInstructions = !bIsShowingCustomInstructions;
	if (bIsShowingCustomInstructions && CustomInstructionsInput.IsValid())
	{
		CustomInstructionsInput->SetText(FText::FromString(CustomInstructions));
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCloseCustomInstructionsClicked()
{
	if (CustomInstructionsInput.IsValid())
	{
		CustomInstructions = CustomInstructionsInput->GetText().ToString();
		GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("CustomInstructions"), *CustomInstructions, GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}
	bIsShowingCustomInstructions = false;
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::UpdateScanStatus(const FString& Message, bool bIsError)
{
	if (ScanStatusTextBlock.IsValid())
	{
		ScanStatusTextBlock->SetText(FText::FromString(Message));
		ScanStatusTextBlock->SetColorAndOpacity(bIsError ? FLinearColor::Red : CurrentTheme.ScannerColor);
	}
}
FReply SBpGeneratorUltimateWidget::OnScanProjectClicked()
{
	if (bIsScanning) return FReply::Handled();
	bIsScanning = true;
	ScanProjectButton->SetEnabled(false);
	UpdateScanStatus(TEXT("Gathering asset list from project..."));
	TSharedPtr<TArray<FAssetData>> AssetDataListPtr = MakeShared<TArray<FAssetData>>();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> TempAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	AssetRegistryModule.Get().GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	AssetRegistryModule.Get().GetAssetsByClass(UEnum::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	AssetRegistryModule.Get().GetAssetsByClass(UScriptStruct::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	AssetRegistryModule.Get().GetAssetsByClass(UDataAsset::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetClassPathName(), TempAssets, true);
	AssetDataListPtr->Append(TempAssets);
	TempAssets.Empty();
	UpdateScanStatus(FString::Printf(TEXT("Found %d assets. Starting analysis on background thread..."), AssetDataListPtr->Num()));
	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this, AssetDataListPtr]()
		{
			HandleScanAndIndexProject(*AssetDataListPtr);
		});
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::HandleScanAndIndexProject(const TArray<FAssetData>& AssetDataList)
{
	TSharedPtr<FJsonObject> IndexObject = MakeShareable(new FJsonObject);
	FBtGraphDescriber BtDescriber;
	TArray<FAssetData> GameAssets;
	for (const FAssetData& Data : AssetDataList)
	{
		if (Data.PackagePath.ToString().StartsWith(TEXT("/Game")))
		{
			GameAssets.Add(Data);
		}
	}
	const int32 TotalAssets = GameAssets.Num();
	const int32 BatchSize = 25;
	int32 ProcessedCount = 0;
	int32 WidgetCount = 0;
	int32 ActorCount = 0;
	int32 AnimBPCount = 0;
	int32 BTCount = 0;
	int32 EnumCount = 0;
	int32 StructCount = 0;
	int32 InterfaceCount = 0;
	int32 DataAssetCount = 0;
	int32 DataTableCount = 0;
	int32 OtherCount = 0;
	for (int32 BatchStart = 0; BatchStart < TotalAssets; BatchStart += BatchSize)
	{
		int32 BatchEnd = FMath::Min(BatchStart + BatchSize, TotalAssets);
		AsyncTask(ENamedThreads::GameThread, [this, BatchStart, TotalAssets]()
		{
			UpdateScanStatus(FString::Printf(TEXT("Scanning... %d/%d assets"), BatchStart, TotalAssets));
		});
		for (int32 i = BatchStart; i < BatchEnd; i++)
		{
			const FAssetData& Data = GameAssets[i];
			FString ClassName = Data.AssetClassPath.ToString();
			if (ClassName.Contains(TEXT("AnimSequence")) ||
			    ClassName.Contains(TEXT("AnimMontage")) ||
			    ClassName.Contains(TEXT("AnimComposite")) ||
			    ClassName.Contains(TEXT("SoundWave")) ||
			    ClassName.Contains(TEXT("Texture2D")) ||
			    ClassName.Contains(TEXT("StaticMesh")) ||
			    ClassName.Contains(TEXT("SkeletalMesh")) ||
			    ClassName.Contains(TEXT("Material")) ||
			    ClassName.Contains(TEXT("PhysicsAsset")) ||
			    ClassName.Contains(TEXT("ParticleSystem")) ||
			    ClassName.Contains(TEXT("NiagaraSystem")))
			{
				ProcessedCount++;
				continue;
			}
			TSharedPtr<UObject*> AssetPtr = MakeShared<UObject*>(nullptr);
			FString AssetPathForLog = Data.GetSoftObjectPath().ToString();
			UE_LOG(LogBpGeneratorUltimate, Verbose, TEXT("Project Scan: Loading asset %s (class: %s)"), *AssetPathForLog, *ClassName);
			FGraphEventRef GameThreadTask = FFunctionGraphTask::CreateAndDispatchWhenReady([AssetDataToLoad = Data, AssetPtr, AssetPathForLog]()
				{
					FString AssetPath = AssetDataToLoad.GetSoftObjectPath().ToString();
					UE_LOG(LogBpGeneratorUltimate, Verbose, TEXT("Project Scan: Loading on game thread: %s"), *AssetPath);
					UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
					if (LoadedAsset)
					{
						*AssetPtr = LoadedAsset;
						UE_LOG(LogBpGeneratorUltimate, Verbose, TEXT("Project Scan: Successfully loaded: %s"), *AssetPath);
					}
					else
					{
						UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Project Scan: Failed to load: %s"), *AssetPath);
					}
				}, TStatId(), nullptr, ENamedThreads::GameThread);
			FTaskGraphInterface::Get().WaitUntilTaskCompletes(GameThreadTask);
			UObject* Asset = *AssetPtr;
			if (!Asset)
			{
				ProcessedCount++;
				continue;
			}
			FString Summary;
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
			{
				if (Blueprint->BlueprintType == BPTYPE_Interface)
				{
					TArray<FString> Functions;
					for (const auto& Func : Blueprint->FunctionGraphs)
					{
						if (Func)
						{
							Functions.Add(Func->GetName());
						}
					}
					Summary = FString::Printf(TEXT("--- INTERFACE SUMMARY ---\nName: %s\nFunctions: %s"),
						*Blueprint->GetName(),
						*FString::Join(Functions, TEXT(", ")));
					InterfaceCount++;
				}
				else
				{
					Summary = FBpSummarizerr::Summarize(Blueprint);
					if (Blueprint->ParentClass)
					{
						FString ParentClassName = Blueprint->ParentClass->GetName();
						if (ParentClassName.Contains(TEXT("UserWidget")) || ParentClassName.Contains(TEXT("Widget")))
						{
							WidgetCount++;
						}
						else if (ParentClassName.Contains(TEXT("AnimInstance")))
						{
							AnimBPCount++;
						}
						else if (ParentClassName.Contains(TEXT("Actor")))
						{
							ActorCount++;
						}
						else
						{
							OtherCount++;
						}
					}
					else
					{
						OtherCount++;
					}
				}
			}
			else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(Asset))
			{
				Summary = BtDescriber.Describe(BehaviorTree);
				BTCount++;
			}
			else if (UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(Asset))
			{
				TArray<FString> EnumValues;
				for (int32 Idx = 0; Idx < UserEnum->NumEnums(); Idx++)
				{
					FString DisplayName = UserEnum->GetDisplayNameTextByIndex(Idx).ToString();
					if (!DisplayName.IsEmpty())
					{
						EnumValues.Add(DisplayName);
					}
				}
				Summary = FString::Printf(TEXT("--- ENUM SUMMARY ---\nName: %s\nValues: %s"),
					*UserEnum->GetName(),
					*FString::Join(EnumValues, TEXT(", ")));
				EnumCount++;
			}
			else if (UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(Asset))
			{
				TArray<FString> StructMembers;
				const TArray<FStructVariableDescription>& VarDescs = FStructureEditorUtils::GetVarDesc(UserStruct);
				for (const FStructVariableDescription& VarDesc : VarDescs)
				{
					StructMembers.Add(VarDesc.VarName.ToString());
				}
				Summary = FString::Printf(TEXT("--- STRUCT SUMMARY ---\nName: %s\nMembers: %s"),
					*UserStruct->GetName(),
					*FString::Join(StructMembers, TEXT(", ")));
				StructCount++;
			}
			else if (UDataAsset* DataAsset = Cast<UDataAsset>(Asset))
			{
				TArray<FString> Properties;
				for (TFieldIterator<FProperty> PropIt(DataAsset->GetClass()); PropIt; ++PropIt)
				{
					FProperty* Prop = *PropIt;
					if (!Prop->IsA<FObjectProperty>() && !Prop->GetName().StartsWith(TEXT("_")))
					{
						Properties.Add(FString::Printf(TEXT("%s (%s)"), *Prop->GetName(), *Prop->GetClass()->GetName()));
					}
				}
				Summary = FString::Printf(TEXT("--- DATA ASSET SUMMARY ---\nName: %s\nClass: %s\nProperties: %s"),
					*DataAsset->GetName(),
					*DataAsset->GetClass()->GetName(),
					*FString::Join(Properties, TEXT(", ")));
				DataAssetCount++;
			}
			else if (UDataTable* DataTable = Cast<UDataTable>(Asset))
			{
				const UScriptStruct* RowStruct = DataTable->GetRowStruct();
				TArray<FString> Columns;
				if (RowStruct)
				{
					if (const UUserDefinedStruct* RowUserStruct = Cast<UUserDefinedStruct>(RowStruct))
					{
						const TArray<FStructVariableDescription>& VarDescs = FStructureEditorUtils::GetVarDesc(RowUserStruct);
						for (const FStructVariableDescription& VarDesc : VarDescs)
						{
							Columns.Add(VarDesc.VarName.ToString());
						}
					}
					else
					{
						for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
						{
							Columns.Add(PropIt->GetName());
						}
					}
				}
				Summary = FString::Printf(TEXT("--- DATA TABLE SUMMARY ---\nName: %s\nRow Type: %s\nColumns: %s"),
					*DataTable->GetName(),
					RowStruct ? *RowStruct->GetName() : TEXT("Unknown"),
					*FString::Join(Columns, TEXT(", ")));
				DataTableCount++;
			}
			else
			{
				OtherCount++;
			}
			if (!Summary.IsEmpty())
			{
				IndexObject->SetStringField(Asset->GetPathName(), Summary);
			}
			ProcessedCount++;
		}
		FPlatformProcess::Sleep(0.005f);
	}
	IndexObject->SetNumberField(TEXT("_total_assets"), ProcessedCount);
	IndexObject->SetNumberField(TEXT("_widget_count"), WidgetCount);
	IndexObject->SetNumberField(TEXT("_actor_count"), ActorCount);
	IndexObject->SetNumberField(TEXT("_anim_bp_count"), AnimBPCount);
	IndexObject->SetNumberField(TEXT("_behavior_tree_count"), BTCount);
	IndexObject->SetNumberField(TEXT("_enum_count"), EnumCount);
	IndexObject->SetNumberField(TEXT("_struct_count"), StructCount);
	IndexObject->SetNumberField(TEXT("_interface_count"), InterfaceCount);
	IndexObject->SetNumberField(TEXT("_data_asset_count"), DataAssetCount);
	IndexObject->SetNumberField(TEXT("_data_table_count"), DataTableCount);
	IndexObject->SetNumberField(TEXT("_other_count"), OtherCount);
	IndexObject->SetStringField(TEXT("_scan_timestamp"), FDateTime::UtcNow().ToString());
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(IndexObject.ToSharedRef(), Writer);
	const FString SavePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavePath), true);
	FString ErrorMessage;
	if (!FFileHelper::SaveStringToFile(OutputString, *SavePath))
	{
		ErrorMessage = FString::Printf(TEXT("Failed to save Project Index file to %s"), *SavePath);
	}
	AsyncTask(ENamedThreads::GameThread, [this, ErrorMessage, ProcessedCount, WidgetCount, ActorCount, AnimBPCount, BTCount, EnumCount, StructCount, InterfaceCount, DataAssetCount, DataTableCount]()
		{
			bIsScanning = false;
			ScanProjectButton->SetEnabled(true);
			if (ErrorMessage.IsEmpty())
			{
				FString StatusMsg = FString::Printf(TEXT("Scan complete! Indexed %d assets"), ProcessedCount);
				UpdateScanStatus(StatusMsg);
				RefreshProjectChatView();
				FString NotificationMsg = FString::Printf(
					TEXT("Widgets: %d | Actors: %d | Anim BPs: %d | BTs: %d\nEnums: %d | Structs: %d | Interfaces: %d\nData Assets: %d | Data Tables: %d"),
					WidgetCount, ActorCount, AnimBPCount, BTCount, EnumCount, StructCount, InterfaceCount, DataAssetCount, DataTableCount);
				FNotificationInfo Info(FText::FromString(NotificationMsg));
				Info.ExpireDuration = 8.0f;
				Info.bUseLargeFont = false;
				TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
				if (NotificationItem.IsValid())
				{
					NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
				}
			}
			else
			{
				UpdateScanStatus(ErrorMessage, true);
			}
		});
}
void SBpGeneratorUltimateWidget::CheckForExistingIndex()
{
	const FString SavePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	if (FPaths::FileExists(SavePath))
	{
		UpdateScanStatus(TEXT("Project index found. You can ask questions or re-scan for updates."));
	}
	else
	{
		UpdateScanStatus(TEXT("Project index not found. Please scan the project to begin."));
	}
}
void SBpGeneratorUltimateWidget::SendProjectChatRequest()
{
	const FString IndexFilePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	if (!FPaths::FileExists(IndexFilePath))
	{
		FString ErrorMsg = TEXT("Error: Project Index file not found. Please scan the project before asking questions.");
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMsg);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsProjectThinking = false;
		RefreshProjectChatView();
		SaveProjectChatHistory(ActiveProjectChatID);
		return;
	}
	FString IndexFileContent;
	if (!FFileHelper::LoadFileToString(IndexFileContent, *IndexFilePath))
	{
		bIsProjectThinking = false;
		RefreshProjectChatView();
		return;
	}
	TSharedPtr<FJsonObject> ProjectIndexObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(IndexFileContent);
	if (!FJsonSerializer::Deserialize(Reader, ProjectIndexObject) || !ProjectIndexObject.IsValid())
	{
		bIsProjectThinking = false;
		RefreshProjectChatView();
		return;
	}
	FString UserQuestion = ProjectConversationHistory.Last()->AsObject()->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text"));
	TArray<FString> Keywords;
	UserQuestion.ParseIntoArray(Keywords, TEXT(" "), true);
	Keywords.RemoveAll([](const FString& Word) {
		return Word.Len() <= 2 || Word.Equals(TEXT("the"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("what"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("where"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("how"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("is"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("are"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("which"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("does"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("has"), ESearchCase::IgnoreCase) || Word.Equals(TEXT("have"), ESearchCase::IgnoreCase);
		});
	TMap<FString, TArray<FString>> SemanticKeywords;
	SemanticKeywords.Add(TEXT("widget"), {TEXT("UserWidget"), TEXT("WBP_"), TEXT("UMG"), TEXT("button"), TEXT("text"), TEXT("image"), TEXT("canvas"), TEXT("hud")});
	SemanticKeywords.Add(TEXT("widgets"), {TEXT("UserWidget"), TEXT("WBP_"), TEXT("UMG")});
	SemanticKeywords.Add(TEXT("ui"), {TEXT("Widget"), TEXT("UMG"), TEXT("HUD"), TEXT("Menu"), TEXT("WBP_")});
	SemanticKeywords.Add(TEXT("controller"), {TEXT("PlayerController"), TEXT("AIController"), TEXT("input"), TEXT("possess"), TEXT("control")});
	SemanticKeywords.Add(TEXT("health"), {TEXT("Health"), TEXT("Damage"), TEXT("TakeDamage"), TEXT("Die"), TEXT("life"), TEXT("hp")});
	SemanticKeywords.Add(TEXT("enemy"), {TEXT("Enemy"), TEXT("AI"), TEXT("Monster"), TEXT("NPC"), TEXT("foe"), TEXT("opponent")});
	SemanticKeywords.Add(TEXT("enemies"), {TEXT("Enemy"), TEXT("AI"), TEXT("Monster"), TEXT("NPC")});
	SemanticKeywords.Add(TEXT("weapon"), {TEXT("Weapon"), TEXT("Gun"), TEXT("Sword"), TEXT("Shoot"), TEXT("Attack"), TEXT("Fire")});
	SemanticKeywords.Add(TEXT("weapons"), {TEXT("Weapon"), TEXT("Gun"), TEXT("Sword")});
	SemanticKeywords.Add(TEXT("player"), {TEXT("Player"), TEXT("Character"), TEXT("Pawn"), TEXT("BP_Player"), TEXT("hero")});
	SemanticKeywords.Add(TEXT("character"), {TEXT("Character"), TEXT("Pawn"), TEXT("Actor"), TEXT("BP_Char")});
	SemanticKeywords.Add(TEXT("camera"), {TEXT("Camera"), TEXT("SpringArm"), TEXT("view")});
	SemanticKeywords.Add(TEXT("movement"), {TEXT("Movement"), TEXT("Move"), TEXT("Walk"), TEXT("Run"), TEXT("Jump")});
	SemanticKeywords.Add(TEXT("animation"), {TEXT("Anim"), TEXT("AnimInstance"), TEXT("ABP_"), TEXT("skeleton")});
	SemanticKeywords.Add(TEXT("audio"), {TEXT("Audio"), TEXT("Sound"), TEXT("Music"), TEXT("SFX")});
	SemanticKeywords.Add(TEXT("sound"), {TEXT("Sound"), TEXT("Audio"), TEXT("SFX")});
	SemanticKeywords.Add(TEXT("inventory"), {TEXT("Inventory"), TEXT("Item"), TEXT("Pickup"), TEXT("Slot")});
	SemanticKeywords.Add(TEXT("score"), {TEXT("Score"), TEXT("Points"), TEXT("Points")});
	SemanticKeywords.Add(TEXT("save"), {TEXT("Save"), TEXT("Load"), TEXT("GameInstance"), TEXT("persist")});
	SemanticKeywords.Add(TEXT("menu"), {TEXT("Menu"), TEXT("WBP_"), TEXT("Widget"), TEXT("pause")});
	SemanticKeywords.Add(TEXT("ai"), {TEXT("AI"), TEXT("BehaviorTree"), TEXT("BT_"), TEXT("Blackboard"), TEXT("Task")});
	SemanticKeywords.Add(TEXT("behavior"), {TEXT("BehaviorTree"), TEXT("BT_"), TEXT("Blackboard")});
	SemanticKeywords.Add(TEXT("combat"), {TEXT("Combat"), TEXT("Attack"), TEXT("Damage"), TEXT("Fight"), TEXT("Melee"), TEXT("Weapon"), TEXT("Hit"), TEXT("Combo"), TEXT("Parry"), TEXT("Block"), TEXT("Dodge")});
	SemanticKeywords.Add(TEXT("melee"), {TEXT("Melee"), TEXT("Attack"), TEXT("Combo"), TEXT("Weapon"), TEXT("Sword"), TEXT("Hit")});
	SemanticKeywords.Add(TEXT("agro"), {TEXT("Agro"), TEXT("Aggro"), TEXT("Threat"), TEXT("Target"), TEXT("Chase"), TEXT("Enemy"), TEXT("AI")});
	SemanticKeywords.Add(TEXT("aggro"), {TEXT("Aggro"), TEXT("Agro"), TEXT("Threat"), TEXT("Target"), TEXT("Chase"), TEXT("Enemy"), TEXT("AI")});
	SemanticKeywords.Add(TEXT("deagro"), {TEXT("Deagro"), TEXT("De-aggro"), TEXT("Reset"), TEXT("Return"), TEXT("Lose"), TEXT("Threat")});
	SemanticKeywords.Add(TEXT("spawn"), {TEXT("Spawn"), TEXT("BeginPlay"), TEXT("Construct")});
	SemanticKeywords.Add(TEXT("material"), {TEXT("Material"), TEXT("M_"), TEXT("texture"), TEXT("shader")});
	SemanticKeywords.Add(TEXT("particle"), {TEXT("Particle"), TEXT("Emitter"), TEXT("VFX"), TEXT("FX")});
	SemanticKeywords.Add(TEXT("physics"), {TEXT("Physics"), TEXT("Collision"), TEXT("Rigidbody")});
	SemanticKeywords.Add(TEXT("network"), {TEXT("Network"), TEXT("Replicate"), TEXT("Multiplayer"), TEXT("RPC")});
	SemanticKeywords.Add(TEXT("multiplayer"), {TEXT("Multiplayer"), TEXT("Network"), TEXT("Replicate"), TEXT("Server"), TEXT("Client")});
	SemanticKeywords.Add(TEXT("vr"), {TEXT("VR"), TEXT("XR"), TEXT("MotionController"), TEXT("HMD")});
	SemanticKeywords.Add(TEXT("vehicle"), {TEXT("Vehicle"), TEXT("Car"), TEXT("WheeledVehicle")});
	SemanticKeywords.Add(TEXT("door"), {TEXT("Door"), TEXT("Gate"), TEXT("Open"), TEXT("Close"), TEXT("Interact")});
	SemanticKeywords.Add(TEXT("interact"), {TEXT("Interact"), TEXT("Interface"), TEXT("Use"), TEXT("Action")});
	SemanticKeywords.Add(TEXT("teleport"), {TEXT("Teleport"), TEXT("Portal"), TEXT("Transport")});
	SemanticKeywords.Add(TEXT("setup"), {TEXT("Setup"), TEXT("Config"), TEXT("Initialize"), TEXT("Init")});
	SemanticKeywords.Add(TEXT("settings"), {TEXT("Settings"), TEXT("Config"), TEXT("Options"), TEXT("Preferences")});
	SemanticKeywords.Add(TEXT("enum"), {TEXT("ENUM SUMMARY"), TEXT("enumeration"), TEXT("E_")});
	SemanticKeywords.Add(TEXT("struct"), {TEXT("STRUCT SUMMARY"), TEXT("structure"), TEXT("F_")});
	SemanticKeywords.Add(TEXT("interface"), {TEXT("INTERFACE SUMMARY"), TEXT("BPI_"), TEXT("contract")});
	SemanticKeywords.Add(TEXT("data"), {TEXT("DATA ASSET SUMMARY"), TEXT("DataAsset"), TEXT("DA_"), TEXT("config")});
	SemanticKeywords.Add(TEXT("type"), {TEXT("ENUM SUMMARY"), TEXT("STRUCT SUMMARY"), TEXT("struct"), TEXT("enum")});
	SemanticKeywords.Add(TEXT("anim"), {TEXT("AnimInstance"), TEXT("ABP_"), TEXT("skeleton"), TEXT("animation")});
	SemanticKeywords.Add(TEXT("table"), {TEXT("DATA TABLE SUMMARY"), TEXT("DataTable"), TEXT("DT_"), TEXT("row")});
	TArray<FString> ExpandedKeywords;
	for (const FString& Word : Keywords)
	{
		ExpandedKeywords.Add(Word);
		FString LowerWord = Word.ToLower();
		if (TArray<FString>* Related = SemanticKeywords.Find(LowerWord))
		{
			ExpandedKeywords.Append(*Related);
		}
	}
	int32 TotalAssets = 0, WidgetCount = 0, ActorCount = 0, AnimBPCount = 0, BTCount = 0;
	int32 EnumCount = 0, StructCount = 0, InterfaceCount = 0, DataAssetCount = 0, DataTableCount = 0;
	ProjectIndexObject->TryGetNumberField(TEXT("_total_assets"), TotalAssets);
	ProjectIndexObject->TryGetNumberField(TEXT("_widget_count"), WidgetCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_actor_count"), ActorCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_anim_bp_count"), AnimBPCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_behavior_tree_count"), BTCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_enum_count"), EnumCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_struct_count"), StructCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_interface_count"), InterfaceCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_data_asset_count"), DataAssetCount);
	ProjectIndexObject->TryGetNumberField(TEXT("_data_table_count"), DataTableCount);
	int32 FoundCount = 0;
	TMap<FString, FString> RelevantContextMap;
	const int32 MaxContextEntries = 60;
	auto GetAssetPriority = [](const FString& AssetPath, const FString& Content) -> int32
	{
		if (Content.Contains(TEXT("AIController")) || Content.Contains(TEXT("PlayerController"))) return 100;
		if (Content.Contains(TEXT("BehaviorTree")) || AssetPath.Contains(TEXT("BT_"))) return 95;
		if (Content.Contains(TEXT("Blackboard")) || AssetPath.Contains(TEXT("BB_"))) return 90;
		if (Content.Contains(TEXT("Parent Class: Actor")) || Content.Contains(TEXT("Parent Class: AActor"))) return 85;
		if (Content.Contains(TEXT("Parent Class: Character")) || Content.Contains(TEXT("Parent Class: ACharacter"))) return 85;
		if (Content.Contains(TEXT("Parent Class: Pawn"))) return 80;
		if (Content.Contains(TEXT("INTERFACE SUMMARY"))) return 75;
		if (Content.Contains(TEXT("STRUCT SUMMARY"))) return 70;
		if (Content.Contains(TEXT("ENUM SUMMARY"))) return 65;
		if (Content.Contains(TEXT("Parent Class: UserWidget"))) return 60;
		if (Content.Contains(TEXT("DATA TABLE SUMMARY"))) return 55;
		if (Content.Contains(TEXT("DATA ASSET SUMMARY"))) return 50;
		if (Content.Contains(TEXT("Parent Class: AnimInstance"))) return 40;
		if (AssetPath.Contains(TEXT("AnimNotify")) || AssetPath.Contains(TEXT("AN_")) || AssetPath.Contains(TEXT("NotifyState"))) return 10;
		if (AssetPath.Contains(TEXT("CameraShake"))) return 15;
		return 30;
	};
	for (const FString& Keyword : ExpandedKeywords)
	{
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			FString FolderPath = FPaths::GetPath(Pair.Key);
			if (FolderPath.Contains(Keyword, ESearchCase::IgnoreCase))
			{
				RelevantContextMap.Add(Pair.Key, Pair.Value->AsString());
				if (RelevantContextMap.Num() >= MaxContextEntries) break;
			}
		}
		if (RelevantContextMap.Num() >= MaxContextEntries) break;
	}
	if (RelevantContextMap.Num() < MaxContextEntries)
	{
		for (const FString& Keyword : ExpandedKeywords)
		{
			for (const auto& Pair : ProjectIndexObject->Values)
			{
				if (Pair.Key.StartsWith(TEXT("_"))) continue;
				if (RelevantContextMap.Contains(Pair.Key)) continue;
				if (Pair.Key.Contains(Keyword, ESearchCase::IgnoreCase))
				{
					RelevantContextMap.Add(Pair.Key, Pair.Value->AsString());
					if (RelevantContextMap.Num() >= MaxContextEntries) break;
				}
			}
			if (RelevantContextMap.Num() >= MaxContextEntries) break;
		}
	}
	if (RelevantContextMap.Num() < MaxContextEntries)
	{
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			FString Content = Pair.Value->AsString();
			for (const FString& Keyword : ExpandedKeywords)
			{
				if (Content.Contains(Keyword, ESearchCase::IgnoreCase))
				{
					RelevantContextMap.Add(Pair.Key, Content);
					if (RelevantContextMap.Num() >= MaxContextEntries) break;
				}
			}
			if (RelevantContextMap.Num() >= MaxContextEntries) break;
		}
	}
	if (RelevantContextMap.Num() == 0 && TotalAssets > 0)
	{
		int32 AddedCount = 0;
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			FString Content = Pair.Value->AsString();
			if (Content.Contains(TEXT("UserWidget")) || Content.Contains(TEXT("UUserWidget")))
			{
				RelevantContextMap.Add(Pair.Key, Content);
				if (++AddedCount >= 3) break;
			}
		}
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			FString Content = Pair.Value->AsString();
			if (Content.Contains(TEXT("Parent Class: Actor")) || Content.Contains(TEXT("Parent Class: AActor")))
			{
				RelevantContextMap.Add(Pair.Key, Content);
				if (++AddedCount >= 6) break;
			}
		}
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			FString Content = Pair.Value->AsString();
			if (Content.Contains(TEXT("ENUM SUMMARY")))
			{
				RelevantContextMap.Add(Pair.Key, Content);
				if (++AddedCount >= 8) break;
			}
		}
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			FString Content = Pair.Value->AsString();
			if (Content.Contains(TEXT("DATA TABLE SUMMARY")))
			{
				RelevantContextMap.Add(Pair.Key, Content);
				if (++AddedCount >= 10) break;
			}
		}
		for (const auto& Pair : ProjectIndexObject->Values)
		{
			if (Pair.Key.StartsWith(TEXT("_"))) continue;
			if (RelevantContextMap.Contains(Pair.Key)) continue;
			RelevantContextMap.Add(Pair.Key, Pair.Value->AsString());
			if (++AddedCount >= 15) break;
		}
	}
	FString RelevantContextString;
	if (RelevantContextMap.Num() > 0)
	{
		TSharedPtr<FJsonObject> RelevantContextObject = MakeShareable(new FJsonObject);
		for (const auto& Pair : RelevantContextMap)
		{
			RelevantContextObject->SetStringField(Pair.Key, Pair.Value);
		}
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RelevantContextString);
		FJsonSerializer::Serialize(RelevantContextObject.ToSharedRef(), Writer);
	}
	else
	{
		RelevantContextString = FString::Printf(
			TEXT("No specific matches found, but the project contains:\n"
			"- %d total assets\n"
			"- %d Widget Blueprints (Parent Class: UserWidget)\n"
			"- %d Actor Blueprints (Parent Class: Actor)\n"
			"- %d Animation Blueprints (Parent Class: AnimInstance)\n"
			"- %d Behavior Trees\n"
			"- %d Enums\n"
			"- %d Structs\n"
			"- %d Blueprint Interfaces\n"
			"- %d Data Assets\n"
			"- %d Data Tables\n"
			"Try searching with different terms, function names, or variable names."),
			TotalAssets, WidgetCount, ActorCount, AnimBPCount, BTCount, EnumCount, StructCount, InterfaceCount, DataAssetCount, DataTableCount);
	}
	TArray<FAttachedImage> ImagesFromHistory;
	if (ProjectConversationHistory.Num() > 0)
	{
		TSharedPtr<FJsonObject> LastMessage = ProjectConversationHistory.Last()->AsObject();
		if (LastMessage.IsValid() && LastMessage->GetStringField(TEXT("role")) == TEXT("user"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Parts;
			if (LastMessage->TryGetArrayField(TEXT("parts"), Parts))
			{
				for (const TSharedPtr<FJsonValue>& Part : *Parts)
				{
					TSharedPtr<FJsonObject> PartObj = Part->AsObject();
					if (PartObj.IsValid() && PartObj->HasField(TEXT("inline_data")))
					{
						TSharedPtr<FJsonObject> InlineData = PartObj->GetObjectField(TEXT("inline_data"));
						FAttachedImage Img;
						Img.MimeType = InlineData->GetStringField(TEXT("mime_type"));
						Img.Base64Data = InlineData->GetStringField(TEXT("data"));
						Img.Name = TEXT("from_history");
						ImagesFromHistory.Add(Img);
					}
				}
			}
		}
	}
	FString FinalUserPrompt = FString::Printf(TEXT("User's Question: %s"), *UserQuestion);
	if (ImagesFromHistory.Num() > 0)
	{
		FinalUserPrompt = FString::Printf(TEXT("The user has attached %d image(s). Please analyze the image(s) and respond to: %s"), ImagesFromHistory.Num(), *UserQuestion);
	}
	FString SystemPrompt = FString::Printf(
		TEXT("You are an expert Unreal Engine project analyst. Your task is to answer a question about a project based on the provided JSON context.\n\n"
		"=== IMAGE CAPABILITY ===\n"
		"You CAN see and analyze images attached by the user. If the user attaches an image (screenshot, diagram, UI mockup, etc.), describe what you see and help with their question.\n\n"
		"=== PROJECT OVERVIEW ===\n"
		"Total Assets Indexed: %d\n"
		"- Widget Blueprints: %d\n"
		"- Actor Blueprints: %d\n"
		"- Animation Blueprints: %d\n"
		"- Behavior Trees: %d\n"
		"- Enums: %d\n"
		"- Structs: %d\n"
		"- Blueprint Interfaces: %d\n"
		"- Data Assets: %d\n"
		"- Data Tables: %d\n\n"
		"=== HOW TO SEARCH (IMPORTANT) ===\n"
		"NOTE: Developers may NOT follow standard naming conventions (BP_, WBP_, BT_ prefixes).\n"
		"ALWAYS search by ASSET TYPE and CONTENT, not just asset names:\n\n"
		"BLUEPRINTS:\n"
		"- Widget/UI: Look for 'Parent Class: UserWidget' or summary contains 'UserWidget'\n"
		"- Actor/Character: Look for 'Parent Class: Actor' or summary contains 'AActor'\n"
		"- Animation: Look for 'Parent Class: AnimInstance' - skeletal animation logic\n"
		"- Controller: Look for 'Parent Class: PlayerController' or 'AIController'\n"
		"- Component: Look for 'Parent Class: ActorComponent' or specific component types\n\n"
		"AI/BEHAVIOR:\n"
		"- Look for 'BehaviorTree' in asset name OR function names like 'OnSeePlayer', 'UpdateAI'\n\n"
		"DATA STRUCTURES:\n"
		"- Enums: Look for 'ENUM SUMMARY' in content - named value lists\n"
		"- Structs: Look for 'STRUCT SUMMARY' in content - data types like FHealth, FDamage\n"
		"- Interfaces: Look for 'INTERFACE SUMMARY' in content - contracts between actors\n"
		"- Data Assets: Look for 'DATA ASSET SUMMARY' - configuration objects\n"
		"- Data Tables: Look for 'DATA TABLE SUMMARY' - row-based lookup tables\n\n"
		"SEARCH BY FUNCTIONALITY:\n"
		"- User asks about 'health'? Search for 'Health', 'TakeDamage', 'Die' in functions/vars\n"
		"- User asks about 'movement'? Search for 'Move', 'Walk', 'Jump', 'Velocity'\n"
		"- User asks about 'inventory'? Search for 'Item', 'Slot', 'Pickup', 'AddItem'\n"
		"- Each asset summary includes: components, variables, functions, and graph logic\n\n"
		"**MANDATORY: CLICKABLE ASSET LINKS**\n"
		"When mentioning ANY asset name, wrap it in: `[AssetName](ue://asset?name=AssetName)`\n\n"
		"Examples:\n"
		"- Write `[BP_EnemyAI](ue://asset?name=BP_EnemyAI)` not just BP_EnemyAI\n"
		"- Write `[MainMenu](ue://asset?name=MainMenu)` for non-prefixed names\n\n"
		"Rules:\n"
		"- Link EVERY asset name in plain text\n"
		"- Do NOT link inside code blocks\n"
		"- Use asset name only, not full path\n\n"
		"=== CHART CAPABILITY ===\n"
		"You HAVE the ability to create visual pie charts. They will be automatically rendered as SVG graphics.\n"
		"```chart\n"
		"Category1 count1, Category2 count2, Category3 count3\n"
		"```\n"
		"Example: Widgets 15, Actors 23, Materials 8\n\n"
		"=== FLOWCHART CAPABILITY ===\n"
		"You HAVE the ability to create flowcharts. Use ONLY this exact syntax:\n"
		"```flow\n"
		"[Start] -> [Step 1]\n"
		"[Step 1] -> [Step 2]\n"
		"[Step 2] -> [End]\n"
		"```\n"
		"CRITICAL RULES:\n"
		"- Every node MUST be in square brackets: [Node Name]\n"
		"- Connections use arrow: ->\n"
		"- NO other syntax like st=> or => is valid\n"
		"- Keep labels short (1-3 words)\n"
		"- NO clickable links inside flowchart nodes - use plain text only\n"
		"- NO markdown links like [Name](url) inside flow blocks\n\n"
		"--- RELEVANT CONTEXT ---\n"
		"%s\n"
		"--- END CONTEXT ---"),
		TotalAssets, WidgetCount, ActorCount, AnimBPCount, BTCount, EnumCount, StructCount, InterfaceCount, DataAssetCount, DataTableCount, *RelevantContextString
	);
	if (!CustomInstructions.IsEmpty())
	{
		SystemPrompt += TEXT("\n\n=== USER CUSTOM INSTRUCTIONS ===\n");
		SystemPrompt += CustomInstructions;
		SystemPrompt += TEXT("\n=== END CUSTOM INSTRUCTIONS ===");
	}
	FString AssetSummary = FAssetReferenceManager::Get().GetSummaryForAI();
	if (!AssetSummary.IsEmpty())
	{
		SystemPrompt += TEXT("\n\n=== REFERENCED ASSETS (from @ mentions) ===\n");
		SystemPrompt += AssetSummary;
		SystemPrompt += TEXT("\n=== END REFERENCED ASSETS ===");
	}
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	UE_LOG(LogTemp, Log, TEXT("BP Gen Project Chat: Using Slot %d, Provider=%s, Model=%s, URL=%s"),
		FApiKeyManager::Get().GetActiveSlotIndex(), *ProviderStr, *ModelName, *BaseURL);
	bool bIsLimitedKey = false;
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		bIsLimitedKey = true;
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		const int32 DecryptedLimit = _obf_A ^ _obf_B;
		if (CurrentCharacters >= DecryptedLimit)
		{
			FString LimitMessage = TEXT("You have reached the token limit for the built-in API key. Please go to 'API Settings' and enter your own API key to continue using the Project Scanner.");
			TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
			ModelContent->SetStringField(TEXT("role"), TEXT("model"));
			TArray<TSharedPtr<FJsonValue>> ModelParts;
			TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
			ModelPartText->SetStringField(TEXT("text"), LimitMessage);
			ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
			ModelContent->SetArrayField(TEXT("parts"), ModelParts);
			ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
			bIsProjectThinking = false;
			RefreshProjectChatView();
			SaveProjectChatHistory(ActiveProjectChatID);
			return;
		}
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		FString ErrorMessage = FString::Printf(TEXT("No API key configured for %s. Please go to 'API Settings' and enter your API key."), *ProviderStr);
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMessage);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsProjectThinking = false;
		RefreshProjectChatView();
		SaveProjectChatHistory(ActiveProjectChatID);
		return;
	}
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	if (ProviderStr == TEXT("Gemini"))
	{
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		HttpRequest->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		UE_LOG(LogTemp, Log, TEXT("BP Gen: Gemini API Request with model: %s"), *GeminiModel);
		TArray<TSharedPtr<FJsonValue>> ContentsArray;
		TSharedPtr<FJsonObject> SystemTurn = MakeShareable(new FJsonObject);
		SystemTurn->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> SystemParts;
		TSharedPtr<FJsonObject> SystemPart = MakeShareable(new FJsonObject);
		SystemPart->SetStringField(TEXT("text"), SystemPrompt);
		SystemParts.Add(MakeShareable(new FJsonValueObject(SystemPart)));
		SystemTurn->SetArrayField(TEXT("parts"), SystemParts);
		ContentsArray.Add(MakeShareable(new FJsonValueObject(SystemTurn)));
		TSharedPtr<FJsonObject> ModelAck = MakeShareable(new FJsonObject);
		ModelAck->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> AckParts;
		TSharedPtr<FJsonObject> AckPart = MakeShareable(new FJsonObject);
		AckPart->SetStringField(TEXT("text"), TEXT("Understood. I will answer the user's question based only on the provided context."));
		AckParts.Add(MakeShareable(new FJsonValueObject(AckPart)));
		ModelAck->SetArrayField(TEXT("parts"), AckParts);
		ContentsArray.Add(MakeShareable(new FJsonValueObject(ModelAck)));
		TSharedPtr<FJsonObject> UserTurn = MakeShareable(new FJsonObject);
		UserTurn->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> UserParts;
		TSharedPtr<FJsonObject> UserPart = MakeShareable(new FJsonObject);
		UserPart->SetStringField(TEXT("text"), FinalUserPrompt);
		UserParts.Add(MakeShareable(new FJsonValueObject(UserPart)));
		for (const FAttachedImage& Image : ImagesFromHistory)
		{
			TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
			TSharedPtr<FJsonObject> InlineData = MakeShareable(new FJsonObject);
			InlineData->SetStringField(TEXT("mime_type"), Image.MimeType);
			InlineData->SetStringField(TEXT("data"), Image.Base64Data);
			ImagePart->SetObjectField(TEXT("inline_data"), InlineData);
			UserParts.Add(MakeShareable(new FJsonValueObject(ImagePart)));
		}
		UserTurn->SetArrayField(TEXT("parts"), UserParts);
		ContentsArray.Add(MakeShareable(new FJsonValueObject(UserTurn)));
		JsonPayload->SetArrayField(TEXT("contents"), ContentsArray);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", FApiKeyManager::Get().GetActiveOpenAIModel());
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", SystemPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField("role", "user");
		TArray<TSharedPtr<FJsonValue>> ContentArray;
		TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
		TextPart->SetStringField("type", "text");
		TextPart->SetStringField("text", FinalUserPrompt);
		ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
		for (const FAttachedImage& Image : ImagesFromHistory)
		{
			TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
			ImagePart->SetStringField("type", "image_url");
			TSharedPtr<FJsonObject> ImageUrl = MakeShareable(new FJsonObject);
			ImageUrl->SetStringField("url", FString::Printf(TEXT("data:%s;base64,%s"), *Image.MimeType, *Image.Base64Data));
			ImagePart->SetObjectField("image_url", ImageUrl);
			ContentArray.Add(MakeShareable(new FJsonValueObject(ImagePart)));
		}
		UserMessage->SetArrayField("content", ContentArray);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		HttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		HttpRequest->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), FApiKeyManager::Get().GetActiveClaudeModel());
		JsonPayload->SetNumberField(TEXT("max_tokens"), 4096);
		JsonPayload->SetStringField(TEXT("system"), SystemPrompt);
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> ContentArray;
		TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
		TextPart->SetStringField("type", "text");
		TextPart->SetStringField("text", FinalUserPrompt);
		ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
		for (const FAttachedImage& Image : ImagesFromHistory)
		{
			TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
			ImagePart->SetStringField("type", "image");
			TSharedPtr<FJsonObject> Source = MakeShareable(new FJsonObject);
			Source->SetStringField("type", "base64");
			Source->SetStringField("media_type", Image.MimeType);
			Source->SetStringField("data", Image.Base64Data);
			ImagePart->SetObjectField("source", Source);
			ContentArray.Add(MakeShareable(new FJsonValueObject(ImagePart)));
		}
		UserMessage->SetArrayField("content", ContentArray);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		HttpRequest->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), TEXT("deepseek-chat"));
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", SystemPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField(TEXT("role"), TEXT("user"));
		UserMessage->SetStringField("content", FinalUserPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	else if (ProviderStr == TEXT("Custom"))
	{
		FString CustomURL = BaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty())
		{
			CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		}
		HttpRequest->SetURL(CustomURL);
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		FString CustomModel = ModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty())
		{
			CustomModel = TEXT("gpt-3.5-turbo");
		}
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		TArray<TSharedPtr<FJsonValue>> MessagesArray;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", SystemPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		TSharedPtr<FJsonObject> UserMessage = MakeShareable(new FJsonObject);
		UserMessage->SetStringField(TEXT("role"), TEXT("user"));
		UserMessage->SetStringField("content", FinalUserPrompt);
		MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessage)));
		JsonPayload->SetArrayField(TEXT("messages"), MessagesArray);
	}
	ProjectAttachedImages.Empty();
	RefreshProjectImagePreview();
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->OnProcessRequestComplete().BindSP(this, &SBpGeneratorUltimateWidget::OnProjectApiResponseReceived);
	HttpRequest->ProcessRequest();
}
void SBpGeneratorUltimateWidget::OnProjectApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Response received"));
	UE_LOG(LogTemp, Log, TEXT("  - bWasSuccessful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log, TEXT("  - Response Valid: %s"), Response.IsValid() ? TEXT("true") : TEXT("false"));
	if (Response.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("  - Response Code: %d"), Response->GetResponseCode());
		UE_LOG(LogTemp, Log, TEXT("  - Content Length: %d"), Response->GetContentLength());
	}
	bIsProjectThinking = false;
	FString AiMessage;
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString ProviderStr = FApiKeyManager::Get().GetActiveProvider();
			if (ProviderStr == TEXT("Gemini"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Candidates;
				if (JsonObject->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& CandidateObject = (*Candidates)[0]->AsObject();
					const TSharedPtr<FJsonObject>* Content = nullptr;
					if (CandidateObject->TryGetObjectField(TEXT("content"), Content))
					{
						const TArray<TSharedPtr<FJsonValue>>* Parts = nullptr;
						if ((*Content)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
						{
							(*Parts)[0]->AsObject()->TryGetStringField(TEXT("text"), AiMessage);
						}
					}
				}
			}
			else if (ProviderStr == TEXT("OpenAI") || ProviderStr == TEXT("Custom") || ProviderStr == TEXT("DeepSeek"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices;
				if (JsonObject->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& ChoiceObject = (*Choices)[0]->AsObject();
					const TSharedPtr<FJsonObject>* MessageObject = nullptr;
					if (ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject))
					{
						(*MessageObject)->TryGetStringField(TEXT("content"), AiMessage);
					}
				}
			}
			else if (ProviderStr == TEXT("Claude"))
			{
				const TArray<TSharedPtr<FJsonValue>>* ContentBlocks = nullptr;
				if (JsonObject->TryGetArrayField(TEXT("content"), ContentBlocks) && ContentBlocks->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& FirstBlock = (*ContentBlocks)[0]->AsObject();
					FirstBlock->TryGetStringField(TEXT("text"), AiMessage);
				}
			}
		}
	}
	if (AiMessage.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BP Gen Project Scanner: HTTP Request Failed"));
		UE_LOG(LogTemp, Error, TEXT("  - bWasSuccessful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("  - Response Valid: %s"), Response.IsValid() ? TEXT("true") : TEXT("false"));
		if (Response.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("  - Response Code: %d"), Response->GetResponseCode());
			UE_LOG(LogTemp, Error, TEXT("  - Content Length: %d"), Response->GetContentLength());
			FString ResponseBody = Response->GetContentAsString();
			UE_LOG(LogTemp, Error, TEXT("  - Response Body (first 500 chars): %s"), *ResponseBody.Left(500));
		}
		AiMessage = FString::Printf(TEXT("Error: Failed to get a valid response from the API.\n\nResponse Code: %d"),
			Response.IsValid() ? Response->GetResponseCode() : 0);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("BP Gen Project Scanner: Successfully parsed AI response (%d chars)"), AiMessage.Len());
	}
	if (FApiKeyManager::Get().GetActiveApiKey().IsEmpty())
	{
		int32 TotalCharsInInteraction = 0;
		const TSharedPtr<FJsonObject>& LastUserMessage = ProjectConversationHistory.Last()->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* PartsArray;
		if (LastUserMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
		{
			FString Content;
			if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
			{
				TotalCharsInInteraction += Content.Len();
			}
		}
		TotalCharsInInteraction += AiMessage.Len();
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		CurrentCharacters += TotalCharsInInteraction;
		const int32 ValueToStore = CurrentCharacters ^ _obf_B;
		GConfig->SetString(_obf_D, _obf_E, *FString::FromInt(ValueToStore), GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}
	const FString PerformanceReportPlaceholder = TEXT("Generating a comprehensive performance report from the findings...");
	bool bWasPerformanceReport = false;
	if (ProjectConversationHistory.Num() > 0)
	{
		const TSharedPtr<FJsonObject>& LastMessage = ProjectConversationHistory.Last()->AsObject();
		FString LastMessageRole;
		if (LastMessage->TryGetStringField(TEXT("role"), LastMessageRole) && LastMessageRole == TEXT("user"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Parts;
			if (LastMessage->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
			{
				FString LastMessageText;
				if ((*Parts)[0]->AsObject()->TryGetStringField(TEXT("text"), LastMessageText) && LastMessageText == PerformanceReportPlaceholder)
				{
					bWasPerformanceReport = true;
				}
			}
		}
	}
	TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
	ModelContent->SetStringField(TEXT("role"), TEXT("model"));
	TArray<TSharedPtr<FJsonValue>> ModelParts;
	TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
	ModelPartText->SetStringField(TEXT("text"), AiMessage);
	ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
	ModelContent->SetArrayField(TEXT("parts"), ModelParts);
	if (bWasPerformanceReport)
	{
		ProjectConversationHistory.Last() = MakeShareable(new FJsonValueObject(ModelContent));
	}
	else
	{
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	}
	SaveProjectChatHistory(ActiveProjectChatID);
	RefreshProjectChatView();
}
void SBpGeneratorUltimateWidget::RefreshProjectChatView()
{
	if (!ProjectChatBrowser.IsValid()) return;
	TStringBuilder<65536> HtmlBuilder;
	static FString MarkedJs;
	static FString GraphreJs;
	static FString NomnomlJs;
	if (MarkedJs.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BpGeneratorUltimate"));
		if (Plugin.IsValid())
		{
			FString MarkedJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("marked.min.js"));
			FFileHelper::LoadFileToString(MarkedJs, *MarkedJsPath);
			FString GraphreJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("graphre.min.js"));
			FFileHelper::LoadFileToString(GraphreJs, *GraphreJsPath);
			FString NomnomlJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("nomnoml.min.js"));
			FFileHelper::LoadFileToString(NomnomlJs, *NomnomlJsPath);
		}
	}
	HtmlBuilder.Append(TEXT("<!DOCTYPE html><html><head><meta charset='utf-8'><script>"));
	HtmlBuilder.Append(MarkedJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(GraphreJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(NomnomlJs);
	HtmlBuilder.Append(TEXT("</script><script>"
	"function createPieChart(dataStr){"
	"var colors=['#4fc3f7','#81c784','#ffb74d','#f06292','#ba68c8','#4db6ac','#ffd54f','#90a4ae','#e57373','#64b5f6'];"
	"var parts=dataStr.split(',').map(function(p){var s=p.trim().split(/\\s+/);return{label:s[0],value:parseInt(s[1])||0};});"
	"var total=parts.reduce(function(s,p){return s+p.value;},0);"
	"if(total===0)return'<p>No data</p>';"
	"var cx=100,cy=100,r=80;"
	"var svg='<svg width=\"300\" height=\"250\" style=\"margin:16px auto;display:block;\">';"
	"var angle=-Math.PI/2;"
	"parts.forEach(function(p,i){"
	"var pct=p.value/total;"
	"var sweep=pct*2*Math.PI;"
	"var x1=cx+r*Math.cos(angle),y1=cy+r*Math.sin(angle);"
	"angle+=sweep;"
	"var x2=cx+r*Math.cos(angle),y2=cy+r*Math.sin(angle);"
	"var large=sweep>Math.PI?1:0;"
	"svg+='<path d=\"M'+cx+','+cy+' L'+x1+','+y1+' A'+r+','+r+' 0 '+large+',1 '+x2+','+y2+' Z\" fill=\"'+colors[i%colors.length]+'\"/>';"
	"});"
	"svg+='<circle cx=\"'+cx+'\" cy=\"'+cy+'\" r=\"35\" fill=\"#1e1e1e\"/>';"
	"svg+='<text x=\"'+cx+'\" y=\"'+(cy+5)+'\" text-anchor=\"middle\" fill=\"#d4d4d4\" font-size=\"12\">'+total+' total</text></svg>';"
	"svg+='<div class=\"chart-legend\" style=\"display:flex;flex-wrap:wrap;gap:8px;justify-content:center;margin:8px 0;\">';"
	"parts.forEach(function(p,i){"
	"var pct=Math.round(p.value/total*100);"
	"svg+='<span style=\"display:flex;align-items:center;gap:4px;font-size:12px;\"><span style=\"width:12px;height:12px;background:'+colors[i%colors.length]+';border-radius:2px;\"></span>'+p.label+' ('+pct+'%)</span>';"
	"});"
	"svg+='</div>';"
	"return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML+this.querySelector(\\'.chart-legend\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML+this.parentElement.querySelector(\\'.chart-legend\\').outerHTML)\">⤢</button></div>';"
	"}"
	"function parseMarkdown(str){"
	"var html=marked.parse(str);"
	"html=html.replace(/<pre><code class=\"language-chart\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){return createPieChart(c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&'));});"
	"html=html.replace(/<pre><code class=\"language-flow\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){var d=c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&').trim();d=d.replace(/\\(([^)]*)\\)/g,'-$1-');var styled='#stroke: #b0b0b0\\n#lineWidth: 2\\n#fill: #2d2d30\\n#fontSize: 14\\n'+d;try{if(typeof nomnoml==='undefined')return'<pre style=\"color:#f44\">Flow error: nomnoml not loaded</pre>';var svg=nomnoml.renderSvg(styled);return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML)\">⤢</button></div>';}catch(e){return'<details><summary style=\"color:#f44;cursor:pointer\">Flow error: '+e.message+'</summary><pre style=\"background:#2d2d30;padding:8px\">'+d.substring(0,200)+'</pre></details>';}});"
	"html=html.replace(/\\[([^\\]]+)\\]\\(ue:\\/\\/asset\\?name=([^)]+)\\)/g,'<a href=\"ue://asset?name=$2\" class=\"asset-link\">$1</a>');"
	"return html;"
	"}"
	"</script><style>"));
	HtmlBuilder.Append(TEXT("body{font:14px 'Segoe UI',Arial;margin:16px;line-height:1.6;color:#d4d4d4;background:#1e1e1e;}"));
	HtmlBuilder.Append(TEXT("a{color:#ffa726 !important;text-decoration:none;}a:hover{color:#ffcc80 !important;}"));
	HtmlBuilder.Append(TEXT(".speaker{font-weight:bold;margin-bottom:2px;}"));
	HtmlBuilder.Append(TEXT(".user-msg{margin-bottom:15px;}"));
	HtmlBuilder.Append(TEXT(".ai-msg{margin-bottom:15px;background:#252526;padding:1px 15px;border-radius:5px;}"));
	HtmlBuilder.Append(TEXT("h1,h2,h3{color:#e0e0e0;border-bottom:1px solid #404040;padding-bottom:0.3em;}"));
	HtmlBuilder.Append(TEXT("code{background:#2d2d30;color:#ce9178;padding:0.2em 0.4em;border-radius:3px;font-family:'Consolas','Courier New',monospace;}"));
	HtmlBuilder.Append(TEXT("pre{background:#2d2d30;padding:16px;border-radius:6px;overflow:auto;} pre code{padding:0;}"));
	HtmlBuilder.Append(TEXT("table{border-collapse:collapse;margin:16px 0;width:100%;}"));
	HtmlBuilder.Append(TEXT("th,td{border:1px solid #404040;padding:8px 12px;text-align:left;}"));
	HtmlBuilder.Append(TEXT("th{background:#2d2d30;color:#e0e0e0;font-weight:600;}"));
	HtmlBuilder.Append(TEXT("tr:nth-child(even){background:#252526;}"));
	HtmlBuilder.Append(TEXT("tr:hover{background:#363636;}"));
	HtmlBuilder.Append(TEXT(".asset-link{color:#ffa726;text-decoration:none;border-bottom:1px dashed #ffa726;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".asset-link:hover{color:#ffcc80;background:rgba(255,167,38,0.15);}"));
	HtmlBuilder.Append(TEXT("#zoom-controls{position:fixed;bottom:10px;right:10px;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:4px 8px;border-radius:4px;z-index:1000;border:1px solid #404040;}"));
	HtmlBuilder.Append(TEXT("#zoom-indicator{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset{background:#444;color:#d4d4d4;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.9);z-index:2000;justify-content:center;align-items:center;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal.show{display:flex;}"));
	HtmlBuilder.Append(TEXT("#modal-content{max-width:90%;max-height:90%;overflow:auto;background:#1e1e1e;border-radius:8px;padding:20px;position:relative;}"));
	HtmlBuilder.Append(TEXT("#modal-close{position:absolute;top:10px;right:10px;background:#444;color:#fff;border:none;width:30px;height:30px;border-radius:50%;cursor:pointer;font-size:16px;z-index:10;}"));
	HtmlBuilder.Append(TEXT("#modal-toolbar{position:sticky;top:0;left:0;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:8px 12px;border-radius:4px;border:1px solid #404040;margin-bottom:12px;z-index:5;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint{color:#888;font-size:11px;display:flex;align-items:center;gap:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint kbd{background:#444;color:#ccc;padding:2px 6px;border-radius:3px;font-size:10px;border:1px solid #555;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-pct{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;border-left:1px solid #404040;padding-left:8px;margin-left:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn{background:#444;color:#d4d4d4;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn{background:#0078d4;color:#fff;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;margin-left:auto;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn:hover{background:#1084d8;}"));
	HtmlBuilder.Append(TEXT(".diagram-container{position:relative;display:inline-block;max-width:150px;max-height:150px;overflow:hidden;border:1px solid #404040;border-radius:4px;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".diagram-container svg{max-width:150px;max-height:150px;overflow:hidden;}"));
	HtmlBuilder.Append(TEXT(".diagram-container .chart-legend{display:none;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand{position:absolute;top:5px;right:5px;background:rgba(45,45,48,0.9);color:#b0b0b0;border:none;width:24px;height:24px;border-radius:4px;cursor:pointer;font-size:14px;opacity:0;transition:opacity 0.2s;}"));
	HtmlBuilder.Append(TEXT(".diagram-container:hover .diagram-expand{opacity:1;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand:hover{background:#2ea043;color:#fff;}"));
	HtmlBuilder.Append(TEXT("</style><script>"));
	HtmlBuilder.Append(TEXT("var zoomLevel=100;function updateZoom(){document.getElementById('zoom-content').style.transform='scale('+zoomLevel/100+')';document.getElementById('zoom-content').style.transformOrigin='top left';document.getElementById('zoom-indicator').textContent=zoomLevel+'%';}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('wheel',function(e){if(e.ctrlKey&&!document.getElementById('diagram-modal').classList.contains('show')){e.preventDefault();zoomLevel+=e.deltaY>0?-10:10;zoomLevel=Math.max(30,Math.min(200,zoomLevel));updateZoom();}},{passive:false});"));
	HtmlBuilder.Append(TEXT("function resetZoom(){zoomLevel=100;updateZoom();}"));
	HtmlBuilder.Append(TEXT("var modalZoom=100;function openModal(svgHtml){document.getElementById('modal-diagram').innerHTML=svgHtml;document.getElementById('diagram-modal').classList.add('show');modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function closeModal(e){if(!e||e.target.id==='diagram-modal')document.getElementById('diagram-modal').classList.remove('show');}"));
	HtmlBuilder.Append(TEXT("function updateModalZoom(){var el=document.getElementById('modal-diagram');el.style.transform='scale('+modalZoom/100+')';el.style.transformOrigin='center center';document.getElementById('modal-zoom-pct').textContent=modalZoom+'%';}"));
	HtmlBuilder.Append(TEXT("function resetModalZoom(){modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function popoutDiagram(){var svgHtml=document.getElementById('modal-diagram').innerHTML;window.location.href='ue://diagram-popout?data='+encodeURIComponent(svgHtml);}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('keydown',function(e){if(e.key==='Escape')closeModal();});"));
	HtmlBuilder.Append(TEXT("</script></head><body>"));
	HtmlBuilder.Append(TEXT("<div id='diagram-modal' onclick='closeModal(event)'><div id='modal-content' onclick='event.stopPropagation()'><button id='modal-close' onclick='closeModal()'>&times;</button><div id='modal-toolbar'><span id='modal-zoom-hint'><kbd>Ctrl</kbd>+Scroll to zoom</span><span id='modal-zoom-pct'>100%</span><button id='modal-reset-btn' onclick='resetModalZoom()'>Reset</button><button id='modal-popout-btn' onclick='popoutDiagram()'>Open in Window</button></div><div id='modal-diagram'></div></div></div>"));
	HtmlBuilder.Append(TEXT("<script>document.getElementById('diagram-modal').addEventListener('wheel',function(e){if(e.ctrlKey){e.preventDefault();modalZoom+=e.deltaY>0?-10:10;modalZoom=Math.max(30,Math.min(200,modalZoom));updateModalZoom();}},{passive:false});</script>"));
	HtmlBuilder.Append(TEXT("<div id='zoom-controls'><span id='zoom-indicator'>100%</span><button id='zoom-reset' onclick='resetZoom()'>Reset</button></div><div id='zoom-content'>"));
	if (bIsProjectThinking)
	{
		HtmlBuilder.Append(TEXT("<div style='display:flex;align-items:center;gap:8px;padding:8px 0;color:#FFD700;font-size:13px;'><span style='animation:spin 1s linear infinite;'>⟳</span><span>Loading...</span></div><style>@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}</style>"));
	}
	for (const TSharedPtr<FJsonValue>& MessageValue : ProjectConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MessageObject = MessageValue->AsObject();
		FString Role, Content;
		if (MessageObject->TryGetStringField(TEXT("role"), Role))
		{
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (MessageObject->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
				{
					FString Speaker = (Role == TEXT("user")) ? TEXT("You") : TEXT("AI");
					FString DivClass = (Role == TEXT("user")) ? TEXT("user-msg") : TEXT("ai-msg");
					FString DisplayContent = Content;
					if (Role == TEXT("user"))
					{
						DisplayContent = DisplayContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
						HtmlBuilder.Appendf(TEXT("<div class='%s'><div class='speaker'>%s:</div><div>%s</div></div>"), *DivClass, *Speaker, *DisplayContent);
					}
					else
					{
						DisplayContent.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
						DisplayContent.ReplaceInline(TEXT("`"), TEXT("\\`"));
						DisplayContent.ReplaceInline(TEXT("$"), TEXT("\\$"));
						HtmlBuilder.Appendf(TEXT("<div class='%s'><div class='speaker'>%s:</div><div id='proj-msg-%d'></div><script>var el=document.getElementById('proj-msg-%d');el.innerHTML=parseMarkdown(`%s`);</script></div>"),
							*DivClass, *Speaker, ProjectConversationHistory.IndexOfByKey(MessageValue), ProjectConversationHistory.IndexOfByKey(MessageValue), *DisplayContent);
					}
				}
			}
		}
	}
	if (bIsProjectThinking)
	{
		HtmlBuilder.Append(TEXT("<div style='display:flex;align-items:center;gap:8px;padding:8px 0;color:#FFD700;font-size:13px;'><span style='animation:spin 1s linear infinite;'>⟳</span><span>Loading...</span></div>"));
	}
	HtmlBuilder.Append(TEXT("<script>window.scrollTo(0, document.body.scrollHeight);</script>"));
	HtmlBuilder.Append(TEXT("</div></body></html>"));
	FString DataUri = TEXT("data:text/html;charset=utf-8,") + FGenericPlatformHttp::UrlEncode(HtmlBuilder.ToString());
	ProjectChatBrowser->LoadURL(DataUri);
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateSettingsWidget()
{
	ThemePresetOptions.Empty();
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Golden Turquoise")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Slate Professional")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Rose Gold")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Emerald")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Cyberpunk")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Solar Flare")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Deep Ocean")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Lavender Dream")));
	ThemePresetOptions.Add(MakeShared<FString>(TEXT("Custom")));
	LanguageOptions.Empty();
	LanguageOptions.Add(MakeShared<FString>(TEXT("English")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Espa\u00f1ol")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Fran\u00e7ais")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Deutsch")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("\u4e2d\u6587")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("\u65e5\u672c\u8a9e")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("\u0420\u0443\u0441\u0441\u043a\u0438\u0439")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Portugu\u00eas")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("\ud55c\uad6d\uc5b4")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Italiano")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("\u0627\u0644\u0639\u0631\u0628\u064a\u0629")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("Nederlands")));
	LanguageOptions.Add(MakeShared<FString>(TEXT("T\u00fcrk\u00e7e")));
	CurrentlyEditingSlotIndex = FApiKeyManager::Get().GetActiveSlotIndex();
	int32 ActiveSlotIndex = FApiKeyManager::Get().GetActiveSlotIndex();
	return SNew(SBorder)
		.Padding(20)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(10, 10, 10, 15)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("settings_title"), TEXT("Settings")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
				]
				+ SVerticalBox::Slot().FillHeight(1).MaxHeight(700)
				[
					SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
								.BorderBackgroundColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.3f))
								.Padding(12)
								[
									SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
										[
											SNew(STextBlock)
												.Text(UIConfig::GetTextAttr(TEXT("language_label"), TEXT("Language:")))
												.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
												.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
										]
										+ SVerticalBox::Slot().AutoHeight()
										[
											SNew(STextComboBox)
												.OptionsSource(&LanguageOptions)
												.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnLanguageChanged)
												.InitiallySelectedItem([this]() -> TSharedPtr<FString> {
													FString CurrentLang = FUIConfigManager::Get().GetLanguage();
													FString LangName = TEXT("English");
													if (CurrentLang == TEXT("es")) LangName = TEXT("Espa\u00f1ol");
													else if (CurrentLang == TEXT("fr")) LangName = TEXT("Fran\u00e7ais");
													else if (CurrentLang == TEXT("de")) LangName = TEXT("Deutsch");
													else if (CurrentLang == TEXT("zh")) LangName = TEXT("\u4e2d\u6587");
													else if (CurrentLang == TEXT("ja")) LangName = TEXT("\u65e5\u672c\u8a9e");
													else if (CurrentLang == TEXT("ru")) LangName = TEXT("\u0420\u0443\u0441\u0441\u043a\u0438\u0439");
													else if (CurrentLang == TEXT("pt")) LangName = TEXT("Portugu\u00eas");
													else if (CurrentLang == TEXT("ko")) LangName = TEXT("\ud55c\uad6d\uc5b4");
													else if (CurrentLang == TEXT("it")) LangName = TEXT("Italiano");
													else if (CurrentLang == TEXT("ar")) LangName = TEXT("\u0627\u0644\u0639\u0631\u0628\u064a\u0629");
													else if (CurrentLang == TEXT("nl")) LangName = TEXT("Nederlands");
													else if (CurrentLang == TEXT("tr")) LangName = TEXT("T\u00fcrk\u00e7e");
													for (const auto& Opt : LanguageOptions)
													{
														if (*Opt == LangName) return Opt;
													}
													return LanguageOptions[0];
												}())
										]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
										[
											SNew(STextBlock)
												.Text(UIConfig::GetTextAttr(TEXT("language_note"), TEXT("Language changes apply immediately to all UI text.")))
												.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
												.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
										]
								]
						]
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(5)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
										.BorderBackgroundColor(CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.3f))
										.Padding(12)
										[
											SNew(SVerticalBox)
												+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
												[
													SNew(STextBlock)
														.Text(UIConfig::GetTextAttr(TEXT("quick_switch_title"), TEXT("API Key Slots (ALT+1-9 to switch slots)")))
														.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
														.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
												]
												+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
												[
													SNew(STextBlock)
														.Text(FText::FromString(FString::Printf(TEXT("Each slot stores a different API provider and key. Switch between slots instantly using keyboard shortcuts 1-9!"))))
														.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
														.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
														.AutoWrapText(true)
												]
												+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
												[
													SAssignNew(ActiveSlotTextBlock, STextBlock)
														.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Active Slot: %d"), FApiKeyManager::Get().GetActiveSlotIndex() + 1)); })
														.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
												]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[0], SButton)
												.Text(FText::FromString(TEXT("1")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((0 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 0)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[1], SButton)
												.Text(FText::FromString(TEXT("2")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((1 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 1)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[2], SButton)
												.Text(FText::FromString(TEXT("3")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((2 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 2)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[3], SButton)
												.Text(FText::FromString(TEXT("4")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((3 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 3)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[4], SButton)
												.Text(FText::FromString(TEXT("5")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((4 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 4)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[5], SButton)
												.Text(FText::FromString(TEXT("6")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((5 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 5)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[6], SButton)
												.Text(FText::FromString(TEXT("7")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((6 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 6)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[7], SButton)
												.Text(FText::FromString(TEXT("8")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((7 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 7)
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)
										[
											SAssignNew(SlotButtons[8], SButton)
												.Text(FText::FromString(TEXT("9")))
												.ButtonColorAndOpacity_Lambda([this]() { int32 ActiveIdx = FApiKeyManager::Get().GetActiveSlotIndex(); return FSlateColor((8 == ActiveIdx) ? CurrentTheme.PrimaryColor.CopyWithNewOpacity(0.6f) : CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)); })
												.ForegroundColor(FSlateColor(FLinearColor::White))
												.OnClicked(this, &SBpGeneratorUltimateWidget::OnSwitchToSlotClicked, 8)
										]
								]
						]
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SAssignNew(SettingsContentBox, SBox)
					[
						CreateSlotConfigSection()
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(5)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderBackgroundColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.3f))
						.Padding(12)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("theme_section_label"), TEXT("Appearance")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("theme_preset_label"), TEXT("Theme Preset:")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextComboBox)
										.OptionsSource(&ThemePresetOptions)
										.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnThemePresetChanged)
										.InitiallySelectedItem(ThemePresetOptions.IsValidIndex((int32)CurrentTheme.Preset) ? ThemePresetOptions[(int32)CurrentTheme.Preset] : ThemePresetOptions[0])
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("theme_restart_note"), TEXT("Note: Theme changes apply after re-opening the plugin's window.")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
										.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
								]
						]
				]
						]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(15, 10, 15, 5)
				[
					SNew(SButton)
						.Text(UIConfig::GetTextAttr(TEXT("save_button"), TEXT("Save and Close")))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnSaveSettingsClicked)
						.ButtonColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
						.ForegroundColor(FSlateColor(FLinearColor::White))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(5, 5, 5, 10)
				[
					SNew(SButton)
						.Text(UIConfig::GetTextAttr(TEXT("forum_button"), TEXT("Join Exclusive Forum")))
						.OnClicked_Lambda([]() {
							FPlatformProcess::LaunchURL(*UIConfig::GetURL(TEXT("forum_url"), TEXT("https://www.gamedevcore.com/community")), nullptr, nullptr);
							return FReply::Handled();
						})
						.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.7f)))
						.ForegroundColor(FSlateColor(FLinearColor::White))
				]
		];
}
void SBpGeneratorUltimateWidget::LoadSettings()
{
	FString ProviderString;
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("Provider"), ProviderString, GEditorPerProjectIni);
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("ApiKey"), CurrentSettings.ApiKey, GEditorPerProjectIni);
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("CustomBaseURL"), CurrentSettings.CustomBaseURL, GEditorPerProjectIni);
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("CustomModelName"), CurrentSettings.CustomModelName, GEditorPerProjectIni);
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("CustomInstructions"), CustomInstructions, GEditorPerProjectIni);
	if (ProviderString == TEXT("OpenAI"))
	{
		CurrentSettings.Provider = EApiProvider::OpenAI;
	}
	else if (ProviderString == TEXT("Claude"))
	{
		CurrentSettings.Provider = EApiProvider::Claude;
	}
	else if (ProviderString == TEXT("DeepSeek"))
	{
		CurrentSettings.Provider = EApiProvider::DeepSeek;
	}
	else if (ProviderString == TEXT("Custom"))
	{
		CurrentSettings.Provider = EApiProvider::Custom;
	}
	else
	{
		CurrentSettings.Provider = EApiProvider::Gemini;
	}
}
void SBpGeneratorUltimateWidget::SaveSettings()
{
	FString ProviderString;
	switch (CurrentSettings.Provider)
	{
	case EApiProvider::OpenAI: ProviderString = TEXT("OpenAI"); break;
	case EApiProvider::Claude: ProviderString = TEXT("Claude"); break;
	case EApiProvider::DeepSeek: ProviderString = TEXT("DeepSeek"); break;
	case EApiProvider::Custom: ProviderString = TEXT("Custom"); break;
	case EApiProvider::Gemini:
	default: ProviderString = TEXT("Gemini"); break;
	}
	GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("Provider"), *ProviderString, GEditorPerProjectIni);
	GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("ApiKey"), *CurrentSettings.ApiKey, GEditorPerProjectIni);
	GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("CustomBaseURL"), *CurrentSettings.CustomBaseURL, GEditorPerProjectIni);
	GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("CustomModelName"), *CurrentSettings.CustomModelName, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}
FReply SBpGeneratorUltimateWidget::OnAnalyzePluginClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}
	FString EngineDir = FPaths::EngineDir();
	FString MarketplacePath = EngineDir / TEXT("Plugins") / TEXT("Marketplace");
	if (!IFileManager::Get().DirectoryExists(*MarketplacePath))
	{
		MarketplacePath = EngineDir;
	}
	FString SelectedFolder;
	bool bOpened = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Select Plugin Folder to Analyze"),
		MarketplacePath,
		SelectedFolder
	);
	if (bOpened && !SelectedFolder.IsEmpty())
	{
		AnalyzePluginSource(SelectedFolder);
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::AnalyzePluginSource(const FString& PluginPath)
{
	FString SourcePath = PluginPath / TEXT("Source");
	if (!IFileManager::Get().DirectoryExists(*SourcePath))
	{
		if (PluginPath.EndsWith(TEXT("Source")) || PluginPath.Contains(TEXT("Source")))
		{
			SourcePath = PluginPath;
		}
		else
		{
			TArray<FString> FoundDirs;
			IFileManager::Get().FindFilesRecursive(FoundDirs, *PluginPath, TEXT("Source"), true, true, false);
			if (FoundDirs.Num() > 0)
			{
				SourcePath = FoundDirs[0];
			}
			else
			{
				TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
				UserMsg->SetStringField(TEXT("role"), TEXT("user"));
				TArray<TSharedPtr<FJsonValue>> UserParts;
				TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
				UserPartText->SetStringField(TEXT("text"),
					FString::Printf(TEXT("I want to analyze the plugin at: %s\n\nThis plugin doesn't appear to have a Source folder with C++ code. It may be a Blueprint-only plugin or the Source folder is in a different location."),
					*PluginPath));
				UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
				UserMsg->SetArrayField(TEXT("parts"), UserParts);
				ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserMsg)));
				RefreshArchitectChatView();
				return;
			}
		}
	}
	TArray<FString> HeaderFiles;
	TArray<FString> CppFiles;
	IFileManager::Get().FindFilesRecursive(HeaderFiles, *SourcePath, TEXT("*.h"), true, false);
	IFileManager::Get().FindFilesRecursive(CppFiles, *SourcePath, TEXT("*.cpp"), true, false);
	FString PluginName = FPaths::GetCleanFilename(PluginPath);
	FString FileInfo;
	FileInfo += FString::Printf(TEXT("Plugin Folder: %s\n"), *PluginName);
	FileInfo += FString::Printf(TEXT("Location: %s\n"), *PluginPath);
	FileInfo += FString::Printf(TEXT("Source: %s\n\n"), *SourcePath);
	FileInfo += FString::Printf(TEXT("=== FILES (%d headers, %d sources) ===\n"), HeaderFiles.Num(), CppFiles.Num());
	for (const FString& File : HeaderFiles)
	{
		FString RelativePath = File.Mid(SourcePath.Len() + 1);
		FileInfo += FString::Printf(TEXT("[H] %s\n"), *RelativePath);
	}
	for (const FString& File : CppFiles)
	{
		FString RelativePath = File.Mid(SourcePath.Len() + 1);
		FileInfo += FString::Printf(TEXT("[C] %s\n"), *RelativePath);
	}
	FString ContentPath = PluginPath / TEXT("Content");
	TArray<FString> UassetFiles;
	IFileManager::Get().FindFilesRecursive(UassetFiles, *ContentPath, TEXT("*.uasset"), true, false);
	bool bPluginEnabled = false;
	FString PluginVirtualPath;
	FString PluginShortName;
	FString PluginFriendlyName;
	TArray<FString> UpluginFiles;
	IFileManager::Get().FindFiles(UpluginFiles, *PluginPath, TEXT("*.uplugin"));
	if (UpluginFiles.Num() > 0)
	{
		PluginShortName = FPaths::GetBaseFilename(UpluginFiles[0]);
		FString UpluginPath = FPaths::Combine(PluginPath, UpluginFiles[0]);
		FString UpluginContent;
		if (FFileHelper::LoadFileToString(UpluginContent, *UpluginPath))
		{
			TSharedPtr<FJsonObject> UpluginJson;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UpluginContent);
			if (FJsonSerializer::Deserialize(Reader, UpluginJson) && UpluginJson.IsValid())
			{
				UpluginJson->TryGetStringField(TEXT("FriendlyName"), PluginFriendlyName);
			}
		}
		if (PluginFriendlyName.IsEmpty())
		{
			PluginFriendlyName = PluginShortName;
		}
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginShortName);
		if (Plugin.IsValid())
		{
			bPluginEnabled = Plugin->IsEnabled();
			PluginVirtualPath = FString::Printf(TEXT("/%s"), *PluginShortName);
		}
	}
	else
	{
		PluginFriendlyName = PluginName;
		PluginShortName = PluginName;
	}
	FileInfo += FString::Printf(TEXT("\n=== PLUGIN INFO ===\n"));
	FileInfo += FString::Printf(TEXT("Friendly Name: %s\n"), *PluginFriendlyName);
	FileInfo += FString::Printf(TEXT("Internal Name: %s\n"), *PluginShortName);
	FileInfo += FString::Printf(TEXT("Enabled: %s\n"), bPluginEnabled ? TEXT("Yes") : TEXT("No"));
	if (bPluginEnabled && !PluginVirtualPath.IsEmpty())
	{
		FileInfo += FString::Printf(TEXT("Virtual Path: %s\n"), *PluginVirtualPath);
		FileInfo += TEXT("PATH FORMAT: For UE virtual paths, the 'Content' folder is MAPPED to the plugin root.\n");
		FileInfo += TEXT("Example: Content/Demo/Abilities/BP.uasset becomes /PluginName/Demo/Abilities/BP.BP\n");
		FileInfo += TEXT("DO NOT include 'Content' in the virtual path!\n");
	}
	if (UassetFiles.Num() > 0)
	{
		FileInfo += FString::Printf(TEXT("\n=== BLUEPRINTS (%d files) ===\n"), UassetFiles.Num());
		if (bPluginEnabled)
		{
			FileInfo += FString::Printf(TEXT("STATUS: Plugin is ENABLED - Use list_assets_in_folder with path \"%s\" to find blueprints\n"), *PluginVirtualPath);
			FileInfo += TEXT("To load a blueprint, convert the disk path to virtual path:\n");
		}
		else
		{
			FileInfo += TEXT("STATUS: Plugin is NOT enabled - Blueprints cannot be analyzed (binary .uasset files)\n");
		}
		for (const FString& File : UassetFiles)
		{
			FString RelativePath = File.Mid(PluginPath.Len() + 1);
			FString VirtualPath;
			if (bPluginEnabled && RelativePath.StartsWith(TEXT("Content/")))
			{
				VirtualPath = PluginVirtualPath + TEXT("/") + RelativePath.Mid(8);
				VirtualPath.RemoveFromEnd(TEXT(".uasset"));
				FString BpName = FPaths::GetBaseFilename(VirtualPath);
				VirtualPath = VirtualPath + TEXT(".") + BpName;
			}
			FileInfo += FString::Printf(TEXT("[BP] %s -> %s\n"), *RelativePath, *VirtualPath);
		}
	}
	if (ArchitectConversationHistory.Num() <= 1)
	{
		FString NewTitle = FString::Printf(TEXT("Plugin: %s"), *PluginFriendlyName);
		if (NewTitle.Len() > 40)
		{
			NewTitle = NewTitle.Left(37) + TEXT("...");
		}
		TSharedPtr<FConversationInfo>* FoundInfo = ArchitectConversationList.FindByPredicate(
			[&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ActiveArchitectChatID; });
		if (FoundInfo)
		{
			(*FoundInfo)->Title = NewTitle;
			SaveArchitectManifest();
			if (ArchitectChatListView.IsValid())
			{
				ArchitectChatListView->RequestListRefresh();
			}
		}
	}
	TSharedPtr<FJsonObject> ContextMsg = MakeShareable(new FJsonObject);
	ContextMsg->SetStringField(TEXT("role"), TEXT("context"));
	ContextMsg->SetStringField(TEXT("content"), FileInfo);
	ContextMsg->SetStringField(TEXT("file_name"), FString::Printf(TEXT("Plugin: %s (%d C++, %d BP)"), *PluginFriendlyName, HeaderFiles.Num() + CppFiles.Num(), UassetFiles.Num()));
	ContextMsg->SetNumberField(TEXT("char_count"), FileInfo.Len());
	ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(ContextMsg)));
	PendingArchitectContext.Empty();
	SaveArchitectChatHistory(ActiveArchitectChatID);
	RefreshArchitectChatView();
	FString Msg = UIConfig::GetValue(TEXT("msg_plugin_context_attached"), TEXT("Plugin '{0}' files attached ({1} C++ files, {2} blueprints). What would you like to know?"));
	Msg.ReplaceInline(TEXT("{0}"), *PluginFriendlyName);
	Msg.ReplaceInline(TEXT("{1}"), *FString::FromInt(HeaderFiles.Num() + CppFiles.Num()));
	Msg.ReplaceInline(TEXT("{2}"), *FString::FromInt(UassetFiles.Num()));
	ArchitectInputTextBox->SetText(FText::FromString(Msg));
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateSlotConfigSection()
{
	if (CurrentlyEditingSlotIndex < 0 || CurrentlyEditingSlotIndex >= MAX_API_KEY_SLOTS)
	{
		CurrentlyEditingSlotIndex = FApiKeyManager::Get().GetActiveSlotIndex();
	}
	FApiKeySlot Slot = FApiKeyManager::Get().GetSlot(CurrentlyEditingSlotIndex);
	bool bIsCustomProvider = Slot.IsCustomProvider();
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
		.Padding(12)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("config_title"), TEXT("Slot Configuration")))
						.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
				[
					SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("Editing Slot %d - ALT+1-9 to switch"), CurrentlyEditingSlotIndex + 1)))
						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("config_instructions"), TEXT("Each slot stores a different API provider and key. Switch instantly using keyboard 1-9!")))
						.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
						.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
						.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("slot_name_label"), TEXT("Slot Name (optional):")))
						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].NameInput, SEditableTextBox)
						.Text(FText::FromString(Slot.Name))
						.HintText(UIConfig::GetText(TEXT("slot_name_hint"), TEXT("My Grok Key")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("slot_provider_label"), TEXT("Provider:")))
						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox, STextComboBox)
						.OptionsSource(&SlotProviderOptions)
						.InitiallySelectedItem([this, Slot]() -> TSharedPtr<FString> {
							for (const TSharedPtr<FString>& Option : SlotProviderOptions)
							{
								if (Option.IsValid() && *Option == Slot.Provider)
								{
									return Option;
								}
							}
							return SlotProviderOptions[0];
						}())
						.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnSlotProviderChanged, CurrentlyEditingSlotIndex)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 5)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("slot_api_key_label"), TEXT("API Key:")))
						.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].ApiKeyInput, SEditableTextBox)
						.Text(FText::FromString(Slot.ApiKey))
						.HintText(UIConfig::GetText(TEXT("slot_api_key_hint"), TEXT("Paste API key for this slot")))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].CustomFieldsWrapper, SBox)
						.Visibility(bIsCustomProvider ? EVisibility::Visible : EVisibility::Collapsed)
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
							[
								SNew(STextBlock)
									.Text(UIConfig::GetTextAttr(TEXT("slot_custom_url_label"), TEXT("Custom Base URL:")))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].CustomBaseURLInput, SEditableTextBox)
									.Text(FText::FromString(Slot.CustomBaseURL))
									.HintText(UIConfig::GetText(TEXT("slot_custom_url_hint"), TEXT("https://api.example.com/v1/chat/completions")))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
							[
								SNew(STextBlock)
									.Text(UIConfig::GetTextAttr(TEXT("slot_custom_model_label"), TEXT("Custom Model Name:")))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].CustomModelNameInput, SEditableTextBox)
									.Text(FText::FromString(Slot.CustomModelName))
									.HintText(UIConfig::GetText(TEXT("slot_custom_model_hint"), TEXT("model-name")))
							]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].GeminiModelWrapper, SBox)
						.Visibility(TAttribute<EVisibility>::Create([this]() -> EVisibility {
							if (SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox.IsValid() &&
								SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem().IsValid() &&
								*SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem() == TEXT("Gemini"))
							{
								return EVisibility::Visible;
							}
							return EVisibility::Collapsed;
						}))
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
							[
								SNew(STextBlock)
									.Text(UIConfig::GetTextAttr(TEXT("gemini_model_label"), TEXT("Gemini Model:")))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									.ToolTipText(UIConfig::GetText(TEXT("gemini_model_tooltip"), TEXT("Select which Gemini model to use")))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].GeminiModelComboBox, STextComboBox)
									.OptionsSource(&GeminiModelOptions)
									.InitiallySelectedItem([this, Slot]() -> TSharedPtr<FString> {
										for (const TSharedPtr<FString>& Option : GeminiModelOptions)
										{
											if (Option.IsValid() && *Option == Slot.GeminiModel)
											{
												return Option;
											}
										}
										return GeminiModelOptions[0];
									}())
							]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].OpenAIModelWrapper, SBox)
						.Visibility(TAttribute<EVisibility>::Create([this]() -> EVisibility {
							if (SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox.IsValid() &&
								SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem().IsValid() &&
								*SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem() == TEXT("OpenAI"))
							{
								return EVisibility::Visible;
							}
							return EVisibility::Collapsed;
						}))
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
							[
								SNew(STextBlock)
									.Text(UIConfig::GetTextAttr(TEXT("openai_model_label"), TEXT("OpenAI Model:")))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									.ToolTipText(UIConfig::GetText(TEXT("openai_model_tooltip"), TEXT("Select which OpenAI model to use")))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].OpenAIModelComboBox, STextComboBox)
									.OptionsSource(&OpenAIModelOptions)
									.InitiallySelectedItem([this, Slot]() -> TSharedPtr<FString> {
										for (const TSharedPtr<FString>& Option : OpenAIModelOptions)
										{
											if (Option.IsValid() && *Option == Slot.OpenAIModel)
											{
												return Option;
											}
										}
										return OpenAIModelOptions[2];
									}())
							]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].ClaudeModelWrapper, SBox)
						.Visibility(TAttribute<EVisibility>::Create([this]() -> EVisibility {
							if (SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox.IsValid() &&
								SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem().IsValid() &&
								*SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem() == TEXT("Claude"))
							{
								return EVisibility::Visible;
							}
							return EVisibility::Collapsed;
						}))
					[
						SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 5)
							[
								SNew(STextBlock)
									.Text(UIConfig::GetTextAttr(TEXT("claude_model_label"), TEXT("Claude Model:")))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									.ToolTipText(UIConfig::GetText(TEXT("claude_model_tooltip"), TEXT("Select which Claude model to use")))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].ClaudeModelComboBox, STextComboBox)
									.OptionsSource(&ClaudeModelOptions)
									.InitiallySelectedItem([this, Slot]() -> TSharedPtr<FString> {
										for (const TSharedPtr<FString>& Option : ClaudeModelOptions)
										{
											if (Option.IsValid() && *Option == Slot.ClaudeModel)
											{
												return Option;
											}
										}
										return ClaudeModelOptions[1];
									}())
							]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 0)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f))
						.Padding(10)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("texture_gen_settings_title"), TEXT("Texture Generation Settings")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 5)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("texture_gen_endpoint_label"), TEXT("Image Gen Endpoint:")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
										.ToolTipText(UIConfig::GetText(TEXT("texture_gen_endpoint_tooltip"), TEXT("API endpoint for image/texture generation")))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].TextureGenEndpointInput, SEditableTextBox)
										.Text(FText::FromString(Slot.TextureGenEndpoint))
										.HintText(UIConfig::GetText(TEXT("texture_gen_endpoint_hint"), TEXT("https://generativelanguage.googleapis.com/v1beta/models/imagen-4.0-generate-001:predict")))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 5)
								[
									SNew(STextBlock)
										.Text(UIConfig::GetTextAttr(TEXT("texture_gen_model_label"), TEXT("Image Gen Model:")))
										.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
										.ToolTipText(UIConfig::GetText(TEXT("texture_gen_model_tooltip"), TEXT("Model identifier for image generation")))
								]
								+ SVerticalBox::Slot().AutoHeight()
								[
									SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModelInput, SEditableTextBox)
										.Text(FText::FromString(Slot.TextureGenModel))
										.HintText(UIConfig::GetText(TEXT("texture_gen_model_hint"), TEXT("imagen-4.0-generate-001")))
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(1.0)
										[
											SNew(STextBlock)
												.Text(UIConfig::GetTextAttr(TEXT("texture_gen_mode_label"), TEXT("Texture Mode")))
												.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
												.ToolTipText(UIConfig::GetText(TEXT("texture_gen_mode_tooltip"), TEXT("When enabled, generates textures for materials. When disabled, generates general images.")))
										]
										+ SHorizontalBox::Slot().AutoWidth()
										[
											SAssignNew(SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModeCheckBox, SCheckBox)
												.IsChecked(Slot.bTextureMode)
												.ToolTipText(UIConfig::GetText(TEXT("texture_gen_mode_checkbox_tooltip"), TEXT("Enable for textures, disable for general images")))
										]
								]
						]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 0)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
								.Text(UIConfig::GetTextAttr(TEXT("save_slot_button"), TEXT("Save Slot")))
								.OnClicked(this, &SBpGeneratorUltimateWidget::OnSaveSlotClicked, CurrentlyEditingSlotIndex)
								.ButtonColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
								.ForegroundColor(FSlateColor(FLinearColor::White))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
						[
							SNew(SButton)
								.Text(UIConfig::GetTextAttr(TEXT("clear_slot_button"), TEXT("Clear Slot")))
								.OnClicked(this, &SBpGeneratorUltimateWidget::OnClearSlotClicked, CurrentlyEditingSlotIndex)
								.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.5f)))
								.ForegroundColor(FSlateColor(FLinearColor::White))
						]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("slot_save_reminder"), TEXT("Don't forget: Click 'Save Slot' first, then 'Save and Close' at the bottom!")))
						.ColorAndOpacity(FSlateColor(FMath::Lerp(FLinearColor::Yellow, FLinearColor(1.0f, 0.5f, 0.0f), 0.5f)))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.AutoWrapText(true)
				]
		];
}
FReply SBpGeneratorUltimateWidget::OnSwitchToSlotClicked(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return FReply::Handled();
	FApiKeyManager::Get().SetActiveSlot(SlotIndex);
	CurrentlyEditingSlotIndex = SlotIndex;
	if (MainSwitcher.IsValid() && MainSwitcher->GetActiveWidgetIndex() == 1 && EntireSettingsBox.IsValid())
	{
		EntireSettingsBox->SetContent(CreateSettingsWidget());
	}
	FApiKeySlot Slot = FApiKeyManager::Get().GetSlot(SlotIndex);
	FString DisplayName = Slot.GetDisplayName();
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Switched to: %s"), *DisplayName)));
	Info.ExpireDuration = 1.5f;
	FSlateNotificationManager::Get().AddNotification(Info);
	UpdateSlotIndicators();
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnSaveSlotClicked(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return FReply::Handled();
	if (!SlotWidgets[SlotIndex].ApiKeyInput.IsValid() || !SlotWidgets[SlotIndex].ProviderComboBox.IsValid()) return FReply::Handled();
	FApiKeySlot NewSlot;
	NewSlot.SlotIndex = SlotIndex;
	if (SlotWidgets[SlotIndex].NameInput.IsValid())
	{
		NewSlot.Name = SlotWidgets[SlotIndex].NameInput->GetText().ToString();
	}
	NewSlot.ApiKey = SlotWidgets[SlotIndex].ApiKeyInput->GetText().ToString();
	NewSlot.Provider = SlotWidgets[SlotIndex].ProviderComboBox->GetSelectedItem().IsValid() ? *SlotWidgets[SlotIndex].ProviderComboBox->GetSelectedItem() : TEXT("Gemini");
	if (SlotWidgets[SlotIndex].CustomBaseURLInput.IsValid())
	{
		NewSlot.CustomBaseURL = SlotWidgets[SlotIndex].CustomBaseURLInput->GetText().ToString();
	}
	if (SlotWidgets[SlotIndex].CustomModelNameInput.IsValid())
	{
		NewSlot.CustomModelName = SlotWidgets[SlotIndex].CustomModelNameInput->GetText().ToString();
	}
	if (SlotWidgets[SlotIndex].GeminiModelComboBox.IsValid() && SlotWidgets[SlotIndex].GeminiModelComboBox->GetSelectedItem().IsValid())
	{
		NewSlot.GeminiModel = *SlotWidgets[SlotIndex].GeminiModelComboBox->GetSelectedItem();
	}
	if (SlotWidgets[SlotIndex].OpenAIModelComboBox.IsValid() && SlotWidgets[SlotIndex].OpenAIModelComboBox->GetSelectedItem().IsValid())
	{
		NewSlot.OpenAIModel = *SlotWidgets[SlotIndex].OpenAIModelComboBox->GetSelectedItem();
	}
	if (SlotWidgets[SlotIndex].ClaudeModelComboBox.IsValid() && SlotWidgets[SlotIndex].ClaudeModelComboBox->GetSelectedItem().IsValid())
	{
		NewSlot.ClaudeModel = *SlotWidgets[SlotIndex].ClaudeModelComboBox->GetSelectedItem();
	}
	if (SlotWidgets[SlotIndex].TextureGenEndpointInput.IsValid())
	{
		NewSlot.TextureGenEndpoint = SlotWidgets[SlotIndex].TextureGenEndpointInput->GetText().ToString();
	}
	if (SlotWidgets[SlotIndex].TextureGenModelInput.IsValid())
	{
		NewSlot.TextureGenModel = SlotWidgets[SlotIndex].TextureGenModelInput->GetText().ToString();
	}
	if (SlotWidgets[SlotIndex].TextureGenModeCheckBox.IsValid())
	{
		NewSlot.bTextureMode = SlotWidgets[SlotIndex].TextureGenModeCheckBox->IsChecked();
	}
	FApiKeyManager::Get().SetSlot(SlotIndex, NewSlot);
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Slot %d saved!"), SlotIndex + 1)));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnClearSlotClicked(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return FReply::Handled();
	FApiKeyManager::Get().ClearSlot(SlotIndex);
	if (SlotWidgets[SlotIndex].NameInput.IsValid())
	{
		SlotWidgets[SlotIndex].NameInput->SetText(FText::GetEmpty());
	}
	if (SlotWidgets[SlotIndex].ApiKeyInput.IsValid())
	{
		SlotWidgets[SlotIndex].ApiKeyInput->SetText(FText::GetEmpty());
	}
	if (SlotWidgets[SlotIndex].CustomBaseURLInput.IsValid())
	{
		SlotWidgets[SlotIndex].CustomBaseURLInput->SetText(FText::GetEmpty());
	}
	if (SlotWidgets[SlotIndex].CustomModelNameInput.IsValid())
	{
		SlotWidgets[SlotIndex].CustomModelNameInput->SetText(FText::GetEmpty());
	}
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Slot %d cleared!"), SlotIndex + 1)));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::OnSlotProviderChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo, int32 SlotIndex)
{
	if (!NewSelection.IsValid() || SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return;
	bool bIsCustomProvider = (*NewSelection == TEXT("Custom"));
	if (SlotWidgets[SlotIndex].CustomFieldsWrapper.IsValid())
	{
		SlotWidgets[SlotIndex].CustomFieldsWrapper->SetVisibility(bIsCustomProvider ? EVisibility::Visible : EVisibility::Collapsed);
	}
	bool bIsGemini = (*NewSelection == TEXT("Gemini"));
	if (SlotWidgets[SlotIndex].GeminiModelWrapper.IsValid())
	{
		SlotWidgets[SlotIndex].GeminiModelWrapper->SetVisibility(bIsGemini ? EVisibility::Visible : EVisibility::Collapsed);
	}
}
void SBpGeneratorUltimateWidget::RefreshSlotUI()
{
}
void SBpGeneratorUltimateWidget::UpdateSlotIndicators()
{
}
void SBpGeneratorUltimateWidget::RebuildSettingsWidget()
{
}
bool SBpGeneratorUltimateWidget::HandleGlobalKeyPress(const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsLeftAltDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsControlDown())
	{
		FKey NumberKey = InKeyEvent.GetKey();
		FName KeyName = NumberKey.GetFName();
		if (KeyName == TEXT("One")) { OnSwitchToSlotClicked(0); return true; }
		if (KeyName == TEXT("Two")) { OnSwitchToSlotClicked(1); return true; }
		if (KeyName == TEXT("Three")) { OnSwitchToSlotClicked(2); return true; }
		if (KeyName == TEXT("Four")) { OnSwitchToSlotClicked(3); return true; }
		if (KeyName == TEXT("Five")) { OnSwitchToSlotClicked(4); return true; }
		if (KeyName == TEXT("Six")) { OnSwitchToSlotClicked(5); return true; }
		if (KeyName == TEXT("Seven")) { OnSwitchToSlotClicked(6); return true; }
		if (KeyName == TEXT("Eight")) { OnSwitchToSlotClicked(7); return true; }
		if (KeyName == TEXT("Nine")) { OnSwitchToSlotClicked(8); return true; }
	}
	return false;
}
FReply SBpGeneratorUltimateWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGlobalKeyPress(InKeyEvent))
	{
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
#if 0
void SBpGeneratorUltimateWidget::OnProfileChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingProfileComboBox || !NewSelection.IsValid())
	{
		return;
	}
	bUpdatingProfileComboBox = true;
	FApiKeyManager& Manager = FApiKeyManager::Get();
	Manager.SetActiveProfile(*NewSelection);
	FApiKeyProfile ActiveProfile = Manager.GetActiveProfile();
	CurrentSettings.ApiKey = ActiveProfile.ApiKey;
	if (ApiKeyInput.IsValid())
	{
		ApiKeyInput->SetText(FText::FromString(ActiveProfile.ApiKey));
	}
	FString ProviderStr = ActiveProfile.Provider;
	if (ProviderStr == TEXT("OpenAI")) CurrentSettings.Provider = EApiProvider::OpenAI;
	else if (ProviderStr == TEXT("Claude")) CurrentSettings.Provider = EApiProvider::Claude;
	else if (ProviderStr == TEXT("DeepSeek")) CurrentSettings.Provider = EApiProvider::DeepSeek;
	else if (ProviderStr == TEXT("Custom")) CurrentSettings.Provider = EApiProvider::Custom;
	else CurrentSettings.Provider = EApiProvider::Gemini;
	if (ProviderComboBox.IsValid())
	{
		int32 ProviderIndex = 0;
		if (CurrentSettings.Provider == EApiProvider::Gemini) ProviderIndex = 0;
		else if (CurrentSettings.Provider == EApiProvider::OpenAI) ProviderIndex = 1;
		else if (CurrentSettings.Provider == EApiProvider::Claude) ProviderIndex = 2;
		else if (CurrentSettings.Provider == EApiProvider::DeepSeek) ProviderIndex = 3;
		else if (CurrentSettings.Provider == EApiProvider::Custom) ProviderIndex = 4;
		if (ProviderOptions.IsValidIndex(ProviderIndex))
		{
			ProviderComboBox->SetSelectedItem(ProviderOptions[ProviderIndex]);
		}
	}
	bUpdatingProfileComboBox = false;
}
FReply SBpGeneratorUltimateWidget::OnSaveProfileClicked()
{
	if (ApiKeyInput.IsValid())
	{
		FString ProfileName = TEXT("Default");
		if (ProfileComboBox.IsValid() && ProfileComboBox->GetSelectedItem().IsValid())
		{
			ProfileName = *ProfileComboBox->GetSelectedItem();
		}
		FApiKeyManager& Manager = FApiKeyManager::Get();
		FString ProviderString;
		switch (CurrentSettings.Provider)
		{
		case EApiProvider::OpenAI: ProviderString = TEXT("OpenAI"); break;
		case EApiProvider::Claude: ProviderString = TEXT("Claude"); break;
		case EApiProvider::DeepSeek: ProviderString = TEXT("DeepSeek"); break;
		case EApiProvider::Custom: ProviderString = TEXT("Custom"); break;
		case EApiProvider::Gemini:
		default: ProviderString = TEXT("Gemini"); break;
		}
		FApiKeyProfile NewProfile(ProfileName, ProviderString, CurrentSettings.ApiKey);
		Manager.SaveProfile(NewProfile);
		FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Profile '%s' saved!"), *ProfileName)));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnNewProfileClicked()
{
	FString NewProfileName = FString::Printf(TEXT("Profile_%d"), FDateTime::Now().GetMillisecond());
	FApiKeyManager& Manager = FApiKeyManager::Get();
	FString ProviderString;
	switch (CurrentSettings.Provider)
	{
	case EApiProvider::OpenAI: ProviderString = TEXT("OpenAI"); break;
	case EApiProvider::Claude: ProviderString = TEXT("Claude"); break;
	case EApiProvider::DeepSeek: ProviderString = TEXT("DeepSeek"); break;
	case EApiProvider::Custom: ProviderString = TEXT("Custom"); break;
	case EApiProvider::Gemini:
	default: ProviderString = TEXT("Gemini"); break;
	}
	FApiKeyProfile NewProfile(NewProfileName, ProviderString, CurrentSettings.ApiKey);
	Manager.SaveProfile(NewProfile);
	Manager.SetActiveProfile(NewProfileName);
	if (ProfileComboBox.IsValid())
	{
		for (int32 i = 0; i < ProfileOptions.Num(); i++)
		{
			if (*ProfileOptions[i] == NewProfileName)
			{
				ProfileComboBox->SetSelectedItem(ProfileOptions[i]);
				break;
			}
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnDeleteProfileClicked()
{
	if (ProfileComboBox.IsValid() && ProfileComboBox->GetSelectedItem().IsValid())
	{
		FString ProfileName = *ProfileComboBox->GetSelectedItem();
		if (ProfileName == TEXT("Default"))
		{
			FNotificationInfo Info(FText::FromString(TEXT("Cannot delete the Default profile.")));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return FReply::Handled();
		}
		FApiKeyManager& Manager = FApiKeyManager::Get();
		Manager.DeleteProfile(ProfileName);
		FApiKeyProfile ActiveProfile = Manager.GetActiveProfile();
		CurrentSettings.ApiKey = ActiveProfile.ApiKey;
		if (ApiKeyInput.IsValid())
		{
			ApiKeyInput->SetText(FText::FromString(ActiveProfile.ApiKey));
		}
		FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Profile '%s' deleted!"), *ProfileName)));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::RefreshProfileList()
{
	if (!ProfileComboBox.IsValid()) return;
	FApiKeyManager& Manager = FApiKeyManager::Get();
	TArray<FApiKeyProfile> Profiles = Manager.GetProfiles();
	ProfileOptions.Empty();
	for (const FApiKeyProfile& Profile : Profiles)
	{
		TSharedPtr<FString> Option = MakeShared<FString>(Profile.ProfileName);
		ProfileOptions.Add(Option);
	}
}
void SBpGeneratorUltimateWidget::UpdateProfileUI()
{
	bUpdatingProfileComboBox = true;
	FApiKeyManager& Manager = FApiKeyManager::Get();
	FApiKeyProfile ActiveProfile = Manager.GetActiveProfile();
	if (ApiKeyInput.IsValid())
	{
		ApiKeyInput->SetText(FText::FromString(ActiveProfile.ApiKey));
	}
	if (ProfileComboBox.IsValid() && ProfileOptions.Num() > 0)
	{
		for (int32 i = 0; i < ProfileOptions.Num(); i++)
		{
			if (ProfileOptions[i].IsValid() && *ProfileOptions[i] == ActiveProfile.ProfileName)
			{
				ProfileComboBox->SetSelectedItem(ProfileOptions[i]);
				break;
			}
		}
	}
	bUpdatingProfileComboBox = false;
}
#endif
void SBpGeneratorUltimateWidget::LoadThemeSettings()
{
	int32 PresetInt = 0;
	GConfig->GetInt(TEXT("BpGeneratorUltimate"), TEXT("ThemePreset"), PresetInt, GEditorPerProjectIni);
	EThemePreset Preset = static_cast<EThemePreset>(PresetInt);
	FString ColorStr;
	if (GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("CustomThemeColors"), ColorStr, GEditorPerProjectIni))
	{
		TArray<FString> ColorValues;
		ColorStr.ParseIntoArray(ColorValues, TEXT("|"));
		if (ColorValues.Num() >= 15)
		{
			auto ParseColor = [](const FString& ColorStr) -> FLinearColor
			{
				FLinearColor Color = FLinearColor::White;
				Color.InitFromString(ColorStr);
				return Color;
			};
			FBpGeneratorTheme Theme;
			Theme.Preset = EThemePreset::Custom;
			Theme.PrimaryColor = ParseColor(ColorValues[0]);
			Theme.SecondaryColor = ParseColor(ColorValues[1]);
			Theme.TertiaryColor = ParseColor(ColorValues[2]);
			Theme.MainBackgroundColor = ParseColor(ColorValues[3]);
			Theme.PanelBackgroundColor = ParseColor(ColorValues[4]);
			Theme.BorderBackgroundColor = ParseColor(ColorValues[5]);
			Theme.PrimaryTextColor = ParseColor(ColorValues[6]);
			Theme.SecondaryTextColor = ParseColor(ColorValues[7]);
			Theme.HintTextColor = ParseColor(ColorValues[8]);
			Theme.AnalystColor = ParseColor(ColorValues[9]);
			Theme.ScannerColor = ParseColor(ColorValues[10]);
			Theme.ArchitectColor = ParseColor(ColorValues[11]);
			Theme.GradientTop = ParseColor(ColorValues[12]);
			Theme.GradientBottom = ParseColor(ColorValues[13]);
			Theme.bUseGradients = FCString::ToBool(*ColorValues[14]);
			CurrentTheme = Theme;
			return;
		}
	}
	CurrentTheme = FBpGeneratorTheme::GetPreset(Preset);
}
void SBpGeneratorUltimateWidget::SaveThemeSettings()
{
	GConfig->SetInt(TEXT("BpGeneratorUltimate"), TEXT("ThemePreset"), static_cast<int32>(CurrentTheme.Preset), GEditorPerProjectIni);
	if (CurrentTheme.Preset == EThemePreset::Custom)
	{
		FString ColorStr = FString::Printf(TEXT("%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s"),
			*CurrentTheme.PrimaryColor.ToString(),
			*CurrentTheme.SecondaryColor.ToString(),
			*CurrentTheme.TertiaryColor.ToString(),
			*CurrentTheme.MainBackgroundColor.ToString(),
			*CurrentTheme.PanelBackgroundColor.ToString(),
			*CurrentTheme.BorderBackgroundColor.ToString(),
			*CurrentTheme.PrimaryTextColor.ToString(),
			*CurrentTheme.SecondaryTextColor.ToString(),
			*CurrentTheme.HintTextColor.ToString(),
			*CurrentTheme.AnalystColor.ToString(),
			*CurrentTheme.ScannerColor.ToString(),
			*CurrentTheme.ArchitectColor.ToString(),
			*CurrentTheme.GradientTop.ToString(),
			*CurrentTheme.GradientBottom.ToString(),
			CurrentTheme.bUseGradients ? TEXT("true") : TEXT("false")
		);
		GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("CustomThemeColors"), *ColorStr, GEditorPerProjectIni);
	}
	GConfig->Flush(false, GEditorPerProjectIni);
}
void SBpGeneratorUltimateWidget::ApplyTheme()
{
}
TSharedPtr<SBorder> SBpGeneratorUltimateWidget::CreateThemedBorder(TSharedPtr<SWidget> Content, const FMargin& Padding)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
		.Padding(Padding)
		[
			Content.IsValid() ? Content.ToSharedRef() : SNew(SBox)
		];
}
TSharedPtr<SButton> SBpGeneratorUltimateWidget::CreateThemedButton(FText Text, FOnClicked OnClicked, const FLinearColor& OverrideColor)
{
	FLinearColor ButtonColor = OverrideColor.A > 0.01f ? OverrideColor : CurrentTheme.PrimaryColor;
	return SNew(SButton)
		.Text(Text)
		.OnClicked(OnClicked)
		.ButtonColorAndOpacity(FSlateColor(ButtonColor))
		.ForegroundColor(FSlateColor(FLinearColor::White));
}
TSharedPtr<STextBlock> SBpGeneratorUltimateWidget::CreateThemedTextBlock(FText Text, bool bIsPrimary)
{
	return SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(FSlateColor(bIsPrimary ? CurrentTheme.PrimaryTextColor : CurrentTheme.SecondaryTextColor));
}
void SBpGeneratorUltimateWidget::UpdateCustomFieldsVisibility()
{
	EVisibility NewVisibility = (CurrentSettings.Provider == EApiProvider::Custom) ? EVisibility::Visible : EVisibility::Collapsed;
	if (CustomBaseURLWrapper.IsValid())
	{
		CustomBaseURLWrapper->SetVisibility(NewVisibility);
	}
	if (CustomModelNameWrapper.IsValid())
	{
		CustomModelNameWrapper->SetVisibility(NewVisibility);
	}
}
FReply SBpGeneratorUltimateWidget::OnShowSettingsClicked()
{
	LoadSettings();
	if (ApiKeyInput.IsValid())
	{
		ApiKeyInput->SetText(FText::FromString(CurrentSettings.ApiKey));
	}
	if (CustomBaseURLInput.IsValid())
	{
		CustomBaseURLInput->SetText(FText::FromString(CurrentSettings.CustomBaseURL));
	}
	if (CustomModelNameInput.IsValid())
	{
		CustomModelNameInput->SetText(FText::FromString(CurrentSettings.CustomModelName));
	}
	if (ProviderComboBox.IsValid())
	{
		int32 ProviderIndex = 0;
		if (CurrentSettings.Provider == EApiProvider::Gemini) ProviderIndex = 0;
		else if (CurrentSettings.Provider == EApiProvider::OpenAI) ProviderIndex = 1;
		else if (CurrentSettings.Provider == EApiProvider::Claude) ProviderIndex = 2;
		else if (CurrentSettings.Provider == EApiProvider::DeepSeek) ProviderIndex = 3;
		else if (CurrentSettings.Provider == EApiProvider::Custom) ProviderIndex = 4;
		if (ProviderOptions.IsValidIndex(ProviderIndex))
		{
			ProviderComboBox->SetSelectedItem(ProviderOptions[ProviderIndex]);
		}
	}
	UpdateCustomFieldsVisibility();
	if (MainSwitcher.IsValid())
	{
		MainSwitcher->SetActiveWidgetIndex(1);
	}
	if (SettingsContentBox.IsValid())
	{
		CurrentlyEditingSlotIndex = FApiKeyManager::Get().GetActiveSlotIndex();
		SettingsContentBox->SetContent(CreateSlotConfigSection());
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnSaveSettingsClicked()
{
	if (CurrentlyEditingSlotIndex >= 0 && CurrentlyEditingSlotIndex < MAX_API_KEY_SLOTS)
	{
		FApiKeySlot NewSlot;
		NewSlot.SlotIndex = CurrentlyEditingSlotIndex;
		if (SlotWidgets[CurrentlyEditingSlotIndex].NameInput.IsValid())
		{
			NewSlot.Name = SlotWidgets[CurrentlyEditingSlotIndex].NameInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].ApiKeyInput.IsValid())
		{
			NewSlot.ApiKey = SlotWidgets[CurrentlyEditingSlotIndex].ApiKeyInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox.IsValid() && SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem().IsValid())
		{
			NewSlot.Provider = *SlotWidgets[CurrentlyEditingSlotIndex].ProviderComboBox->GetSelectedItem();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].CustomBaseURLInput.IsValid())
		{
			NewSlot.CustomBaseURL = SlotWidgets[CurrentlyEditingSlotIndex].CustomBaseURLInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].CustomModelNameInput.IsValid())
		{
			NewSlot.CustomModelName = SlotWidgets[CurrentlyEditingSlotIndex].CustomModelNameInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].GeminiModelComboBox.IsValid() && SlotWidgets[CurrentlyEditingSlotIndex].GeminiModelComboBox->GetSelectedItem().IsValid())
		{
			NewSlot.GeminiModel = *SlotWidgets[CurrentlyEditingSlotIndex].GeminiModelComboBox->GetSelectedItem();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].OpenAIModelComboBox.IsValid() && SlotWidgets[CurrentlyEditingSlotIndex].OpenAIModelComboBox->GetSelectedItem().IsValid())
		{
			NewSlot.OpenAIModel = *SlotWidgets[CurrentlyEditingSlotIndex].OpenAIModelComboBox->GetSelectedItem();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].ClaudeModelComboBox.IsValid() && SlotWidgets[CurrentlyEditingSlotIndex].ClaudeModelComboBox->GetSelectedItem().IsValid())
		{
			NewSlot.ClaudeModel = *SlotWidgets[CurrentlyEditingSlotIndex].ClaudeModelComboBox->GetSelectedItem();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].TextureGenEndpointInput.IsValid())
		{
			NewSlot.TextureGenEndpoint = SlotWidgets[CurrentlyEditingSlotIndex].TextureGenEndpointInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModelInput.IsValid())
		{
			NewSlot.TextureGenModel = SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModelInput->GetText().ToString();
		}
		if (SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModeCheckBox.IsValid())
		{
			NewSlot.bTextureMode = SlotWidgets[CurrentlyEditingSlotIndex].TextureGenModeCheckBox->IsChecked();
		}
		FApiKeyManager::Get().SetSlot(CurrentlyEditingSlotIndex, NewSlot);
	}
	if (ApiKeyInput.IsValid())
	{
		CurrentSettings.ApiKey = ApiKeyInput->GetText().ToString();
	}
	if (CustomBaseURLInput.IsValid())
	{
		CurrentSettings.CustomBaseURL = CustomBaseURLInput->GetText().ToString();
	}
	if (CustomModelNameInput.IsValid())
	{
		CurrentSettings.CustomModelName = CustomModelNameInput->GetText().ToString();
	}
	SaveSettings();
	if (MainSwitcher.IsValid())
	{
		MainSwitcher->SetActiveWidgetIndex(0);
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::OnProviderChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid()) return;
	if (*NewSelection == TEXT("OpenAI"))
	{
		CurrentSettings.Provider = EApiProvider::OpenAI;
	}
	else if (*NewSelection == TEXT("Anthropic Claude"))
	{
		CurrentSettings.Provider = EApiProvider::Claude;
	}
	else if (*NewSelection == TEXT("DeepSeek"))
	{
		CurrentSettings.Provider = EApiProvider::DeepSeek;
	}
	else if (*NewSelection == TEXT("Custom API Provider"))
	{
		CurrentSettings.Provider = EApiProvider::Custom;
	}
	else
	{
		CurrentSettings.Provider = EApiProvider::Gemini;
	}
	UpdateCustomFieldsVisibility();
}
void SBpGeneratorUltimateWidget::OnThemePresetChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid()) return;
	if (*NewSelection == TEXT("Slate Professional"))
	{
		CurrentTheme = FBpGeneratorTheme::GetSlateProfessionalTheme();
	}
	else if (*NewSelection == TEXT("Rose Gold"))
	{
		CurrentTheme = FBpGeneratorTheme::GetRoseGoldTheme();
	}
	else if (*NewSelection == TEXT("Emerald"))
	{
		CurrentTheme = FBpGeneratorTheme::GetEmeraldTheme();
	}
	else if (*NewSelection == TEXT("Cyberpunk"))
	{
		CurrentTheme = FBpGeneratorTheme::GetCyberpunkTheme();
	}
	else if (*NewSelection == TEXT("Solar Flare"))
	{
		CurrentTheme = FBpGeneratorTheme::GetSolarFlareTheme();
	}
	else if (*NewSelection == TEXT("Deep Ocean"))
	{
		CurrentTheme = FBpGeneratorTheme::GetDeepOceanTheme();
	}
	else if (*NewSelection == TEXT("Lavender Dream"))
	{
		CurrentTheme = FBpGeneratorTheme::GetLavenderDreamTheme();
	}
	else
	{
		CurrentTheme = FBpGeneratorTheme::GetGoldenTurquoiseTheme();
	}
	SaveThemeSettings();
}
FString SBpGeneratorUltimateWidget::LanguageNameToCode(const FString& LanguageName)
{
	if (LanguageName == TEXT("Espa\u00f1ol")) return TEXT("es");
	if (LanguageName == TEXT("Fran\u00e7ais")) return TEXT("fr");
	if (LanguageName == TEXT("Deutsch")) return TEXT("de");
	if (LanguageName == TEXT("\u4e2d\u6587")) return TEXT("zh");
	if (LanguageName == TEXT("\u65e5\u672c\u8a9e")) return TEXT("ja");
	if (LanguageName == TEXT("\u0420\u0443\u0441\u0441\u043a\u0438\u0439")) return TEXT("ru");
	if (LanguageName == TEXT("Portugu\u00eas")) return TEXT("pt");
	if (LanguageName == TEXT("\ud55c\uad6d\uc5b4")) return TEXT("ko");
	if (LanguageName == TEXT("Italiano")) return TEXT("it");
	if (LanguageName == TEXT("\u0627\u0644\u0639\u0631\u0628\u064a\u0629")) return TEXT("ar");
	if (LanguageName == TEXT("Nederlands")) return TEXT("nl");
	if (LanguageName == TEXT("T\u00fcrk\u00e7e")) return TEXT("tr");
	return TEXT("en");
}
void SBpGeneratorUltimateWidget::OnLanguageChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid()) return;
	FString LangCode = LanguageNameToCode(*NewSelection);
	FUIConfigManager::Get().SetLanguage(LangCode);
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Language changed to: %s"), **NewSelection)));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	if (EntireSettingsBox.IsValid())
	{
		EntireSettingsBox->SetContent(CreateSettingsWidget());
	}
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateHowToUseWidget()
{
	const FLinearColor HeaderColor(1.0f, 0.6f, 0.0f);
	const FMargin StepPadding(0, 0, 0, 15);
	const FMargin SubStepPadding(20, 0, 0, 10);
	return SNew(SBorder)
		.Padding(20)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 20)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("howto_title"), TEXT("How to Use the Blueprint Analyst")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("howto_header1"), TEXT("To Analyze a Full Asset (Blueprint, Material, etc.)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(HeaderColor)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step1_1"), TEXT("1. Select a single Blueprint, Material, Widget, AnimBP or Behavior Tree asset in the Content Browser.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step1_2"), TEXT("2. Click the 'Add Full Asset Context' button. You'll see a confirmation in the text box.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step1_3"), TEXT("3. Type your question (e.g., 'Explain this Blueprint,' 'Check for performance issues') and click 'Send'.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
				[
					SNew(STextBlock)
						.Text(UIConfig::GetTextAttr(TEXT("howto_header2"), TEXT("To Analyze Specific Nodes")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(HeaderColor)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step2_1"), TEXT("1. Open the asset editor (e.g., Blueprint Editor) and select one or more nodes in the graph.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step2_2"), TEXT("2. Click the 'Add Selected Nodes Context' button.")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
				[
					SNew(STextBlock).AutoWrapText(true)
						.Text(UIConfig::GetTextAttr(TEXT("howto_step2_3"), TEXT("3. Ask your question (e.g., 'What does this logic do?,' 'How can I improve this?,' 'Debug this for me') and click 'Send'.")))
				]
		];
}
void SBpGeneratorUltimateWidget::RefreshChatHistoryView()
{
	if (!ChatHistoryBrowser.IsValid()) return;
	TStringBuilder<65536> HtmlBuilder;
	static FString MarkedJs;
	static FString GraphreJs;
	static FString NomnomlJs;
	if (MarkedJs.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BpGeneratorUltimate"));
		if (Plugin.IsValid())
		{
			FString MarkedJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("marked.min.js"));
			FFileHelper::LoadFileToString(MarkedJs, *MarkedJsPath);
			FString GraphreJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("graphre.min.js"));
			FFileHelper::LoadFileToString(GraphreJs, *GraphreJsPath);
			FString NomnomlJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("nomnoml.min.js"));
			FFileHelper::LoadFileToString(NomnomlJs, *NomnomlJsPath);
		}
	}
	HtmlBuilder.Append(TEXT("<!DOCTYPE html><html><head><meta charset='utf-8'><script>"));
	HtmlBuilder.Append(MarkedJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(GraphreJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(NomnomlJs);
	HtmlBuilder.Append(TEXT("</script><script>"
	"function createPieChart(dataStr){"
	"var colors=['#4fc3f7','#81c784','#ffb74d','#f06292','#ba68c8','#4db6ac','#ffd54f','#90a4ae','#e57373','#64b5f6'];"
	"var parts=dataStr.split(',').map(function(p){var s=p.trim().split(/\\s+/);return{label:s[0],value:parseInt(s[1])||0};});"
	"var total=parts.reduce(function(s,p){return s+p.value;},0);"
	"if(total===0)return'<p>No data</p>';"
	"var cx=100,cy=100,r=80;"
	"var svg='<svg width=\"300\" height=\"250\" style=\"margin:16px auto;display:block;\">';"
	"var angle=-Math.PI/2;"
	"parts.forEach(function(p,i){"
	"var pct=p.value/total;"
	"var sweep=pct*2*Math.PI;"
	"var x1=cx+r*Math.cos(angle),y1=cy+r*Math.sin(angle);"
	"angle+=sweep;"
	"var x2=cx+r*Math.cos(angle),y2=cy+r*Math.sin(angle);"
	"var large=sweep>Math.PI?1:0;"
	"svg+='<path d=\"M'+cx+','+cy+' L'+x1+','+y1+' A'+r+','+r+' 0 '+large+',1 '+x2+','+y2+' Z\" fill=\"'+colors[i%colors.length]+'\"/>';"
	"});"
	"svg+='<circle cx=\"'+cx+'\" cy=\"'+cy+'\" r=\"35\" fill=\"#1e1e1e\"/>';"
	"svg+='<text x=\"'+cx+'\" y=\"'+(cy+5)+'\" text-anchor=\"middle\" fill=\"#d4d4d4\" font-size=\"12\">'+total+' total</text></svg>';"
	"svg+='<div class=\"chart-legend\" style=\"display:flex;flex-wrap:wrap;gap:8px;justify-content:center;margin:8px 0;\">';"
	"parts.forEach(function(p,i){"
	"var pct=Math.round(p.value/total*100);"
	"svg+='<span style=\"display:flex;align-items:center;gap:4px;font-size:12px;\"><span style=\"width:12px;height:12px;background:'+colors[i%colors.length]+';border-radius:2px;\"></span>'+p.label+' ('+pct+'%)</span>';"
	"});"
	"svg+='</div>';"
	"return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML+this.querySelector(\\'.chart-legend\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML+this.parentElement.querySelector(\\'.chart-legend\\').outerHTML)\">⤢</button></div>';"
	"}"
	"function parseMarkdown(str){"
	"var html=marked.parse(str);"
	"html=html.replace(/<pre><code class=\"language-chart\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){return createPieChart(c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&'));});"
	"html=html.replace(/<pre><code class=\"language-flow\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){var d=c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&').trim();d=d.replace(/\\(([^)]*)\\)/g,'-$1-');var styled='#stroke: #b0b0b0\\n#lineWidth: 2\\n#fill: #2d2d30\\n#fontSize: 14\\n'+d;try{if(typeof nomnoml==='undefined')return'<pre style=\"color:#f44\">Flow error: nomnoml not loaded</pre>';var svg=nomnoml.renderSvg(styled);return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML)\">⤢</button></div>';}catch(e){return'<details><summary style=\"color:#f44;cursor:pointer\">Flow error: '+e.message+'</summary><pre style=\"background:#2d2d30;padding:8px\">'+d.substring(0,200)+'</pre></details>';}});"
	"html=html.replace(/\\[([^\\]]+)\\]\\(ue:\\/\\/asset\\?name=([^)]+)\\)/g,'<a href=\"ue://asset?name=$2\" class=\"asset-link\">$1</a>');"
	"return html;"
	"}"
	"</script><style>"));
	HtmlBuilder.Append(TEXT("body{font:14px 'Segoe UI',Arial;margin:16px;line-height:1.6;color:#d4d4d4;background:#1e1e1e;}"));
	HtmlBuilder.Append(TEXT("a{color:#ffa726 !important;text-decoration:none;}a:hover{color:#ffcc80 !important;}"));
	HtmlBuilder.Append(TEXT(".speaker{font-weight:bold;margin-bottom:2px;}"));
	HtmlBuilder.Append(TEXT(".user-msg{margin-bottom:15px;}"));
	HtmlBuilder.Append(TEXT(".ai-msg{margin-bottom:15px;background:#252526;padding:1px 15px;border-radius:5px;}"));
	HtmlBuilder.Append(TEXT("h1,h2,h3{color:#e0e0e0;border-bottom:1px solid #404040;padding-bottom:0.3em;}"));
	HtmlBuilder.Append(TEXT("code{background:#2d2d30;color:#ce9178;padding:0.2em 0.4em;border-radius:3px;font-family:'Consolas','Courier New',monospace;}"));
	HtmlBuilder.Append(TEXT("pre{background:#2d2d30;padding:16px;border-radius:6px;overflow:auto;} pre code{padding:0;}"));
	HtmlBuilder.Append(TEXT("table{border-collapse:collapse;margin:16px 0;width:100%;}"));
	HtmlBuilder.Append(TEXT("th,td{border:1px solid #404040;padding:8px 12px;text-align:left;}"));
	HtmlBuilder.Append(TEXT("th{background:#2d2d30;color:#e0e0e0;font-weight:600;}"));
	HtmlBuilder.Append(TEXT("tr:nth-child(even){background:#252526;}"));
	HtmlBuilder.Append(TEXT("tr:hover{background:#363636;}"));
	HtmlBuilder.Append(TEXT(".tool-result{background:#2d2d30;border-radius:6px;margin:8px 0;cursor:pointer;border:1px solid #404040;}"));
	HtmlBuilder.Append(TEXT(".tool-result:hover{border-color:#0078d4;}"));
	HtmlBuilder.Append(TEXT(".tool-header{display:flex;align-items:center;padding:8px 12px;gap:8px;}"));
	HtmlBuilder.Append(TEXT(".tool-icon{font-size:16px;}"));
	HtmlBuilder.Append(TEXT(".tool-name{flex:1;font-weight:500;color:#e0e0e0;}"));
	HtmlBuilder.Append(TEXT(".tool-chevron{color:#808080;transition:transform 0.2s;}"));
	HtmlBuilder.Append(TEXT(".tool-result.expanded .tool-chevron{transform:rotate(90deg);}"));
	HtmlBuilder.Append(TEXT(".tool-content{padding:0 12px 12px;display:none;}"));
	HtmlBuilder.Append(TEXT(".tool-content pre{margin:0;white-space:pre-wrap;word-wrap:break-word;font-size:12px;}"));
	HtmlBuilder.Append(TEXT(".asset-link{color:#ffa726;text-decoration:none;border-bottom:1px dashed #ffa726;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".asset-link:hover{color:#ffcc80;background:rgba(255,167,38,0.15);}"));
	HtmlBuilder.Append(TEXT("#zoom-controls{position:fixed;bottom:10px;right:10px;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:4px 8px;border-radius:4px;z-index:1000;border:1px solid #404040;}"));
	HtmlBuilder.Append(TEXT("#zoom-indicator{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset{background:#444;color:#d4d4d4;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.9);z-index:2000;justify-content:center;align-items:center;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal.show{display:flex;}"));
	HtmlBuilder.Append(TEXT("#modal-content{max-width:90%;max-height:90%;overflow:auto;background:#1e1e1e;border-radius:8px;padding:20px;position:relative;}"));
	HtmlBuilder.Append(TEXT("#modal-close{position:absolute;top:10px;right:10px;background:#444;color:#fff;border:none;width:30px;height:30px;border-radius:50%;cursor:pointer;font-size:16px;z-index:10;}"));
	HtmlBuilder.Append(TEXT("#modal-toolbar{position:sticky;top:0;left:0;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:8px 12px;border-radius:4px;border:1px solid #404040;margin-bottom:12px;z-index:5;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint{color:#888;font-size:11px;display:flex;align-items:center;gap:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint kbd{background:#444;color:#ccc;padding:2px 6px;border-radius:3px;font-size:10px;border:1px solid #555;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-pct{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;border-left:1px solid #404040;padding-left:8px;margin-left:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn{background:#444;color:#d4d4d4;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn{background:#0078d4;color:#fff;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;margin-left:auto;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn:hover{background:#1084d8;}"));
	HtmlBuilder.Append(TEXT(".diagram-container{position:relative;display:inline-block;max-width:150px;max-height:150px;overflow:hidden;border:1px solid #404040;border-radius:4px;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".diagram-container svg{max-width:150px;max-height:150px;overflow:hidden;}"));
	HtmlBuilder.Append(TEXT(".diagram-container .chart-legend{display:none;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand{position:absolute;top:5px;right:5px;background:rgba(45,45,48,0.9);color:#b0b0b0;border:none;width:24px;height:24px;border-radius:4px;cursor:pointer;font-size:14px;opacity:0;transition:opacity 0.2s;}"));
	HtmlBuilder.Append(TEXT(".diagram-container:hover .diagram-expand{opacity:1;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand:hover{background:#2ea043;color:#fff;}"));
	HtmlBuilder.Append(TEXT("</style><script>function toggleTool(id){var el=document.getElementById(id);if(el){el.classList.toggle('expanded');var content=el.querySelector('.tool-content');if(content){content.style.display=el.classList.contains('expanded')?'block':'none';}}}"));
	HtmlBuilder.Append(TEXT("var zoomLevel=100;function updateZoom(){document.getElementById('zoom-content').style.transform='scale('+zoomLevel/100+')';document.getElementById('zoom-content').style.transformOrigin='top left';document.getElementById('zoom-indicator').textContent=zoomLevel+'%';}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('wheel',function(e){if(e.ctrlKey&&!document.getElementById('diagram-modal').classList.contains('show')){e.preventDefault();zoomLevel+=e.deltaY>0?-10:10;zoomLevel=Math.max(30,Math.min(200,zoomLevel));updateZoom();}},{passive:false});"));
	HtmlBuilder.Append(TEXT("function resetZoom(){zoomLevel=100;updateZoom();}"));
	HtmlBuilder.Append(TEXT("var modalZoom=100;function openModal(svgHtml){document.getElementById('modal-diagram').innerHTML=svgHtml;document.getElementById('diagram-modal').classList.add('show');modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function closeModal(e){if(!e||e.target.id==='diagram-modal')document.getElementById('diagram-modal').classList.remove('show');}"));
	HtmlBuilder.Append(TEXT("function updateModalZoom(){var el=document.getElementById('modal-diagram');el.style.transform='scale('+modalZoom/100+')';el.style.transformOrigin='center center';document.getElementById('modal-zoom-pct').textContent=modalZoom+'%';}"));
	HtmlBuilder.Append(TEXT("function resetModalZoom(){modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function popoutDiagram(){var svgHtml=document.getElementById('modal-diagram').innerHTML;window.location.href='ue://diagram-popout?data='+encodeURIComponent(svgHtml);}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('keydown',function(e){if(e.key==='Escape')closeModal();});"));
	HtmlBuilder.Append(TEXT("</script></head><body>"));
	HtmlBuilder.Append(TEXT("<div id='diagram-modal' onclick='closeModal(event)'><div id='modal-content' onclick='event.stopPropagation()'><button id='modal-close' onclick='closeModal()'>&times;</button><div id='modal-toolbar'><span id='modal-zoom-hint'><kbd>Ctrl</kbd>+Scroll to zoom</span><span id='modal-zoom-pct'>100%</span><button id='modal-reset-btn' onclick='resetModalZoom()'>Reset</button><button id='modal-popout-btn' onclick='popoutDiagram()'>Open in Window</button></div><div id='modal-diagram'></div></div></div>"));
	HtmlBuilder.Append(TEXT("<script>document.getElementById('diagram-modal').addEventListener('wheel',function(e){if(e.ctrlKey){e.preventDefault();modalZoom+=e.deltaY>0?-10:10;modalZoom=Math.max(30,Math.min(200,modalZoom));updateModalZoom();}},{passive:false});</script>"));
	HtmlBuilder.Append(TEXT("<div id='zoom-controls'><span id='zoom-indicator'>100%</span><button id='zoom-reset' onclick='resetZoom()'>Reset</button></div><div id='zoom-content'>"));
	if (bIsThinking)
	{
		HtmlBuilder.Append(TEXT("<div style='display:flex;align-items:center;gap:8px;padding:8px 0;color:#FFD700;font-size:13px;'><span style='animation:spin 1s linear infinite;'>⟳</span><span>Loading...</span></div><style>@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}</style>"));
	}
	int32 ToolResultCounter = 0;
	for (const TSharedPtr<FJsonValue>& MessageValue : AnalystConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MessageObject = MessageValue->AsObject();
		FString Role, Content;
		if (MessageObject->TryGetStringField(TEXT("role"), Role))
		{
			if (Role == TEXT("context"))
			{
				FString FileName;
				FString FileContent;
				int32 CharCount = 0;
				bool bExpanded = false;
				MessageObject->TryGetStringField(TEXT("file_name"), FileName);
				MessageObject->TryGetStringField(TEXT("content"), FileContent);
				MessageObject->TryGetNumberField(TEXT("char_count"), CharCount);
				MessageObject->TryGetBoolField(TEXT("expanded"), bExpanded);
				FString EscapedContent = FileContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
				HtmlBuilder.Appendf(TEXT("<div class='tool-result%s' id='file-%d' onclick='toggleTool(\"file-%d\")'><div class='tool-header'><div class='tool-icon'>&#128196;</div><span class='tool-name'>%s (%d chars)</span><span class='tool-chevron'>&#9654;</span></div><div class='tool-content' style='display:%s'><pre>%s</pre></div></div>"), bExpanded ? TEXT(" expanded") : TEXT(""), ToolResultCounter, ToolResultCounter, *FileName, CharCount, bExpanded ? TEXT("block") : TEXT("none"), *EscapedContent);
				ToolResultCounter++;
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (MessageObject->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				FString Speaker = (Role == TEXT("user")) ? TEXT("You") : TEXT("AI");
				FString DivClass = (Role == TEXT("user")) ? TEXT("user-msg") : TEXT("ai-msg");
				HtmlBuilder.Appendf(TEXT("<div class='%s'><div class='speaker'>%s:</div><div class='content-container'>"), *DivClass, *Speaker);
				for (const TSharedPtr<FJsonValue>& PartValue : *PartsArray)
				{
					const TSharedPtr<FJsonObject>& PartObj = PartValue->AsObject();
					FString TextContent;
					if (PartObj->TryGetStringField(TEXT("text"), TextContent))
					{
						FString DisplayContent = TextContent;
						if (Role == TEXT("user"))
						{
							int32 ContextEndIndex = DisplayContent.Find(TEXT("---"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
							if (ContextEndIndex != INDEX_NONE)
							{
								DisplayContent = DisplayContent.RightChop(ContextEndIndex + 3).TrimStart();
							}
							DisplayContent = DisplayContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
							HtmlBuilder.Appendf(TEXT("<div class='text-content'>%s</div>"), *DisplayContent);
						}
						else
						{
							DisplayContent.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
							DisplayContent.ReplaceInline(TEXT("`"), TEXT("\\`"));
							DisplayContent.ReplaceInline(TEXT("$"), TEXT("\\$"));
							HtmlBuilder.Appendf(TEXT("<div id='msg-%d'></div><script>var el=document.getElementById('msg-%d');el.innerHTML=parseMarkdown(`%s`);</script>"),
								AnalystConversationHistory.IndexOfByKey(MessageValue), AnalystConversationHistory.IndexOfByKey(MessageValue), *DisplayContent);
						}
					}
				}
				HtmlBuilder.Append(TEXT("</div></div>"));
			}
		}
	}
	if (bIsThinking)
	{
		HtmlBuilder.Append(TEXT("<div class='ai-msg'><div class='speaker'>AI:</div><div><i>Thinking...</i></div></div>"));
	}
	HtmlBuilder.Append(TEXT("<script>window.scrollTo(0, document.body.scrollHeight);</script>"));
	HtmlBuilder.Append(TEXT("</div></body></html>"));
	FString DataUri = TEXT("data:text/html;charset=utf-8,") + FGenericPlatformHttp::UrlEncode(HtmlBuilder.ToString());
	ChatHistoryBrowser->LoadURL(DataUri);
}
FReply SBpGeneratorUltimateWidget::OnAddAssetContextClicked()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> SelectedAssetsData;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssetsData);
	if (SelectedAssetsData.Num() != 1)
	{
		InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_select_one_asset"), TEXT("Error: Please select exactly one asset in the Content Browser."))));
		return FReply::Handled();
	}
	UObject* SelectedObject = SelectedAssetsData[0].GetAsset();
	if (!SelectedObject)
	{
		InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_invalid_asset"), TEXT("Error: The selected asset is invalid or could not be loaded."))));
		return FReply::Handled();
	}
	FString RawSummary;
	FString AssetTypeName;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject))
	{
		RawSummary = FBpSummarizerr::Summarize(Blueprint);
		AssetTypeName = TEXT("Blueprint");
	}
	else if (UMaterial* Material = Cast<UMaterial>(SelectedObject))
	{
		FMaterialGraphDescriber Describer;
		RawSummary = Describer.Describe(Material);
		AssetTypeName = TEXT("Material");
	}
	else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(SelectedObject))
	{
		FBtGraphDescriber Describer;
		RawSummary = Describer.Describe(BehaviorTree);
		AssetTypeName = TEXT("Behavior Tree");
	}
	else
	{
		FString ErrorMsg = UIConfig::GetValue(TEXT("error_unsupported_asset_type"), TEXT("Error: The selected asset ('{0}') is not a supported type for summary (Blueprint, Material, or Behavior Tree)."));
		ErrorMsg.ReplaceInline(TEXT("{0}"), *SelectedObject->GetClass()->GetName());
		InputTextBox->SetText(FText::FromString(ErrorMsg));
		return FReply::Handled();
	}
	if (RawSummary.IsEmpty())
	{
		InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_empty_summary"), TEXT("Error: Could not generate a raw summary for the selected asset. It might be empty or corrupted."))));
		return FReply::Handled();
	}
	PendingContext = FString::Printf(TEXT("Here is the context for the %s '%s':\n\n--- Data ---\n%s\n---"), *AssetTypeName, *SelectedObject->GetName(), *RawSummary);
	FString Msg = UIConfig::GetValue(TEXT("msg_asset_context_attached"), TEXT("Context for asset '{0}' attached. Now, what would you like to know?"));
	Msg.ReplaceInline(TEXT("{0}"), *SelectedObject->GetName());
	InputTextBox->SetText(FText::FromString(Msg));
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnAddNodeContextClicked()
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem) {  return FReply::Handled(); }
	IAssetEditorInstance* ActiveEditor = nullptr;
	double LastActivationTime = 0.0;
	for (UObject* Asset : AssetEditorSubsystem->GetAllEditedAssets())
	{
		for (IAssetEditorInstance* Editor : AssetEditorSubsystem->FindEditorsForAsset(Asset))
		{
			if (Editor && Editor->GetLastActivationTime() > LastActivationTime)
			{
				LastActivationTime = Editor->GetLastActivationTime();
				ActiveEditor = Editor;
			}
		}
	}
	if (!ActiveEditor)
	{
		InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_no_active_editor"), TEXT("Error: No active asset editor found. Please open a Blueprint, Material, or Behavior Tree."))));
		return FReply::Handled();
	}
	const FName EditorName = ActiveEditor->GetEditorName();
	TSet<UObject*> SelectedNodes;
	FString NodesSummary;
	bool bFoundEditor = false;
	if (EditorName == FName(TEXT("BlueprintEditor")) || EditorName == FName(TEXT("AnimationBlueprintEditor")))
	{
		FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
		SelectedNodes = BlueprintEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("WidgetBlueprintEditor")))
	{
		FWidgetBlueprintEditor* WidgetEditor = static_cast<FWidgetBlueprintEditor*>(ActiveEditor);
		SelectedNodes = WidgetEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("MaterialEditor")))
	{
		FMaterialEditor* MaterialEditor = static_cast<FMaterialEditor*>(ActiveEditor);
		SelectedNodes = MaterialEditor->GetSelectedNodes();
		bFoundEditor = true;
		FMaterialNodeDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("Behavior Tree")))
	{
		FBehaviorTreeEditor* BehaviorTreeEditor = static_cast<FBehaviorTreeEditor*>(ActiveEditor);
		SelectedNodes = BehaviorTreeEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBtGraphDescriber Describer;
		NodesSummary = Describer.DescribeSelection(SelectedNodes);
	}
	if (!bFoundEditor)
	{
		FString ErrorMsg = UIConfig::GetValue(TEXT("error_unsupported_editor"), TEXT("Error: The active editor ('{0}') is not supported for node selection."));
		ErrorMsg.ReplaceInline(TEXT("{0}"), *EditorName.ToString());
		InputTextBox->SetText(FText::FromString(ErrorMsg));
		return FReply::Handled();
	}
	if (SelectedNodes.Num() == 0)
	{
		InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_no_nodes_selected"), TEXT("Error: No nodes are selected in the active graph editor."))));
		return FReply::Handled();
	}
	PendingContext = FString::Printf(TEXT("Here is the context for the currently selected nodes:\n\n--- Node Data ---\n%s\n---"), *NodesSummary);
	InputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("msg_node_context_attached"), TEXT("Context for selected nodes has been attached. Now, what is your question?"))));
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnSendClicked()
{
	FString UserInput = InputTextBox->GetText().ToString();
	if (UserInput.IsEmpty() || bIsThinking || ActiveChatID.IsEmpty())
	{
		return FReply::Handled();
	}
	InputTextBox->SetText(FText());
	if (AnalystConversationHistory.Num() <= 1)
	{
		FString NewTitle = UserInput;
		if (NewTitle.Len() > 40)
		{
			NewTitle = NewTitle.Left(37) + TEXT("...");
		}
		TSharedPtr<FConversationInfo>* FoundInfo = ConversationList.FindByPredicate(
			[&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ActiveChatID; });
		if (FoundInfo)
		{
			(*FoundInfo)->Title = NewTitle;
			SaveManifest();
			if (ChatListView.IsValid())
			{
				ChatListView->RequestListRefresh();
			}
		}
	}
	FString FullPromptToAI = UserInput;
	if (!PendingContext.IsEmpty())
	{
		FullPromptToAI = FString::Printf(TEXT("%s\n\n%s"), *PendingContext, *UserInput);
		PendingContext.Empty();
	}
	for (const FAttachedFileContext& File : AnalystAttachedFiles)
	{
		TSharedPtr<FJsonObject> FileContextMsg = MakeShareable(new FJsonObject);
		FileContextMsg->SetStringField(TEXT("role"), TEXT("context"));
		FileContextMsg->SetStringField(TEXT("file_name"), File.FileName);
		FileContextMsg->SetStringField(TEXT("content"), File.Content);
		FileContextMsg->SetNumberField(TEXT("char_count"), File.CharCount);
		FileContextMsg->SetBoolField(TEXT("expanded"), File.bExpanded);
		TArray<TSharedPtr<FJsonValue>> FileParts;
		TSharedPtr<FJsonObject> FilePart = MakeShareable(new FJsonObject);
		FilePart->SetStringField(TEXT("text"), FString::Printf(TEXT("--- File: %s (%d chars, click to expand) ---"), *File.FileName, File.CharCount));
		FileParts.Add(MakeShareable(new FJsonValueObject(FilePart)));
		FileContextMsg->SetArrayField(TEXT("parts"), FileParts);
		AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(FileContextMsg)));
	}
	AnalystAttachedFiles.Empty();
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), FullPromptToAI);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	for (const FAttachedImage& Image : AnalystAttachedImages)
	{
		TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
		TSharedPtr<FJsonObject> InlineData = MakeShareable(new FJsonObject);
		InlineData->SetStringField(TEXT("mime_type"), Image.MimeType);
		InlineData->SetStringField(TEXT("data"), Image.Base64Data);
		ImagePart->SetObjectField(TEXT("inline_data"), InlineData);
		UserParts.Add(MakeShareable(new FJsonValueObject(ImagePart)));
	}
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	SaveChatHistory(ActiveChatID);
	AnalystAttachedImages.Empty();
	RefreshAnalystImagePreview();
	bIsThinking = true;
	RefreshChatHistoryView();
	SendChatRequest();
	return FReply::Handled();
}
const TCHAR* const _obf_C = TEXT("TokenLimitReached");
void SBpGeneratorUltimateWidget::SendChatRequest()
{
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	UE_LOG(LogTemp, Log, TEXT("BP Gen Analyst: Using Slot %d, Provider=%s, Model=%s, URL=%s"),
		FApiKeyManager::Get().GetActiveSlotIndex(), *ProviderStr, *ModelName, *BaseURL);
	bool bIsLimited = false;
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		bIsLimited = true;
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		const int32 DecryptedLimit = _obf_A ^ _obf_B;
		if (CurrentCharacters >= DecryptedLimit)
		{
			FString LimitMessage = TEXT("You have reached the token limit for the built-in API key. Please go to 'API Settings' and enter your own Google Gemini, OpenAI, Anthropic Claude, or DeepSeek API key to continue.");
			TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
			ModelContent->SetStringField(TEXT("role"), TEXT("model"));
			TArray<TSharedPtr<FJsonValue>> ModelParts;
			TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
			ModelPartText->SetStringField(TEXT("text"), LimitMessage);
			ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
			ModelContent->SetArrayField(TEXT("parts"), ModelParts);
			AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
			bIsThinking = false;
			RefreshChatHistoryView();
			return;
		}
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		FString ErrorMessage = FString::Printf(TEXT("No API key configured for %s. Please go to 'API Settings' and enter your API key."), *ProviderStr);
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMessage);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsThinking = false;
		RefreshChatHistoryView();
		return;
	}
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	FString FormattingInstruction = TEXT("You are an expert Unreal Engine assistant. Analyze the provided context. You must format all of your responses using Markdown.\n\n**MANDATORY: CLICKABLE ASSET LINKS**\nWhen mentioning ANY asset name (blueprints, materials, textures, widgets, etc.), you MUST wrap it in a clickable link using this EXACT format: `[AssetName](ue://asset?name=AssetName)`\n\nExamples:\n- Write `[BP_EnemyAI](ue://asset?name=BP_EnemyAI)` not just BP_EnemyAI\n- Write `[M_Wood](ue://asset?name=M_Wood)` not just M_Wood\n- Write `[WBP_Inventory](ue://asset?name=WBP_Inventory)` not just WBP_Inventory\n\nRules:\n- Link EVERY asset name mentioned in plain text\n- Do NOT link inside code blocks (```code```)\n- Use the asset name only, not the full path");
	if (!CustomInstructions.IsEmpty())
	{
		FormattingInstruction += TEXT("\n\n=== USER CUSTOM INSTRUCTIONS ===\n");
		FormattingInstruction += CustomInstructions;
		FormattingInstruction += TEXT("\n=== END CUSTOM INSTRUCTIONS ===");
	}
	FString GddContent = GetGddContentForAI();
	if (!GddContent.IsEmpty())
	{
		FormattingInstruction += TEXT("\n\n");
		FormattingInstruction += GddContent;
	}
	FString AiMemoryContent = GetAiMemoryContentForAI();
	if (!AiMemoryContent.IsEmpty())
	{
		FormattingInstruction += TEXT("\n\n");
		FormattingInstruction += AiMemoryContent;
	}
	if (ProviderStr == TEXT("Custom"))
	{
		FString CustomURL = BaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty())
		{
			CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		}
		HttpRequest->SetURL(CustomURL);
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		FString CustomModel = ModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty())
		{
			CustomModel = TEXT("gpt-3.5-turbo");
		}
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Analyst: Custom API Request - URL=%s, Model=%s"), *CustomURL, *CustomModel);
		TArray<TSharedPtr<FJsonValue>> OpenAiMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", FormattingInstruction);
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		FString AnalystCustomFileContext;
		bool bCustomFileContextAdded = false;
		for (const TSharedPtr<FJsonValue>& MessageValue : AnalystConversationHistory)
		{
			const TSharedPtr<FJsonObject>& GeminiMessage = MessageValue->AsObject();
			FString Role;
			GeminiMessage->TryGetStringField(TEXT("role"), Role);
			if (Role == TEXT("context"))
			{
				FString FileName, FileContent;
				GeminiMessage->TryGetStringField(TEXT("file_name"), FileName);
				GeminiMessage->TryGetStringField(TEXT("content"), FileContent);
				AnalystCustomFileContext += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (GeminiMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				FString Content;
				if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
				{
					if (!bCustomFileContextAdded && !AnalystCustomFileContext.IsEmpty() && Role == TEXT("user"))
					{
						Content = AnalystCustomFileContext + Content;
						bCustomFileContextAdded = true;
					}
					TSharedPtr<FJsonObject> OpenAiMessage = MakeShareable(new FJsonObject);
					OpenAiMessage->SetStringField("role", (Role == TEXT("model")) ? TEXT("assistant") : Role);
					OpenAiMessage->SetStringField("content", Content);
					OpenAiMessages.Add(MakeShareable(new FJsonValueObject(OpenAiMessage)));
				}
			}
		}
		JsonPayload->SetArrayField(TEXT("messages"), OpenAiMessages);
	}
	else if (ProviderStr == TEXT("Gemini"))
	{
		FString FileContextToPrepend;
		TArray<TSharedPtr<FJsonValue>> GeminiContents;
		for (const TSharedPtr<FJsonValue>& Message : AnalystConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role;
			if (MsgObj->TryGetStringField(TEXT("role"), Role))
			{
				if (Role == TEXT("context"))
				{
					FString FileContent;
					if (MsgObj->TryGetStringField(TEXT("content"), FileContent))
					{
						FString FileName;
						MsgObj->TryGetStringField(TEXT("file_name"), FileName);
						FileContextToPrepend += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
					}
					continue;
				}
				GeminiContents.Add(Message);
			}
		}
		bool bFileContextAdded = false;
		for (int32 i = 0; i < GeminiContents.Num(); i++)
		{
			const TSharedPtr<FJsonObject>& MsgObj = GeminiContents[i]->AsObject();
			FString Role;
			if (MsgObj->TryGetStringField(TEXT("role"), Role) && Role == TEXT("user"))
			{
				const TArray<TSharedPtr<FJsonValue>>* PartsArray;
				if (MsgObj->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
				{
					FString OriginalText;
					if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), OriginalText))
					{
						FString NewText = FormattingInstruction + TEXT("\n\n--- CONTEXT & QUESTION ---\n") + FileContextToPrepend + OriginalText;
						TSharedPtr<FJsonObject> ModifiedMessage = MakeShareable(new FJsonObject);
						ModifiedMessage->SetStringField(TEXT("role"), TEXT("user"));
						TArray<TSharedPtr<FJsonValue>> NewParts;
						TSharedPtr<FJsonObject> NewPart = MakeShareable(new FJsonObject);
						NewPart->SetStringField(TEXT("text"), NewText);
						NewParts.Add(MakeShareable(new FJsonValueObject(NewPart)));
						ModifiedMessage->SetArrayField(TEXT("parts"), NewParts);
						GeminiContents[i] = MakeShareable(new FJsonValueObject(ModifiedMessage));
						bFileContextAdded = true;
						break;
					}
				}
			}
		}
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		HttpRequest->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		JsonPayload->SetArrayField(TEXT("contents"), GeminiContents);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Analyst: Gemini API Request with model: %s"), *GeminiModel);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		HttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", FApiKeyManager::Get().GetActiveOpenAIModel());
		TArray<TSharedPtr<FJsonValue>> OpenAiMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", FormattingInstruction);
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		FString AnalystOpenAIFileContext;
		bool bOpenAIFileContextAdded = false;
		for (const TSharedPtr<FJsonValue>& MessageValue : AnalystConversationHistory)
		{
			const TSharedPtr<FJsonObject>& GeminiMessage = MessageValue->AsObject();
			FString Role;
			GeminiMessage->TryGetStringField(TEXT("role"), Role);
			if (Role == TEXT("context"))
			{
				FString FileName, FileContent;
				GeminiMessage->TryGetStringField(TEXT("file_name"), FileName);
				GeminiMessage->TryGetStringField(TEXT("content"), FileContent);
				AnalystOpenAIFileContext += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (GeminiMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				TSharedPtr<FJsonObject> OpenAiMessage = MakeShareable(new FJsonObject);
				OpenAiMessage->SetStringField("role", (Role == TEXT("model")) ? TEXT("assistant") : TEXT("user"));
				TArray<TSharedPtr<FJsonValue>> ContentArray;
				for (const TSharedPtr<FJsonValue>& PartValue : *PartsArray)
				{
					const TSharedPtr<FJsonObject>& PartObj = PartValue->AsObject();
					FString TextContent;
					if (PartObj->TryGetStringField(TEXT("text"), TextContent))
					{
						if (!bOpenAIFileContextAdded && !AnalystOpenAIFileContext.IsEmpty() && Role == TEXT("user"))
						{
							TextContent = AnalystOpenAIFileContext + TextContent;
							bOpenAIFileContextAdded = true;
						}
						TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
						TextPart->SetStringField("type", "text");
						TextPart->SetStringField("text", TextContent);
						ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
					}
					const TSharedPtr<FJsonObject, ESPMode::ThreadSafe>* InlineData = nullptr;
					if (PartObj->TryGetObjectField(TEXT("inline_data"), InlineData))
					{
						FString MimeType = (*InlineData)->GetStringField(TEXT("mime_type"));
						FString Data = (*InlineData)->GetStringField(TEXT("data"));
						TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
						ImagePart->SetStringField("type", "image_url");
						TSharedPtr<FJsonObject> ImageUrl = MakeShareable(new FJsonObject);
						ImageUrl->SetStringField("url", FString::Printf(TEXT("data:%s;base64,%s"), *MimeType, *Data));
						ImagePart->SetObjectField("image_url", ImageUrl);
						ContentArray.Add(MakeShareable(new FJsonValueObject(ImagePart)));
					}
				}
				if (ContentArray.Num() > 0)
				{
					OpenAiMessage->SetArrayField("content", ContentArray);
					OpenAiMessages.Add(MakeShareable(new FJsonValueObject(OpenAiMessage)));
				}
			}
		}
		JsonPayload->SetArrayField(TEXT("messages"), OpenAiMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Analyst: OpenAI API Request"));
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		HttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		HttpRequest->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		HttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), FApiKeyManager::Get().GetActiveClaudeModel());
		JsonPayload->SetNumberField(TEXT("max_tokens"), 4096);
		JsonPayload->SetStringField(TEXT("system"), FormattingInstruction);
		TArray<TSharedPtr<FJsonValue>> ClaudeMessages;
		FString AnalystClaudeFileContext;
		bool bClaudeFileContextAdded = false;
		for (const TSharedPtr<FJsonValue>& MessageValue : AnalystConversationHistory)
		{
			const TSharedPtr<FJsonObject>& GeminiMessage = MessageValue->AsObject();
			FString Role;
			GeminiMessage->TryGetStringField(TEXT("role"), Role);
			if (Role == TEXT("context"))
			{
				FString FileName, FileContent;
				GeminiMessage->TryGetStringField(TEXT("file_name"), FileName);
				GeminiMessage->TryGetStringField(TEXT("content"), FileContent);
				AnalystClaudeFileContext += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (GeminiMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				TSharedPtr<FJsonObject> ClaudeMessage = MakeShareable(new FJsonObject);
				ClaudeMessage->SetStringField("role", (Role == TEXT("model")) ? TEXT("assistant") : TEXT("user"));
				TArray<TSharedPtr<FJsonValue>> ContentArray;
				for (const TSharedPtr<FJsonValue>& PartValue : *PartsArray)
				{
					const TSharedPtr<FJsonObject>& PartObj = PartValue->AsObject();
					FString TextContent;
					if (PartObj->TryGetStringField(TEXT("text"), TextContent))
					{
						if (!bClaudeFileContextAdded && !AnalystClaudeFileContext.IsEmpty() && Role == TEXT("user"))
						{
							TextContent = AnalystClaudeFileContext + TextContent;
							bClaudeFileContextAdded = true;
						}
						TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
						TextPart->SetStringField("type", "text");
						TextPart->SetStringField("text", TextContent);
						ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
					}
					const TSharedPtr<FJsonObject, ESPMode::ThreadSafe>* InlineData = nullptr;
					if (PartObj->TryGetObjectField(TEXT("inline_data"), InlineData))
					{
						FString MimeType = (*InlineData)->GetStringField(TEXT("mime_type"));
						FString Data = (*InlineData)->GetStringField(TEXT("data"));
						TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
						ImagePart->SetStringField("type", "image");
						TSharedPtr<FJsonObject> ImageSource = MakeShareable(new FJsonObject);
						ImageSource->SetStringField("type", "base64");
						ImageSource->SetStringField("media_type", MimeType);
						ImageSource->SetStringField("data", Data);
						ImagePart->SetObjectField("source", ImageSource);
						ContentArray.Add(MakeShareable(new FJsonValueObject(ImagePart)));
					}
				}
				if (ContentArray.Num() > 0)
				{
					ClaudeMessage->SetArrayField("content", ContentArray);
					ClaudeMessages.Add(MakeShareable(new FJsonValueObject(ClaudeMessage)));
				}
			}
		}
		JsonPayload->SetArrayField(TEXT("messages"), ClaudeMessages);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Analyst: Claude API Request"));
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		HttpRequest->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), TEXT("deepseek-chat"));
		TArray<TSharedPtr<FJsonValue>> DeepSeekMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", FormattingInstruction);
		DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		FString AnalystDeepSeekFileContext;
		bool bDeepSeekFileContextAdded = false;
		for (const TSharedPtr<FJsonValue>& MessageValue : AnalystConversationHistory)
		{
			const TSharedPtr<FJsonObject>& GeminiMessage = MessageValue->AsObject();
			FString Role;
			GeminiMessage->TryGetStringField(TEXT("role"), Role);
			if (Role == TEXT("context"))
			{
				FString FileName, FileContent;
				GeminiMessage->TryGetStringField(TEXT("file_name"), FileName);
				GeminiMessage->TryGetStringField(TEXT("content"), FileContent);
				AnalystDeepSeekFileContext += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
				continue;
			}
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (GeminiMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				TSharedPtr<FJsonObject> DeepSeekMessage = MakeShareable(new FJsonObject);
				DeepSeekMessage->SetStringField("role", (Role == TEXT("model")) ? TEXT("assistant") : TEXT("user"));
				TArray<TSharedPtr<FJsonValue>> ContentArray;
				for (const TSharedPtr<FJsonValue>& PartValue : *PartsArray)
				{
					const TSharedPtr<FJsonObject>& PartObj = PartValue->AsObject();
					FString TextContent;
					if (PartObj->TryGetStringField(TEXT("text"), TextContent))
					{
						if (!bDeepSeekFileContextAdded && !AnalystDeepSeekFileContext.IsEmpty() && Role == TEXT("user"))
						{
							TextContent = AnalystDeepSeekFileContext + TextContent;
							bDeepSeekFileContextAdded = true;
						}
						TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
						TextPart->SetStringField("type", "text");
						TextPart->SetStringField("text", TextContent);
						ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
					}
					const TSharedPtr<FJsonObject, ESPMode::ThreadSafe>* InlineData = nullptr;
					if (PartObj->TryGetObjectField(TEXT("inline_data"), InlineData))
					{
						FString MimeType = (*InlineData)->GetStringField(TEXT("mime_type"));
						FString Data = (*InlineData)->GetStringField(TEXT("data"));
						TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
						ImagePart->SetStringField("type", "image_url");
						TSharedPtr<FJsonObject> ImageUrl = MakeShareable(new FJsonObject);
						ImageUrl->SetStringField("url", FString::Printf(TEXT("data:%s;base64,%s"), *MimeType, *Data));
						ImagePart->SetObjectField("image_url", ImageUrl);
						ContentArray.Add(MakeShareable(new FJsonValueObject(ImagePart)));
					}
				}
				if (ContentArray.Num() > 0)
				{
					DeepSeekMessage->SetArrayField("content", ContentArray);
					DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(DeepSeekMessage)));
				}
			}
		}
		JsonPayload->SetArrayField(TEXT("messages"), DeepSeekMessages);
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	HttpRequest->OnProcessRequestComplete().BindSP(this, &SBpGeneratorUltimateWidget::OnApiResponseReceived);
	HttpRequest->ProcessRequest();
}
void SBpGeneratorUltimateWidget::OnApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bIsThinking = false;
	FString AiMessage;
	int32 TotalCharsInInteraction = 0;
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString ProviderStr = FApiKeyManager::Get().GetActiveProvider();
			if (ProviderStr == TEXT("Gemini"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Candidates;
				if (JsonObject->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& CandidateObject = (*Candidates)[0]->AsObject();
					const TSharedPtr<FJsonObject>* Content = nullptr;
					if (CandidateObject->TryGetObjectField(TEXT("content"), Content))
					{
						const TArray<TSharedPtr<FJsonValue>>* Parts = nullptr;
						if ((*Content)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
						{
							(*Parts)[0]->AsObject()->TryGetStringField(TEXT("text"), AiMessage);
						}
					}
					const TSharedPtr<FJsonObject>* UsageMetadata = nullptr;
					if (CandidateObject->TryGetObjectField(TEXT("usageMetadata"), UsageMetadata))
					{
						int32 TotalTokenCount = 0;
						if ((*UsageMetadata)->TryGetNumberField(TEXT("totalTokenCount"), TotalTokenCount))
						{
							UpdateConversationTokens(ActiveChatID, TotalTokenCount, TotalTokenCount);
						}
					}
				}
			}
			else if (ProviderStr == TEXT("OpenAI") || ProviderStr == TEXT("Custom") || ProviderStr == TEXT("DeepSeek"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices;
				if (JsonObject->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& ChoiceObject = (*Choices)[0]->AsObject();
					const TSharedPtr<FJsonObject>* MessageObject = nullptr;
					if (ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject))
					{
						(*MessageObject)->TryGetStringField(TEXT("content"), AiMessage);
					}
				}
				const TSharedPtr<FJsonObject>* UsageObj = nullptr;
				if (JsonObject->TryGetObjectField(TEXT("usage"), UsageObj))
				{
					int32 PromptTokens = 0;
					int32 CompletionTokens = 0;
					(*UsageObj)->TryGetNumberField(TEXT("prompt_tokens"), PromptTokens);
					(*UsageObj)->TryGetNumberField(TEXT("completion_tokens"), CompletionTokens);
					UpdateConversationTokens(ActiveChatID, PromptTokens, CompletionTokens);
				}
			}
			else if (ProviderStr == TEXT("Claude"))
			{
				const TArray<TSharedPtr<FJsonValue>>* ContentBlocks = nullptr;
				if (JsonObject->TryGetArrayField(TEXT("content"), ContentBlocks) && ContentBlocks->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& FirstBlock = (*ContentBlocks)[0]->AsObject();
					FirstBlock->TryGetStringField(TEXT("text"), AiMessage);
				}
				const TSharedPtr<FJsonObject>* UsageObj = nullptr;
				if (JsonObject->TryGetObjectField(TEXT("usage"), UsageObj))
				{
					int32 InputTokens = 0;
					int32 OutputTokens = 0;
					(*UsageObj)->TryGetNumberField(TEXT("input_tokens"), InputTokens);
					(*UsageObj)->TryGetNumberField(TEXT("output_tokens"), OutputTokens);
					UpdateConversationTokens(ActiveChatID, InputTokens, OutputTokens);
				}
			}
		}
	}
	if (AiMessage.IsEmpty())
	{
		AiMessage = FString::Printf(TEXT("Error: Failed to get a valid response from the API.\n\nResponse Code: %d\nResponse Body:\n%s"),
			Response.IsValid() ? Response->GetResponseCode() : 0,
			Response.IsValid() ? *Response->GetContentAsString() : TEXT("N/A"));
	}
	if (FApiKeyManager::Get().GetActiveApiKey().IsEmpty())
	{
		const TSharedPtr<FJsonObject>& LastUserMessage = AnalystConversationHistory.Last()->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* PartsArray;
		if (LastUserMessage->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
		{
			FString Content;
			if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
			{
				TotalCharsInInteraction += Content.Len();
			}
		}
		TotalCharsInInteraction += AiMessage.Len();
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		CurrentCharacters += TotalCharsInInteraction;
		const int32 ValueToStore = CurrentCharacters ^ _obf_B;
		GConfig->SetString(_obf_D, _obf_E, *FString::FromInt(ValueToStore), GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}
	TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
	ModelContent->SetStringField(TEXT("role"), TEXT("model"));
	TArray<TSharedPtr<FJsonValue>> ModelParts;
	TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
	ModelPartText->SetStringField(TEXT("text"), AiMessage);
	ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
	ModelContent->SetArrayField(TEXT("parts"), ModelParts);
	AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	SaveChatHistory(ActiveChatID);
	RefreshChatHistoryView();
	if (AnalystTokenCounterText.IsValid())
	{
		int32 TokenCount = EstimateConversationTokens(AnalystConversationHistory);
		AnalystTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: ~%d"), TokenCount)));
	}
}
void SBpGeneratorUltimateWidget::LoadChatHistory(const FString& ChatID)
{
	AnalystConversationHistory.Empty();
	if (ChatID.IsEmpty())
	{
		RefreshChatHistoryView();
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / ChatID + TEXT(".json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* HistoryArray;
				if (RootObject->TryGetArrayField(TEXT("history"), HistoryArray))
				{
					AnalystConversationHistory = *HistoryArray;
				}
			}
		}
	}
	if (AnalystConversationHistory.Num() == 0 && !ChatID.IsEmpty())
	{
		const FString Greeting = TEXT("Hello! Select a Blueprint, Material, Widget, AnimBP or Behavior Tree and use the context buttons below to get started, or simply ask me a question.");
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), Greeting);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		AnalystConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	}
	RefreshChatHistoryView();
}
void SBpGeneratorUltimateWidget::SaveChatHistory(const FString& ChatID)
{
	if (ChatID.IsEmpty() || AnalystConversationHistory.Num() == 0)
	{
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / ChatID + TEXT(".json");
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField(TEXT("history"), AnalystConversationHistory);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
void SBpGeneratorUltimateWidget::LoadManifest()
{
	ConversationList.Empty();
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("conversations.json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* ManifestArray;
				if (RootObject->TryGetArrayField(TEXT("manifest"), ManifestArray))
				{
					for (const TSharedPtr<FJsonValue>& Value : *ManifestArray)
					{
						const TSharedPtr<FJsonObject>& ConvObject = Value->AsObject();
						if (ConvObject.IsValid())
						{
							TSharedPtr<FConversationInfo> Info = MakeShared<FConversationInfo>();
							ConvObject->TryGetStringField(TEXT("id"), Info->ID);
							ConvObject->TryGetStringField(TEXT("title"), Info->Title);
							ConvObject->TryGetStringField(TEXT("last_updated"), Info->LastUpdated);
							ConvObject->TryGetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
							ConvObject->TryGetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
							ConvObject->TryGetNumberField(TEXT("total_tokens"), Info->TotalTokens);
							ConversationList.Add(Info);
						}
					}
				}
			}
		}
	}
	if (ChatListView.IsValid())
	{
		ChatListView->RequestListRefresh();
	}
	if (ConversationList.Num() > 0)
	{
		ChatListView->SetSelection(ConversationList[0]);
	}
	else
	{
		OnNewChatClicked();
	}
}
FReply SBpGeneratorUltimateWidget::OnInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGlobalKeyPress(InKeyEvent))
	{
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (!InKeyEvent.IsShiftDown())
		{
			OnSendClicked();
			return FReply::Handled();
		}
	}
	if (InKeyEvent.GetKey() == EKeys::V && InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsAltDown())
	{
		if (TryPasteImageFromClipboard(AnalystAttachedImages))
		{
			RefreshAnalystImagePreview();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}
void SBpGeneratorUltimateWidget::SaveManifest()
{
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("conversations.json");
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> ManifestArray;
	for (const TSharedPtr<FConversationInfo>& Info : ConversationList)
	{
		TSharedPtr<FJsonObject> ConvObject = MakeShareable(new FJsonObject);
		ConvObject->SetStringField(TEXT("id"), Info->ID);
		ConvObject->SetStringField(TEXT("title"), Info->Title);
		ConvObject->SetStringField(TEXT("last_updated"), Info->LastUpdated);
		ConvObject->SetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
		ConvObject->SetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
		ConvObject->SetNumberField(TEXT("total_tokens"), Info->TotalTokens);
		ManifestArray.Add(MakeShareable(new FJsonValueObject(ConvObject)));
	}
	RootObject->SetArrayField(TEXT("manifest"), ManifestArray);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
FReply SBpGeneratorUltimateWidget::OnNewChatClicked()
{
	FString NewID = FString::Printf(TEXT("%lld"), FDateTime::UtcNow().ToUnixTimestamp());
	TSharedPtr<FConversationInfo> NewConversation = MakeShared<FConversationInfo>();
	NewConversation->ID = NewID;
	NewConversation->Title = TEXT("New Chat");
	NewConversation->LastUpdated = FDateTime::UtcNow().ToIso8601();
	ConversationList.Insert(NewConversation, 0);
	if (ChatListView.IsValid())
	{
		ChatListView->RequestListRefresh();
		ChatListView->SetSelection(NewConversation);
	}
	SaveManifest();
	return FReply::Handled();
}
void SConversationListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	ConversationInfo = InArgs._ConversationInfo;
	ScribeWidget = InArgs._ScribeWidget;
	ViewType = InArgs._ViewType;
	STableRow<TSharedPtr<FConversationInfo>>::Construct(
		STableRow<TSharedPtr<FConversationInfo>>::FArguments()
		.Content()
		[
			SAssignNew(WidgetSwitcher, SWidgetSwitcher)
				.WidgetIndex(0)
				+ SWidgetSwitcher::Slot()
				[
					SNew(STextBlock)
						.Text_Lambda([this] { return FText::FromString(ConversationInfo->Title); })
						.ToolTipText_Lambda([this] { return FText::FromString(FString::Printf(TEXT("ID: %s\nLast Updated: %s\nTokens: %d"), *ConversationInfo->ID, *ConversationInfo->LastUpdated, ConversationInfo->TotalTokens)); })
				]
				+ SWidgetSwitcher::Slot()
				[
					SAssignNew(RenameTextBox, SEditableTextBox)
						.Text(FText::FromString(ConversationInfo->Title))
						.OnTextCommitted(this, &SConversationListRow::OnRenameTextCommitted)
						.SelectAllTextWhenFocused(true)
				]
		],
		InOwnerTableView
	);
}
void SConversationListRow::EnterEditMode()
{
	if (WidgetSwitcher.IsValid() && RenameTextBox.IsValid())
	{
		RenameTextBox->SetText(FText::FromString(ConversationInfo->Title));
		WidgetSwitcher->SetActiveWidgetIndex(1);
		FSlateApplication::Get().SetKeyboardFocus(RenameTextBox.ToSharedRef(), EFocusCause::SetDirectly);
	}
}
void SConversationListRow::OnRenameTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	if (InCommitType == ETextCommit::OnEnter)
	{
		TSharedPtr<SBpGeneratorUltimateWidget> ScribeWidgetPtr = ScribeWidget.Pin();
		if (ConversationInfo.IsValid() && ScribeWidgetPtr.IsValid())
		{
			ConversationInfo->Title = InText.ToString();
			ConversationInfo->LastUpdated = FDateTime::UtcNow().ToIso8601();
			if (ViewType == EAnalystView::AssetAnalyst)
			{
				ScribeWidgetPtr->SaveManifest();
			}
			else if (ViewType == EAnalystView::ProjectScanner)
			{
				ScribeWidgetPtr->SaveProjectManifest();
			}
			else
			{
				ScribeWidgetPtr->SaveArchitectManifest();
			}
		}
	}
	if (WidgetSwitcher.IsValid())
	{
		WidgetSwitcher->SetActiveWidgetIndex(0);
	}
}
void SBpGeneratorUltimateWidget::ExportConversationToFile(const FString& ChatID, const FString& Title, const FString& FileFormat, const TArray<TSharedPtr<FJsonValue>>& ConversationHistory)
{
	if (ChatID.IsEmpty() || ConversationHistory.Num() == 0)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Export failed: No active chat or history is empty."));
		return;
	}
	TStringBuilder<16384> ContentBuilder;
	for (const TSharedPtr<FJsonValue>& MessageValue : ConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MessageObject = MessageValue->AsObject();
		FString Role, Content;
		if (MessageObject->TryGetStringField(TEXT("role"), Role))
		{
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (MessageObject->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
				{
					if (FileFormat == TEXT("txt"))
					{
						FString Speaker = (Role == TEXT("user")) ? TEXT("You:") : TEXT("AI:");
						ContentBuilder.Appendf(TEXT("%s\n%s\n\n-----------------\n\n"), *Speaker, *Content);
					}
					else
					{
						FString Speaker = (Role == TEXT("user")) ? TEXT("**You:**") : TEXT("**AI:**");
						ContentBuilder.Appendf(TEXT("%s\n\n%s\n\n---\n\n"), *Speaker, *Content);
					}
				}
			}
		}
	}
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("Exported Conversations");
	IFileManager::Get().MakeDirectory(*SaveDirectory, true);
	FString SafeFileName = FPaths::MakeValidFileName(Title);
	if (SafeFileName.IsEmpty()) SafeFileName = ChatID;
	const FString FullPath = SaveDirectory / FString::Printf(TEXT("%s.%s"), *SafeFileName, *FileFormat);
	if (FFileHelper::SaveStringToFile(ContentBuilder.ToString(), *FullPath))
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Successfully exported conversation to: %s"), *FullPath);
		FNotificationInfo Info(FText::FromString(TEXT("Conversation saved successfully!")));
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;
		Info.Hyperlink = FSimpleDelegate::CreateLambda([SaveDirectory]() {
			FPlatformProcess::ExploreFolder(*SaveDirectory);
			});
		Info.HyperlinkText = FText::FromString(TEXT("Open Folder..."));
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Failed to export conversation to: %s"), *FullPath);
		FNotificationInfo Info(FText::FromString(TEXT("Failed to save conversation.")));
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}
FReply SBpGeneratorUltimateWidget::OnImportContextClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Desktop platform not available."));
		return FReply::Handled();
	}
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Text Files (*.txt)|*.txt|Markdown Files (*.md)|*.md|All Files (*.*)|*.*");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Import Context File"),
		FPaths::ProjectSavedDir() / TEXT("Exported Conversations"),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		const FString& FilePath = OutFiles[0];
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			FString FileName = FPaths::GetCleanFilename(FilePath);
			PendingContext = FString::Printf(TEXT("Here is the context from the file '%s':\n\n--- Data ---\n%s\n---"), *FileName, *FileContent);
			FString Msg = UIConfig::GetValue(TEXT("msg_file_context_attached"), TEXT("Context from file '{0}' attached. Now, what is your question?"));
			Msg.ReplaceInline(TEXT("{0}"), *FileName);
			InputTextBox->SetText(FText::FromString(Msg));
		}
		else
		{
			FString ErrorMsg = UIConfig::GetValue(TEXT("error_file_read"), TEXT("Error: Could not read the file '{0}'."));
			ErrorMsg.ReplaceInline(TEXT("{0}"), *FilePath);
			InputTextBox->SetText(FText::FromString(ErrorMsg));
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnImportAnalystContextClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Desktop platform not available."));
		return FReply::Handled();
	}
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Text Files (*.txt)|*.txt|Markdown Files (*.md)|*.md|All Files (*.*)|*.*");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Import Context File"),
		FPaths::ProjectSavedDir() / TEXT("Exported Conversations"),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		const FString& FilePath = OutFiles[0];
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			FString FileName = FPaths::GetCleanFilename(FilePath);
			FAttachedFileContext NewFile;
			NewFile.FileName = FileName;
			NewFile.Content = FileContent;
			NewFile.CharCount = FileContent.Len();
			NewFile.bExpanded = false;
			AnalystAttachedFiles.Add(NewFile);
			FString Msg = UIConfig::GetValue(TEXT("msg_file_context_attached"), TEXT("Context from file '{0}' attached. Now, what is your question?"));
			Msg.ReplaceInline(TEXT("{0}"), *FileName);
			InputTextBox->SetText(FText::FromString(Msg));
			if (AnalystTokenCounterText.IsValid())
			{
				int32 CurrentTokens = 0;
				if (AnalystConversationHistory.Num() > 0)
				{
					CurrentTokens = EstimateConversationTokens(AnalystConversationHistory);
				}
				for (const FAttachedFileContext& File : AnalystAttachedFiles)
				{
					CurrentTokens += File.CharCount / 4;
				}
				AnalystTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: ~%d"), CurrentTokens)));
			}
			SaveChatHistory(ActiveChatID);
		}
		else
		{
			FString ErrorMsg = UIConfig::GetValue(TEXT("error_file_read"), TEXT("Error: Could not read the file '{0}'."));
			ErrorMsg.ReplaceInline(TEXT("{0}"), *FilePath);
			InputTextBox->SetText(FText::FromString(ErrorMsg));
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnAddArchitectAssetContextClicked()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> SelectedAssetsData;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssetsData);
	if (SelectedAssetsData.Num() != 1)
	{
		ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_select_one_asset"), TEXT("Error: Please select exactly one asset in Content Browser."))));
		return FReply::Handled();
	}
	UObject* SelectedObject = SelectedAssetsData[0].GetAsset();
	if (!SelectedObject)
	{
		ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_invalid_asset"), TEXT("Error: The selected asset is invalid or could not be loaded."))));
		return FReply::Handled();
	}
	FString RawSummary;
	FString AssetTypeName;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject))
	{
		RawSummary = FBpSummarizerr::Summarize(Blueprint);
		AssetTypeName = TEXT("Blueprint");
	}
	else if (UMaterial* Material = Cast<UMaterial>(SelectedObject))
	{
		FMaterialGraphDescriber Describer;
		RawSummary = Describer.Describe(Material);
		AssetTypeName = TEXT("Material");
	}
	else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(SelectedObject))
	{
		FBtGraphDescriber Describer;
		RawSummary = Describer.Describe(BehaviorTree);
		AssetTypeName = TEXT("Behavior Tree");
	}
	else
	{
		FString ErrorMsg = UIConfig::GetValue(TEXT("error_unsupported_asset_type"), TEXT("Error: The selected asset ('{0}') is not a supported type for summary (Blueprint, Material, or Behavior Tree)."));
		ErrorMsg.ReplaceInline(TEXT("{0}"), *SelectedObject->GetClass()->GetName());
		ArchitectInputTextBox->SetText(FText::FromString(ErrorMsg));
		return FReply::Handled();
	}
	if (RawSummary.IsEmpty())
	{
		ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_empty_summary"), TEXT("Error: Could not generate a raw summary for selected asset. It might be empty or corrupted."))));
		return FReply::Handled();
	}
	PendingArchitectContext = FString::Printf(TEXT("Here is context for %s '%s':\n\n--- Data ---\n%s\n---"), *AssetTypeName, *SelectedObject->GetName(), *RawSummary);
	FString Msg = UIConfig::GetValue(TEXT("msg_asset_context_attached_architect"), TEXT("Context for asset '{0}' attached. Now, what would you like to do?"));
	Msg.ReplaceInline(TEXT("{0}"), *SelectedObject->GetName());
	ArchitectInputTextBox->SetText(FText::FromString(Msg));
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnAddArchitectNodeContextClicked()
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem) { return FReply::Handled(); }
	IAssetEditorInstance* ActiveEditor = nullptr;
	double LastActivationTime = 0.0;
	for (UObject* Asset : AssetEditorSubsystem->GetAllEditedAssets())
	{
		for (IAssetEditorInstance* Editor : AssetEditorSubsystem->FindEditorsForAsset(Asset))
		{
			if (Editor && Editor->GetLastActivationTime() > LastActivationTime)
			{
				LastActivationTime = Editor->GetLastActivationTime();
				ActiveEditor = Editor;
			}
		}
	}
	if (!ActiveEditor)
	{
		ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_no_active_editor"), TEXT("Error: No active asset editor found. Please open a Blueprint, Material, or Behavior Tree."))));
		return FReply::Handled();
	}
	const FName EditorName = ActiveEditor->GetEditorName();
	TSet<UObject*> SelectedNodes;
	FString NodesSummary;
	bool bFoundEditor = false;
	if (EditorName == FName(TEXT("BlueprintEditor")) || EditorName == FName(TEXT("AnimationBlueprintEditor")))
	{
		FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
		SelectedNodes = BlueprintEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("WidgetBlueprintEditor")))
	{
		FWidgetBlueprintEditor* WidgetEditor = static_cast<FWidgetBlueprintEditor*>(ActiveEditor);
		SelectedNodes = WidgetEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("MaterialEditor")))
	{
		FMaterialEditor* MaterialEditor = static_cast<FMaterialEditor*>(ActiveEditor);
		SelectedNodes = MaterialEditor->GetSelectedNodes();
		bFoundEditor = true;
		FMaterialNodeDescriber Describer;
		NodesSummary = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("Behavior Tree")))
	{
		FBehaviorTreeEditor* BehaviorTreeEditor = static_cast<FBehaviorTreeEditor*>(ActiveEditor);
		SelectedNodes = BehaviorTreeEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBtGraphDescriber Describer;
		NodesSummary = Describer.DescribeSelection(SelectedNodes);
	}
	if (!bFoundEditor)
	{
		FString ErrorMsg = UIConfig::GetValue(TEXT("error_unsupported_editor"), TEXT("Error: The active editor ('{0}') is not supported for node selection."));
		ErrorMsg.ReplaceInline(TEXT("{0}"), *EditorName.ToString());
		ArchitectInputTextBox->SetText(FText::FromString(ErrorMsg));
		return FReply::Handled();
	}
	if (SelectedNodes.Num() == 0)
	{
		ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("error_no_nodes_selected"), TEXT("Error: No nodes are selected in active graph editor."))));
		return FReply::Handled();
	}
	PendingArchitectContext = FString::Printf(TEXT("Here is context for currently selected nodes:\n\n--- Node Data ---\n%s\n---"), *NodesSummary);
	ArchitectInputTextBox->SetText(FText::FromString(UIConfig::GetValue(TEXT("msg_node_context_attached_architect"), TEXT("Context for selected nodes has been attached. Now, what would you like to do?"))));
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnImportArchitectContextClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Desktop platform not available."));
		return FReply::Handled();
	}
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Text Files (*.txt)|*.txt|Markdown Files (*.md)|*.md|All Files (*.*)|*.*");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Import Context File"),
		FPaths::ProjectSavedDir() / TEXT("Exported Conversations"),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		const FString& FilePath = OutFiles[0];
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			FString FileName = FPaths::GetCleanFilename(FilePath);
			FAttachedFileContext NewFile;
			NewFile.FileName = FileName;
			NewFile.Content = FileContent;
			NewFile.CharCount = FileContent.Len();
			NewFile.bExpanded = false;
			ArchitectAttachedFiles.Add(NewFile);
			FString Msg = UIConfig::GetValue(TEXT("msg_file_context_attached_architect"), TEXT("Context from file '{0}' attached. Now, what would you like to do?"));
			Msg.ReplaceInline(TEXT("{0}"), *FileName);
			ArchitectInputTextBox->SetText(FText::FromString(Msg));
			if (ArchitectTokenCounterText.IsValid())
			{
				int32 CurrentTokens = 0;
				TSharedPtr<FConversationInfo>* CurrentConv = ArchitectConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ActiveArchitectChatID; });
				if (CurrentConv != nullptr)
				{
					CurrentTokens = (*CurrentConv)->TotalTokens;
				}
				for (const FAttachedFileContext& File : ArchitectAttachedFiles)
				{
					CurrentTokens += File.CharCount / 4;
				}
				ArchitectTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: ~%d"), CurrentTokens)));
			}
			SaveArchitectChatHistory(ActiveArchitectChatID);
		}
		else
		{
			FString ErrorMsg = UIConfig::GetValue(TEXT("error_file_read"), TEXT("Error: Could not read the file '{0}'."));
			ErrorMsg.ReplaceInline(TEXT("{0}"), *FilePath);
			ArchitectInputTextBox->SetText(FText::FromString(ErrorMsg));
		}
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::LoadAndAttachImage(const FString& ImagePath, TArray<FAttachedImage>& Attachments)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *ImagePath))
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Failed to load image: %s"), *ImagePath);
		return;
	}
	const int64 MaxImageSize = 20 * 1024 * 1024;
	if (FileData.Num() > MaxImageSize)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Image file too large: %s (%d bytes, max is %d bytes)"), *ImagePath, FileData.Num(), MaxImageSize);
		return;
	}
	FString Base64Data = FBase64::Encode(FileData);
	FString Extension = FPaths::GetExtension(ImagePath).ToLower();
	FString MimeType = TEXT("image/png");
	if (Extension == TEXT("jpg") || Extension == TEXT("jpeg")) MimeType = TEXT("image/jpeg");
	else if (Extension == TEXT("gif")) MimeType = TEXT("image/gif");
	else if (Extension == TEXT("bmp")) MimeType = TEXT("image/bmp");
	else if (Extension == TEXT("webp")) MimeType = TEXT("image/webp");
	FAttachedImage NewImage;
	NewImage.Name = FPaths::GetBaseFilename(ImagePath);
	NewImage.MimeType = MimeType;
	NewImage.Base64Data = Base64Data;
	Attachments.Add(NewImage);
}
bool SBpGeneratorUltimateWidget::TryPasteImageFromClipboard(TArray<FAttachedImage>& Attachments)
{
#if PLATFORM_WINDOWS
	if (!::OpenClipboard(nullptr))
	{
		return false;
	}
	HGLOBAL hClipboardData = ::GetClipboardData(CF_DIB);
	if (!hClipboardData)
	{
		::CloseClipboard();
		return false;
	}
	BITMAPINFO* pBitmapInfo = (BITMAPINFO*)::GlobalLock(hClipboardData);
	if (!pBitmapInfo)
	{
		::GlobalUnlock(hClipboardData);
		::CloseClipboard();
		return false;
	}
	BITMAPINFOHEADER& bih = pBitmapInfo->bmiHeader;
	int32 Width = bih.biWidth;
	int32 Height = FMath::Abs(bih.biHeight);
	int32 Stride = ((Width * bih.biBitCount + 31) / 32) * 4;
	uint8* pPixels = (uint8*)pBitmapInfo + sizeof(BITMAPINFOHEADER);
	if (bih.biBitCount <= 8)
	{
		pPixels = (uint8*)pBitmapInfo + sizeof(BITMAPINFOHEADER) + (bih.biClrUsed * sizeof(RGBQUAD));
	}
	TArray<uint8> PNGData;
	bool bSuccess = false;
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (ImageWrapper.IsValid())
	{
		TArray<uint8> RawData;
		RawData.SetNumUninitialized(Stride * Height);
		if (bih.biHeight > 0)
		{
			for (int32 y = 0; y < Height; y++)
			{
				FMemory::Memcpy(RawData.GetData() + y * Stride, pPixels + (Height - 1 - y) * Stride, Stride);
			}
		}
		else
		{
			FMemory::Memcpy(RawData.GetData(), pPixels, RawData.Num());
		}
		ERGBFormat RGBFormat = ERGBFormat::BGRA;
		int32 BitDepth = 8;
		if (bih.biBitCount == 24)
		{
			TArray<uint8> BGRAData;
			BGRAData.SetNumUninitialized(Width * Height * 4);
			for (int32 y = 0; y < Height; y++)
			{
				for (int32 x = 0; x < Width; x++)
				{
					int32 SrcIdx = y * Stride + x * 3;
					int32 DstIdx = y * Width * 4 + x * 4;
					BGRAData[DstIdx + 0] = RawData[SrcIdx + 0];
					BGRAData[DstIdx + 1] = RawData[SrcIdx + 1];
					BGRAData[DstIdx + 2] = RawData[SrcIdx + 2];
					BGRAData[DstIdx + 3] = 255;
				}
			}
			RawData = MoveTemp(BGRAData);
		}
		if (ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), Width, Height, RGBFormat, BitDepth))
		{
			PNGData = ImageWrapper->GetCompressed();
			bSuccess = PNGData.Num() > 0;
		}
	}
	::GlobalUnlock(hClipboardData);
	::CloseClipboard();
	if (bSuccess && PNGData.Num() > 0)
	{
		const int64 MaxImageSize = 20 * 1024 * 1024;
		if (PNGData.Num() > MaxImageSize)
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Clipboard image too large: %d bytes"), PNGData.Num());
			return false;
		}
		FAttachedImage NewImage;
		NewImage.Name = FString::Printf(TEXT("Clipboard_%d"), FDateTime::Now().GetTicks());
		NewImage.MimeType = TEXT("image/png");
		NewImage.Base64Data = FBase64::Encode(PNGData);
		Attachments.Add(NewImage);
		return true;
	}
	return false;
#elif PLATFORM_MAC
	NSPasteboard* Pasteboard = [NSPasteboard generalPasteboard];
	NSArray* Types = [Pasteboard types];
	if (![Types containsObject:NSPasteboardTypeTIFF] && ![Types containsObject:NSPasteboardTypePNG])
	{
		return false;
	}
	NSData* ImageData = nil;
	if ([Types containsObject:NSPasteboardTypePNG])
	{
		ImageData = [Pasteboard dataForType:NSPasteboardTypePNG];
	}
	else if ([Types containsObject:NSPasteboardTypeTIFF])
	{
		ImageData = [Pasteboard dataForType:NSPasteboardTypeTIFF];
	}
	if (!ImageData || [ImageData length] == 0)
	{
		return false;
	}
	const int64 MaxImageSize = 20 * 1024 * 1024;
	if ([ImageData length] > MaxImageSize)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Clipboard image too large: %llu bytes"), (unsigned long long)[ImageData length]);
		return false;
	}
	TArray<uint8> ImageBytes;
	ImageBytes.SetNumUninitialized([ImageData length]);
	FMemory::Memcpy(ImageBytes.GetData(), [ImageData bytes], [ImageData length]);
	FString MimeType = TEXT("image/png");
	if ([Types containsObject:NSPasteboardTypePNG] == NO)
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::TIFF);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(ImageBytes.GetData(), ImageBytes.Num()))
		{
			ImageBytes = ImageWrapper->GetCompressed();
			MimeType = TEXT("image/png");
		}
	}
	FAttachedImage NewImage;
	NewImage.Name = FString::Printf(TEXT("Clipboard_%d"), FDateTime::Now().GetTicks());
	NewImage.MimeType = MimeType;
	NewImage.Base64Data = FBase64::Encode(ImageBytes);
	Attachments.Add(NewImage);
	return true;
#else
	return false;
#endif
}
void SBpGeneratorUltimateWidget::RefreshAnalystImagePreview()
{
	if (!AnalystImagePreviewBox.IsValid()) return;
	AnalystImagePreviewBox->ClearChildren();
	for (int32 i = 0; i < AnalystAttachedImages.Num(); i++)
	{
		const FAttachedImage& Image = AnalystAttachedImages[i];
		AnalystImagePreviewBox->AddSlot().AutoWidth().Padding(4, 0)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("🖼️")))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0).VAlign(VAlign_Center).MaxWidth(150)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Image.Name))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("RemoveImage", "✕"))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnRemoveAnalystImageClicked, i)
						.ButtonColorAndOpacity(FSlateColor(FLinearColor::Red))
						.ForegroundColor(FSlateColor(FLinearColor::White))
						.ToolTipText(LOCTEXT("RemoveImageTooltip", "Remove image"))
				]
		];
	}
}
void SBpGeneratorUltimateWidget::RefreshArchitectImagePreview()
{
	if (!ArchitectImagePreviewBox.IsValid()) return;
	ArchitectImagePreviewBox->ClearChildren();
	for (int32 i = 0; i < ArchitectAttachedImages.Num(); i++)
	{
		const FAttachedImage& Image = ArchitectAttachedImages[i];
		ArchitectImagePreviewBox->AddSlot().AutoWidth().Padding(4, 0)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("🖼️")))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0).VAlign(VAlign_Center).MaxWidth(150)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Image.Name))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("RemoveImage", "✕"))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnRemoveArchitectImageClicked, i)
						.ButtonColorAndOpacity(FSlateColor(FLinearColor::Red))
						.ForegroundColor(FSlateColor(FLinearColor::White))
						.ToolTipText(LOCTEXT("RemoveImageTooltip", "Remove image"))
				]
		];
	}
}
void SBpGeneratorUltimateWidget::RefreshProjectImagePreview()
{
	if (!ProjectImagePreviewBox.IsValid()) return;
	ProjectImagePreviewBox->ClearChildren();
	for (int32 i = 0; i < ProjectAttachedImages.Num(); i++)
	{
		const FAttachedImage& Image = ProjectAttachedImages[i];
		ProjectImagePreviewBox->AddSlot().AutoWidth().Padding(4, 0)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("🖼️")))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0).VAlign(VAlign_Center).MaxWidth(150)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Image.Name))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("RemoveImage", "✕"))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnRemoveProjectImageClicked, i)
						.ButtonColorAndOpacity(FSlateColor(FLinearColor::Red))
						.ForegroundColor(FSlateColor(FLinearColor::White))
						.ToolTipText(LOCTEXT("RemoveImageTooltip", "Remove image"))
				]
		];
	}
}
FReply SBpGeneratorUltimateWidget::OnAttachAnalystImageClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Image Files|*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.webp|");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Select Images"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::Multiple,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		for (const FString& File : OutFiles)
		{
			LoadAndAttachImage(File, AnalystAttachedImages);
		}
		RefreshAnalystImagePreview();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnAttachArchitectImageClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Image Files|*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.webp|");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Select Images"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::Multiple,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		for (const FString& File : OutFiles)
		{
			LoadAndAttachImage(File, ArchitectAttachedImages);
		}
		RefreshArchitectImagePreview();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnAttachProjectImageClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Image Files|*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.webp|");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Select Images"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::Multiple,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		for (const FString& File : OutFiles)
		{
			LoadAndAttachImage(File, ProjectAttachedImages);
		}
		RefreshProjectImagePreview();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnRemoveAnalystImageClicked(int32 ImageIndex)
{
	if (AnalystAttachedImages.IsValidIndex(ImageIndex))
	{
		AnalystAttachedImages.RemoveAt(ImageIndex);
		RefreshAnalystImagePreview();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnRemoveArchitectImageClicked(int32 ImageIndex)
{
	if (ArchitectAttachedImages.IsValidIndex(ImageIndex))
	{
		ArchitectAttachedImages.RemoveAt(ImageIndex);
		RefreshArchitectImagePreview();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnRemoveProjectImageClicked(int32 ImageIndex)
{
	if (ProjectAttachedImages.IsValidIndex(ImageIndex))
	{
		ProjectAttachedImages.RemoveAt(ImageIndex);
		RefreshProjectImagePreview();
	}
	return FReply::Handled();
}
FReply SConversationListRow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TSharedPtr<SBpGeneratorUltimateWidget> ScribeWidgetPtr = ScribeWidget.Pin();
		if (ScribeWidgetPtr.IsValid())
		{
			TSharedPtr<SWidget> ContextMenu = ScribeWidgetPtr->OnGetContextMenuForChatList(ViewType);
			if (ContextMenu.IsValid())
			{
				FSlateApplication::Get().PushMenu(
					AsShared(),
					FWidgetPath(),
					ContextMenu.ToSharedRef(),
					MouseEvent.GetScreenSpacePosition(),
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
				);
				return FReply::Handled();
			}
		}
	}
	return STableRow<TSharedPtr<FConversationInfo>>::OnMouseButtonUp(MyGeometry, MouseEvent);
}
TSharedRef<ITableRow> SBpGeneratorUltimateWidget::OnGenerateRowForChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SConversationListRow, OwnerTable)
		.ConversationInfo(InItem)
		.ScribeWidget(StaticCastSharedRef<SBpGeneratorUltimateWidget>(AsShared()))
	    .ViewType(EAnalystView::AssetAnalyst);
}
void SBpGeneratorUltimateWidget::OnChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Selected new chat with ID: %s"), *InItem->ID);
		ActiveChatID = InItem->ID;
		LoadChatHistory(ActiveChatID);
		int32 TokenCount = InItem->TotalTokens;
		if (TokenCount == 0 && AnalystConversationHistory.Num() > 0)
		{
			TokenCount = EstimateConversationTokens(AnalystConversationHistory);
		}
		if (AnalystTokenCounterText.IsValid())
		{
			AnalystTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: %d"), TokenCount)));
		}
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Chat selection cleared."));
		ActiveChatID.Empty();
		AnalystConversationHistory.Empty();
		RefreshChatHistoryView();
		if (AnalystTokenCounterText.IsValid())
		{
			AnalystTokenCounterText->SetText(FText::FromString(TEXT("Tokens: 0")));
		}
	}
}
TSharedPtr<SWidget> SBpGeneratorUltimateWidget::OnGetContextMenuForChatList(EAnalystView ForView)
{
	TArray<TSharedPtr<FConversationInfo>> SelectedItems;
	TSharedPtr<FConversationInfo> SelectedItem;
	if (ForView == EAnalystView::AssetAnalyst)
	{
		SelectedItems = ChatListView->GetSelectedItems();
	}
	else if (ForView == EAnalystView::ProjectScanner)
	{
		SelectedItems = ProjectChatListView->GetSelectedItems();
	}
	else
	{
		SelectedItems = ArchitectChatListView->GetSelectedItems();
	}
	if (SelectedItems.Num() == 0) return nullptr;
	SelectedItem = SelectedItems[0];
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr);
	MenuBuilder.AddMenuEntry(LOCTEXT("RenameChatLabel", "Rename"), FText(), FSlateIcon(), FUIAction(
		FExecuteAction::CreateLambda([this, SelectedItem, ForView]() {
			TSharedPtr<SListView<TSharedPtr<FConversationInfo>>> TargetListView;
			if (ForView == EAnalystView::AssetAnalyst)
			{
				TargetListView = ChatListView;
			}
			else if (ForView == EAnalystView::ProjectScanner)
			{
				TargetListView = ProjectChatListView;
			}
			else
			{
				TargetListView = ArchitectChatListView;
			}
			if (SelectedItem.IsValid() && TargetListView.IsValid())
			{
				TSharedPtr<ITableRow> RowWidget = TargetListView->WidgetFromItem(SelectedItem);
				if (RowWidget.IsValid())
				{
					StaticCastSharedPtr<SConversationListRow>(RowWidget)->EnterEditMode();
				}
			}
			})
	));
	MenuBuilder.AddMenuEntry(LOCTEXT("DeleteChatLabel", "Delete"), FText(), FSlateIcon(), FUIAction(
		FExecuteAction::CreateLambda([this, SelectedItem, ForView]() {
			if (!SelectedItem.IsValid()) return;
			if (ForView == EAnalystView::AssetAnalyst)
			{
				ConversationList.Remove(SelectedItem);
				SaveManifest();
				ChatListView->RequestListRefresh();
				FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / SelectedItem->ID + TEXT(".json");
				IFileManager::Get().Delete(*FilePath);
				if (ActiveChatID == SelectedItem->ID)
				{
				}
			}
			else if (ForView == EAnalystView::ProjectScanner)
			{
				ProjectConversationList.Remove(SelectedItem);
				SaveProjectManifest();
				ProjectChatListView->RequestListRefresh();
				FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("project_chats") / SelectedItem->ID + TEXT(".json");
				IFileManager::Get().Delete(*FilePath);
				if (ActiveProjectChatID == SelectedItem->ID)
				{
				}
			}
			else
			{
				ArchitectConversationList.Remove(SelectedItem);
				SaveArchitectManifest();
				ArchitectChatListView->RequestListRefresh();
				FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("architect_chats") / SelectedItem->ID + TEXT(".json");
				IFileManager::Get().Delete(*FilePath);
				if (ActiveArchitectChatID == SelectedItem->ID)
				{
				}
			}
			})
	));
	MenuBuilder.BeginSection("ExportActions", LOCTEXT("ExportActionsSection", "Export"));
	{
		auto CanExportLambda = [this, SelectedItem, ForView]() {
			return SelectedItem.IsValid() &&
				((ForView == EAnalystView::AssetAnalyst && ActiveChatID == SelectedItem->ID) ||
					(ForView == EAnalystView::ProjectScanner && ActiveProjectChatID == SelectedItem->ID) ||
					(ForView == EAnalystView::BlueprintArchitect && ActiveArchitectChatID == SelectedItem->ID));
			};
		auto GetHistoryForView = [this, ForView]() -> TArray<TSharedPtr<FJsonValue>>* {
			if (ForView == EAnalystView::AssetAnalyst)
			{
				return &AnalystConversationHistory;
			}
			else if (ForView == EAnalystView::ProjectScanner)
			{
				return &ProjectConversationHistory;
			}
			else
			{
				return &ArchitectConversationHistory;
			}
		};
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ExportTxtLabel", "Export as .txt"),
			LOCTEXT("ExportTxtTooltip", "Saves the conversation as a plain text file."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, SelectedItem, GetHistoryForView]() {
					if (SelectedItem.IsValid())
					{
						TArray<TSharedPtr<FJsonValue>>* HistoryToExport = GetHistoryForView();
						ExportConversationToFile(SelectedItem->ID, SelectedItem->Title, TEXT("txt"), *HistoryToExport);
					}
					}),
				FCanExecuteAction::CreateLambda(CanExportLambda)
			)
		);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ExportMdLabel", "Export as .md"),
			LOCTEXT("ExportMdTooltip", "Saves the conversation as a Markdown file."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this, SelectedItem, GetHistoryForView]() {
					if (SelectedItem.IsValid())
					{
						TArray<TSharedPtr<FJsonValue>>* HistoryToExport = GetHistoryForView();
						ExportConversationToFile(SelectedItem->ID, SelectedItem->Title, TEXT("md"), *HistoryToExport);
					}
					}),
				FCanExecuteAction::CreateLambda(CanExportLambda)
			)
		);
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}
void SBpGeneratorUltimateWidget::LoadProjectManifest()
{
	ProjectConversationList.Empty();
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("project_conversations.json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* ManifestArray;
				if (RootObject->TryGetArrayField(TEXT("manifest"), ManifestArray))
				{
					for (const TSharedPtr<FJsonValue>& Value : *ManifestArray)
					{
						const TSharedPtr<FJsonObject>& ConvObject = Value->AsObject();
						if (ConvObject.IsValid())
						{
							TSharedPtr<FConversationInfo> Info = MakeShared<FConversationInfo>();
							ConvObject->TryGetStringField(TEXT("id"), Info->ID);
							ConvObject->TryGetStringField(TEXT("title"), Info->Title);
							ConvObject->TryGetStringField(TEXT("last_updated"), Info->LastUpdated);
							ConvObject->TryGetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
							ConvObject->TryGetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
							ConvObject->TryGetNumberField(TEXT("total_tokens"), Info->TotalTokens);
							ProjectConversationList.Add(Info);
						}
					}
				}
			}
		}
	}
	if (ProjectChatListView.IsValid())
	{
		ProjectChatListView->RequestListRefresh();
	}
	if (ProjectConversationList.Num() > 0)
	{
		ProjectChatListView->SetSelection(ProjectConversationList[0]);
	}
	else
	{
		OnNewProjectChatClicked();
	}
}
FReply SBpGeneratorUltimateWidget::OnNewProjectChatClicked()
{
	FString NewID = FString::Printf(TEXT("proj_%lld"), FDateTime::UtcNow().ToUnixTimestamp());
	TSharedPtr<FConversationInfo> NewConversation = MakeShared<FConversationInfo>();
	NewConversation->ID = NewID;
	NewConversation->Title = TEXT("New Project Chat");
	NewConversation->LastUpdated = FDateTime::UtcNow().ToIso8601();
	ProjectConversationList.Insert(NewConversation, 0);
	if (ProjectChatListView.IsValid())
	{
		ProjectChatListView->RequestListRefresh();
		ProjectChatListView->SetSelection(NewConversation);
	}
	SaveProjectManifest();
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::SaveProjectManifest()
{
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("project_conversations.json");
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> ManifestArray;
	for (const TSharedPtr<FConversationInfo>& Info : ProjectConversationList)
	{
		TSharedPtr<FJsonObject> ConvObject = MakeShareable(new FJsonObject);
		ConvObject->SetStringField(TEXT("id"), Info->ID);
		ConvObject->SetStringField(TEXT("title"), Info->Title);
		ConvObject->SetStringField(TEXT("last_updated"), Info->LastUpdated);
		ConvObject->SetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
		ConvObject->SetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
		ConvObject->SetNumberField(TEXT("total_tokens"), Info->TotalTokens);
		ManifestArray.Add(MakeShareable(new FJsonValueObject(ConvObject)));
	}
	RootObject->SetArrayField(TEXT("manifest"), ManifestArray);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
void SBpGeneratorUltimateWidget::OnProjectChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		ActiveProjectChatID = InItem->ID;
		LoadProjectChatHistory(ActiveProjectChatID);
	}
	else
	{
		ActiveProjectChatID.Empty();
		ProjectConversationHistory.Empty();
		RefreshProjectChatView();
	}
}
TSharedRef<ITableRow> SBpGeneratorUltimateWidget::OnGenerateRowForProjectChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SConversationListRow, OwnerTable)
		.ConversationInfo(InItem)
		.ScribeWidget(StaticCastSharedRef<SBpGeneratorUltimateWidget>(AsShared()))
		.ViewType(EAnalystView::ProjectScanner);
}
void SBpGeneratorUltimateWidget::SaveProjectChatHistory(const FString& ChatID)
{
	if (ChatID.IsEmpty() || ProjectConversationHistory.Num() == 0)
	{
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("project_chats") / ChatID + TEXT(".json");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField(TEXT("history"), ProjectConversationHistory);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
void SBpGeneratorUltimateWidget::LoadProjectChatHistory(const FString& ChatID)
{
	ProjectConversationHistory.Empty();
	if (ChatID.IsEmpty())
	{
		RefreshProjectChatView();
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("project_chats") / ChatID + TEXT(".json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* HistoryArray;
				if (RootObject->TryGetArrayField(TEXT("history"), HistoryArray))
				{
					ProjectConversationHistory = *HistoryArray;
				}
			}
		}
	}
	if (ProjectConversationHistory.Num() == 0 && !ChatID.IsEmpty())
	{
		const FString Greeting = TEXT("Hello! You can ask me questions about your project now that it has been scanned.");
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), Greeting);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	}
	RefreshProjectChatView();
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateProjectScannerHowToUseWidget()
{
	const FLinearColor HeaderColor(0.0f, 0.5f, 1.0f);
	const FLinearColor AccentColor(0.3f, 0.7f, 1.0f);
	const FMargin SectionPadding(0, 0, 0, 15);
	const FMargin SubPadding(15, 0, 0, 8);
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 15)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScannerTitle", "Project Scanner - How to Use"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("The Project Scanner indexes your entire project so you can ask high-level questions about your codebase. It understands Blueprint relationships, asset dependencies, and project architecture.")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScanner_HowItWorks", "HOW IT WORKS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("1. Click 'Scan and Index Project' - This reads all Blueprints, Widgets, AnimBPs, Behavior Trees, Materials, Enums, Structs, and DataAssets\n\n") TEXT("2. Wait for completion - Large projects may take a minute\n\n") TEXT("3. Ask questions - The AI answers based on the scanned index")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScanner_Examples", "EXAMPLE QUESTIONS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - \"Where is the player's health variable managed?\"\n") TEXT("  - \"Find all Blueprints that use timers\"\n") TEXT("  - \"Give me a performance report of Blueprints using Event Tick\"\n") TEXT("  - \"Which Behavior Tree handles enemy AI?\"\n") TEXT("  - \"Show me all assets that reference BP_Player\"\n") TEXT("  - \"Create a pie chart of asset types in my project\"")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScanner_Diagrams", "VISUAL GRAPHS & DIAGRAMS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Generate pie charts to visualize asset distributions\n") TEXT("  - Create flowcharts for architecture overviews\n") TEXT("  - Click diagrams to view fullscreen or pop out to separate window\n") TEXT("  - Zoom with CTRL+mouse wheel")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScanner_Actions", "QUICK ACTIONS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Performance Report - Analyze Event Tick usage and expensive operations\n") TEXT("  - Project Overview - Get a summary of your project structure")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseScanner_Tips", "TIPS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Re-scan after making significant structural changes\n") TEXT("  - Use @ to reference specific assets in your questions\n") TEXT("  - Ask for diagrams to visualize complex relationships")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(LOCTEXT("HowToUseScanner_Note", "Note: The AI answers based ONLY on the scanned index. It cannot see real-time changes until you re-scan."))
								.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
						]
				]
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateBlueprintArchitectHowToUseWidget()
{
	const FLinearColor HeaderColor(0.0f, 0.7f, 0.3f);
	const FLinearColor AccentColor(0.25f, 0.88f, 0.82f);
	const FMargin SectionPadding(0, 0, 0, 15);
	const FMargin SubPadding(15, 0, 0, 8);
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 15)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitectTitle", "Blueprint Architect - How to Use"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("The Architect is your AI assistant for creating and modifying Unreal Engine content. Just describe what you want - no technical knowledge required.\n\nTip: If you get stuck, ask \"What tools do you have?\" and the Architect will explain its capabilities.")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_BigFeatures", "BIG FEATURES"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Blueprints", "Blueprints & Actors"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Create new Blueprints (characters, props, game logic, etc.)\n") TEXT("  - Generate Blueprint logic and node graphs\n") TEXT("  - Add components (meshes, cameras, audio, particles)\n") TEXT("  - Create variables, functions, and events\n") TEXT("  - Spawn actors into your level")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Widgets", "Widget/UI Creation"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Create complete UI layouts (menus, HUDs, health bars, inventory)\n") TEXT("  - Add buttons, text, images, panels, and more\n") TEXT("  - Edit widget properties and styling\n") TEXT("  - Build responsive layouts")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Materials", "Materials & Textures"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Create materials from scratch\n") TEXT("  - Generate AI textures from text descriptions\n") TEXT("  - Generate full PBR materials (Base Color, Normal, Roughness, Metallic, AO)\n") TEXT("  - Create materials from existing texture files")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_3DModels", "3D Model Generation"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Generate 3D models from text descriptions\n") TEXT("  - Convert images to 3D models\n") TEXT("  - Preview and refine quality")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Analysis", "Blueprint Analysis"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Explain what a Blueprint does\n") TEXT("  - Describe selected nodes in a graph\n") TEXT("  - Analyze materials and behavior trees\n") TEXT("  - Get asset summaries")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Diagrams", "Visual Graphs & Diagrams"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(AccentColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("  - Generate pie charts to visualize data distributions\n") TEXT("  - Create flowcharts for logic flows and state machines\n") TEXT("  - Click diagrams to view fullscreen or pop out to separate window\n") TEXT("  - Zoom with CTRL+mouse wheel")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_HelpfulFeatures", "HELPFUL FEATURES"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("Asset Management: Move assets between folders, create folders, find assets by name, list assets in a folder")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("Data Structures: Create custom data types (structs), dropdown lists (enums), and data tables")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("Input System: Create input actions (jump, shoot, interact) and input mappings (keyboard, gamepad)")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("Graph Editing: Insert generated code at selected nodes, delete nodes, undo last generated code")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("C++ Support: Read C++ source files, list plugins and their files (for code projects)")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Tips", "USAGE TIPS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("1. Be specific - \"Create a first-person character with walking, jumping, and shooting\" works better than \"make a character\"\n\n") TEXT("2. Use @ to reference assets - Type @ to mention existing blueprints in your project for context\n\n") TEXT("3. Attach files - Import code or text files for additional context\n\n") TEXT("4. Select a blueprint in the content browser and prompt the AI with: 'In the selected blueprint do XY...'\n\n") TEXT("5. Switch API slots - Press Left ALT+1-9 to switch between different AI providers\n\n") TEXT("6. Stop button - Cancel ongoing requests if needed\n\n") TEXT("7. Contact the developer on Discord, Email, or the Website's Community if you have issues")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SectionPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUseArchitect_Shortcuts", "KEYBOARD SHORTCUTS"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									TEXT("Left ALT + 1-9  =  Switch API slot\n") TEXT("Enter  =  Send message\n") TEXT("@  =  Reference asset")
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(LOCTEXT("HowToUseArchitect_Note", "Note: The Architect creates real assets in your project. Always describe what you want clearly and specify paths when needed."))
								.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
						]
				]
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::Create3DModelGenHowToUseWidget()
{
	const FLinearColor HeaderColor(0.9f, 0.5f, 0.1f);
	const FMargin StepPadding(0, 0, 0, 15);
	const FMargin SubStepPadding(20, 0, 0, 10);
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 20)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGenTitle", "How to Use 3D Model Generation"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header1", "Text-to-3D Generation"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(LOCTEXT("HowToUse3DGen_Step1_1", "Generate 3D models directly from text descriptions using the Meshy API."))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Steps:\n")) +
									FString(TEXT("  1. Enter a detailed description of your 3D model\n")) +
									FString(TEXT("  2. Optionally specify a custom model name (e.g., SM_MyModel)\n")) +
									FString(TEXT("  3. Choose quality settings: Preview (fast) or Full (PBR textures)\n")) +
									FString(TEXT("  4. Click Generate and wait for the model to complete\n")) +
									FString(TEXT("  5. Model auto-imports as a Static Mesh to your Content Browser"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header2", "Image-to-3D Conversion"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(LOCTEXT("HowToUse3DGen_Step2_1", "Convert 2D images into 3D models. Upload concept art or reference images to generate 3D assets."))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Use the AI Chat with the image_to_model_3d tool:\n")) +
									FString(TEXT("  # Provide the image path\n")) +
									FString(TEXT("  # Optionally add a prompt to guide the conversion\n")) +
									FString(TEXT("  # Specify model name and save path"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header3", "Model Settings"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Model Types:\n")) +
									FString(TEXT("  # Meshy v4.0 (3D) - Latest model for general 3D generation\n")) +
									FString(TEXT("  # Meshy v3.0 (3D + Texture) - Includes texture generation\n")) +
									FString(TEXT("  # Custom - Use your own endpoint"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Quality Options:\n")) +
									FString(TEXT("  # Preview - Fast generation, lower quality (good for iteration)\n")) +
									FString(TEXT("  # Full - High quality with PBR textures (auto-refined if enabled)\n\n")) +
									FString(TEXT("Auto-Refine: Automatically refine preview models to full quality"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header4", "AI Chat Tools"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Available Tools:\n")) +
									FString(TEXT("  # create_model_3d - Generate 3D model from text\n")) +
									FString(TEXT("  # refine_model_3d - Refine a preview to full quality\n")) +
									FString(TEXT("  # image_to_model_3d - Convert image to 3D model"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header5", "Tips & Best Practices"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("Prompt Writing:\n")) +
									FString(TEXT("  # Be specific about shape, size, and details\n")) +
									FString(TEXT("  # Mention materials (metal, wood, fabric, etc.)\n")) +
									FString(TEXT("  # Describe pose and orientation for characters\n\n")) +
									FString(TEXT("When to Use Preview vs Full:\n")) +
									FString(TEXT("  # Preview: Quick iterations, testing concepts\n")) +
									FString(TEXT("  # Full: Final assets, production-ready models"))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(StepPadding)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("HowToUse3DGen_Header6", "Example Prompts"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(HeaderColor)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(SubStepPadding)
						[
							SNew(STextBlock).AutoWrapText(true)
								.Text(FText::FromString(
									FString(TEXT("\"A futuristic sci-fi crate with glowing blue edges, metallic surface, 50cm x 50cm x 50cm\"\n\n")) +
									FString(TEXT("\"A wooden medieval chair with intricate carvings, worn leather seat\"\n\n")) +
									FString(TEXT("\"A low-poly fantasy sword with a crystal embedded in the hilt\"\n\n")) +
									FString(TEXT("\"A robotic arm with 3 segments, metallic gray with orange highlights\""))
								))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0)
							[
								SNew(STextBlock).AutoWrapText(true)
									.Text(LOCTEXT("HowToUse3DGen_Note", "Note: 3D generation requires a Meshy API key. Configure your API key in the settings panel. Generation typically takes 1-3 minutes for Preview mode."))
									.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
							]
				]
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateCustomInstructionsWidget()
{
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 15)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("CustomInstructionsTitle", "Custom Instructions"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
				[
					SNew(STextBlock)
						.AutoWrapText(true)
						.Text(LOCTEXT("CustomInstructionsDesc", "Set persistent instructions that will be added to every AI request. These rules will help guide the AI's behavior."))
						.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 15)
				[
					SNew(STextBlock)
						.AutoWrapText(true)
						.Text(FText::FromString(
							FString(TEXT("Examples:\n")) +
							FString(TEXT("• \"Always explain your reasoning before generating code.\"\n")) +
							FString(TEXT("• \"Verify with the user before making changes to existing Blueprints.\"\n")) +
							FString(TEXT("• \"Use descriptive variable names, not generic names like Var1, Var2.\"\n")) +
							FString(TEXT("• \"Use formal language like we're friends since childhood.\""))
						))
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 0, 10)
				[
					SAssignNew(CustomInstructionsInput, SMultiLineEditableTextBox)
						.HintText(LOCTEXT("CustomInstructionsHint", "Enter your custom instructions here..."))
						.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().Padding(0, 0, 5, 0)
						[
							SNew(SButton)
								.Text(LOCTEXT("SaveCustomInstructions", "Save & Close"))
								.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseCustomInstructionsClicked)
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SButton)
								.Text(LOCTEXT("CancelCustomInstructions", "Cancel"))
								.OnClicked(this, &SBpGeneratorUltimateWidget::OnCustomInstructionsClicked)
						]
				]
		];
}
void SBpGeneratorUltimateWidget::SaveGddManifest()
{
	FString GddDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*GddDir))
	{
		PlatformFile.CreateDirectoryTree(*GddDir);
	}
	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
	JsonObj->SetNumberField(TEXT("Version"), 1);
	TArray<TSharedPtr<FJsonValue>> FilesArray;
	for (const TSharedPtr<FGddFileEntry>& FilePtr : GddFiles)
	{
		if (!FilePtr.IsValid()) continue;
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("FileId"), FilePtr->FileId.ToString());
		FileObj->SetStringField(TEXT("FileName"), FilePtr->FileName);
		FileObj->SetStringField(TEXT("FilePath"), FilePtr->FilePath);
		FileObj->SetNumberField(TEXT("CharCount"), FilePtr->CharCount);
		FileObj->SetNumberField(TEXT("EstimatedTokens"), FilePtr->EstimatedTokens);
		FileObj->SetBoolField(TEXT("bEnabled"), FilePtr->bEnabled);
		FileObj->SetStringField(TEXT("LastModified"), FilePtr->LastModified.ToString());
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	JsonObj->SetArrayField(TEXT("Files"), FilesArray);
	FString ManifestPath = GddDir / TEXT("gdd_manifest.json");
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(JsonString, *ManifestPath);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("GDD manifest saved: %d files"), GddFiles.Num());
}
void SBpGeneratorUltimateWidget::LoadGddManifest()
{
	FString ManifestPath = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd") / TEXT("gdd_manifest.json");
	if (!FPaths::FileExists(ManifestPath))
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("No GDD manifest found, starting fresh"));
		return;
	}
	FString JsonString;
	FFileHelper::LoadFileToString(JsonString, *ManifestPath);
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid())
	{
		GddFiles.Empty();
		const TArray<TSharedPtr<FJsonValue>>* FilesArray;
		if (JsonObj->TryGetArrayField(TEXT("Files"), FilesArray))
		{
			for (const TSharedPtr<FJsonValue>& FileValue : *FilesArray)
			{
				TSharedPtr<FJsonObject> FileObj = FileValue->AsObject();
				if (!FileObj.IsValid()) continue;
				TSharedPtr<FGddFileEntry> File = MakeShareable(new FGddFileEntry);
				FString FileIdStr;
				if (FileObj->TryGetStringField(TEXT("FileId"), FileIdStr))
				{
					File->FileId = FGuid(FileIdStr);
				}
				FileObj->TryGetStringField(TEXT("FileName"), File->FileName);
				FileObj->TryGetStringField(TEXT("FilePath"), File->FilePath);
				FileObj->TryGetNumberField(TEXT("CharCount"), File->CharCount);
				FileObj->TryGetNumberField(TEXT("EstimatedTokens"), File->EstimatedTokens);
				FileObj->TryGetBoolField(TEXT("bEnabled"), File->bEnabled);
				FString LastModifiedStr;
				if (FileObj->TryGetStringField(TEXT("LastModified"), LastModifiedStr))
				{
					FDateTime::Parse(LastModifiedStr, File->LastModified);
				}
				GddFiles.Add(File);
			}
		}
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("GDD manifest loaded: %d files"), GddFiles.Num());
	}
}
FString SBpGeneratorUltimateWidget::GetGddContentForAI() const
{
	if (GddFiles.Num() == 0)
	{
		return FString();
	}
	bool bHasEnabledFiles = false;
	for (const TSharedPtr<FGddFileEntry>& File : GddFiles)
	{
		if (File.IsValid() && File->bEnabled)
		{
			bHasEnabledFiles = true;
			break;
		}
	}
	if (!bHasEnabledFiles)
	{
		return FString();
	}
	FString Content = TEXT("=== GAME DESIGN DOCUMENT CONTEXT ===\n\n");
	for (const TSharedPtr<FGddFileEntry>& File : GddFiles)
	{
		if (!File.IsValid() || !File->bEnabled) continue;
		FString FullPath = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd") / File->FilePath;
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FullPath))
		{
			Content += FString::Printf(TEXT("--- Section: %s ---\n%s\n\n"), *File->FileName, *FileContent);
		}
	}
	Content += TEXT("=== END GDD CONTEXT ===\n");
	return Content;
}
int32 SBpGeneratorUltimateWidget::GetGddTokenCount() const
{
	int32 TotalTokens = 0;
	for (const TSharedPtr<FGddFileEntry>& File : GddFiles)
	{
		if (File.IsValid() && File->bEnabled)
		{
			TotalTokens += File->EstimatedTokens;
		}
	}
	return TotalTokens;
}
FReply SBpGeneratorUltimateWidget::OnGddClicked()
{
	if (bIsShowingGddOverlay)
	{
		bIsShowingGddOverlay = false;
		if (GddOverlay.IsValid())
		{
			GddOverlay->SetVisibility(EVisibility::Collapsed);
		}
	}
	else
	{
		bIsShowingGddOverlay = true;
		if (GddOverlay.IsValid())
		{
			GddOverlay->SetContent(CreateGddWidget());
			GddOverlay->SetVisibility(EVisibility::Visible);
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCloseGddClicked()
{
	bIsShowingGddOverlay = false;
	if (GddOverlay.IsValid())
	{
		GddOverlay->SetVisibility(EVisibility::Collapsed);
	}
	return FReply::Handled();
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateGddWidget()
{
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 15)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("GddTitle", "Game Design Documents"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("GddHelpBtn", "?"))
						.ToolTipText(LOCTEXT("GddHelpTooltip", "Toggle help information"))
						.ButtonColorAndOpacity(FLinearColor(0.2f, 0.4f, 0.6f))
						.OnClicked_Lambda([this]() {
							bShowingGddHelp = !bShowingGddHelp;
							return FReply::Handled();
						})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SBorder)
					.Visibility_Lambda([this]() { return bShowingGddHelp ? EVisibility::Visible : EVisibility::Collapsed; })
					.BorderBackgroundColor(FLinearColor(0.15f, 0.15f, 0.2f))
					.Padding(10)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("GddHelpTitle", "How GDD Works"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(FLinearColor(0.4f, 0.8f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
						[
							SNew(STextBlock)
								.AutoWrapText(true)
								.Text(LOCTEXT("GddHelpText",
									"• Import .txt or .md files containing your game design docs\n"
									"• Enable/disable individual files to control what AI sees\n"
									"• GDD content is injected into AI context for all conversations\n"
									"• Use for design specs, lore, mechanics, style guides, etc.\n"
									"• Token count shows approximate context usage"))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("GddDesc", "Add game design documents as persistent context. The AI will reference these documents when generating code, analyzing assets, or answering questions."))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SAssignNew(GddStatusText, STextBlock)
						.Text_Lambda([this]() {
							int32 EnabledCount = 0;
							int32 TotalTokens = 0;
							for (const TSharedPtr<FGddFileEntry>& File : GddFiles)
							{
								if (File.IsValid() && File->bEnabled)
								{
									EnabledCount++;
									TotalTokens += File->EstimatedTokens;
								}
							}
							return FText::FromString(FString::Printf(TEXT("%d of %d files enabled (~%d tokens)"), EnabledCount, GddFiles.Num(), TotalTokens));
						})
						.ColorAndOpacity(FLinearColor(0.6f, 1.0f, 0.6f))
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 0, 10)
			[
				SNew(SBox)
					.MaxDesiredHeight(250.0f)
					[
						SAssignNew(GddListView, SListView<TSharedPtr<FGddFileEntry>>)
							.ListItemsSource(&GddFiles)
							.OnGenerateRow(this, &SBpGeneratorUltimateWidget::OnGenerateRowForGddList)
							.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnGddSelectionChanged)
							.SelectionMode(ESelectionMode::Single)
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("ImportGddButton", "Import File..."))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnImportGddClicked)
				]
				+ SHorizontalBox::Slot().Padding(5, 0, 0, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("DeleteGddButton", "Delete"))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnDeleteGddClicked)
						.IsEnabled_Lambda([this]() { return SelectedGddFileId.IsValid(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
					.Text_Lambda([this]() {
						if (SelectedGddFileId.IsValid())
						{
							for (const TSharedPtr<FGddFileEntry>& File : GddFiles)
							{
								if (File.IsValid() && File->FileId == SelectedGddFileId)
								{
									return FText::FromString(FString::Printf(TEXT("Editing: %s"), *File->FileName));
								}
							}
						}
						return FText::FromString(TEXT("Select a file to edit or import a new one"));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 0, 10)
			[
				SAssignNew(GddInput, SMultiLineEditableTextBox)
					.HintText(LOCTEXT("GddInputHint", "Select a file to edit its content..."))
					.AutoWrapText(true)
					.IsReadOnly_Lambda([this]() { return !SelectedGddFileId.IsValid(); })
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("SaveGddButton", "Save"))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnSaveGddClicked)
						.IsEnabled_Lambda([this]() { return SelectedGddFileId.IsValid(); })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("CloseGddButton", "Close"))
						.ButtonColorAndOpacity(FLinearColor(0.6f, 0.15f, 0.15f))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseGddClicked)
				]
			]
		];
}
TSharedRef<ITableRow> SBpGeneratorUltimateWidget::OnGenerateRowForGddList(TSharedPtr<FGddFileEntry> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FGddFileEntry>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 2)
			[
				SNew(SCheckBox)
					.IsChecked_Lambda([InItem]() { return (InItem.IsValid() && InItem->bEnabled) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([this, InItem](ECheckBoxState NewState) {
						if (InItem.IsValid())
						{
							OnToggleGddEnabled(InItem->FileId);
						}
					}))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 2)
			[
				SNew(STextBlock)
					.Text_Lambda([InItem]() { return InItem.IsValid() ? FText::FromString(InItem->FileName) : FText(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 2)
			[
				SNew(STextBlock)
					.Text_Lambda([InItem]() { return InItem.IsValid() ? FText::FromString(FString::Printf(TEXT("~%d tokens"), InItem->EstimatedTokens)) : FText(); })
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
			]
		];
}
void SBpGeneratorUltimateWidget::OnGddSelectionChanged(TSharedPtr<FGddFileEntry> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		SelectedGddFileId = InItem->FileId;
		FString GddDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd");
		FString FilePath = GddDir / InItem->FilePath;
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			if (GddInput.IsValid())
			{
				GddInput->SetText(FText::FromString(FileContent));
			}
		}
	}
	else
	{
		SelectedGddFileId = FGuid();
		if (GddInput.IsValid())
		{
			GddInput->SetText(FText());
		}
	}
}
void SBpGeneratorUltimateWidget::OnToggleGddEnabled(const FGuid& FileId)
{
	for (TSharedPtr<FGddFileEntry>& File : GddFiles)
	{
		if (File.IsValid() && File->FileId == FileId)
		{
			File->bEnabled = !File->bEnabled;
			SaveGddManifest();
			if (GddListView.IsValid())
			{
				GddListView->RequestListRefresh();
			}
			break;
		}
	}
}
FReply SBpGeneratorUltimateWidget::OnImportGddClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("Desktop platform not available for GDD import"));
		return FReply::Handled();
	}
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("GDD Files (*.txt;*.md)|*.txt;*.md|Text Files (*.txt)|*.txt|Markdown Files (*.md)|*.md|All Files (*.*)|*.*");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Import GDD Files"),
		FPaths::ProjectDir(),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::Multiple,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		int32 ImportedCount = 0;
		int32 TotalTokens = 0;
		FString LastImportedFileName;
		for (const FString& SourceFilePath : OutFiles)
		{
			FString FileContent;
			if (FFileHelper::LoadFileToString(FileContent, *SourceFilePath))
			{
				TSharedPtr<FGddFileEntry> NewFile = MakeShareable(new FGddFileEntry);
				NewFile->FileName = FPaths::GetCleanFilename(SourceFilePath);
				NewFile->FilePath = FString::Printf(TEXT("gdd_%s.txt"), *NewFile->FileId.ToString(EGuidFormats::Digits));
				NewFile->CharCount = FileContent.Len();
				NewFile->EstimatedTokens = FileContent.Len() / 4;
				NewFile->bEnabled = true;
				NewFile->LastModified = FDateTime::Now();
				FString GddDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd");
				FString SavePath = GddDir / NewFile->FilePath;
				FFileHelper::SaveStringToFile(FileContent, *SavePath);
				GddFiles.Add(NewFile);
				ImportedCount++;
				TotalTokens += NewFile->EstimatedTokens;
				LastImportedFileName = NewFile->FileName;
				SelectedGddFileId = NewFile->FileId;
				UE_LOG(LogBpGeneratorUltimate, Log, TEXT("GDD imported: %s (%d chars, ~%d tokens)"), *NewFile->FileName, NewFile->CharCount, NewFile->EstimatedTokens);
			}
		}
		if (ImportedCount > 0)
		{
			SaveGddManifest();
			if (GddListView.IsValid())
			{
				GddListView->RequestListRefresh();
			}
			if (GddInput.IsValid() && OutFiles.Num() > 0)
			{
				FString LastFileContent;
				FFileHelper::LoadFileToString(LastFileContent, *OutFiles.Last());
				GddInput->SetText(FText::FromString(LastFileContent));
			}
			FString NotificationText;
			if (ImportedCount == 1)
			{
				NotificationText = FString::Printf(TEXT("GDD imported: %s (~%d tokens)"), *LastImportedFileName, TotalTokens);
			}
			else
			{
				NotificationText = FString::Printf(TEXT("%d GDD files imported (~%d tokens total)"), ImportedCount, TotalTokens);
			}
			FNotificationInfo Info(FText::FromString(NotificationText));
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Success);
			}
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnSaveGddClicked()
{
	if (!SelectedGddFileId.IsValid() || !GddInput.IsValid())
	{
		return FReply::Handled();
	}
	for (TSharedPtr<FGddFileEntry>& File : GddFiles)
	{
		if (File.IsValid() && File->FileId == SelectedGddFileId)
		{
			FString NewContent = GddInput->GetText().ToString();
			File->CharCount = NewContent.Len();
			File->EstimatedTokens = NewContent.Len() / 4;
			File->LastModified = FDateTime::Now();
			FString GddDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd");
			FString SavePath = GddDir / File->FilePath;
			FFileHelper::SaveStringToFile(NewContent, *SavePath);
			SaveGddManifest();
			if (GddListView.IsValid())
			{
				GddListView->RequestListRefresh();
			}
			FString NotificationText = FString::Printf(TEXT("GDD saved: %s (~%d tokens)"), *File->FileName, File->EstimatedTokens);
			FNotificationInfo Info(FText::FromString(NotificationText));
			Info.ExpireDuration = 3.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Success);
			}
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("GDD saved: %s (%d chars, ~%d tokens)"), *File->FileName, File->CharCount, File->EstimatedTokens);
			break;
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnDeleteGddClicked()
{
	if (!SelectedGddFileId.IsValid())
	{
		return FReply::Handled();
	}
	FString DeletedFileName;
	for (int32 i = 0; i < GddFiles.Num(); i++)
	{
		if (GddFiles[i].IsValid() && GddFiles[i]->FileId == SelectedGddFileId)
		{
			DeletedFileName = GddFiles[i]->FileName;
			FString GddDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("gdd");
			FString FilePath = GddDir / GddFiles[i]->FilePath;
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			PlatformFile.DeleteFile(*FilePath);
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("GDD deleted: %s"), *DeletedFileName);
			GddFiles.RemoveAt(i);
			break;
		}
	}
	SelectedGddFileId = FGuid();
	if (GddInput.IsValid())
	{
		GddInput->SetText(FText());
	}
	SaveGddManifest();
	if (GddListView.IsValid())
	{
		GddListView->RequestListRefresh();
	}
	if (!DeletedFileName.IsEmpty())
	{
		FString NotificationText = FString::Printf(TEXT("GDD deleted: %s"), *DeletedFileName);
		FNotificationInfo Info(FText::FromString(NotificationText));
		Info.ExpireDuration = 3.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::SaveAiMemoryManifest()
{
	FString MemoryDir = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate");
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*MemoryDir))
	{
		PlatformFile.CreateDirectoryTree(*MemoryDir);
	}
	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
	JsonObj->SetNumberField(TEXT("Version"), 1);
	JsonObj->SetStringField(TEXT("ProjectName"), FApp::GetProjectName());
	JsonObj->SetStringField(TEXT("LastUpdated"), FDateTime::Now().ToString());
	TArray<TSharedPtr<FJsonValue>> MemoriesArray;
	for (const TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (!Memory.IsValid()) continue;
		TSharedPtr<FJsonObject> MemObj = MakeShareable(new FJsonObject);
		MemObj->SetStringField(TEXT("MemoryId"), Memory->MemoryId.ToString());
		MemObj->SetNumberField(TEXT("Category"), (int32)Memory->Category);
		MemObj->SetStringField(TEXT("Content"), Memory->Content);
		MemObj->SetStringField(TEXT("Source"), Memory->Source);
		MemObj->SetStringField(TEXT("CreatedAt"), Memory->CreatedAt.ToString());
		MemObj->SetStringField(TEXT("LastAccessed"), Memory->LastAccessed.ToString());
		MemObj->SetNumberField(TEXT("AccessCount"), Memory->AccessCount);
		MemObj->SetBoolField(TEXT("bEnabled"), Memory->bEnabled);
		MemoriesArray.Add(MakeShareable(new FJsonValueObject(MemObj)));
	}
	JsonObj->SetArrayField(TEXT("Memories"), MemoriesArray);
	FString ManifestPath = MemoryDir / TEXT("ai_memory.json");
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(JsonString, *ManifestPath);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("AI Memory manifest saved: %d memories"), AiMemories.Num());
}
void SBpGeneratorUltimateWidget::LoadAiMemoryManifest()
{
	FString ManifestPath = FPaths::ProjectSavedDir() / TEXT("BpGeneratorUltimate") / TEXT("ai_memory.json");
	if (!FPaths::FileExists(ManifestPath))
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("No AI Memory manifest found, starting fresh"));
		return;
	}
	FString JsonString;
	FFileHelper::LoadFileToString(JsonString, *ManifestPath);
	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid())
	{
		AiMemories.Empty();
		const TArray<TSharedPtr<FJsonValue>>* MemoriesArray;
		if (JsonObj->TryGetArrayField(TEXT("Memories"), MemoriesArray))
		{
			for (const TSharedPtr<FJsonValue>& MemValue : *MemoriesArray)
			{
				TSharedPtr<FJsonObject> MemObj = MemValue->AsObject();
				if (!MemObj.IsValid()) continue;
				TSharedPtr<FAiMemoryEntry> Memory = MakeShareable(new FAiMemoryEntry);
				FString MemoryIdStr;
				if (MemObj->TryGetStringField(TEXT("MemoryId"), MemoryIdStr))
				{
					Memory->MemoryId = FGuid(MemoryIdStr);
				}
				int32 CategoryInt;
				if (MemObj->TryGetNumberField(TEXT("Category"), CategoryInt))
				{
					Memory->Category = (EAiMemoryCategory)CategoryInt;
				}
				MemObj->TryGetStringField(TEXT("Content"), Memory->Content);
				MemObj->TryGetStringField(TEXT("Source"), Memory->Source);
				FString CreatedAtStr, LastAccessedStr;
				if (MemObj->TryGetStringField(TEXT("CreatedAt"), CreatedAtStr))
				{
					FDateTime::Parse(CreatedAtStr, Memory->CreatedAt);
				}
				if (MemObj->TryGetStringField(TEXT("LastAccessed"), LastAccessedStr))
				{
					FDateTime::Parse(LastAccessedStr, Memory->LastAccessed);
				}
				MemObj->TryGetNumberField(TEXT("AccessCount"), Memory->AccessCount);
				MemObj->TryGetBoolField(TEXT("bEnabled"), Memory->bEnabled);
				AiMemories.Add(Memory);
			}
		}
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("AI Memory manifest loaded: %d memories"), AiMemories.Num());
	}
}
FString SBpGeneratorUltimateWidget::GetAiMemoryContentForAI() const
{
	if (AiMemories.Num() == 0)
	{
		return FString();
	}
	bool bHasEnabledMemories = false;
	for (const TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (Memory.IsValid() && Memory->bEnabled)
		{
			bHasEnabledMemories = true;
			break;
		}
	}
	if (!bHasEnabledMemories)
	{
		return FString();
	}
	FString Content = TEXT("=== AI MEMORY ===\n\n");
	TMap<EAiMemoryCategory, TArray<FString>> CategorizedMemories;
	for (const TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (!Memory.IsValid() || !Memory->bEnabled) continue;
		CategorizedMemories.FindOrAdd(Memory->Category).Add(Memory->Content);
	}
	auto OutputCategory = [&](EAiMemoryCategory Category, const FString& CategoryName) {
		if (TArray<FString>* Memos = CategorizedMemories.Find(Category))
		{
			if (Memos->Num() > 0)
			{
				Content += FString::Printf(TEXT("## %s\n"), *CategoryName);
				for (const FString& Memo : *Memos)
				{
					Content += FString::Printf(TEXT("- %s\n"), *Memo);
				}
				Content += TEXT("\n");
			}
		}
	};
	OutputCategory(EAiMemoryCategory::ProjectInfo, TEXT("Project Info"));
	OutputCategory(EAiMemoryCategory::RecentWork, TEXT("Recent Work"));
	OutputCategory(EAiMemoryCategory::Preferences, TEXT("Preferences"));
	OutputCategory(EAiMemoryCategory::Patterns, TEXT("Patterns"));
	OutputCategory(EAiMemoryCategory::AssetRelations, TEXT("Asset Relations"));
	OutputCategory(EAiMemoryCategory::Decisions, TEXT("Decisions"));
	Content += TEXT("=== END AI MEMORY ===\n");
	return Content;
}
int32 SBpGeneratorUltimateWidget::GetAiMemoryTokenCount() const
{
	int32 TotalTokens = 0;
	for (const TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (Memory.IsValid() && Memory->bEnabled)
		{
			TotalTokens += Memory->Content.Len() / 4;
		}
	}
	return TotalTokens;
}
FString SBpGeneratorUltimateWidget::GetCategoryDisplayName(EAiMemoryCategory Category) const
{
	switch (Category)
	{
	case EAiMemoryCategory::ProjectInfo: return TEXT("Project Info");
	case EAiMemoryCategory::RecentWork: return TEXT("Recent Work");
	case EAiMemoryCategory::Preferences: return TEXT("Preferences");
	case EAiMemoryCategory::Patterns: return TEXT("Patterns");
	case EAiMemoryCategory::AssetRelations: return TEXT("Asset Relations");
	case EAiMemoryCategory::Decisions: return TEXT("Decisions");
	default: return TEXT("Unknown");
	}
}
FLinearColor SBpGeneratorUltimateWidget::GetCategoryColor(EAiMemoryCategory Category) const
{
	switch (Category)
	{
	case EAiMemoryCategory::ProjectInfo: return FLinearColor(0.3f, 0.5f, 0.9f);
	case EAiMemoryCategory::RecentWork: return FLinearColor(0.3f, 0.8f, 0.4f);
	case EAiMemoryCategory::Preferences: return FLinearColor(0.7f, 0.4f, 0.9f);
	case EAiMemoryCategory::Patterns: return FLinearColor(0.9f, 0.6f, 0.2f);
	case EAiMemoryCategory::AssetRelations: return FLinearColor(0.2f, 0.7f, 0.8f);
	case EAiMemoryCategory::Decisions: return FLinearColor(0.9f, 0.8f, 0.2f);
	default: return FLinearColor::White;
	}
}
FReply SBpGeneratorUltimateWidget::OnAiMemoryClicked()
{
	if (bIsShowingAiMemoryOverlay)
	{
		bIsShowingAiMemoryOverlay = false;
		if (AiMemoryOverlay.IsValid())
		{
			AiMemoryOverlay->SetVisibility(EVisibility::Collapsed);
		}
	}
	else
	{
		bIsShowingAiMemoryOverlay = true;
		SelectedMemoryId = FGuid();
		if (AiMemoryOverlay.IsValid())
		{
			AiMemoryOverlay->SetContent(CreateAiMemoryWidget());
			AiMemoryOverlay->SetVisibility(EVisibility::Visible);
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCloseAiMemoryClicked()
{
	bIsShowingAiMemoryOverlay = false;
	if (AiMemoryOverlay.IsValid())
	{
		AiMemoryOverlay->SetVisibility(EVisibility::Collapsed);
	}
	return FReply::Handled();
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateAiMemoryWidget()
{
	return SNew(SBorder)
		.Padding(FMargin(20, 25, 20, 20))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("AiMemoryTitle", "AI Memory"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("MemoryHelpBtn", "?"))
						.ToolTipText(LOCTEXT("MemoryHelpTooltip", "Toggle help information"))
						.ButtonColorAndOpacity(FLinearColor(0.2f, 0.4f, 0.6f))
						.OnClicked_Lambda([this]() {
							bShowingMemoryHelp = !bShowingMemoryHelp;
							return FReply::Handled();
						})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SBorder)
					.Visibility_Lambda([this]() { return bShowingMemoryHelp ? EVisibility::Visible : EVisibility::Collapsed; })
					.BorderBackgroundColor(FLinearColor(0.15f, 0.15f, 0.2f))
					.Padding(10)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("MemoryHelpTitle", "How AI Memory Works"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(FLinearColor(0.4f, 0.8f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
						[
							SNew(STextBlock)
								.AutoWrapText(true)
								.Text(LOCTEXT("MemoryHelpText",
									"• AI automatically extracts memories from your conversations\n"
									"• Memories are suggested after N user prompts (configurable below)\n"
									"• Review pending memories to approve or reject them\n"
									"• Approved memories are injected into AI context for better responses\n"
									"• Use categories to organize: Project Info, Preferences, Patterns, etc."))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("ExtractionThresholdLabel", "Extract after prompts:"))
						.ToolTipText(LOCTEXT("ExtractionThresholdTooltip", "How many user prompts before AI tries to extract memories"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&MemoryThresholdOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
							return SNew(STextBlock).Text(FText::FromString(*InItem));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type) {
							if (InItem.IsValid())
							{
								MemoryExtractionThreshold = FCString::Atoi(**InItem);
							}
						})
						.InitiallySelectedItem(MemoryThresholdOptions.Num() > 3 ? MemoryThresholdOptions[3] : MemoryThresholdOptions[0])
						[
							SNew(STextBlock)
								.Text_Lambda([this]() {
									return FText::FromString(FString::Printf(TEXT("%d"), MemoryExtractionThreshold));
								})
						]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("AiMemoryDesc", "AI learns from your interactions and suggests memories. Review pending memories to approve or reject them."))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("MemoriesTab", "Memories"))
						.IsEnabled_Lambda([this]() { return bShowingPendingMemories; })
						.OnClicked_Lambda([this]() {
							bShowingPendingMemories = false;
							if (MemoryListView.IsValid())
							{
								MemoryListView->SetItemsSource(&AiMemories);
								MemoryListView->RequestListRefresh();
							}
							SelectedMemoryId = FGuid();
							if (MemoryInput.IsValid()) MemoryInput->SetText(FText());
							return FReply::Handled();
						})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
				[
					SNew(SButton)
						.Text_Lambda([this]() {
							int32 PendingCount = SuggestedMemories.Num();
							return FText::FromString(FString::Printf(TEXT("Pending (%d)"), PendingCount));
						})
						.IsEnabled_Lambda([this]() { return !bShowingPendingMemories; })
						.OnClicked_Lambda([this]() {
							bShowingPendingMemories = true;
							if (MemoryListView.IsValid())
							{
								MemoryListView->SetItemsSource(&SuggestedMemories);
								MemoryListView->RequestListRefresh();
							}
							SelectedMemoryId = FGuid();
							if (MemoryInput.IsValid()) MemoryInput->SetText(FText());
							return FReply::Handled();
						})
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
				[
					SAssignNew(MemoryStatusText, STextBlock)
						.Text_Lambda([this]() {
							if (bShowingPendingMemories)
							{
								return FText::FromString(FString::Printf(TEXT("%d pending memories"), SuggestedMemories.Num()));
							}
							int32 EnabledCount = 0;
							int32 TotalTokens = 0;
							for (const TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
							{
								if (Memory.IsValid() && Memory->bEnabled)
								{
									EnabledCount++;
									TotalTokens += Memory->Content.Len() / 4;
								}
							}
							return FText::FromString(FString::Printf(TEXT("%d enabled (~%d tokens)"), EnabledCount, TotalTokens));
						})
						.ColorAndOpacity(FLinearColor(0.6f, 1.0f, 0.6f))
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 0, 10)
			[
				SNew(SBox)
					.MaxDesiredHeight(200.0f)
					[
						SAssignNew(MemoryListView, SListView<TSharedPtr<FAiMemoryEntry>>)
							.ListItemsSource(bShowingPendingMemories ? &SuggestedMemories : &AiMemories)
							.OnGenerateRow(this, &SBpGeneratorUltimateWidget::OnGenerateRowForMemoryList)
							.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnMemorySelectionChanged)
							.SelectionMode(ESelectionMode::Single)
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("ApproveButton", "Approve"))
						.Visibility_Lambda([this]() { return bShowingPendingMemories ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnApproveMemoryClicked)
						.IsEnabled_Lambda([this]() { return SelectedMemoryId.IsValid() && bShowingPendingMemories; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("RejectButton", "Reject"))
						.Visibility_Lambda([this]() { return bShowingPendingMemories ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnRejectMemoryClicked)
						.IsEnabled_Lambda([this]() { return SelectedMemoryId.IsValid() && bShowingPendingMemories; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("AddMemoryButton", "Add"))
						.Visibility_Lambda([this]() { return !bShowingPendingMemories ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnAddMemoryClicked)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("DeleteMemoryButton", "Delete"))
						.Visibility_Lambda([this]() { return !bShowingPendingMemories ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnDeleteMemoryClicked)
						.IsEnabled_Lambda([this]() { return SelectedMemoryId.IsValid() && !bShowingPendingMemories; })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("CategoryLabel", "Category:"))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.IsEnabled_Lambda([this]() { return SelectedMemoryId.IsValid() && !bShowingPendingMemories; })
						.OptionsSource(&MemoryCategoryOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
							return SNew(STextBlock).Text(FText::FromString(*InItem));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type) {
							if (InItem.IsValid() && SelectedMemoryId.IsValid())
							{
								EAiMemoryCategory NewCategory = EAiMemoryCategory::RecentWork;
								if (*InItem == TEXT("Project Info")) NewCategory = EAiMemoryCategory::ProjectInfo;
								else if (*InItem == TEXT("Recent Work")) NewCategory = EAiMemoryCategory::RecentWork;
								else if (*InItem == TEXT("Preferences")) NewCategory = EAiMemoryCategory::Preferences;
								else if (*InItem == TEXT("Patterns")) NewCategory = EAiMemoryCategory::Patterns;
								else if (*InItem == TEXT("Asset Relations")) NewCategory = EAiMemoryCategory::AssetRelations;
								else if (*InItem == TEXT("Decisions")) NewCategory = EAiMemoryCategory::Decisions;
								for (TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
								{
									if (Memory.IsValid() && Memory->MemoryId == SelectedMemoryId)
									{
										Memory->Category = NewCategory;
										SaveAiMemoryManifest();
										if (MemoryListView.IsValid())
										{
											MemoryListView->RequestListRefresh();
										}
										break;
									}
								}
							}
						})
						.InitiallySelectedItem(MemoryCategoryOptions.Num() > 1 ? MemoryCategoryOptions[1] : MemoryCategoryOptions[0])
						[
							SNew(STextBlock)
								.Text_Lambda([this]() {
									if (SelectedMemoryId.IsValid())
									{
										auto& List = bShowingPendingMemories ? SuggestedMemories : AiMemories;
										for (const TSharedPtr<FAiMemoryEntry>& Memory : List)
										{
											if (Memory.IsValid() && Memory->MemoryId == SelectedMemoryId)
											{
												return FText::FromString(GetCategoryDisplayName(Memory->Category));
											}
										}
									}
									return FText::FromString(bShowingPendingMemories ? TEXT("Select a pending memory") : TEXT("Select a memory"));
								})
								.ColorAndOpacity_Lambda([this]() {
									if (SelectedMemoryId.IsValid())
									{
										auto& List = bShowingPendingMemories ? SuggestedMemories : AiMemories;
										for (const TSharedPtr<FAiMemoryEntry>& Memory : List)
										{
											if (Memory.IsValid() && Memory->MemoryId == SelectedMemoryId)
											{
												return GetCategoryColor(Memory->Category);
											}
										}
									}
									return FLinearColor(0.7f, 0.7f, 0.7f);
								})
						]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0, 0, 0, 10)
			[
				SAssignNew(MemoryInput, SMultiLineEditableTextBox)
					.HintText(LOCTEXT("MemoryInputHint", "Select a memory to edit..."))
					.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().Padding(0, 0, 5, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("SaveMemoryButton", "Save"))
						.Visibility_Lambda([this]() { return !bShowingPendingMemories ? EVisibility::Visible : EVisibility::Collapsed; })
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnSaveMemoryClicked)
						.IsEnabled_Lambda([this]() { return SelectedMemoryId.IsValid() && !bShowingPendingMemories; })
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SButton)
						.Text(LOCTEXT("CloseMemoryButton", "Close"))
						.ButtonColorAndOpacity(FLinearColor(0.6f, 0.15f, 0.15f))
						.OnClicked(this, &SBpGeneratorUltimateWidget::OnCloseAiMemoryClicked)
				]
			]
		];
}
TSharedRef<ITableRow> SBpGeneratorUltimateWidget::OnGenerateRowForMemoryList(TSharedPtr<FAiMemoryEntry> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString CategoryName = InItem.IsValid() ? GetCategoryDisplayName(InItem->Category) : TEXT("");
	FLinearColor CategoryColor = InItem.IsValid() ? GetCategoryColor(InItem->Category) : FLinearColor::White;
	FString ContentPreview;
	if (InItem.IsValid())
	{
		ContentPreview = InItem->Content;
		if (ContentPreview.Len() > 60)
		{
			ContentPreview = ContentPreview.Left(57) + TEXT("...");
		}
	}
	return SNew(STableRow<TSharedPtr<FAiMemoryEntry>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 2)
			[
				SNew(SCheckBox)
					.IsChecked_Lambda([InItem]() { return (InItem.IsValid() && InItem->bEnabled) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([this, InItem](ECheckBoxState NewState) {
						if (InItem.IsValid())
						{
							OnToggleMemoryEnabled(InItem->MemoryId);
						}
					}))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5, 2)
			[
				SNew(SBorder)
					.Padding(FMargin(4, 2))
					.BorderBackgroundColor(CategoryColor)
					[
						SNew(STextBlock)
							.Text(FText::FromString(CategoryName))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FLinearColor::White)
					]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5, 2)
			[
				SNew(STextBlock)
					.Text_Lambda([ContentPreview]() { return FText::FromString(ContentPreview); })
			]
		];
}
void SBpGeneratorUltimateWidget::OnMemorySelectionChanged(TSharedPtr<FAiMemoryEntry> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		SelectedMemoryId = InItem->MemoryId;
		SelectedMemoryCategory = InItem->Category;
		if (MemoryInput.IsValid())
		{
			MemoryInput->SetText(FText::FromString(InItem->Content));
		}
	}
	else
	{
		SelectedMemoryId = FGuid();
		if (MemoryInput.IsValid())
		{
			MemoryInput->SetText(FText());
		}
	}
}
void SBpGeneratorUltimateWidget::OnToggleMemoryEnabled(const FGuid& MemoryId)
{
	for (TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (Memory.IsValid() && Memory->MemoryId == MemoryId)
		{
			Memory->bEnabled = !Memory->bEnabled;
			SaveAiMemoryManifest();
			if (MemoryListView.IsValid())
			{
				MemoryListView->RequestListRefresh();
			}
			break;
		}
	}
}
FReply SBpGeneratorUltimateWidget::OnAddMemoryClicked()
{
	TSharedPtr<FAiMemoryEntry> NewMemory = MakeShareable(new FAiMemoryEntry);
	NewMemory->Category = EAiMemoryCategory::ProjectInfo;
	NewMemory->Content = TEXT("New memory - edit this");
	NewMemory->Source = TEXT("user_added");
	NewMemory->bEnabled = true;
	AiMemories.Add(NewMemory);
	SaveAiMemoryManifest();
	if (MemoryListView.IsValid())
	{
		MemoryListView->RequestListRefresh();
	}
	SelectedMemoryId = NewMemory->MemoryId;
	SelectedMemoryCategory = NewMemory->Category;
	if (MemoryInput.IsValid())
	{
		MemoryInput->SetText(FText::FromString(NewMemory->Content));
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnSaveMemoryClicked()
{
	if (!SelectedMemoryId.IsValid() || !MemoryInput.IsValid())
	{
		return FReply::Handled();
	}
	for (TSharedPtr<FAiMemoryEntry>& Memory : AiMemories)
	{
		if (Memory.IsValid() && Memory->MemoryId == SelectedMemoryId)
		{
			Memory->Content = MemoryInput->GetText().ToString();
			Memory->LastAccessed = FDateTime::Now();
			SaveAiMemoryManifest();
			if (MemoryListView.IsValid())
			{
				MemoryListView->RequestListRefresh();
			}
			FString NotificationText = FString::Printf(TEXT("Memory saved: %s"), *GetCategoryDisplayName(Memory->Category));
			FNotificationInfo Info(FText::FromString(NotificationText));
			Info.ExpireDuration = 2.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Success);
			}
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("AI Memory saved: %s"), *Memory->Content);
			break;
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnDeleteMemoryClicked()
{
	if (!SelectedMemoryId.IsValid())
	{
		return FReply::Handled();
	}
	FString DeletedCategory;
	for (int32 i = 0; i < AiMemories.Num(); i++)
	{
		if (AiMemories[i].IsValid() && AiMemories[i]->MemoryId == SelectedMemoryId)
		{
			DeletedCategory = GetCategoryDisplayName(AiMemories[i]->Category);
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("AI Memory deleted: %s"), *AiMemories[i]->Content);
			AiMemories.RemoveAt(i);
			break;
		}
	}
	SelectedMemoryId = FGuid();
	if (MemoryInput.IsValid())
	{
		MemoryInput->SetText(FText());
	}
	SaveAiMemoryManifest();
	if (MemoryListView.IsValid())
	{
		MemoryListView->RequestListRefresh();
	}
	if (!DeletedCategory.IsEmpty())
	{
		FString NotificationText = FString::Printf(TEXT("Memory deleted: %s"), *DeletedCategory);
		FNotificationInfo Info(FText::FromString(NotificationText));
		Info.ExpireDuration = 2.0f;
		Info.bUseLargeFont = false;
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnApproveMemoryClicked()
{
	if (!SelectedMemoryId.IsValid())
	{
		return FReply::Handled();
	}
	for (int32 i = 0; i < SuggestedMemories.Num(); i++)
	{
		if (SuggestedMemories[i].IsValid() && SuggestedMemories[i]->MemoryId == SelectedMemoryId)
		{
			TSharedPtr<FAiMemoryEntry> ApprovedMemory = SuggestedMemories[i];
			ApprovedMemory->Source = TEXT("ai_suggested");
			AiMemories.Add(ApprovedMemory);
			SuggestedMemories.RemoveAt(i);
			SaveAiMemoryManifest();
			FString NotificationText = FString::Printf(TEXT("Memory approved: %s"), *GetCategoryDisplayName(ApprovedMemory->Category));
			FNotificationInfo Info(FText::FromString(NotificationText));
			Info.ExpireDuration = 2.0f;
			Info.bUseLargeFont = false;
			TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
			if (Notification.IsValid())
			{
				Notification->SetCompletionState(SNotificationItem::CS_Success);
			}
			break;
		}
	}
	SelectedMemoryId = FGuid();
	if (MemoryInput.IsValid())
	{
		MemoryInput->SetText(FText());
	}
	if (MemoryListView.IsValid())
	{
		MemoryListView->RequestListRefresh();
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnRejectMemoryClicked()
{
	if (!SelectedMemoryId.IsValid())
	{
		return FReply::Handled();
	}
	for (int32 i = 0; i < SuggestedMemories.Num(); i++)
	{
		if (SuggestedMemories[i].IsValid() && SuggestedMemories[i]->MemoryId == SelectedMemoryId)
		{
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Memory rejected: %s"), *SuggestedMemories[i]->Content);
			SuggestedMemories.RemoveAt(i);
			break;
		}
	}
	SelectedMemoryId = FGuid();
	if (MemoryInput.IsValid())
	{
		MemoryInput->SetText(FText());
	}
	if (MemoryListView.IsValid())
	{
		MemoryListView->RequestListRefresh();
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::ExtractMemoriesFromConversation()
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== MEMORY EXTRACTION STARTED ==="));
	TArray<TSharedPtr<FJsonValue>> RecentMessages;
	int32 StartIndex = FMath::Max(0, ArchitectConversationHistory.Num() - 10);
	for (int32 i = StartIndex; i < ArchitectConversationHistory.Num(); i++)
	{
		RecentMessages.Add(ArchitectConversationHistory[i]);
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Recent messages count: %d"), RecentMessages.Num());
	int32 UserMessageCount = 0;
	for (const TSharedPtr<FJsonValue>& MsgValue : RecentMessages)
	{
		TSharedPtr<FJsonObject> MsgObj = MsgValue->AsObject();
		if (MsgObj.IsValid())
		{
			FString Role;
			MsgObj->TryGetStringField(TEXT("role"), Role);
			if (Role == TEXT("user"))
			{
				UserMessageCount++;
			}
		}
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  User message count: %d"), UserMessageCount);
	if (UserMessageCount < MemoryExtractionThreshold)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Not enough user messages (need %d, have %d) - skipping"), MemoryExtractionThreshold, UserMessageCount);
		return;
	}
	FString ConversationText;
	for (const TSharedPtr<FJsonValue>& MsgValue : RecentMessages)
	{
		TSharedPtr<FJsonObject> MsgObj = MsgValue->AsObject();
		if (!MsgObj.IsValid()) continue;
		FString Role;
		MsgObj->TryGetStringField(TEXT("role"), Role);
		if (Role == TEXT("context")) continue;
		FString Content;
		MsgObj->TryGetStringField(TEXT("content"), Content);
		if (Content.IsEmpty())
		{
			const TArray<TSharedPtr<FJsonValue>>* Parts;
			if (MsgObj->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
			{
				TSharedPtr<FJsonObject> FirstPart = (*Parts)[0]->AsObject();
				if (FirstPart.IsValid())
				{
					FirstPart->TryGetStringField(TEXT("text"), Content);
				}
			}
		}
		if (!Content.IsEmpty())
		{
			if (Role == TEXT("user"))
			{
				ConversationText += TEXT("USER: ") + Content + TEXT("\n\n");
			}
			else if (Role == TEXT("assistant") || Role == TEXT("model"))
			{
				if (Content.Len() > 500)
				{
					Content = Content.Left(500) + TEXT("...");
				}
				ConversationText += TEXT("AI: ") + Content + TEXT("\n\n");
			}
		}
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Conversation text length: %d"), ConversationText.Len());
	if (ConversationText.IsEmpty())
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  No conversation text - skipping"));
		return;
	}
	FString ExtractionPrompt = TEXT(
		"You are a memory extraction assistant. Analyze the conversation and identify information worth remembering for future interactions.\n\n"
		"Extract memories about:\n"
		"1. Project Info: Game name, genre, platform, art style\n"
		"2. Recent Work: What the user created, modified, or worked on\n"
		"3. Preferences: User's coding style, naming conventions, preferred approaches\n"
		"4. Patterns: Recurring patterns in user's requests or code\n"
		"5. Asset Relations: Connections between assets (blueprint A uses widget B)\n"
		"6. Decisions: Important technical or design decisions made\n\n"
		"Output ONLY a valid JSON array. If nothing worth remembering, output [].\n\n"
		"Format:\n"
		"[{\"category\": \"recent_work\", \"content\": \"Description of what was done\"}]\n\n"
		"Category must be one of: project_info, recent_work, preferences, patterns, asset_relations, decisions\n\n"
		"Conversation:\n"
	) + ConversationText;
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKey = ActiveSlot.ApiKey;
	FString Provider = ActiveSlot.Provider;
	FString Model = ActiveSlot.CustomModelName;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Provider: %s | API Key empty: %s"), *Provider, ApiKey.IsEmpty() ? TEXT("YES") : TEXT("NO"));
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  No API key - skipping memory extraction"));
		return;
	}
	FString Endpoint;
	if (Provider == TEXT("OpenAI"))
	{
		Endpoint = TEXT("https://api.openai.com/v1/chat/completions");
		Model = ActiveSlot.OpenAIModel.IsEmpty() ? TEXT("gpt-4o-mini") : ActiveSlot.OpenAIModel;
	}
	else if (Provider == TEXT("Claude"))
	{
		Endpoint = TEXT("https://api.anthropic.com/v1/messages");
		Model = ActiveSlot.ClaudeModel.IsEmpty() ? TEXT("claude-3-5-haiku-20241022") : ActiveSlot.ClaudeModel;
	}
	else if (Provider == TEXT("Gemini"))
	{
		Model = ActiveSlot.GeminiModel.IsEmpty() ? TEXT("gemini-2.0-flash") : ActiveSlot.GeminiModel;
		Endpoint = FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *Model, *ApiKey);
	}
	else if (Provider == TEXT("Custom") && !ActiveSlot.CustomBaseURL.IsEmpty())
	{
		Endpoint = ActiveSlot.CustomBaseURL;
		Model = ActiveSlot.CustomModelName;
	}
	else
	{
		return;
	}
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetTimeout(60.0f);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> RequestBody = MakeShareable(new FJsonObject);
	if (Provider == TEXT("Gemini"))
	{
		TArray<TSharedPtr<FJsonValue>> Contents;
		TSharedPtr<FJsonObject> ContentObj = MakeShareable(new FJsonObject);
		ContentObj->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> Parts;
		TSharedPtr<FJsonObject> PartObj = MakeShareable(new FJsonObject);
		PartObj->SetStringField(TEXT("text"), ExtractionPrompt);
		Parts.Add(MakeShareable(new FJsonValueObject(PartObj)));
		ContentObj->SetArrayField(TEXT("parts"), Parts);
		Contents.Add(MakeShareable(new FJsonValueObject(ContentObj)));
		RequestBody->SetArrayField(TEXT("contents"), Contents);
		RequestBody->SetObjectField(TEXT("generationConfig"), MakeShareable(new FJsonObject));
	}
	else if (Provider == TEXT("Claude"))
	{
		Request->SetHeader(TEXT("x-api-key"), ApiKey);
		Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		RequestBody->SetStringField(TEXT("model"), Model);
		RequestBody->SetNumberField(TEXT("max_tokens"), 1024);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> MsgObj = MakeShareable(new FJsonObject);
		MsgObj->SetStringField(TEXT("role"), TEXT("user"));
		MsgObj->SetStringField(TEXT("content"), ExtractionPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(MsgObj)));
		RequestBody->SetArrayField(TEXT("messages"), Messages);
	}
	else
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
		RequestBody->SetStringField(TEXT("model"), Model);
		RequestBody->SetNumberField(TEXT("max_tokens"), 1024);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> MsgObj = MakeShareable(new FJsonObject);
		MsgObj->SetStringField(TEXT("role"), TEXT("user"));
		MsgObj->SetStringField(TEXT("content"), ExtractionPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(MsgObj)));
		RequestBody->SetArrayField(TEXT("messages"), Messages);
	}
	FString RequestString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestString);
	FJsonSerializer::Serialize(RequestBody.ToSharedRef(), Writer);
	Request->SetContentAsString(RequestString);
	Request->SetURL(Endpoint);
	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) {
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== MEMORY EXTRACTION RESPONSE ==="));
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Success: %s | Valid: %s | Code: %d"),
			bSuccess ? TEXT("true") : TEXT("false"),
			Resp.IsValid() ? TEXT("true") : TEXT("false"),
			Resp.IsValid() ? Resp->GetResponseCode() : 0);
		if (!bSuccess || !Resp.IsValid())
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Memory extraction request failed"));
			return;
		}
		FString ResponseStr = Resp->GetContentAsString();
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Response length: %d"), ResponseStr.Len());
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Response preview: %s"), *ResponseStr.Left(500));
		FString JsonContent;
		if (ResponseStr.Contains(TEXT("\"candidates\"")))
		{
			TSharedPtr<FJsonObject> RespObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
			if (FJsonSerializer::Deserialize(Reader, RespObj) && RespObj.IsValid())
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Parsed response object"));
				const TArray<TSharedPtr<FJsonValue>>* Candidates;
				if (RespObj->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
				{
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Found %d candidates"), Candidates->Num());
					TSharedPtr<FJsonObject> Candidate = (*Candidates)[0]->AsObject();
					if (Candidate.IsValid())
					{
						UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Candidate valid"));
						const TSharedPtr<FJsonObject>* ContentObj = nullptr;
						if (Candidate->TryGetObjectField(TEXT("content"), ContentObj) && ContentObj != nullptr)
						{
							UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Found content object"));
							const TArray<TSharedPtr<FJsonValue>>* Parts;
							if ((*ContentObj)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
							{
								UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Found %d parts"), Parts->Num());
								TSharedPtr<FJsonObject> PartObj = (*Parts)[0]->AsObject();
								if (PartObj.IsValid())
								{
									PartObj->TryGetStringField(TEXT("text"), JsonContent);
									UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Gemini: Extracted text length: %d"), JsonContent.Len());
								}
							}
						}
					}
				}
			}
		}
		else if (ResponseStr.Contains(TEXT("\"content\"")))
		{
			TSharedPtr<FJsonObject> RespObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
			if (FJsonSerializer::Deserialize(Reader, RespObj) && RespObj.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Content;
				if (RespObj->TryGetArrayField(TEXT("content"), Content) && Content->Num() > 0)
				{
					TSharedPtr<FJsonObject> FirstContent = (*Content)[0]->AsObject();
					if (FirstContent.IsValid())
					{
						FirstContent->TryGetStringField(TEXT("text"), JsonContent);
					}
				}
			}
		}
		else
		{
			TSharedPtr<FJsonObject> RespObj;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
			if (FJsonSerializer::Deserialize(Reader, RespObj) && RespObj.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices;
				if (RespObj->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
				{
					TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
					if (Choice.IsValid())
					{
						const TSharedPtr<FJsonObject>* MessagePtr = nullptr;
						if (Choice->TryGetObjectField(TEXT("message"), MessagePtr) && MessagePtr != nullptr && MessagePtr->IsValid())
						{
							(*MessagePtr)->TryGetStringField(TEXT("content"), JsonContent);
						}
					}
				}
			}
		}
		if (JsonContent.IsEmpty())
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Memory extraction: no content in response"));
			return;
		}
		int32 JsonStart = JsonContent.Find(TEXT("["));
		int32 JsonEnd = INDEX_NONE;
		JsonContent.FindLastChar(']', JsonEnd);
		if (JsonStart != INDEX_NONE && JsonEnd != INDEX_NONE && JsonEnd > JsonStart)
		{
			JsonContent = JsonContent.Mid(JsonStart, JsonEnd - JsonStart + 1);
		}
		TArray<TSharedPtr<FJsonValue>> MemoriesArray;
		TSharedRef<TJsonReader<>> ArrayReader = TJsonReaderFactory<>::Create(JsonContent);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Parsing JSON: %s"), *JsonContent.Left(500));
		if (FJsonSerializer::Deserialize(ArrayReader, MemoriesArray))
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Parsed %d items from JSON"), MemoriesArray.Num());
			int32 NewMemoryCount = 0;
			for (const TSharedPtr<FJsonValue>& MemValue : MemoriesArray)
			{
				TSharedPtr<FJsonObject> MemObj = MemValue->AsObject();
				if (!MemObj.IsValid()) continue;
				FString CategoryStr, ContentStr;
				MemObj->TryGetStringField(TEXT("category"), CategoryStr);
				MemObj->TryGetStringField(TEXT("content"), ContentStr);
				if (ContentStr.IsEmpty())
				{
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Skipping empty content"));
					continue;
				}
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Processing memory: [%s] %s"), *CategoryStr, *ContentStr.Left(100));
				EAiMemoryCategory Category = EAiMemoryCategory::RecentWork;
				if (CategoryStr == TEXT("project_info")) Category = EAiMemoryCategory::ProjectInfo;
				else if (CategoryStr == TEXT("recent_work")) Category = EAiMemoryCategory::RecentWork;
				else if (CategoryStr == TEXT("preferences")) Category = EAiMemoryCategory::Preferences;
				else if (CategoryStr == TEXT("patterns")) Category = EAiMemoryCategory::Patterns;
				else if (CategoryStr == TEXT("asset_relations")) Category = EAiMemoryCategory::AssetRelations;
				else if (CategoryStr == TEXT("decisions")) Category = EAiMemoryCategory::Decisions;
				bool bAlreadyExists = false;
				for (const TSharedPtr<FAiMemoryEntry>& Existing : AiMemories)
				{
					if (Existing.IsValid() && Existing->Content == ContentStr)
					{
						bAlreadyExists = true;
						break;
					}
				}
				for (const TSharedPtr<FAiMemoryEntry>& Existing : SuggestedMemories)
				{
					if (Existing.IsValid() && Existing->Content == ContentStr)
					{
						bAlreadyExists = true;
						break;
					}
				}
				if (!bAlreadyExists)
				{
					TSharedPtr<FAiMemoryEntry> NewMemory = MakeShareable(new FAiMemoryEntry);
					NewMemory->Category = Category;
					NewMemory->Content = ContentStr;
					NewMemory->Source = TEXT("ai_suggested");
					SuggestedMemories.Add(NewMemory);
					NewMemoryCount++;
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Memory added to suggestions: %s"), *ContentStr.Left(100));
				}
				else
				{
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Memory already exists - skipping"));
				}
			}
			if (NewMemoryCount > 0)
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== %d NEW MEMORIES SUGGESTED ==="), NewMemoryCount);
				AsyncTask(ENamedThreads::GameThread, [this, NewMemoryCount]() {
					FString NotificationText = FString::Printf(TEXT("%d new memorie(s) suggested - review in AI Memory"), NewMemoryCount);
					FNotificationInfo Info(FText::FromString(NotificationText));
					Info.ExpireDuration = 5.0f;
					Info.bUseLargeFont = false;
					TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
					if (Notification.IsValid())
					{
						Notification->SetCompletionState(SNotificationItem::CS_Pending);
					}
				});
			}
			else
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  No new memories to add"));
			}
		}
		else
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Failed to parse JSON array"));
		}
	});
	Request->ProcessRequest();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Memory extraction request sent to %s"), *Endpoint);
}
FReply SBpGeneratorUltimateWidget::OnSendProjectQuestionClicked()
{
	FString UserInput = ProjectInputTextBox->GetText().ToString();
	if (UserInput.IsEmpty() || bIsProjectThinking || ActiveProjectChatID.IsEmpty())
	{
		return FReply::Handled();
	}
	ProjectInputTextBox->SetText(FText());
	if (ProjectConversationHistory.Num() <= 1)
	{
		FString NewTitle = UserInput;
		if (NewTitle.Len() > 40)
		{
			NewTitle = NewTitle.Left(37) + TEXT("...");
		}
		TSharedPtr<FConversationInfo>* FoundInfo = ProjectConversationList.FindByPredicate(
			[&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ActiveProjectChatID; });
		if (FoundInfo)
		{
			(*FoundInfo)->Title = NewTitle;
			SaveProjectManifest();
			if (ProjectChatListView.IsValid())
			{
				ProjectChatListView->RequestListRefresh();
			}
		}
	}
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), UserInput);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	for (const FAttachedImage& Image : ProjectAttachedImages)
	{
		TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
		TSharedPtr<FJsonObject> InlineData = MakeShareable(new FJsonObject);
		InlineData->SetStringField(TEXT("mime_type"), Image.MimeType);
		InlineData->SetStringField(TEXT("data"), Image.Base64Data);
		ImagePart->SetObjectField(TEXT("inline_data"), InlineData);
		UserParts.Add(MakeShareable(new FJsonValueObject(ImagePart)));
	}
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	ProjectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	SaveProjectChatHistory(ActiveProjectChatID);
	ProjectAttachedImages.Empty();
	RefreshProjectImagePreview();
	bIsProjectThinking = true;
	RefreshProjectChatView();
	SendProjectChatRequest();
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnArchitectViewSelected()
{
	if (MainViewSwitcher.IsValid())
	{
		MainViewSwitcher->SetActiveWidgetIndex(0);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnNewArchitectChatClicked()
{
	FAssetReferenceManager::Get().UnlinkAll();
	FString NewID = FString::Printf(TEXT("arch_%lld"), FDateTime::UtcNow().ToUnixTimestamp());
	TSharedPtr<FConversationInfo> NewConversation = MakeShared<FConversationInfo>();
	NewConversation->ID = NewID;
	NewConversation->Title = TEXT("New Architect Chat");
	NewConversation->LastUpdated = FDateTime::UtcNow().ToIso8601();
	ArchitectConversationList.Insert(NewConversation, 0);
	if (ArchitectChatListView.IsValid())
	{
		ArchitectChatListView->RequestListRefresh();
		ArchitectChatListView->SetSelection(NewConversation);
	}
	SaveArchitectManifest();
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::SaveArchitectManifest()
{
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("architect_conversations.json");
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> ManifestArray;
	for (const TSharedPtr<FConversationInfo>& Info : ArchitectConversationList)
	{
		TSharedPtr<FJsonObject> ConvObject = MakeShareable(new FJsonObject);
		ConvObject->SetStringField(TEXT("id"), Info->ID);
		ConvObject->SetStringField(TEXT("title"), Info->Title);
		ConvObject->SetStringField(TEXT("last_updated"), Info->LastUpdated);
		ConvObject->SetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
		ConvObject->SetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
		ConvObject->SetNumberField(TEXT("total_tokens"), Info->TotalTokens);
		ManifestArray.Add(MakeShareable(new FJsonValueObject(ConvObject)));
	}
	RootObject->SetArrayField(TEXT("manifest"), ManifestArray);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
void SBpGeneratorUltimateWidget::LoadArchitectManifest()
{
	ArchitectConversationList.Empty();
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("architect_conversations.json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* ManifestArray;
				if (RootObject->TryGetArrayField(TEXT("manifest"), ManifestArray))
				{
					for (const TSharedPtr<FJsonValue>& Value : *ManifestArray)
					{
						const TSharedPtr<FJsonObject>& ConvObject = Value->AsObject();
						if (ConvObject.IsValid())
						{
							TSharedPtr<FConversationInfo> Info = MakeShared<FConversationInfo>();
							ConvObject->TryGetStringField(TEXT("id"), Info->ID);
							ConvObject->TryGetStringField(TEXT("title"), Info->Title);
							ConvObject->TryGetStringField(TEXT("last_updated"), Info->LastUpdated);
							ConvObject->TryGetNumberField(TEXT("prompt_tokens"), Info->TotalPromptTokens);
							ConvObject->TryGetNumberField(TEXT("completion_tokens"), Info->TotalCompletionTokens);
							ConvObject->TryGetNumberField(TEXT("total_tokens"), Info->TotalTokens);
							ArchitectConversationList.Add(Info);
						}
					}
				}
			}
		}
	}
	if (ArchitectChatListView.IsValid())
	{
		ArchitectChatListView->RequestListRefresh();
	}
	if (ArchitectConversationList.Num() > 0)
	{
		ArchitectChatListView->SetSelection(ArchitectConversationList[0]);
	}
	else
	{
		OnNewArchitectChatClicked();
	}
}
void SBpGeneratorUltimateWidget::SaveArchitectChatHistory(const FString& ChatID)
{
	if (ChatID.IsEmpty() || ArchitectConversationHistory.Num() == 0)
	{
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("architect_chats") / ChatID + TEXT(".json");
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField(TEXT("history"), ArchitectConversationHistory);
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}
void SBpGeneratorUltimateWidget::LoadArchitectChatHistory(const FString& ChatID)
{
	ArchitectConversationHistory.Empty();
	if (ChatID.IsEmpty())
	{
		RefreshArchitectChatView();
		return;
	}
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CodeExplainer") / TEXT("architect_chats") / ChatID + TEXT(".json");
	if (FPaths::FileExists(FilePath))
	{
		FString FileContent;
		if (FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			TSharedPtr<FJsonObject> RootObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
			if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* HistoryArray;
				if (RootObject->TryGetArrayField(TEXT("history"), HistoryArray))
				{
					ArchitectConversationHistory = *HistoryArray;
				}
			}
		}
	}
	if (ArchitectConversationHistory.Num() == 0 && !ChatID.IsEmpty())
	{
		const FString Greeting = TEXT("Hello! I am the Blueprint Architect. Tell me what you want to create or modify.");
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), Greeting);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	}
	RefreshArchitectChatView();
}
TSharedRef<ITableRow> SBpGeneratorUltimateWidget::OnGenerateRowForArchitectChatList(TSharedPtr<FConversationInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SConversationListRow, OwnerTable)
		.ConversationInfo(InItem)
		.ScribeWidget(StaticCastSharedRef<SBpGeneratorUltimateWidget>(AsShared()))
		.ViewType(EAnalystView::BlueprintArchitect);
}
void SBpGeneratorUltimateWidget::OnArchitectChatSelectionChanged(TSharedPtr<FConversationInfo> InItem, ESelectInfo::Type SelectInfo)
{
	if (InItem.IsValid())
	{
		ActiveArchitectChatID = InItem->ID;
		LoadArchitectChatHistory(ActiveArchitectChatID);
		int32 TokenCount = InItem->TotalTokens;
		if (TokenCount == 0 && ArchitectConversationHistory.Num() > 0)
		{
			TokenCount = EstimateFullRequestTokens(ArchitectConversationHistory);
		}
		if (ArchitectTokenCounterText.IsValid())
		{
			ArchitectTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: %d"), TokenCount)));
		}
	}
	else
	{
		ActiveArchitectChatID.Empty();
		ArchitectConversationHistory.Empty();
		bPatternsInjectedForSession = false;
		RefreshArchitectChatView();
		if (ArchitectTokenCounterText.IsValid())
		{
			ArchitectTokenCounterText->SetText(FText::FromString(TEXT("Tokens: 0")));
		}
	}
}
FReply SBpGeneratorUltimateWidget::OnArchitectInputKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (HandleGlobalKeyPress(InKeyEvent))
	{
		return FReply::Handled();
	}
	if (bAssetPickerOpen && ArchitectAssetPickerPopup.IsValid())
	{
		if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Enter || InKeyEvent.GetKey() == EKeys::Up || InKeyEvent.GetKey() == EKeys::Down)
		{
			if (ArchitectAssetPickerPopup->HandleKeyDown(InKeyEvent))
			{
				return FReply::Handled();
			}
		}
	}
	if (InKeyEvent.GetKey() == EKeys::Enter && !InKeyEvent.IsShiftDown())
	{
		OnSendArchitectMessageClicked();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::V && InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsAltDown())
	{
		if (TryPasteImageFromClipboard(ArchitectAttachedImages))
		{
			RefreshArchitectImagePreview();
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}
void SBpGeneratorUltimateWidget::OnArchitectInputTextChanged(const FText& NewText)
{
	FString Text = NewText.ToString();
	if (bAssetPickerOpen && ArchitectAssetPickerPopup.IsValid() && AtSymbolPosition != INDEX_NONE)
	{
		if (AtSymbolPosition >= Text.Len() || Text[AtSymbolPosition] != TEXT('@'))
		{
			bAssetPickerOpen = false;
			AtSymbolPosition = INDEX_NONE;
			PreviousArchitectInputText = Text;
			return;
		}
		if (AtSymbolPosition < Text.Len() - 1)
		{
			FString SearchQuery = Text.RightChop(AtSymbolPosition + 1);
			if (SearchQuery.StartsWith(TEXT(" ")))
			{
				bAssetPickerOpen = false;
				AtSymbolPosition = INDEX_NONE;
				PreviousArchitectInputText = Text;
				return;
			}
			ArchitectAssetPickerPopup->UpdateSearchFilter(SearchQuery);
		}
		else
		{
			ArchitectAssetPickerPopup->UpdateSearchFilter(TEXT(""));
		}
		PreviousArchitectInputText = Text;
		return;
	}
	if (!bAssetPickerOpen)
	{
		int32 PrevLen = PreviousArchitectInputText.Len();
		if (Text.Len() > PrevLen)
		{
			int32 AtIndex = Text.Find(TEXT("@"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (AtIndex != INDEX_NONE)
			{
				bool bWasJustTyped = false;
				if (AtIndex >= PrevLen || PreviousArchitectInputText[AtIndex] != TEXT('@'))
				{
					bWasJustTyped = true;
				}
				if (bWasJustTyped)
				{
					AtSymbolPosition = AtIndex;
					ShowAssetPicker();
				}
			}
		}
	}
	PreviousArchitectInputText = Text;
}
void SBpGeneratorUltimateWidget::OnProjectInputTextChanged(const FText& NewText)
{
	FString Text = NewText.ToString();
	if (bAssetPickerOpen && ProjectAssetPickerPopup.IsValid() && AtSymbolPosition != INDEX_NONE)
	{
		if (AtSymbolPosition >= Text.Len() || Text[AtSymbolPosition] != TEXT('@'))
		{
			bAssetPickerOpen = false;
			AtSymbolPosition = INDEX_NONE;
			PreviousProjectInputText = Text;
			return;
		}
		if (AtSymbolPosition < Text.Len() - 1)
		{
			FString SearchQuery = Text.RightChop(AtSymbolPosition + 1);
			if (SearchQuery.StartsWith(TEXT(" ")))
			{
				bAssetPickerOpen = false;
				AtSymbolPosition = INDEX_NONE;
				PreviousProjectInputText = Text;
				return;
			}
			ProjectAssetPickerPopup->UpdateSearchFilter(SearchQuery);
		}
		else
		{
			ProjectAssetPickerPopup->UpdateSearchFilter(TEXT(""));
		}
		PreviousProjectInputText = Text;
		return;
	}
	if (!bAssetPickerOpen)
	{
		int32 PrevLen = PreviousProjectInputText.Len();
		if (Text.Len() > PrevLen)
		{
			int32 AtIndex = Text.Find(TEXT("@"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			if (AtIndex != INDEX_NONE)
			{
				bool bWasJustTyped = false;
				if (AtIndex >= PrevLen || PreviousProjectInputText[AtIndex] != TEXT('@'))
				{
					bWasJustTyped = true;
				}
				if (bWasJustTyped)
				{
					AtSymbolPosition = AtIndex;
					ShowAssetPickerForProject();
				}
			}
		}
	}
	PreviousProjectInputText = Text;
}
void SBpGeneratorUltimateWidget::ShowAssetPicker()
{
	if (!ArchitectAssetPickerPopup.IsValid())
	{
		return;
	}
	bAssetPickerOpen = true;
	AssetPickerSourceView = 0;
	FString SearchQuery;
	if (ArchitectInputTextBox.IsValid())
	{
		FText CurrentText = ArchitectInputTextBox->GetText();
		FString TextStr = CurrentText.ToString();
		if (AtSymbolPosition != INDEX_NONE && AtSymbolPosition < TextStr.Len() - 1)
		{
			SearchQuery = TextStr.RightChop(AtSymbolPosition + 1);
		}
	}
	ArchitectAssetPickerPopup->UpdateSearchFilter(SearchQuery);
}
void SBpGeneratorUltimateWidget::ShowAssetPickerForProject()
{
	if (!ProjectAssetPickerPopup.IsValid())
	{
		return;
	}
	bAssetPickerOpen = true;
	AssetPickerSourceView = 1;
	FString SearchQuery;
	if (ProjectInputTextBox.IsValid())
	{
		FText CurrentText = ProjectInputTextBox->GetText();
		FString TextStr = CurrentText.ToString();
		if (AtSymbolPosition != INDEX_NONE && AtSymbolPosition < TextStr.Len() - 1)
		{
			SearchQuery = TextStr.RightChop(AtSymbolPosition + 1);
		}
	}
	ProjectAssetPickerPopup->UpdateSearchFilter(SearchQuery);
}
void SBpGeneratorUltimateWidget::OnAssetRefSelected(const FAssetRefItem& SelectedItem)
{
	FString TextStr;
	if (AssetPickerSourceView == 0 && ArchitectInputTextBox.IsValid())
	{
		FText CurrentText = ArchitectInputTextBox->GetText();
		TextStr = CurrentText.ToString();
		if (AtSymbolPosition != INDEX_NONE)
		{
			FString BeforeAt = TextStr.Left(AtSymbolPosition);
			FString AssetRef = FString::Printf(TEXT("@%s"), *SelectedItem.Label);
			TextStr = BeforeAt + AssetRef + TEXT(" ");
		}
		ArchitectInputTextBox->SetText(FText::FromString(TextStr));
		PreviousArchitectInputText = TextStr;
	}
	else if (AssetPickerSourceView == 1 && ProjectInputTextBox.IsValid())
	{
		FText CurrentText = ProjectInputTextBox->GetText();
		TextStr = CurrentText.ToString();
		if (AtSymbolPosition != INDEX_NONE)
		{
			FString BeforeAt = TextStr.Left(AtSymbolPosition);
			FString AssetRef = FString::Printf(TEXT("@%s"), *SelectedItem.Label);
			TextStr = BeforeAt + AssetRef + TEXT(" ");
		}
		ProjectInputTextBox->SetText(FText::FromString(TextStr));
		PreviousProjectInputText = TextStr;
	}
	bAssetPickerOpen = false;
	AtSymbolPosition = INDEX_NONE;
	FAssetReferenceManager::Get().LinkAsset(SelectedItem.AssetPath, SelectedItem.Label, SelectedItem.Type);
}
void SBpGeneratorUltimateWidget::OnAssetPickerDismissed()
{
	bAssetPickerOpen = false;
	AtSymbolPosition = INDEX_NONE;
	if (AssetPickerSourceView == 0 && ArchitectInputTextBox.IsValid())
	{
		PreviousArchitectInputText = ArchitectInputTextBox->GetText().ToString();
	}
	else if (AssetPickerSourceView == 1 && ProjectInputTextBox.IsValid())
	{
		PreviousProjectInputText = ProjectInputTextBox->GetText().ToString();
	}
}
EUserIntent SBpGeneratorUltimateWidget::ClassifyUserIntentWithAI(const FString& UserMessage)
{
	EUserIntent Intent = EUserIntent::Chat;
	if (SendIntentClassificationRequest(UserMessage, Intent))
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== AI INTENT CLASSIFICATION: %d ==="), (int32)Intent);
		return Intent;
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== AI INTENT CLASSIFICATION FAILED, defaulting to Chat ==="));
	return EUserIntent::Chat;
}
bool SBpGeneratorUltimateWidget::SendIntentClassificationRequest(const FString& UserMessage, EUserIntent& OutIntent)
{
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification: No API key, defaulting to Chat"));
		OutIntent = EUserIntent::Chat;
		return false;
	}
	FString IntentPrompt = FString::Printf(
		TEXT("Classify the user's intent. Reply with ONLY ONE of these words: CODE, MODEL, TEXTURE, CHAT\n\n")
		TEXT("CODE = User wants to generate Blueprint code, logic, functions, variables, or modify Blueprint graphs\n")
		TEXT("MODEL = User wants to generate or create a 3D model, mesh, or 3D asset\n")
		TEXT("TEXTURE = User wants to generate or create textures, materials, PBR materials, or images\n")
		TEXT("CHAT = User is asking a question, having a conversation, or requesting information\n\n")
		TEXT("User message: \"%s\"\n\n")
		TEXT("Reply with ONLY one word:"),
		*UserMessage
	);
	TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetTimeout(30.0f);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	if (ProviderStr == TEXT("Custom"))
	{
		FString CustomURL = BaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty()) CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		Request->SetURL(CustomURL);
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		FString CustomModel = ModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty()) CustomModel = TEXT("gpt-3.5-turbo");
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (ProviderStr == TEXT("Gemini"))
	{
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		Request->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		TArray<TSharedPtr<FJsonValue>> Contents;
		TSharedPtr<FJsonObject> Content = MakeShareable(new FJsonObject);
		Content->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> Parts;
		TSharedPtr<FJsonObject> Part = MakeShareable(new FJsonObject);
		Part->SetStringField(TEXT("text"), IntentPrompt);
		Parts.Add(MakeShareable(new FJsonValueObject(Part)));
		Content->SetArrayField(TEXT("parts"), Parts);
		Contents.Add(MakeShareable(new FJsonValueObject(Content)));
		JsonPayload->SetArrayField(TEXT("contents"), Contents);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		FString OpenAIModel = FApiKeyManager::Get().GetActiveOpenAIModel();
		if (OpenAIModel.IsEmpty()) OpenAIModel = TEXT("gpt-4o-mini");
		Request->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), OpenAIModel);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		FString ClaudeModel = FApiKeyManager::Get().GetActiveClaudeModel();
		if (ClaudeModel.IsEmpty()) ClaudeModel = TEXT("claude-3-5-haiku-20241022");
		Request->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		Request->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), ClaudeModel);
		JsonPayload->SetNumberField(TEXT("max_tokens"), 10);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		Request->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), TEXT("deepseek-chat"));
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	Request->SetContentAsString(RequestBody);
	double StartTime = FPlatformTime::Seconds();
	FString ResponseContent;
	bool bSuccess = false;
	std::atomic<bool> bRequestComplete{false};
	std::atomic<bool> bRequestSuccess{false};
	FString ResponseText;
	Request->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			ResponseText = Response->GetContentAsString();
			bRequestSuccess = true;
		}
		bRequestComplete = true;
	});
	Request->ProcessRequest();
	double WaitTimeout = 25.0;
	while (!bRequestComplete && (FPlatformTime::Seconds() - StartTime) < WaitTimeout)
	{
		FPlatformProcess::Sleep(0.01f);
	}
	if (!bRequestComplete)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification request timed out"));
		Request->CancelRequest();
		return false;
	}
	if (!bRequestSuccess || ResponseText.IsEmpty())
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification request failed"));
		return false;
	}
	FString IntentResult;
	if (ProviderStr == TEXT("Gemini"))
	{
		TSharedPtr<FJsonObject> ResponseJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);
		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Candidates;
			if (ResponseJson->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* CandidateObj;
				if ((*Candidates)[0]->TryGetObject(CandidateObj))
				{
					const TSharedPtr<FJsonObject>* ContentObj;
					if ((*CandidateObj)->TryGetObjectField(TEXT("content"), ContentObj))
					{
						const TArray<TSharedPtr<FJsonValue>>* Parts;
						if ((*ContentObj)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
						{
							const TSharedPtr<FJsonObject>* PartObj;
							if ((*Parts)[0]->TryGetObject(PartObj))
							{
								(*PartObj)->TryGetStringField(TEXT("text"), IntentResult);
							}
						}
					}
				}
			}
		}
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		TSharedPtr<FJsonObject> ResponseJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);
		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Content;
			if (ResponseJson->TryGetArrayField(TEXT("content"), Content) && Content->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* ContentObj;
				if ((*Content)[0]->TryGetObject(ContentObj))
				{
					(*ContentObj)->TryGetStringField(TEXT("text"), IntentResult);
				}
			}
		}
	}
	else
	{
		TSharedPtr<FJsonObject> ResponseJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);
		if (FJsonSerializer::Deserialize(Reader, ResponseJson) && ResponseJson.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Choices;
			if (ResponseJson->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
			{
				const TSharedPtr<FJsonObject>* ChoiceObj;
				if ((*Choices)[0]->TryGetObject(ChoiceObj))
				{
					const TSharedPtr<FJsonObject>* MessageObj;
					if ((*ChoiceObj)->TryGetObjectField(TEXT("message"), MessageObj))
					{
						(*MessageObj)->TryGetStringField(TEXT("content"), IntentResult);
					}
				}
			}
		}
	}
	if (IntentResult.IsEmpty())
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification: Could not parse response"));
		return false;
	}
	IntentResult = IntentResult.TrimStartAndEnd().ToUpper();
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Intent classification raw response: %s"), *IntentResult);
	if (IntentResult.Contains(TEXT("CODE")))
	{
		OutIntent = EUserIntent::CodeGeneration;
	}
	else if (IntentResult.Contains(TEXT("ASSET")))
	{
		OutIntent = EUserIntent::AssetCreation;
	}
	else if (IntentResult.Contains(TEXT("MODEL")))
	{
		OutIntent = EUserIntent::Model3DGeneration;
	}
	else if (IntentResult.Contains(TEXT("TEXTURE")))
	{
		OutIntent = EUserIntent::TextureGeneration;
	}
	else
	{
		OutIntent = EUserIntent::Chat;
	}
	double Elapsed = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification completed in %.2fs: %d"), Elapsed, (int32)OutIntent);
	return true;
}
FReply SBpGeneratorUltimateWidget::OnSendArchitectMessageClicked()
{
	FString UserInput = ArchitectInputTextBox->GetText().ToString();
	if (UserInput.IsEmpty() || bIsArchitectThinking || ActiveArchitectChatID.IsEmpty())
	{
		return FReply::Handled();
	}
	ArchitectInputTextBox->SetText(FText());
	for (const FAttachedFileContext& File : ArchitectAttachedFiles)
	{
		TSharedPtr<FJsonObject> FileContextMsg = MakeShareable(new FJsonObject);
		FileContextMsg->SetStringField(TEXT("role"), TEXT("context"));
		FileContextMsg->SetStringField(TEXT("file_name"), File.FileName);
		FileContextMsg->SetStringField(TEXT("content"), File.Content);
		FileContextMsg->SetNumberField(TEXT("char_count"), File.CharCount);
		FileContextMsg->SetBoolField(TEXT("expanded"), File.bExpanded);
		TArray<TSharedPtr<FJsonValue>> FileParts;
		TSharedPtr<FJsonObject> FilePart = MakeShareable(new FJsonObject);
		FilePart->SetStringField(TEXT("text"), FString::Printf(TEXT("--- File: %s (%d chars, click to expand) ---"), *File.FileName, File.CharCount));
		FileParts.Add(MakeShareable(new FJsonValueObject(FilePart)));
		FileContextMsg->SetArrayField(TEXT("parts"), FileParts);
		ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(FileContextMsg)));
	}
	ArchitectAttachedFiles.Empty();
	if (ArchitectConversationHistory.Num() <= 1)
	{
		FString NewTitle = UserInput;
		if (NewTitle.Len() > 40)
		{
			NewTitle = NewTitle.Left(37) + TEXT("...");
		}
		TSharedPtr<FConversationInfo>* FoundInfo = ArchitectConversationList.FindByPredicate(
			[&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ActiveArchitectChatID; });
		if (FoundInfo)
		{
			(*FoundInfo)->Title = NewTitle;
			SaveArchitectManifest();
			if (ArchitectChatListView.IsValid())
			{
				ArchitectChatListView->RequestListRefresh();
			}
		}
	}
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), UserInput);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	for (const FAttachedImage& Image : ArchitectAttachedImages)
	{
		TSharedPtr<FJsonObject> ImagePart = MakeShareable(new FJsonObject);
		TSharedPtr<FJsonObject> InlineData = MakeShareable(new FJsonObject);
		InlineData->SetStringField(TEXT("mime_type"), Image.MimeType);
		InlineData->SetStringField(TEXT("data"), Image.Base64Data);
		ImagePart->SetObjectField(TEXT("inline_data"), InlineData);
		UserParts.Add(MakeShareable(new FJsonValueObject(ImagePart)));
	}
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	SaveArchitectChatHistory(ActiveArchitectChatID);
	ArchitectAttachedImages.Empty();
	RefreshArchitectImagePreview();
	LastArchitectUserMessageLength = UserInput.Len();
	bIsArchitectThinking = true;
	RefreshArchitectChatView();
	SendArchitectChatRequest();
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnStopClicked()
{
	if (ActiveArchitectHttpRequest.IsValid())
	{
		ActiveArchitectHttpRequest->CancelRequest();
		ActiveArchitectHttpRequest.Reset();
	}
	TSharedPtr<FJsonObject> CancelMsg = MakeShareable(new FJsonObject);
	CancelMsg->SetStringField(TEXT("role"), TEXT("model"));
	TArray<TSharedPtr<FJsonValue>> CancelParts;
	TSharedPtr<FJsonObject> CancelPartText = MakeShareable(new FJsonObject);
	CancelPartText->SetStringField(TEXT("text"), TEXT("_Request cancelled by user._"));
	CancelParts.Add(MakeShareable(new FJsonValueObject(CancelPartText)));
	CancelMsg->SetArrayField(TEXT("parts"), CancelParts);
	ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(CancelMsg)));
	SaveArchitectChatHistory(ActiveArchitectChatID);
	bIsArchitectThinking = false;
	RefreshArchitectChatView();
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::OnJustChatModeClicked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		OnInteractionModeChanged(EAIInteractionMode::JustChat);
	}
}
void SBpGeneratorUltimateWidget::OnAskBeforeEditModeClicked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		OnInteractionModeChanged(EAIInteractionMode::AskBeforeEdit);
	}
}
void SBpGeneratorUltimateWidget::OnAutoEditModeClicked(ECheckBoxState NewState)
{
	if (NewState == ECheckBoxState::Checked)
	{
		OnInteractionModeChanged(EAIInteractionMode::AutoEdit);
	}
}
void SBpGeneratorUltimateWidget::OnInteractionModeChanged(EAIInteractionMode NewMode)
{
	ArchitectInteractionMode = NewMode;
}
bool SBpGeneratorUltimateWidget::IsReadOnlyTool(const FString& ToolName) const
{
	static const TArray<FString> ReadOnlyTools = {
		TEXT("classify_intent"),
		TEXT("get_api_context"),
		TEXT("get_asset_summary"),
		TEXT("find_asset_by_name"),
		TEXT("get_current_folder"),
		TEXT("list_assets_in_folder"),
		TEXT("get_selected_assets"),
		TEXT("get_selected_nodes"),
		TEXT("get_detailed_blueprint_summary"),
		TEXT("get_material_graph_summary"),
		TEXT("get_project_root_path"),
		TEXT("get_data_asset_details"),
		TEXT("get_all_scene_actors"),
		TEXT("check_project_source")
	};
	return ReadOnlyTools.Contains(ToolName);
}
void SBpGeneratorUltimateWidget::RefreshArchitectChatView()
{
	if (!ArchitectChatBrowser.IsValid()) return;
	TStringBuilder<65536> HtmlBuilder;
	static FString MarkedJs;
	static FString GraphreJs;
	static FString NomnomlJs;
	if (MarkedJs.IsEmpty())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BpGeneratorUltimate"));
		if (Plugin.IsValid())
		{
			FString MarkedJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("marked.min.js"));
			FFileHelper::LoadFileToString(MarkedJs, *MarkedJsPath);
			FString GraphreJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("graphre.min.js"));
			FFileHelper::LoadFileToString(GraphreJs, *GraphreJsPath);
			FString NomnomlJsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("nomnoml.min.js"));
			FFileHelper::LoadFileToString(NomnomlJs, *NomnomlJsPath);
		}
	}
	HtmlBuilder.Append(TEXT("<!DOCTYPE html><html><head><meta charset='utf-8'><script>"));
	HtmlBuilder.Append(MarkedJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(GraphreJs);
	HtmlBuilder.Append(TEXT("</script><script>"));
	HtmlBuilder.Append(NomnomlJs);
	HtmlBuilder.Append(TEXT("</script><script>"
	"function createPieChart(dataStr){"
	"var colors=['#4fc3f7','#81c784','#ffb74d','#f06292','#ba68c8','#4db6ac','#ffd54f','#90a4ae','#e57373','#64b5f6'];"
	"var parts=dataStr.split(',').map(function(p){var s=p.trim().split(/\\s+/);return{label:s[0],value:parseInt(s[1])||0};});"
	"var total=parts.reduce(function(s,p){return s+p.value;},0);"
	"if(total===0)return'<p>No data</p>';"
	"var cx=100,cy=100,r=80;"
	"var svg='<svg width=\"300\" height=\"250\" style=\"margin:16px auto;display:block;\">';"
	"var angle=-Math.PI/2;"
	"parts.forEach(function(p,i){"
	"var pct=p.value/total;"
	"var sweep=pct*2*Math.PI;"
	"var x1=cx+r*Math.cos(angle),y1=cy+r*Math.sin(angle);"
	"angle+=sweep;"
	"var x2=cx+r*Math.cos(angle),y2=cy+r*Math.sin(angle);"
	"var large=sweep>Math.PI?1:0;"
	"svg+='<path d=\"M'+cx+','+cy+' L'+x1+','+y1+' A'+r+','+r+' 0 '+large+',1 '+x2+','+y2+' Z\" fill=\"'+colors[i%colors.length]+'\"/>';"
	"});"
	"svg+='<circle cx=\"'+cx+'\" cy=\"'+cy+'\" r=\"35\" fill=\"#1e1e1e\"/>';"
	"svg+='<text x=\"'+cx+'\" y=\"'+(cy+5)+'\" text-anchor=\"middle\" fill=\"#d4d4d4\" font-size=\"12\">'+total+' total</text></svg>';"
	"svg+='<div class=\"chart-legend\" style=\"display:flex;flex-wrap:wrap;gap:8px;justify-content:center;margin:8px 0;\">';"
	"parts.forEach(function(p,i){"
	"var pct=Math.round(p.value/total*100);"
	"svg+='<span style=\"display:flex;align-items:center;gap:4px;font-size:12px;\"><span style=\"width:12px;height:12px;background:'+colors[i%colors.length]+';border-radius:2px;\"></span>'+p.label+' ('+pct+'%)</span>';"
	"});"
	"svg+='</div>';"
	"return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML+this.querySelector(\\'.chart-legend\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML+this.parentElement.querySelector(\\'.chart-legend\\').outerHTML)\">⤢</button></div>';"
	"}"
	"function parseMarkdown(str){"
	"var html=marked.parse(str);"
	"html=html.replace(/<pre><code class=\"language-chart\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){return createPieChart(c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&'));});"
	"html=html.replace(/<pre><code class=\"language-flow\">([\\s\\S]*?)<\\/code><\\/pre>/g,function(m,c){var d=c.replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&').trim();d=d.replace(/\\(([^)]*)\\)/g,'-$1-');var styled='#stroke: #b0b0b0\\n#lineWidth: 2\\n#fill: #2d2d30\\n#fontSize: 14\\n'+d;try{if(typeof nomnoml==='undefined')return'<pre style=\"color:#f44\">Flow error: nomnoml not loaded</pre>';var svg=nomnoml.renderSvg(styled);return '<div class=\"diagram-container\" onclick=\"openModal(this.querySelector(\\'svg\\').outerHTML)\">'+svg+'<button class=\"diagram-expand\" onclick=\"event.stopPropagation();openModal(this.parentElement.querySelector(\\'svg\\').outerHTML)\">⤢</button></div>';}catch(e){return'<details><summary style=\"color:#f44;cursor:pointer\">Flow error: '+e.message+'</summary><pre style=\"background:#2d2d30;padding:8px\">'+d.substring(0,200)+'</pre></details>';}});"
	"html=html.replace(/\\[([^\\]]+)\\]\\(ue:\\/\\/asset\\?name=([^)]+)\\)/g,'<a href=\"ue://asset?name=$2\" class=\"asset-link\">$1</a>');"
	"return html;"
	"}"
	"</script><style>"));
	HtmlBuilder.Append(TEXT("body{font:14px 'Segoe UI',Arial;margin:16px;line-height:1.6;color:#d4d4d4;background:#1e1e1e;}"));
	HtmlBuilder.Append(TEXT("a{color:#ffa726 !important;text-decoration:none;}a:hover{color:#ffcc80 !important;}"));
	HtmlBuilder.Append(TEXT(".speaker{font-weight:bold;margin-bottom:2px;}"));
	HtmlBuilder.Append(TEXT(".user-msg{margin-bottom:15px;}"));
	HtmlBuilder.Append(TEXT(".ai-msg{margin-bottom:15px;background:#252526;padding:1px 15px;border-radius:5px;}"));
	HtmlBuilder.Append(TEXT("h1,h2,h3{color:#e0e0e0;border-bottom:1px solid #404040;padding-bottom:0.3em;}"));
	HtmlBuilder.Append(TEXT("code{background:#2d2d30;color:#ce9178;padding:0.2em 0.4em;border-radius:3px;font-family:'Consolas','Courier New',monospace;}"));
	HtmlBuilder.Append(TEXT("pre{background:#2d2d30;padding:16px;border-radius:6px;overflow:auto;} pre code{padding:0;}"));
	HtmlBuilder.Append(TEXT("table{border-collapse:collapse;margin:16px 0;width:100%;}"));
	HtmlBuilder.Append(TEXT("th,td{border:1px solid #404040;padding:8px 12px;text-align:left;}"));
	HtmlBuilder.Append(TEXT("th{background:#2d2d30;color:#e0e0e0;font-weight:600;}"));
	HtmlBuilder.Append(TEXT("tr:nth-child(even){background:#252526;}"));
	HtmlBuilder.Append(TEXT("tr:hover{background:#363636;}"));
	HtmlBuilder.Append(TEXT(".tool-result{margin-bottom:12px;border:1px solid #3c3c3c;border-radius:6px;overflow:hidden;}"));
	HtmlBuilder.Append(TEXT(".tool-header{display:flex;align-items:center;padding:8px 12px;background:#2d2d30;cursor:pointer;user-select:none;}"));
	HtmlBuilder.Append(TEXT(".tool-header:hover{background:#363636;}"));
	HtmlBuilder.Append(TEXT(".tool-icon{width:16px;height:16px;margin-right:8px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:10px;}"));
	HtmlBuilder.Append(TEXT(".tool-icon.success{background:#2ea043;color:white;}"));
	HtmlBuilder.Append(TEXT(".tool-icon.error{background:#f85149;color:white;}"));
	HtmlBuilder.Append(TEXT(".tool-name{flex:1;font-size:13px;color:#9d9d9d;}"));
	HtmlBuilder.Append(TEXT(".tool-chevron{color:#9d9d9d;transition:transform 0.2s;margin-left:8px;}"));
	HtmlBuilder.Append(TEXT(".tool-result.expanded .tool-chevron{transform:rotate(90deg);}"));
	HtmlBuilder.Append(TEXT(".tool-content{display:none;padding:10px 12px;background:#1a1a1a;border-top:1px solid #3c3c3c;font-size:12px;max-height:200px;overflow:auto;}"));
	HtmlBuilder.Append(TEXT(".tool-result.expanded .tool-content{display:block;}"));
	HtmlBuilder.Append(TEXT(".tool-content pre{margin:0;padding:8px;font-size:11px;}"));
	HtmlBuilder.Append(TEXT(".asset-link{color:#ffa726;text-decoration:none;border-bottom:1px dashed #ffa726;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".asset-link:hover{color:#ffcc80;background:rgba(255,167,38,0.15);}"));
	HtmlBuilder.Append(TEXT("#zoom-controls{position:fixed;bottom:10px;right:10px;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:4px 8px;border-radius:4px;z-index:1000;border:1px solid #404040;}"));
	HtmlBuilder.Append(TEXT("#zoom-indicator{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset{background:#444;color:#d4d4d4;border:none;padding:2px 8px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#zoom-reset:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.9);z-index:2000;justify-content:center;align-items:center;}"));
	HtmlBuilder.Append(TEXT("#diagram-modal.show{display:flex;}"));
	HtmlBuilder.Append(TEXT("#modal-content{max-width:90%;max-height:90%;overflow:auto;background:#1e1e1e;border-radius:8px;padding:20px;position:relative;}"));
	HtmlBuilder.Append(TEXT("#modal-close{position:absolute;top:10px;right:10px;background:#444;color:#fff;border:none;width:30px;height:30px;border-radius:50%;cursor:pointer;font-size:16px;z-index:10;}"));
	HtmlBuilder.Append(TEXT("#modal-toolbar{position:sticky;top:0;left:0;display:flex;gap:8px;align-items:center;background:#2d2d30;padding:8px 12px;border-radius:4px;border:1px solid #404040;margin-bottom:12px;z-index:5;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint{color:#888;font-size:11px;display:flex;align-items:center;gap:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-hint kbd{background:#444;color:#ccc;padding:2px 6px;border-radius:3px;font-size:10px;border:1px solid #555;}"));
	HtmlBuilder.Append(TEXT("#modal-zoom-pct{color:#b0b0b0;font-size:12px;min-width:40px;text-align:center;border-left:1px solid #404040;padding-left:8px;margin-left:4px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn{background:#444;color:#d4d4d4;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;}"));
	HtmlBuilder.Append(TEXT("#modal-reset-btn:hover{background:#555;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn{background:#0078d4;color:#fff;border:none;padding:4px 10px;border-radius:3px;cursor:pointer;font-size:11px;margin-left:auto;}"));
	HtmlBuilder.Append(TEXT("#modal-popout-btn:hover{background:#1084d8;}"));
	HtmlBuilder.Append(TEXT(".diagram-container{position:relative;display:inline-block;max-width:150px;max-height:150px;overflow:hidden;border:1px solid #404040;border-radius:4px;cursor:pointer;}"));
	HtmlBuilder.Append(TEXT(".diagram-container svg{max-width:150px;max-height:150px;overflow:hidden;}"));
	HtmlBuilder.Append(TEXT(".diagram-container .chart-legend{display:none;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand{position:absolute;top:5px;right:5px;background:rgba(45,45,48,0.9);color:#b0b0b0;border:none;width:24px;height:24px;border-radius:4px;cursor:pointer;font-size:14px;opacity:0;transition:opacity 0.2s;}"));
	HtmlBuilder.Append(TEXT(".diagram-container:hover .diagram-expand{opacity:1;}"));
	HtmlBuilder.Append(TEXT(".diagram-expand:hover{background:#2ea043;color:#fff;}"));
	HtmlBuilder.Append(TEXT("</style><script>function toggleTool(id){var el=document.getElementById(id);if(el){el.classList.toggle('expanded');var content=el.querySelector('.tool-content');if(content){content.style.display=el.classList.contains('expanded')?'block':'none';}}}"));
	HtmlBuilder.Append(TEXT("var zoomLevel=100;function updateZoom(){document.getElementById('zoom-content').style.transform='scale('+zoomLevel/100+')';document.getElementById('zoom-content').style.transformOrigin='top left';document.getElementById('zoom-indicator').textContent=zoomLevel+'%';}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('wheel',function(e){if(e.ctrlKey&&!document.getElementById('diagram-modal').classList.contains('show')){e.preventDefault();zoomLevel+=e.deltaY>0?-10:10;zoomLevel=Math.max(30,Math.min(200,zoomLevel));updateZoom();}},{passive:false});"));
	HtmlBuilder.Append(TEXT("function resetZoom(){zoomLevel=100;updateZoom();}"));
	HtmlBuilder.Append(TEXT("var modalZoom=100;function openModal(svgHtml){document.getElementById('modal-diagram').innerHTML=svgHtml;document.getElementById('diagram-modal').classList.add('show');modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function closeModal(e){if(!e||e.target.id==='diagram-modal')document.getElementById('diagram-modal').classList.remove('show');}"));
	HtmlBuilder.Append(TEXT("function updateModalZoom(){var el=document.getElementById('modal-diagram');el.style.transform='scale('+modalZoom/100+')';el.style.transformOrigin='center center';document.getElementById('modal-zoom-pct').textContent=modalZoom+'%';}"));
	HtmlBuilder.Append(TEXT("function resetModalZoom(){modalZoom=100;updateModalZoom();}"));
	HtmlBuilder.Append(TEXT("function popoutDiagram(){var svgHtml=document.getElementById('modal-diagram').innerHTML;window.location.href='ue://diagram-popout?data='+encodeURIComponent(svgHtml);}"));
	HtmlBuilder.Append(TEXT("document.addEventListener('keydown',function(e){if(e.key==='Escape')closeModal();});"));
	HtmlBuilder.Append(TEXT("</script></head><body>"));
	HtmlBuilder.Append(TEXT("<div id='diagram-modal' onclick='closeModal(event)'><div id='modal-content' onclick='event.stopPropagation()'><button id='modal-close' onclick='closeModal()'>&times;</button><div id='modal-toolbar'><span id='modal-zoom-hint'><kbd>Ctrl</kbd>+Scroll to zoom</span><span id='modal-zoom-pct'>100%</span><button id='modal-reset-btn' onclick='resetModalZoom()'>Reset</button><button id='modal-popout-btn' onclick='popoutDiagram()'>Open in Window</button></div><div id='modal-diagram'></div></div></div>"));
	HtmlBuilder.Append(TEXT("<script>document.getElementById('diagram-modal').addEventListener('wheel',function(e){if(e.ctrlKey){e.preventDefault();modalZoom+=e.deltaY>0?-10:10;modalZoom=Math.max(30,Math.min(200,modalZoom));updateModalZoom();}},{passive:false});</script>"));
	HtmlBuilder.Append(TEXT("<div id='zoom-controls'><span id='zoom-indicator'>100%</span><button id='zoom-reset' onclick='resetZoom()'>Reset</button></div><div id='zoom-content'>"));
	if (bIsArchitectThinking)
	{
		HtmlBuilder.Append(TEXT("<div class='ai-msg' style='background:linear-gradient(135deg,rgba(42,160,255,0.1),rgba(0,200,150,0.1));border-left:3px solid #00d4aa;padding:12px 16px;'><div style='display:flex;align-items:center;gap:12px;'><span style='font-size:18px;animation:spin 1s linear infinite;'>⟳</span><span style='color:#00d4aa;'>AI is thinking</span></div><style>@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}</style></div>"));
	}
	int32 ToolResultCounter = 0;
	for (const TSharedPtr<FJsonValue>& MessageValue : ArchitectConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MessageObject = MessageValue->AsObject();
		FString Role, Content;
		if (MessageObject->TryGetStringField(TEXT("role"), Role))
		{
			const TArray<TSharedPtr<FJsonValue>>* PartsArray;
			if (MessageObject->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
			{
				if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), Content))
				{
					if (Role == TEXT("context"))
					{
						FString FileName;
						FString FileContent;
						int32 CharCount = 0;
						bool bExpanded = false;
						MessageObject->TryGetStringField(TEXT("file_name"), FileName);
						MessageObject->TryGetStringField(TEXT("content"), FileContent);
						MessageObject->TryGetNumberField(TEXT("char_count"), CharCount);
						MessageObject->TryGetBoolField(TEXT("expanded"), bExpanded);
						FString EscapedContent = FileContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
						HtmlBuilder.Appendf(TEXT("<div class='tool-result%s' id='file-%d' onclick='toggleTool(\"file-%d\")'><div class='tool-header'><div class='tool-icon'>&#128196;</div><span class='tool-name'>%s (%d chars)</span><span class='tool-chevron'>&#9654;</span></div><div class='tool-content' style='display:%s'><pre>%s</pre></div></div>"), bExpanded ? TEXT(" expanded") : TEXT(""), ToolResultCounter, ToolResultCounter, *FileName, CharCount, bExpanded ? TEXT("block") : TEXT("none"), *EscapedContent);
						ToolResultCounter++;
					}
					else
					{
						FString Speaker = (Role == TEXT("user")) ? TEXT("You") : TEXT("Architect");
						FString DivClass = (Role == TEXT("user")) ? TEXT("user-msg") : TEXT("ai-msg");
						FString DisplayContent = Content;
						if (Role == TEXT("user"))
					{
						if (Content.StartsWith(TEXT("[TOOL_RESULT:")))
						{
							FString ToolInfo = Content;
							ToolInfo.RemoveFromStart(TEXT("[TOOL_RESULT:"));
							int32 StatusIndex = ToolInfo.Find(TEXT(":"));
							int32 EndIndex = ToolInfo.Find(TEXT("]"));
							FString ToolName = ToolInfo.Left(StatusIndex);
							FString Status = ToolInfo.Mid(StatusIndex + 1, EndIndex - StatusIndex - 1);
							FString ResultContent = Content.Mid(Content.Find(TEXT("]")) + 1).TrimStart();
							bool bIsSuccess = Status == TEXT("success");
							FString IconClass = bIsSuccess ? TEXT("success") : TEXT("error");
							FString IconSymbol = bIsSuccess ? TEXT("&#10003;") : TEXT("&#10007;");
							ResultContent = ResultContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
							HtmlBuilder.Appendf(
								TEXT("<div class='tool-result' id='tool-%d' onclick='toggleTool(\"tool-%d\")'>"
									"<div class='tool-header'>"
									"<div class='tool-icon %s'>%s</div>"
									"<span class='tool-name'>%s</span>"
									"<span class='tool-chevron'>&#9654;</span>"
									"</div>"
									"<div class='tool-content'><pre>%s</pre></div>"
									"</div>"),
								ToolResultCounter, ToolResultCounter,
								*IconClass, *IconSymbol,
								*ToolName,
								*ResultContent
							);
							ToolResultCounter++;
						}
						else
						{
							DisplayContent = DisplayContent.Replace(TEXT("<"), TEXT("&lt;")).Replace(TEXT(">"), TEXT("&gt;"));
							HtmlBuilder.Appendf(TEXT("<div class='%s'><div class='speaker'>%s:</div><div>%s</div></div>"), *DivClass, *Speaker, *DisplayContent);
						}
					}
					else
					{
						DisplayContent.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
						DisplayContent.ReplaceInline(TEXT("`"), TEXT("\\`"));
						DisplayContent.ReplaceInline(TEXT("$"), TEXT("\\$"));
						HtmlBuilder.Appendf(TEXT("<div class='%s'><div class='speaker'>%s:</div><div id='arch-msg-%d'></div><script>var el=document.getElementById('arch-msg-%d');el.innerHTML=parseMarkdown(`%s`);</script></div>"),
							*DivClass, *Speaker, ArchitectConversationHistory.IndexOfByKey(MessageValue), ArchitectConversationHistory.IndexOfByKey(MessageValue), *DisplayContent);
					}
					}
				}
			}
		}
	}
	if (bIsArchitectThinking)
	{
		HtmlBuilder.Append(TEXT("<div style='display:flex;align-items:center;gap:8px;padding:8px 0;color:#FFD700;font-size:13px;'><span style='animation:spin 1s linear infinite;'>⟳</span><span>Loading...</span></div><style>@keyframes spin{from{transform:rotate(0deg);}to{transform:rotate(360deg);}}</style>"));
	}
	HtmlBuilder.Append(TEXT("<script>window.scrollTo(0, document.body.scrollHeight);</script>"));
	HtmlBuilder.Append(TEXT("</div></body></html>"));
	FString DataUri = TEXT("data:text/html;charset=utf-8,") + FGenericPlatformHttp::UrlEncode(HtmlBuilder.ToString());
	ArchitectChatBrowser->LoadURL(DataUri);
}
void SBpGeneratorUltimateWidget::SendArchitectChatRequest()
{
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: Using Slot %d, Provider=%s, Model=%s, URL=%s"),
		FApiKeyManager::Get().GetActiveSlotIndex(), *ProviderStr, *ModelName, *BaseURL);
	if (ApiKeyToUse.IsEmpty() && ProviderStr == TEXT("Gemini"))
	{
		ApiKeyToUse = AssembleTextFormat(EngineStart, POS);
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		int32 DecryptedLimit = _obf_A ^ _obf_B;
		if (CurrentCharacters >= DecryptedLimit)
		{
			FString LimitMsg = TEXT("You've used up the free trial tokens. Check the documentation to get your own API key - Gemini offers a free tier, or connect any provider key. You can even run the plugin with your Local LLM.");
			TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
			ModelContent->SetStringField(TEXT("role"), TEXT("model"));
			TArray<TSharedPtr<FJsonValue>> ModelParts;
			TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
			ModelPartText->SetStringField(TEXT("text"), LimitMsg);
			ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
			ModelContent->SetArrayField(TEXT("parts"), ModelParts);
			ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
			bIsArchitectThinking = false;
			SaveArchitectChatHistory(ActiveArchitectChatID);
			RefreshArchitectChatView();
			return;
		}
	}
	else if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		FString ErrorMessage = FString::Printf(TEXT("No API key configured for %s. Please go to 'API Settings' and enter your API key."), *ProviderStr);
		TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
		ModelContent->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> ModelParts;
		TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
		ModelPartText->SetStringField(TEXT("text"), ErrorMessage);
		ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
		ModelContent->SetArrayField(TEXT("parts"), ModelParts);
		ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
		bIsArchitectThinking = false;
		SaveArchitectChatHistory(ActiveArchitectChatID);
		RefreshArchitectChatView();
		return;
	}
	ActiveArchitectHttpRequest = FHttpModule::Get().CreateRequest();
	ActiveArchitectHttpRequest->SetTimeout(300.0f);
	ActiveArchitectHttpRequest->SetVerb(TEXT("POST"));
	ActiveArchitectHttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	double RequestStartTime = FPlatformTime::Seconds();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=================================================="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ARCHITECT API REQUEST START ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Provider: %s | Model: %s"), *ProviderStr, *ModelName);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Conversation History: %d messages"), ArchitectConversationHistory.Num());
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	FString SystemPrompt = GenerateArchitectSystemPrompt();
	int32 EstimatedTokens = EstimateFullRequestTokens(ArchitectConversationHistory);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Estimated total tokens to send: ~%d"), EstimatedTokens);
	FString ArchitectFileContextForAPI;
	for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
		FString Role;
		if (MsgObj->TryGetStringField(TEXT("role"), Role) && Role == TEXT("context"))
		{
			FString FileName, FileContent;
			MsgObj->TryGetStringField(TEXT("file_name"), FileName);
			MsgObj->TryGetStringField(TEXT("content"), FileContent);
			ArchitectFileContextForAPI += FString::Printf(TEXT("--- File: %s ---\n%s\n\n"), *FileName, *FileContent);
		}
	}
	if (ProviderStr == TEXT("Custom"))
	{
		FString CustomURL = BaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty())
		{
			CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		}
		ActiveArchitectHttpRequest->SetURL(CustomURL);
		ActiveArchitectHttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		FString CustomModel = ModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty())
		{
			CustomModel = TEXT("gpt-3.5-turbo");
		}
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: Custom API Request - URL=%s, Model=%s"), *CustomURL, *CustomModel);
		TArray<TSharedPtr<FJsonValue>> CustomMessages;
		TSharedPtr<FJsonObject> CustomSystemMessage = MakeShareable(new FJsonObject);
		CustomSystemMessage->SetStringField("role", "system");
		CustomSystemMessage->SetStringField("content", SystemPrompt);
		CustomMessages.Add(MakeShareable(new FJsonValueObject(CustomSystemMessage)));
		bool bFileContextAddedCustom = false;
		for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role = MsgObj->GetStringField(TEXT("role"));
			if (Role == TEXT("context")) continue;
			FString Text = MsgObj->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text"));
			if (!bFileContextAddedCustom && !ArchitectFileContextForAPI.IsEmpty() && Role == TEXT("user"))
			{
				Text = ArchitectFileContextForAPI + Text;
				bFileContextAddedCustom = true;
			}
			TSharedPtr<FJsonObject> CustomMessage = MakeShareable(new FJsonObject);
			CustomMessage->SetStringField("role", Role == TEXT("model") ? TEXT("assistant") : Role);
			CustomMessage->SetStringField("content", Text);
			CustomMessages.Add(MakeShareable(new FJsonValueObject(CustomMessage)));
		}
		JsonPayload->SetArrayField(TEXT("messages"), CustomMessages);
	}
	else if (ProviderStr == TEXT("Gemini"))
	{
		FString GeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
		ActiveArchitectHttpRequest->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *GeminiModel, *ApiKeyToUse));
		UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: Gemini API Request with model: %s"), *GeminiModel);
		TArray<TSharedPtr<FJsonValue>> GeminiContents;
		TSharedPtr<FJsonObject> SystemTurn = MakeShareable(new FJsonObject);
		SystemTurn->SetStringField(TEXT("role"), TEXT("user"));
		TArray<TSharedPtr<FJsonValue>> SystemParts;
		TSharedPtr<FJsonObject> SystemPart = MakeShareable(new FJsonObject);
		SystemPart->SetStringField(TEXT("text"), SystemPrompt);
		SystemParts.Add(MakeShareable(new FJsonValueObject(SystemPart)));
		SystemTurn->SetArrayField(TEXT("parts"), SystemParts);
		GeminiContents.Add(MakeShareable(new FJsonValueObject(SystemTurn)));
		TSharedPtr<FJsonObject> ModelAck = MakeShareable(new FJsonObject);
		ModelAck->SetStringField(TEXT("role"), TEXT("model"));
		TArray<TSharedPtr<FJsonValue>> AckParts;
		TSharedPtr<FJsonObject> AckPart = MakeShareable(new FJsonObject);
		AckPart->SetStringField(TEXT("text"), TEXT("Understood. I will generate Blueprint pseudo-code and use tools as needed."));
		AckParts.Add(MakeShareable(new FJsonValueObject(AckPart)));
		ModelAck->SetArrayField(TEXT("parts"), AckParts);
		GeminiContents.Add(MakeShareable(new FJsonValueObject(ModelAck)));
		bool bFileContextAdded = false;
		for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role = MsgObj->GetStringField(TEXT("role"));
			if (Role == TEXT("context")) continue;
			if (!bFileContextAdded && !ArchitectFileContextForAPI.IsEmpty() && Role == TEXT("user"))
			{
				const TArray<TSharedPtr<FJsonValue>>* PartsArray;
				if (MsgObj->TryGetArrayField(TEXT("parts"), PartsArray) && PartsArray->Num() > 0)
				{
					FString OriginalText;
					if ((*PartsArray)[0]->AsObject()->TryGetStringField(TEXT("text"), OriginalText))
					{
						TSharedPtr<FJsonObject> ModifiedMessage = MakeShareable(new FJsonObject);
						ModifiedMessage->SetStringField(TEXT("role"), TEXT("user"));
						TArray<TSharedPtr<FJsonValue>> NewParts;
						TSharedPtr<FJsonObject> NewPart = MakeShareable(new FJsonObject);
						NewPart->SetStringField(TEXT("text"), ArchitectFileContextForAPI + OriginalText);
						NewParts.Add(MakeShareable(new FJsonValueObject(NewPart)));
						ModifiedMessage->SetArrayField(TEXT("parts"), NewParts);
						GeminiContents.Add(MakeShareable(new FJsonValueObject(ModifiedMessage)));
						bFileContextAdded = true;
						continue;
					}
				}
			}
			GeminiContents.Add(Message);
		}
		JsonPayload->SetArrayField(TEXT("contents"), GeminiContents);
	}
	else if (ProviderStr == TEXT("OpenAI"))
	{
		ActiveArchitectHttpRequest->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		ActiveArchitectHttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField("model", FApiKeyManager::Get().GetActiveOpenAIModel());
		UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: OpenAI API Request"));
		TArray<TSharedPtr<FJsonValue>> OpenAiMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", SystemPrompt);
		OpenAiMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		bool bFileContextAddedOpenAI = false;
		for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role = MsgObj->GetStringField(TEXT("role"));
			if (Role == TEXT("context")) continue;
			FString Text = MsgObj->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text"));
			if (!bFileContextAddedOpenAI && !ArchitectFileContextForAPI.IsEmpty() && Role == TEXT("user"))
			{
				Text = ArchitectFileContextForAPI + Text;
				bFileContextAddedOpenAI = true;
			}
			TSharedPtr<FJsonObject> OpenAiMessage = MakeShareable(new FJsonObject);
			OpenAiMessage->SetStringField("role", Role == TEXT("model") ? TEXT("assistant") : Role);
			OpenAiMessage->SetStringField("content", Text);
			OpenAiMessages.Add(MakeShareable(new FJsonValueObject(OpenAiMessage)));
		}
		JsonPayload->SetArrayField(TEXT("messages"), OpenAiMessages);
	}
	else if (ProviderStr == TEXT("Claude"))
	{
		ActiveArchitectHttpRequest->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		ActiveArchitectHttpRequest->SetHeader(TEXT("x-api-key"), ApiKeyToUse);
		ActiveArchitectHttpRequest->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), FApiKeyManager::Get().GetActiveClaudeModel());
		JsonPayload->SetNumberField(TEXT("max_tokens"), 8192);
		JsonPayload->SetStringField(TEXT("system"), SystemPrompt);
		UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: Claude API Request"));
		TArray<TSharedPtr<FJsonValue>> ClaudeMessages;
		bool bFileContextAddedClaude = false;
		for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role = MsgObj->GetStringField(TEXT("role"));
			if (Role == TEXT("context")) continue;
			FString Text = MsgObj->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text"));
			if (!bFileContextAddedClaude && !ArchitectFileContextForAPI.IsEmpty() && Role == TEXT("user"))
			{
				Text = ArchitectFileContextForAPI + Text;
				bFileContextAddedClaude = true;
			}
			TSharedPtr<FJsonObject> ClaudeMessage = MakeShareable(new FJsonObject);
			ClaudeMessage->SetStringField("role", Role == TEXT("model") ? TEXT("assistant") : Role);
			TArray<TSharedPtr<FJsonValue>> ContentArray;
			TSharedPtr<FJsonObject> TextPart = MakeShareable(new FJsonObject);
			TextPart->SetStringField("type", "text");
			TextPart->SetStringField("text", Text);
			ContentArray.Add(MakeShareable(new FJsonValueObject(TextPart)));
			ClaudeMessage->SetArrayField("content", ContentArray);
			ClaudeMessages.Add(MakeShareable(new FJsonValueObject(ClaudeMessage)));
		}
		JsonPayload->SetArrayField(TEXT("messages"), ClaudeMessages);
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		ActiveArchitectHttpRequest->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		ActiveArchitectHttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), TEXT("deepseek-chat"));
		UE_LOG(LogTemp, Log, TEXT("BP Gen Architect: DeepSeek API Request"));
		TArray<TSharedPtr<FJsonValue>> DeepSeekMessages;
		TSharedPtr<FJsonObject> SystemMessage = MakeShareable(new FJsonObject);
		SystemMessage->SetStringField("role", "system");
		SystemMessage->SetStringField("content", SystemPrompt);
		DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(SystemMessage)));
		bool bFileContextAddedDeepSeek = false;
		for (const TSharedPtr<FJsonValue>& Message : ArchitectConversationHistory)
		{
			const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
			FString Role = MsgObj->GetStringField(TEXT("role"));
			if (Role == TEXT("context")) continue;
			FString Text = MsgObj->GetArrayField(TEXT("parts"))[0]->AsObject()->GetStringField(TEXT("text"));
			if (!bFileContextAddedDeepSeek && !ArchitectFileContextForAPI.IsEmpty() && Role == TEXT("user"))
			{
				Text = ArchitectFileContextForAPI + Text;
				bFileContextAddedDeepSeek = true;
			}
			TSharedPtr<FJsonObject> DeepSeekMessage = MakeShareable(new FJsonObject);
			DeepSeekMessage->SetStringField("role", Role == TEXT("model") ? TEXT("assistant") : Role);
			DeepSeekMessage->SetStringField("content", Text);
			DeepSeekMessages.Add(MakeShareable(new FJsonValueObject(DeepSeekMessage)));
		}
		JsonPayload->SetArrayField(TEXT("messages"), DeepSeekMessages);
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	ActiveArchitectHttpRequest->SetContentAsString(RequestBody);
	ActiveArchitectHttpRequest->OnProcessRequestComplete().BindSP(this, &SBpGeneratorUltimateWidget::OnArchitectApiResponseReceived);
	int32 RequestSize = RequestBody.Len();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Request payload: %d chars (~%d tokens)"), RequestSize, RequestSize / 4);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== SENDING API REQUEST... ==="));
	ActiveArchitectHttpRequest->ProcessRequest();
}
void SBpGeneratorUltimateWidget::OnArchitectApiResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	double ResponseTime = FPlatformTime::Seconds();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== API RESPONSE RECEIVED ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Success: %s | Response Valid: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		Response.IsValid() ? TEXT("true") : TEXT("false"));
	if (Response.IsValid())
	{
		int32 ResponseSize = Response->GetContentAsString().Len();
		int32 ResponseTokens = ResponseSize / 4;
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Response: %d chars (~%d tokens) | HTTP Code: %d"),
			ResponseSize, ResponseTokens, Response->GetResponseCode());
	}
	ActiveArchitectHttpRequest.Reset();
	FString AiMessage;
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("BP Gen Architect: HTTP Request Failed!"));
		UE_LOG(LogTemp, Error, TEXT("  bWasSuccessful: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("  Response Valid: %s"), Response.IsValid() ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("  Provider: %s"), *FApiKeyManager::Get().GetActiveProvider());
		AiMessage = TEXT("Error: HTTP request failed. Check your internet connection and API key.");
	}
	else if (Response->GetResponseCode() != 200)
	{
		FString ResponseBody = Response->GetContentAsString();
		UE_LOG(LogTemp, Error, TEXT("BP Gen Architect: API Returned Error!"));
		UE_LOG(LogTemp, Error, TEXT("  Response Code: %d"), Response->GetResponseCode());
		UE_LOG(LogTemp, Error, TEXT("  Response Body: %s"), *ResponseBody);
		UE_LOG(LogTemp, Error, TEXT("  Provider: %s"), *FApiKeyManager::Get().GetActiveProvider());
		AiMessage = FString::Printf(TEXT("Error: API returned error code %d\n\nResponse: %s"),
			Response->GetResponseCode(), *ResponseBody.Left(500));
	}
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString ProviderStr = FApiKeyManager::Get().GetActiveProvider();
			if (ProviderStr == TEXT("Gemini"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Candidates;
				if (JsonObject->TryGetArrayField(TEXT("candidates"), Candidates) && Candidates->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& CandidateObject = (*Candidates)[0]->AsObject();
					const TSharedPtr<FJsonObject>* Content = nullptr;
					if (CandidateObject->TryGetObjectField(TEXT("content"), Content))
					{
						const TArray<TSharedPtr<FJsonValue>>* Parts = nullptr;
						if ((*Content)->TryGetArrayField(TEXT("parts"), Parts) && Parts->Num() > 0)
						{
							(*Parts)[0]->AsObject()->TryGetStringField(TEXT("text"), AiMessage);
						}
					}
					const TSharedPtr<FJsonObject>* UsageMetadata = nullptr;
					if (CandidateObject->TryGetObjectField(TEXT("usageMetadata"), UsageMetadata))
					{
						int32 TotalTokenCount = 0;
						if ((*UsageMetadata)->TryGetNumberField(TEXT("totalTokenCount"), TotalTokenCount))
						{
							UpdateConversationTokens(ActiveArchitectChatID, TotalTokenCount, TotalTokenCount);
						}
					}
				}
			}
			else if (ProviderStr == TEXT("OpenAI") || ProviderStr == TEXT("Custom") || ProviderStr == TEXT("DeepSeek"))
			{
				const TArray<TSharedPtr<FJsonValue>>* Choices;
				if (JsonObject->TryGetArrayField(TEXT("choices"), Choices) && Choices->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& ChoiceObject = (*Choices)[0]->AsObject();
					const TSharedPtr<FJsonObject>* MessageObject = nullptr;
					if (ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject))
					{
						(*MessageObject)->TryGetStringField(TEXT("content"), AiMessage);
					}
				}
				const TSharedPtr<FJsonObject>* UsageObj = nullptr;
				if (JsonObject->TryGetObjectField(TEXT("usage"), UsageObj))
				{
					int32 PromptTokens = 0;
					int32 CompletionTokens = 0;
					(*UsageObj)->TryGetNumberField(TEXT("prompt_tokens"), PromptTokens);
					(*UsageObj)->TryGetNumberField(TEXT("completion_tokens"), CompletionTokens);
					UpdateConversationTokens(ActiveArchitectChatID, PromptTokens, CompletionTokens);
				}
			}
			else if (ProviderStr == TEXT("Claude"))
			{
				const TArray<TSharedPtr<FJsonValue>>* ContentBlocks = nullptr;
				if (JsonObject->TryGetArrayField(TEXT("content"), ContentBlocks) && ContentBlocks->Num() > 0)
				{
					const TSharedPtr<FJsonObject>& FirstBlock = (*ContentBlocks)[0]->AsObject();
					FirstBlock->TryGetStringField(TEXT("text"), AiMessage);
				}
				const TSharedPtr<FJsonObject>* UsageObj = nullptr;
				if (JsonObject->TryGetObjectField(TEXT("usage"), UsageObj))
				{
					int32 InputTokens = 0;
					int32 OutputTokens = 0;
					(*UsageObj)->TryGetNumberField(TEXT("input_tokens"), InputTokens);
					(*UsageObj)->TryGetNumberField(TEXT("output_tokens"), OutputTokens);
					UpdateConversationTokens(ActiveArchitectChatID, InputTokens, OutputTokens);
				}
			}
		}
	}
	if (AiMessage.IsEmpty())
	{
		int32 ResponseCode = Response.IsValid() ? Response->GetResponseCode() : 0;
		FString ResponseBody = Response.IsValid() ? Response->GetContentAsString() : TEXT("No response");
		UE_LOG(LogTemp, Error, TEXT("BP Gen Architect: API Request Failed!"));
		UE_LOG(LogTemp, Error, TEXT("  Response Code: %d"), ResponseCode);
		UE_LOG(LogTemp, Error, TEXT("  Response Body: %s"), *ResponseBody);
		UE_LOG(LogTemp, Error, TEXT("  Provider: %s"), *FApiKeyManager::Get().GetActiveProvider());
		AiMessage = FString::Printf(TEXT("Error: Failed to get a valid response for the Architect.\n\nResponse Code: %d\n\nResponse: %s"),
			ResponseCode, *ResponseBody.Left(500));
	}
	if (FApiKeyManager::Get().GetActiveApiKey().IsEmpty())
	{
		int32 TotalCharsInInteraction = LastArchitectUserMessageLength + AiMessage.Len();
		FString EncryptedUsageString;
		int32 CurrentCharacters = 0;
		if (GConfig->GetString(_obf_D, _obf_E, EncryptedUsageString, GEditorPerProjectIni) && !EncryptedUsageString.IsEmpty())
		{
			const int32 EncryptedValue = FCString::Atoi(*EncryptedUsageString);
			CurrentCharacters = EncryptedValue ^ _obf_B;
		}
		CurrentCharacters += TotalCharsInInteraction;
		const int32 ValueToStore = CurrentCharacters ^ _obf_B;
		GConfig->SetString(_obf_D, _obf_E, *FString::FromInt(ValueToStore), GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}
	TSharedPtr<FJsonObject> ModelContent = MakeShareable(new FJsonObject);
	ModelContent->SetStringField(TEXT("role"), TEXT("model"));
	TArray<TSharedPtr<FJsonValue>> ModelParts;
	TSharedPtr<FJsonObject> ModelPartText = MakeShareable(new FJsonObject);
	ModelPartText->SetStringField(TEXT("text"), AiMessage);
	ModelParts.Add(MakeShareable(new FJsonValueObject(ModelPartText)));
	ModelContent->SetArrayField(TEXT("parts"), ModelParts);
	ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(ModelContent)));
	SaveArchitectChatHistory(ActiveArchitectChatID);
	RefreshArchitectChatView();
	if (ArchitectTokenCounterText.IsValid())
	{
		int32 TokenCount = EstimateFullRequestTokens(ArchitectConversationHistory);
		ArchitectTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: ~%d"), TokenCount)));
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== HTTP RESPONSE COMPLETE - CALLING ProcessArchitectResponse ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  AiMessage length: %d chars"), AiMessage.Len());
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  AiMessage preview: %s"), *AiMessage.Left(200));
	ProcessArchitectResponse(AiMessage);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ProcessArchitectResponse RETURNED ==="));
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== SETTING MEMORY EXTRACTION TIMER (2s delay) ==="));
		FTimerHandle MemoryExtractionTimer;
		GEditor->GetTimerManager()->SetTimer(
			MemoryExtractionTimer,
			FTimerDelegate::CreateSP(this, &SBpGeneratorUltimateWidget::ExtractMemoriesFromConversation),
			2.0f,
			false
		);
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== MEMORY EXTRACTION SKIPPED (not successful response) ==="));
	}
}
FString SBpGeneratorUltimateWidget::GenerateArchitectSystemPrompt()
{
	double StartTime = FPlatformTime::Seconds();
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== GENERATING SYSTEM PROMPT ==="));
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  PatternsInjected: %d, Mode: %d"), bPatternsInjectedForSession, (int32)ArchitectInteractionMode);
	FString Ret = TEXT("You are the Blueprint Architect, an AI assistant specialized in generating Unreal Engine Blueprint logic.\n\n");
	FString StaticPrompt = GetCachedStaticSystemPrompt();
	Ret += StaticPrompt;
	int32 StaticTokens = StaticPrompt.Len() / 4;
	int32 PatternTokens = 0;
	if (bPatternsInjectedForSession)
	{
		Ret += TEXT("\n\n");
		FString Patterns = GetCachedPatternsSection();
		Ret += Patterns;
		PatternTokens = Patterns.Len() / 4;
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Patterns included: ~%d tokens"), PatternTokens);
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Patterns NOT included (not yet injected)"));
	}
	int32 CustomTokens = 0;
	if (!CustomInstructions.IsEmpty())
	{
		Ret += TEXT("\\n\\n=== USER CUSTOM INSTRUCTIONS ===\\n");
		Ret += CustomInstructions;
		Ret += TEXT("\\n=== END CUSTOM INSTRUCTIONS ===\\n");
		CustomTokens = CustomInstructions.Len() / 4;
	}
	FString AssetSummary = FAssetReferenceManager::Get().GetSummaryForAI();
	int32 AssetTokens = 0;
	if (!AssetSummary.IsEmpty())
	{
		Ret += TEXT("\\n\\n");
		Ret += AssetSummary;
		AssetTokens = AssetSummary.Len() / 4;
	}
	int32 GddTokens = 0;
	FString GddContent = GetGddContentForAI();
	if (!GddContent.IsEmpty())
	{
		Ret += TEXT("\\n\\n");
		Ret += GddContent;
		GddTokens = GetGddTokenCount();
	}
	int32 AiMemoryTokens = 0;
	FString AiMemoryContent = GetAiMemoryContentForAI();
	if (!AiMemoryContent.IsEmpty())
	{
		Ret += TEXT("\\n\\n");
		Ret += AiMemoryContent;
		AiMemoryTokens = GetAiMemoryTokenCount();
	}
	Ret += TEXT("\\n\\n**MANDATORY: CLICKABLE ASSET LINKS**\\n");
	Ret += TEXT("When mentioning ANY asset name, you MUST wrap it in a clickable link: `[AssetName](ue://asset?name=AssetName)`\\n\\n");
	Ret += TEXT("Examples:\\n");
	Ret += TEXT("- Write `[BP_Player](ue://asset?name=BP_Player)` not just BP_Player\\n");
	Ret += TEXT("- Write `[WBP_Menu](ue://asset?name=WBP_Menu)` not just WBP_Menu\\n\\n");
	Ret += TEXT("Rules: Link EVERY asset name in plain text. Do NOT link inside code blocks. Use asset name only.");
	int32 TotalTokens = Ret.Len() / 4;
	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== SYSTEM PROMPT GENERATED ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Static: ~%d | Patterns: ~%d | Custom: ~%d | Assets: ~%d | GDD: ~%d | AI Memory: ~%d"),
		StaticTokens, PatternTokens, CustomTokens, AssetTokens, GddTokens, AiMemoryTokens);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TOTAL: ~%d tokens (%d chars) in %.2fms"), TotalTokens, Ret.Len(), ElapsedMs);
	return Ret;
}
bool SBpGeneratorUltimateWidget::ShouldInvalidateStaticCache() const
{
	FString CurrentProvider = FApiKeyManager::Get().GetActiveSlot().Provider;
	if (CurrentProvider != CachedProviderForPrompt)
	{
		return true;
	}
	if (ArchitectInteractionMode != CachedInteractionMode)
	{
		return true;
	}
	return false;
}
void SBpGeneratorUltimateWidget::InvalidatePromptCaches()
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== PROMPT CACHE INVALIDATION ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Previous state: StaticValid=%d, PatternsValid=%d, PatternsInjected=%d"),
		bStaticPromptCacheValid, bPatternsCacheValid, bPatternsInjectedForSession);
	bStaticPromptCacheValid = false;
	bPatternsCacheValid = false;
	CachedStaticSystemPrompt.Empty();
	CachedPatternsSection.Empty();
	CachedStaticPromptHash = 0;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Caches cleared (PatternsInjected preserved=%d)"), bPatternsInjectedForSession);
}
FString SBpGeneratorUltimateWidget::GetCachedStaticSystemPrompt()
{
	double StartTime = FPlatformTime::Seconds();
	if (ShouldInvalidateStaticCache())
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== CACHE INVALIDATION TRIGGERED ==="));
		InvalidatePromptCaches();
	}
	CachedProviderForPrompt = FApiKeyManager::Get().GetActiveSlot().Provider;
	CachedInteractionMode = ArchitectInteractionMode;
	if (bStaticPromptCacheValid && !CachedStaticSystemPrompt.IsEmpty())
	{
		int32 CachedTokens = CachedStaticSystemPrompt.Len() / 4;
		double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== STATIC PROMPT CACHE HIT ==="));
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Tokens: ~%d chars: %d time: %.2fms"), CachedTokens, CachedStaticSystemPrompt.Len(), ElapsedMs);
		return CachedStaticSystemPrompt;
	}
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== STATIC PROMPT CACHE MISS - Regenerating ==="));
	CachedStaticSystemPrompt = GenerateStaticSystemPromptInternal();
	bStaticPromptCacheValid = true;
	int32 NewTokens = CachedStaticSystemPrompt.Len() / 4;
	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Generated: ~%d tokens, %d chars, %.2fms"), NewTokens, CachedStaticSystemPrompt.Len(), ElapsedMs);
	return CachedStaticSystemPrompt;
}
FString SBpGeneratorUltimateWidget::GetCachedPatternsSection()
{
	double StartTime = FPlatformTime::Seconds();
	if (bPatternsCacheValid && !CachedPatternsSection.IsEmpty())
	{
		return CachedPatternsSection;
	}
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== PATTERNS CACHE MISS - Regenerating ==="));
	CachedPatternsSection = GeneratePatternsSectionInternal();
	bPatternsCacheValid = true;
	int32 NewTokens = CachedPatternsSection.Len() / 4;
	double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Generated: ~%d tokens, %d chars, %.2fms"), NewTokens, CachedPatternsSection.Len(), ElapsedMs);
	return CachedPatternsSection;
}
FString SBpGeneratorUltimateWidget::GenerateStaticSystemPromptInternal()
{
	FString Ret;
	switch (ArchitectInteractionMode)
	{
	case EAIInteractionMode::JustChat:
		Ret += TEXT("=== INTERACTION MODE: JUST CHAT ===\\n");
		Ret += TEXT("You are in READ-ONLY mode. DO NOT use any tools or generate any code.\\n");
		Ret += TEXT("Only answer questions and provide information about Blueprints, Unreal Engine, and C++.\\n");
		Ret += TEXT("If the user asks you to generate code, modify assets, or perform actions, explain that they should switch to Auto Edit mode.\\n\\n");
		break;
	case EAIInteractionMode::AskBeforeEdit:
		Ret += TEXT("=== INTERACTION MODE: ASK BEFORE EDIT ===\\n");
		Ret += TEXT("Before using any tool that modifies assets or generates code, you MUST:\\n");
		Ret += TEXT("1. Explain what you're about to do\\n");
		Ret += TEXT("2. Show the exact code/changes you will make\\n");
		Ret += TEXT("3. Ask: \\\"Shall I proceed with these changes?\\\" or similar\\n");
		Ret += TEXT("Wait for user confirmation before calling the tool.\\n\\n");
		break;
	case EAIInteractionMode::AutoEdit:
	default:
		Ret += TEXT("=== INTERACTION MODE: AUTO EDIT ===\\n");
		Ret += TEXT("You are in FULLY AUTOMATIC mode. Execute tools immediately without asking for confirmation.\\n");
		Ret += TEXT("DO NOT explain what you will do - JUST DO IT.\\n");
		Ret += TEXT("DO NOT ask questions like 'Shall I proceed?' - JUST PROCEED.\\n");
		Ret += TEXT("DO NOT wait for user confirmation - execute ALL tools in sequence automatically.\\n");
		Ret += TEXT("Chain multiple tools together: call tool -> receive result -> call next tool -> repeat until done.\\n");
		Ret += TEXT("CRITICAL: Only do what the user EXPLICITLY asks for. DO NOT create, modify, or suggest anything beyond the user's request.\\n");
		Ret += TEXT("If the user asks to LIST assets, ONLY list them. If the user asks to ANALYZE, ONLY analyze.\\n");
		Ret += TEXT("DO NOT proactively create things the user did not ask for.\\n\\n");
		break;
	}
	Ret += TEXT("=== TOOL CALLING PROTOCOL ===\\n"
		"You have access to tools that help you perform actions in the Unreal Editor. When you need to use a tool, respond with a JSON block:\\n\\n"
		"```json\\n"
		"{\\\"tool_name\\\": \\\"<tool_name>\\\", \\\"arguments\\\": {<arguments>}}\\n"
		"```\\n\\n"
		"You may include conversational text before or after the JSON block. After each tool call, you will receive the results back. "
		"You can then continue the conversation, call another tool, or provide a final answer.\\n"
		"When you are done and have no more tools to call, respond WITHOUT a JSON tool block - just provide your final answer or code.\\n\\n"
		"=== INTENT CLASSIFICATION (CALL THIS FIRST) ===\\n"
		"Before doing anything else, classify the user's intent by calling:\\n"
		"```json\\n{\\\"tool_name\\\": \\\"classify_intent\\\", \\\"arguments\\\": {\\\"user_message\\\": \\\"<the user's message>\\\"}}\\n```\\n"
		"This returns: CODE, ASSET, MODEL, TEXTURE, or CHAT. Then proceed accordingly:\\n"
		"- CODE: Call generate_blueprint_logic or other code tools\\n"
		"- ASSET: Call create_blueprint, create_enum, create_struct, create_data_table, create_material, create_input_action, create_input_mapping_context, add_input_mapping to CREATE new assets\\n"
		"- MODEL: Call 3D model generation tools\\n"
		"- TEXTURE: Call generate_texture or generate_pbr_material\\n"
		"- CHAT: Just respond conversationally, no tools needed\\n\\n"
		"=== AVAILABLE TOOLS ===\\n\\n"
		"**classify_intent** - Classify user intent. Args: user_message. Returns: CODE, ASSET, MODEL, TEXTURE, or CHAT\\n"
		"**find_asset_by_name** - Search for assets by name pattern. Args: name_pattern, asset_type (optional)\\n"
		"**generate_blueprint_logic** - Compile C++ pseudo-code to Blueprint. Args: code, blueprint_path (optional)\\n"
		"**get_current_folder** - Get current Content Browser folder. Args: none\\n"
		"**list_assets_in_folder** - List assets in folder. Args: folder_path (optional), asset_type, recursive\\n"
		"**create_blueprint** - Create new Blueprint. Args: name, parent_class, save_path\\n"
		"**add_component** - Add component to Blueprint. Args: blueprint_path, component_class, component_name\\n"
		"**add_variable** - Add variable to Blueprint. Args: blueprint_path, var_name, var_type, default_value (optional), category (optional). Supports: basic types, TArray<Type>, TSubclassOf<Class>, asset paths (/Game/...), structs, enums, widget refs\\n"
		"**get_asset_summary** - Get asset description. Args: asset_path\\n"
		"**get_selected_assets** - Get selected Content Browser assets. Args: none\\n"
		"**spawn_actors** - Spawn actors in level (bulk). Args: actors array\\n"
		"**get_all_scene_actors** - Get all level actors. Args: none\\n"
		"**create_material** - Create material. Args: name, save_path, color\\n"
		"**generate_pbr_material** - Generate PBR with textures. Args: name, description, save_path\\n"
		"**generate_texture** - Generate single texture. Args: prompt, texture_name, save_path\\n"
		"**create_material_from_textures** - Create material from textures. Args: material_name, source, folder_path\\n"
		"**create_model_3d** - Generate 3D mesh from text. Args: prompt (required), model_name (optional), save_path (optional, default /Game/GeneratedModels)\\n"
		"**image_to_model_3d** - Generate 3D mesh from image. Args: image_path (required), prompt (optional), model_name (optional), save_path (optional)\\n"
		"**set_actor_materials** - Apply material to actors. Args: material_path, actor_labels\\n"
		"**get_api_context** - Search API cheatsheet. Args: query\\n"
		"**get_selected_nodes** - Get selected graph nodes. Args: none\\n"
		"**create_struct** - Create struct. Args: name, save_path, variables\\n"
		"**create_enum** - Create enum. Args: name, save_path, enumerators\\n"
		"**get_data_asset_details** - Get enum/struct members. Args: asset_path\\n"
		"**edit_component_property** - Edit component property. Args: blueprint_path, component_name, property_name, value\\n"
		"**select_actors** - Select actors in level. Args: actor_labels\\n"
		"**create_project_folder** - Create folder. Args: folder_path\\n"
		"**move_asset** - Move single asset. Args: asset_path, destination_folder\\n"
		"**move_assets** - Move multiple assets. Args: asset_paths, destination_folder\\n"
		"**get_detailed_blueprint_summary** - Detailed Blueprint analysis. Args: blueprint_path\\n"
		"**get_material_graph_summary** - Material graph description. Args: material_path\\n"
		"**insert_code** - Insert code at selected node. Args: blueprint_path, code\\n"
		"**get_project_root_path** - Get project path. Args: none\\n"
		"**add_widget_to_user_widget** - Add widget. Args: widget_path, widget_type, widget_name, parent_name\\n"
		"**edit_widget_property** - Edit widget property. Args: widget_path, widget_name, property_name, value\\n"
		"**get_batch_widget_properties** - Get widget properties. Args: widget_classes\\n"
		"**add_widgets_to_layout** - Add multiple widgets. Args: widget_path, layout\\n"
		"**create_widget_from_layout** - Create widget hierarchy. Args: widget_path, layout\\n"
		"**get_behavior_tree_summary** - Behavior Tree description. Args: bt_path\\n\\n"
		"=== WIDGET CREATION WORKFLOW (MANDATORY) ===\\n"
		"STEP 1: Call get_current_folder to get save path\\n"
		"STEP 2: Call create_blueprint with parent_class=\\\"UserWidget\\\" to create the widget\\n"
		"STEP 3: Call create_widget_from_layout with the layout\\n"
		"NEVER skip create_blueprint - the widget MUST exist before create_widget_from_layout!\\n\\n"
		"=== WIDGET LAYOUT FORMAT ===\\n"
		"layout MUST be an ARRAY of widget objects.\\n"
		"Each widget needs: type, name, parent_name (empty for root), properties (array of {key,value}), children (array).\\n\\n"
		"CRITICAL PROPERTY NAMES (use EXACTLY these):\\n"
		"- Slot.Position: \\\"(50,50)\\\" - position relative to anchor\\n"
		"- Slot.Size: \\\"(200,30)\\\" - width and height\\n"
		"- Slot.Anchors: \\\"top_left\\\" or \\\"center\\\" or \\\"bottom_right\\\" (preset names only)\\n\\n"
		"WRONG - DO NOT USE: AnchorData, Anchors, Offsets, SizeToContent\\n"
		"CORRECT: Slot.Position, Slot.Size, Slot.Anchors\\n\\n"
		"Example widget: {\\\"type\\\":\\\"TextBlock\\\",\\\"name\\\":\\\"Title\\\",\\\"parent_name\\\":\\\"RootCanvas\\\",\\\"properties\\\":[{\\\"key\\\":\\\"Slot.Anchors\\\",\\\"value\\\":\\\"top_left\\\"},{\\\"key\\\":\\\"Slot.Position\\\",\\\"value\\\":\\\"(50,50)\\\"},{\\\"key\\\":\\\"Slot.Size\\\",\\\"value\\\":\\\"(200,30)\\\"},{\\\"key\\\":\\\"Text\\\",\\\"value\\\":\\\"Hello\\\"}]}\\n\\n"
		"**edit_data_asset_defaults** - Edit Data Asset values. Args: asset_path, edits\\n"
		"**delete_variable** - Delete variable. Args: blueprint_path, var_name\\n"
		"**categorize_variables** - Organize variables. Args: blueprint_path, categories\\n"
		"**delete_component** - Delete component. Args: blueprint_path, component_name\\n"
		"**delete_widget** - Delete widget. Args: widget_path, widget_name\\n"
		"**delete_nodes** - Delete selected nodes. Args: none\\n"
		"**delete_function** - Delete function. Args: blueprint_path, function_name\\n"
		"**undo_last_generated** - Undo last code gen. Args: none\\n"
		"**check_project_source** - Check for C++ Source folder. Args: none\\n"
		"**select_folder** - Open folder picker. Args: none\\n"
		"**scan_directory** - Scan for C++ files. Args: directory_path\\n"
		"**read_cpp_file** - Read C++ file. Args: file_path\\n"
		"**write_cpp_file** - Write C++ file. Args: file_path, content\\n"
		"**create_data_table** - Create DataTable asset. Args: table_name, save_path (optional), row_struct (optional), rows (optional array)\\n"
		"**add_data_table_rows** - Add rows to existing DataTable. Args: table_path, rows (array of {row_name, field1, field2, ...})\\n"
		"**create_input_action** - Create Input Action for Enhanced Input. Args: name, save_path (optional), value_type (bool, float, vector2d, vector3d)\\n"
		"**create_input_mapping_context** - Create Input Mapping Context. Args: name, save_path (optional), mappings (optional array of {action, key})\\n"
		"**add_input_mapping** - Add key mapping to existing context. Args: context_path, action (or action_path), key\\n\\n"
		"=== CRITICAL WORKFLOW RULES ===\\n"
		"1. When user mentions a blueprint name, use list_assets_in_folder to find it ONCE.\\n"
		"2. When user does NOT specify a Blueprint, call get_selected_assets FIRST.\\n"
		"3. ONE TOOL PER RESPONSE. Wait for results before calling next tool.\\n"
		"4. NEVER ask questions - ALWAYS proceed automatically.\\n"
		"5. ALWAYS include 'blueprint_path' in generate_blueprint_logic.\\n"
		"6. BEFORE generating code, use get_api_context to find correct UE functions.\\n"
		"7. For BULK operations, ALWAYS use bulk tools (spawn_actors, set_actor_materials) in SINGLE call.\\n\\n"
		"NOTE: When you need code generation patterns for generate_blueprint_logic, they will be provided to you.\\n\\n"
		"=== CHART GENERATION ===\\n"
		"You HAVE the ability to create visual pie charts. They will be automatically rendered as SVG graphics.\\n"
		"When asked for charts, visualizations, or distributions, use this EXACT format:\\n"
		"```chart\\n"
		"Category1 count1, Category2 count2, Category3 count3\\n"
		"```\\n"
		"Example: Widgets 15, Actors 23, Materials 8\\n"
		"IMPORTANT: You CAN generate charts. Do not say you cannot. Just use the chart code block format.\\n\\n"
		"=== FLOWCHART GENERATION ===\\n"
		"You HAVE the ability to create flowcharts. Use ONLY this exact syntax:\\n"
		"```flow\\n"
		"[Start] -> [Step 1]\\n"
		"[Step 1] -> [Step 2]\\n"
		"[Step 2] -> [End]\\n"
		"```\\n"
		"CRITICAL RULES:\\n"
		"- Every node MUST be in square brackets: [Node Name]\\n"
		"- Connections use arrow: ->\\n"
		"- NO other syntax like st=> or => is valid\\n"
		"- Keep labels short (1-3 words)\\n"
		"- NO parentheses () in labels - use dashes instead: [Attack Type - S_DetermineAttackType]\\n"
		"- NO special characters in labels: only letters, numbers, spaces, dashes, underscores\\n"
		"- NO clickable links inside flowchart nodes - use plain text only\\n"
		"- NO markdown links like [Name](url) inside flow blocks\\n\\n"
	);
	return Ret;
}
FString SBpGeneratorUltimateWidget::GeneratePatternsSectionInternal()
{
	FString Ret;
	Ret += TEXT("=== BLUEPRINT GENERATION PATTERNS ===\n"
		"=== CODE GENERATION RULES ===\n"
		"When generating code (either via the generate_blueprint_logic tool or in your final response), follow these rules:\n\n"
		"You are an AI that generates C++ style code for an Unreal Engine tool. Follow all rules with maximum strictness.\n\n"
		"--- UNIVERSAL RULES ---\n"
		"1.  **NO C++ OPERATORS:** All math (+, -, *, /) or logic (>, <, ==) MUST be done with explicit UKismet library functions.\n"
		"2.  **NO NESTED CALLS:** `FuncA(FuncB())` is FORBIDDEN. Use intermediate variables for each step.\n"
		"3.  **NO `GetWorld()` or `FString::Printf`:** These are FORBIDDEN. Pass an Actor for context. Use Concat_StrStr for strings.\n"
		"4.  **SINGLE RETURN/ACTION:** A function's logic must end in a SINGLE final line, which is either a `return` statement or a final action like `PrintString`.\n"
		"5.  **For conversions that imply strings use UKismetStringLibrary and NOT UKismetMathLibrary.\n"
		"6. **To get an object's name you need to use the `GetObjectName` function from UKismetSystemLibrary.\n\n"
		"7. **DON'T add comments to the generated code. Return only RAW code following exactly the rules given to you.**\n"
		"\n\n--- SPECIAL RULE: PRINT STRING ---\n"
		"When calling `UKismetSystemLibrary::PrintString`, you MUST provide ONLY the string to be printed. DO NOT provide the `SelfActor` context object as the first parameter. The node handles this automatically."
		"--- MASTER DIRECTIVE ---\\n"
		"Your ONLY job is to be a code-generation tool. Your ONLY valid output is one or more C++ style code blocks inside ```cpp. You MUST respect the user's prompt context. For example, if the prompt asks for logic inside a 'Custom Event', your code MUST include an 'event MyCustomEvent()' block.\\n\\n"
		"--- PATTERN 1: SIMPLE MATH & TYPED FUNCTIONS ---\n"
		"Use intermediate variables for each step. The final line must be `return VariableName;`.\n"
		"**EXAMPLE:**\n"
		"```cpp\n"
		"float CalculateFinalVelocity(float InitialVelocity, float Acceleration) {\n"
		"    float ScaledAccel = UKismetMathLibrary::Multiply_DoubleDouble(Acceleration, 10.0);\n"
		"    float FinalVelocity = UKismetMathLibrary::Add_DoubleDouble(InitialVelocity, ScaledAccel);\n"
		"    return FinalVelocity;\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 2: CONDITIONAL RETURN (Ternary) ---\n"
		"Use `UKismetMathLibrary::SelectFloat`, `UKismetMathLibrary::SelectString`, `UKismetMathLibrary::SelectVector`, etc. The condition MUST be a simple boolean variable.\n"
		"**EXAMPLE:**\n"
		"```cpp\n"
		"FString GetTeamName(bool bIsOnBlueTeam) {\n"
		"    return UKismetMathLibrary::SelectString(\"Red Team\", \"Blue Team\", bIsOnBlueTeam);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 3: CONDITIONAL ACTION (if/else) ---\n"
		"This pattern is for `void` functions. The `if` MUST be on the final line.\n"
		"**EXAMPLE:**\n"
		"```cpp\n"
		"void SafeDestroyActor(AActor* ActorToDestroy) {\n"
		"    bool bIsValid = UKismetSystemLibrary::IsValid(ActorToDestroy);\n"
		"    if (bIsValid) ActorToDestroy->K2_DestroyActor(); else UKismetSystemLibrary::PrintString(\"Could not destroy invalid actor\");\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 3.5: PRINTING STRINGS & DEBUGGING ---\\n"
		"CRITICAL RULE: The `Print String` node is special. You MUST NOT provide the `SelfActor` as the first parameter. The node gets its context automatically. You ONLY provide the string value you want to print.\\n\\n"
		"**CORRECT:**\\n`UKismetSystemLibrary::PrintString(MyString);`\\n\\n"
		"**INCORRECT (will create bugs):**\\n`UKismetSystemLibrary::PrintString(SelfActor, MyString);`\\n\\n"
		"**EXAMPLE:**\\n"
		"// User Prompt: \"On begin play, print the actor's own name.\"\\n"
		"```cpp\\n"
		"void OnBeginPlay() {\\n"
		"    FString MyName = UKismetSystemLibrary::GetObjectName(SelfActor);\\n"
		"    // CORRECT: Notice there is no 'SelfActor' here.\\n"
		"    UKismetSystemLibrary::PrintString(MyName);\\n"
		"}\\n"
		"```\\n"
		"--- PATTERN 4: LOOPS ---\n"
		"To loop WITH AN INDEX, you MUST use the `for (ElementType ElementName, int IndexVarName : ArrayName)` syntax. This is the only way to get the index.\n"
		"Manual index variables (e.g., `int Index = 0;`) and manual incrementing (e.g., `Index = Index + 1;`) are STRICTLY FORBIDDEN and will fail.\n"
		"The use of `UKismetArrayLibrary::Array_Find` to get an index is also FORBIDDEN.\n\n"
		"**CORRECT LOOP WITH INDEX EXAMPLE:**\n"
		"```cpp\n"
		"void PrintActorNamesWithIndex(const TArray<AActor*>& ActorArray) {\n"
		"    for (AActor* Actor, int i : ActorArray) {\n"
		"        FString IndexString = UKismetStringLibrary::Conv_IntToString(i);\n"
		"        FString NameString = UKismetSystemLibrary::GetObjectName(Actor);\n"
		"        FString TempString = UKismetStringLibrary::Concat_StrStr(IndexString, \": \");\n"
		"        FString FinalString = UKismetStringLibrary::Concat_StrStr(TempString, NameString);\n"
		"        UKismetSystemLibrary::PrintString(Actor, FinalString);\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 5: ACTIONS ON ACTORS (Member Functions) ---\n"
		"To perform an action ON an actor, you MUST call the function on the actor variable itself using the `->` operator.\n"
		"**CORRECT EXAMPLE:**\n"
		"```cpp\n"
		"void SetAllActorsHidden(const TArray<AActor*>& ActorArray) {\n"
		"    for (AActor* OneActor : ActorArray) {\n"
		"        OneActor->SetActorHiddenInGame(true);\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 6: ENUMS & CONDITIONALS ---\n"
		"To compare enums, use the `EqualEqual_EnumEnum` function. Provide the enum type and value using the `EEnumType::Value` syntax.\n"
		"**EXAMPLE:**\n"
		"```cpp\n"
		"bool IsPlayerFalling(EMovementMode MovementMode) {\n"
		"    return UKismetMathLibrary::EqualEqual_EnumEnum(MovementMode, EMovementMode::MOVE_Falling);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 7: BREAKING STRUCTS ---\n"
		"To get a member from a struct variable, use the `.` operator. **You can only access ONE LEVEL DEEP at a time.** Chained access like `MyStruct.InnerStruct.Value` is FORBIDDEN.\n"
		"To access nested members, you must first get the inner struct into its own variable, and then break that new variable on the next line.\n"
		"**CORRECT EXAMPLE (Nested Access):**\n"
		"```cpp\n"
		"bool GetNestedBool(const FTestStruct& MyTestStruct) {\n"
		"    FInsideStruct Inner = MyTestStruct.InsideStruct;\n"
		"    bool Result = Inner.MemberVar_0;\n"
		"    return Result;\n"
		"}\n"
		"```\n"
		"**CORRECT EXAMPLE (Simple Access):**\n"
		"```cpp\n"
		"FVector GetHitLocation(const FHitResult& MyHitResult) {\n"
		"    return MyHitResult.Location;\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 8: MAKING STRUCTS & TEXT ---\n"
		"To create a new struct, you MUST use the appropriate `Make...` function from `UKismetMathLibrary`.\n"
		"To combine text, you MUST use `Format` from `UKismetTextLibrary`.\n"
		"**CRITICAL:** `MakeTransform` takes a `Vector`, a `Rotator`, and a `Vector`. Do NOT convert the rotator to a Quat.\n"
		"**EXAMPLE (Make Transform):**\n"
		"```cpp\n"
		"FTransform MakeTransformFromParts(FVector Location, FRotator Rotation, FVector Scale) {\n"
		"    return UKismetMathLibrary::MakeTransform(Location, Rotation, Scale);\n"
		"}\n"
		"```\n"
		"**EXAMPLE (Make Text):**\n"
		"```cpp\n"
		"FText CombineTwoStrings(FString A, FString B) {\n"
		"    FText TextA = UKismetTextLibrary::Conv_StringToText(A);\n"
		"    FText TextB = UKismetTextLibrary::Conv_StringToText(B);\n"
		"    FString FormatPattern = \"{0}{1}\";\n"
		"    // This creates a special Format Text node\n"
		"    return UKismetTextLibrary::Format(FormatPattern, TextA, TextB);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 9: NESTED LOGIC (IF inside LOOP) ---\n"
		"You can and should nest `if` statements inside a `for` loop body.\n"
		"**CORRECT EXAMPLE:**\n"
		"```cpp\n"
		"void DestroyValidActors(const TArray<AActor*>& ActorArray) {\n"
		"    for (AActor* Actor : ActorArray) {\n"
		"        bool bIsValid = UKismetSystemLibrary::IsValid(Actor);\n"
		"        if (bIsValid) {\n"
		"            Actor->K2_DestroyActor();\n"
		"        }\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 10: GETTING DATA TABLE ROWS ---\n"
		"To get a row from a Data Table, you MUST use a special 'if' check on 'UGameplayStatics::GetDataTableRow'.\n"
		"This creates a node with 'Row Found' and 'Row Not Found' execution pins.\n"
		"The 'out' row variable MUST be declared on the line immediately before the 'if' statement. This is required to define the output row type.\n"
		"**EXAMPLE:**\n"
		"```cpp\n"
		"void ApplyItemStats(UDataTable* ItemTable, FName ItemID, AActor* TargetActor) {\n"
		"    FItemStruct FoundRow;\n"
		"    if (UGameplayStatics::GetDataTableRow(ItemTable, ItemID, FoundRow)) {\n"
		"        // Code here runs if the row is found\n"
		"        FString Desc = FoundRow.Description;\n"
		"        UKismetSystemLibrary::PrintString(TargetActor, Desc);\n"
		"    }\n"
		"    else {\n"
		"        // Code here runs if the row is NOT found\n"
		"        UKismetSystemLibrary::PrintString(TargetActor, \"Item not found!\");\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 11: EVENT GRAPH LOGIC ---\n"
		"To add logic to the main Event Graph, define a function named `OnBeginPlay` or `OnTick`.\n"
		"1.  **SIGNATURE:** These functions MUST be `void` and take NO parameters. `void OnBeginPlay()` is correct. `void OnBeginPlay(AActor* MyActor)` is FORBIDDEN.\n"
		"2.  **CONTEXT:** Inside these functions, a special `AActor*` variable named `SelfActor` is automatically available, referring to the Blueprint instance itself. Use this for any actions on the actor, like `GetActorLocation` or `K2_DestroyActor`.\n"
		"3.  **SPECIAL VARIABLE (OnTick):** Inside `void OnTick()`, a float variable named `DeltaSeconds` is also automatically available.\n"
		"**EXAMPLE (Begin Play):**\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // This will print the name of the Blueprint instance when it spawns.\n"
		"    FString MyName = UKismetSystemLibrary::GetObjectName(SelfActor);\n"
		"    UKismetSystemLibrary::PrintString(MyName);\n"
		"}\n"
		"```\n"
		"**EXAMPLE (Tick - Move Up):**\n"
		"```cpp\n"
		"void OnTick() {\n"
		"    // This code will be added to the Event Tick node and will move the actor up every frame.\n"
		"    FVector CurrentLocation = SelfActor->GetActorLocation();\n"
		"    FVector UpVector = UKismetMathLibrary::GetUpVector();\n"
		"    float MoveSpeed = 100.0;\n"
		"    float ScaledSpeed = UKismetMathLibrary::Multiply_DoubleDouble(MoveSpeed, DeltaSeconds);\n"
		"    FVector ScaledUp = UKismetMathLibrary::Multiply_VectorFloat(UpVector, ScaledSpeed);\n"
		"    FVector NewLocation = UKismetMathLibrary::Add_VectorVector(CurrentLocation, ScaledUp);\n"
		"    SelfActor->K2_SetActorLocation(NewLocation, false, false, false);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 12: COMPONENT AWARENESS ---\n"
		"To access a component that already exists on the Blueprint, **you MUST treat it as a pre-existing variable**. You can call functions directly on the component's name.\n"
		"**CRITICAL:** The compiler automatically knows about components like 'Muzzle' or 'StaticMesh'. Do NOT declare them yourself.\n"
		"**EXAMPLE (Get Component Location):**\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // This finds a component named 'Muzzle' and prints its world location.\n"
		"    FVector MuzzleLocation = Muzzle->GetWorldLocation();\n"
		"    FString LocString = UKismetStringLibrary::Conv_VectorToString(MuzzleLocation);\n"
		"    UKismetSystemLibrary::PrintString(LocString);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 13: CALLING LOCAL BLUEPRINT FUNCTIONS ---\n"
		"To call a function that you have already created on the *same* Blueprint, you MUST call it by its name directly. The compiler will automatically know it's a local function.\n"
		"**CRITICAL:** Do NOT use `SelfActor->`. Just use the function name.\n"
		"**EXAMPLE:**\n"
		"// Assumes a function 'CheckSprint(bool bIsSprinting)' and a bool 'IsSprinting' already exist on the Blueprint.\n"
		"void OnBeginPlay() {\n"
		"    CheckSprint(IsSprinting);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 13B: USING EXISTING COMPONENTS ---\n"
		"To access a component that is already on the Blueprint (like the 'Mesh' or 'CapsuleComponent' on a Character), you MUST treat it as a pre-existing variable. You can call functions directly on the component's name.\n"
		"**CRITICAL:** The compiler automatically finds these components. Do NOT declare them yourself and do NOT use `Get...()` functions like `GetMesh()` or `GetCharacterMovement()`. The AI should simply use the variable name `Mesh` directly.\n\n"
		"**EXAMPLE (Using the Character's Mesh):**\n"
		"// User Prompt: \"Create a function to toggle the mesh's visibility.\"\n"
		"```cpp\n"
		"void ToggleMeshVisibility() {\n"
		"    bool bIsCurrentlyVisible = Mesh->IsVisible();\n"
		"    bool bNewVisibility = UKismetMathLibrary::Not_PreBool(bIsCurrentlyVisible);\n"
		"    Mesh->SetVisibility(bNewVisibility);\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 14: SWITCH STATEMENTS ---\n"
		"To create a Switch node, you MUST use a standard `switch` statement. The compiler will create the correct Switch node based on the variable's type.\n"
		"**CRITICAL:** For enums, you MUST `case` on the value name ONLY (e.g., `case North:`), not the full `case EMyEnum::North:`.\n\n"
		"**EXAMPLE (Switch on Enum):**\n"
		"```cpp\n"
		"void PrintDirection(E_TestDirection Direction) {\n"
		"    switch (Direction) {\n"
		"        case North:\n"
		"            UKismetSystemLibrary::PrintString(SelfActor, \"Going North\");\n"
		"            break;\n"
		"        case East:\n"
		"            UKismetSystemLibrary::PrintString(SelfActor, \"Going East\");\n"
		"            break;\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"**EXAMPLE (Switch on Name):**\n"
		"```cpp\n"
		"void CheckPlayerLevel(int PlayerLevel) {\n"
		"    FName LevelName = UKismetStringLibrary::Conv_IntToName(PlayerLevel);\n"
		"    switch (LevelName) {\n"
		"        case \"1\":\n"
		"            UKismetSystemLibrary::PrintString(SelfActor, \"Beginner\");\n"
		"            break;\n"
		"        case \"10\":\n"
		"            UKismetSystemLibrary::PrintString(SelfActor, \"Advanced\");\n"
		"            break;\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 15: SETTING EXISTING VARIABLES ---\n"
		"To change the value of a variable that **already exists** on the Blueprint, you MUST use a direct assignment. Do NOT include the type. The compiler finds the existing variable automatically.\n"
		"**CORRECT:** `bWasTriggered = true;`\n"
		"**FORBIDDEN (for existing vars):** `bool bWasTriggered = true;`\n\n"
		"**EXAMPLE (Set on Begin Play):**\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // This assumes a bool variable 'bIsInitialized' already exists on the Blueprint.\n"
		"    bIsInitialized = true;\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 16: CREATING NEW VARIABLES ---\n"
		"When the user's prompt includes \"create\", \"make\", or \"new variable\", you MUST generate a C++-style declaration including the type. The compiler will see the type and create a new, persistent Blueprint variable.\n"
		"**CRITICAL RULE:** If the user asks to create a variable, you MUST output its type (`bool`, `int`, `FString`, etc.) before the variable name.\n\n"
		"**EXAMPLE (Create and Set):**\n"
		"// User Prompt: \"On begin play, create a new boolean called bIsReady and set it to true.\"\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    bool bIsReady = true;\n"
		"}\n"
		"```\n\n"
		"**EXAMPLE (Create without a value):**\n"
		"// User Prompt: \"Create a new integer variable called PlayerScore.\"\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // The compiler will create the variable and give it a default value of 0.\n"
		"    int PlayerScore = 0;\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 17: CUSTOM EVENTS ---\\n"
		"To create a custom event in the Event Graph, you MUST start the signature with the `event` keyword. DO NOT include `void`. Custom events can have input parameters.\\n\\n"
		"**CORRECT:**\\n"
		"```cpp\\n"
		"event MyCustomEvent(float DamageAmount, AActor* DamageCauser) {\\n"
		"    // ... logic ...\\n"
		"}\\n"
		"```\\n\\n"
		"**INCORRECT (will create a function instead):**\\n"
		"```cpp\\n"
		"event void MyCustomEvent() { ... }\\n"
		"```\\n"
		"\\n\\n--- PATTERN 19: TIMELINES AND EVENTS (MASTER PATTERN) ---\\n"
		"CRITICAL RULE: Declaring a `timeline` inside ANY event (`OnBeginPlay`, `OnKeyPressed_F`, `event MyEvent`, etc.) is **STRICTLY FORBIDDEN** and will fail. This is the most important rule for timelines. **If a user asks to create a timeline inside an event, you MUST correct them by declaring the timeline at the top-level and then calling it from the event.**\\n"
		"THE CORRECT STRUCTURE IS ALWAYS TWO PARTS:\\n"
		"1.  **The DECLARATION:** The `timeline MyTimeline() { ... }` block. This MUST ALWAYS be at the top-level of the code, outside of any other function.\\n"
		"2.  **The CALL:** A single line like `MyTimeline.PlayFromStart();`. This line goes INSIDE the event that should trigger the timeline.\\n\\n"
		"**PERFECT EXAMPLE (Handling Multiple Events):**\\n"
		"// User Prompt: \"When the R key is pressed, play a timeline. When the R key is released, print a message.\"\\n"
		"```cpp\\n"
		"timeline MyKeyTimeline() {\\n"
		"    float Alpha;\\n"
		"    Alpha.AddKey(0.0, 0.0);\\n"
		"    Alpha.AddKey(1.0, 1.0);\\n"
		"    OnUpdate() {\\n"
		"    }\\n"
		"}\\n\\n"
		"// Step 2: The 'Pressed' event simply CALLS the timeline. It does not contain the timeline declaration.\\n"
		"void OnKeyPressed_R() {\\n"
		"    MyKeyTimeline.PlayFromStart();\\n"
		"}\\n\\n"
		"// Step 3: The 'Released' event performs a completely different and unrelated action.\\n"
		"void OnKeyReleased_R() {\\n"
		"    UKismetSystemLibrary::PrintString(\"Key was released!\");\\n"
		"}\\n"
		"```\\n\\n"
		"\\n\\n--- PATTERN 21: COMPONENT-BOUND OVERLAP EVENTS ---\n"
		"To bind to a component's overlap event, you MUST use the `ComponentName::EventName` syntax. This finds the component on the Blueprint and creates the event node for you.\n"
		"CRITICAL: The function signature, including all parameter types and names, MUST exactly match the delegate's signature. The compiler knows about `OnComponentBeginOverlap` and `OnComponentEndOverlap`.\n"
		"The logic inside the function body will be wired to the event's output execution pin.\n\n"
		"**EXAMPLE (OnComponentBeginOverlap):**\n"
		"// User Prompt: \"When my 'TriggerVolume' component is overlapped, destroy the other actor.\"\n"
		"```cpp\n"
		"void TriggerVolume::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {\n"
		"    // This code executes when the overlap occurs.\n"
		"    OtherActor->K2_DestroyActor();\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 22: CASTING ---\n"
		"CRITICAL RULE: When casting to a Blueprint class (like 'BP_PlayerCharacter'), you MUST use the exact name given. DO NOT add 'A' or 'U' prefixes.\n"
		"To create a 'Cast To' node, you MUST use an `if` statement with an initializer. This is the only supported syntax for casting.\n"
		"The code inside the `if` block will be wired to the 'Cast Succeeded' pin. The variable you create (`MyCastedChar` in the example) will be available inside this block.\n"
		"The code inside the optional `else` block will be wired to the 'Cast Failed' pin.\n\n"
		"**EXAMPLE (Casting OtherActor from an Overlap Event):**\n"
		"```cpp\n"
		"void TriggerVolume::OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {\n"
		"    // Check if the overlapping actor is our specific character Blueprint\n"
		"    if (BP_MyCharacter* MyCastedChar = Cast<BP_MyCharacter>(OtherActor)) {\n"
		"        MyCastedChar->DoCharacterSpecificFunction();\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 23: COMMON 'GET' FUNCTIONS ---\n"
		"To get common game objects like the Player Character, Controller, or Game Mode, you MUST call the specific static function from `UGameplayStatics`.\n"
		"CRITICAL: The function call MUST be empty, with no parameters. Correct: `GetPlayerCharacter()`. Incorrect: `GetPlayerCharacter(SelfActor, 0)` or `GetGameMode(GetWorld())`.\n\n"
		"**SUPPORTED FUNCTIONS:**\n"
		"- `UGameplayStatics::GetPlayerCharacter()`\n"
		"- `UGameplayStatics::GetPlayerController()`\n"
		"- `UGameplayStatics::GetPlayerPawn()`\n"
		"- `UGameplayStatics::GetGameMode()`\n\n"
		"**EXAMPLE (Get and Cast Player Controller):**\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    APlayerController* PlayerController = UGameplayStatics::GetPlayerController();\n"
		"    if (BP_Controller* MyController = Cast<BP_Controller>(PlayerController)) {\n"
		"        MyController->CheckControls();\n"
		"    }\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 24: DIRECT WIRING & NESTED CALLS ---\\n"
		"CRITICAL: To create clean Blueprints, you MUST wire nodes directly. This is represented in code by nesting function calls. You should nest calls instead of creating temporary variables.\\n\\n"
		"**BAD (Creates extra 'Set' and 'Get' nodes):**\\n"
		"```cpp\\n"
		"OnUpdate() {\\n"
		"    FString ValueString = UKismetStringLibrary::Conv_FloatToString(Alpha); // <-- UNNECESSARY VARIABLE\\n"
		"    UKismetSystemLibrary::PrintString(ValueString);\\n"
		"}\\n"
		"```\\n\\n"
		"**PERFECT (Clean, direct wiring):**\\n"
		"```cpp\\n"
		"OnUpdate() {\\n"
		"    // This nested call creates a direct wire from the conversion node to the Print String node\\n"
		"    UKismetSystemLibrary::PrintString(UKismetStringLibrary::Conv_FloatToString(Alpha));\\n"
		"}\\n"
		"```\\n\\n"
		"\\n\\n--- PATTERN 25: SELF-CONTEXT ACTOR FUNCTIONS (K2_ PREFIX REQUIRED) ---\n"
		"To call a function on the Blueprint actor itself (like getting its location), you MUST use the `SelfActor->FunctionName()` syntax.\n"
		"CRITICAL: For many built-in actor functions, you MUST use the `K2_` prefixed version to ensure the correct Blueprint node is created.\n\n"
		"**CORRECT SYNTAX (MUST USE K2_ PREFIX):**\n"
		"`SelfActor->K2_GetActorLocation()`\n"
		"`SelfActor->K2_GetActorRotation()`\n"
		"`SelfActor->K2_SetActorLocation(...)`\n"
		"`SelfActor->K2_SetActorRotation(...)`\n"
		"`SelfActor->K2_SetActorTransform(...)`\n"
		"`SelfActor->K2_DestroyActor()`\n\n"
		"**INCORRECT SYNTAX (WILL FAIL):**\n"
		"`SelfActor->GetActorLocation()`\n"
		"`SelfActor->DestroyActor()`\n\n"
		"**EXAMPLE (Get Location and Print):**\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // Use the K2_ prefixed function name.\n"
		"    FVector Loc = SelfActor->K2_GetActorLocation();\n"
		"    UKismetSystemLibrary::PrintString(SelfActor, UKismetStringLibrary::Conv_VectorToString(Loc));\n"
		"}\n"
		"```\n\n"
		"--- PATTERN 26: SPAWNING ACTORS (THE ONLY CORRECT WAY) ---\n"
		"CRITICAL RULE: To spawn an actor, you MUST use the C++ template syntax `SpawnActor<T>(...)`. All other function calls like `UGameplayStatics::SpawnActor` or `GetWorld()->SpawnActor` are STRICTLY FORBIDDEN and will fail.\n\n"
		"**EXAMPLE:**\n"
		"// User Prompt: \"On begin play, spawn an actor of class BP_MyProjectile at the location of SelfActor\"\n"
		"```cpp\n"
		"void OnBeginPlay() {\n"
		"    // First, get the transform where we want to spawn the actor.\n"
		"    FVector SpawnLoc = SelfActor->K2_GetActorLocation();\n"
		"    FRotator SpawnRot = SelfActor->K2_GetActorRotation();\n\n"
		"    // This is the only correct way to spawn an actor.\n"
		"    // The compiler will create a 'Spawn Actor From Class' node from this line.\n"
		"    BP_MyProjectile* NewProjectile = SpawnActor<BP_MyProjectile>(SpawnLoc, SpawnRot);\n"
		"}\n"
		"```\n\n"
		"\\n\\n--- PATTERN 27: SEQUENCE NODE ---\\n"
		"To create a Sequence node, you MUST use the `sequence` keyword followed by two or more code blocks in `{}`. Each block corresponds to a 'Then' output pin on the node. The main execution chain stops after the sequence node is created.\\n\\n"
		"**EXAMPLE:**\\n"
		"// User Prompt: \"On begin play, first print \"Hello\" and then print \"World\" in order.\"\\n"
		"```cpp\\n"
		"void OnBeginPlay() {\\n"
		"    sequence {\\n"
		"        // This will be connected to 'Then 0'\\n"
		"        UKismetSystemLibrary::PrintString(\"Hello\");\\n"
		"    }\\n"
		"    {\\n"
		"        // This will be connected to 'Then 1'\\n"
		"        UKismetSystemLibrary::PrintString(\"World\");\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 28: KEYBOARD INPUT EVENTS ---\\n"
		"To create logic for keyboard events, you MUST define a function with the special name `OnKeyPressed_KEYNAME` or `OnKeyReleased_KEYNAME`. For the 'F' key, use `OnKeyPressed_F`. For the space bar, use `OnKeyReleased_SpaceBar`. These events are always `void` and take no parameters.\\n\\n"
		"**EXAMPLE (Print on F key press and release):**\\n"
		"```cpp\\n"
		"void OnKeyPressed_F() {\\n"
		"    // This logic runs when the 'F' key is pressed.\\n"
		"    UKismetSystemLibrary::PrintString(\"F Key Was Pressed\");\\n"
		"}\\n\\n"
		"void OnKeyReleased_F() {\\n"
		"    // This logic runs when the 'F' key is released.\\n"
		"    UKismetSystemLibrary::PrintString(\"F Key Was Released\");\\n"
		"}\\n"
		"\\n\\n--- PATTERN 29: GETTING PROPERTIES FROM EXTERNAL OBJECTS ---\\n"
		"To get a component (like `CharacterMovement`) or a variable (like `Float1`) from an object reference, you MUST access it using the `->` operator.\\n"
		"**CRITICAL:** When you need to get a value and store it in a new variable, you MUST perform the get and the new variable declaration in a **single, atomic line**. This is the only way the compiler can correctly create and connect the nodes to avoid engine garbage collection.\\n\\n"
		"**INCORRECT (WILL FAIL):**\\n"
		"```cpp\\n"
		"// This two-step process creates an orphaned 'get' node that is instantly deleted.\\n"
		"UCharacterMovementComponent* TempComp = MyCharacter->CharacterMovement;\\n"
		"UCharacterMovementComponent* MyMovementComponent = TempComp;\\n"
		"```\\n\\n"
		"**PERFECT (Compiler handles this correctly):**\\n"
		"// User Prompt: \"...get its character movement component, and create a new variable from it.\""
		"```cpp\\n"
		"// This single line ensures the 'get' and 'set' nodes are created and connected atomically.\\n"
		"UCharacterMovementComponent* MyMovementComponent = MyCharacter->CharacterMovement;\\n"
		"```\\n"
		"\\n\\n--- PATTERN 30: ANIMATION BLUEPRINT EVENTS ---\\n"
		"To add logic to an Animation Blueprint's Event Graph, you MUST use special event function names. These events MUST be `void` and take no parameters.\\n"
		"1. `void OnUpdateAnimation()`: Corresponds to the 'Event Blueprint Update Animation' node. A `float` variable named `DeltaSeconds` is automatically available inside this event.\\n"
		"2. `void OnInitializeAnimation()`: Corresponds to the 'Event Blueprint Initialize Animation' node.\\n"
		"3. `void OnPostEvaluateAnimation()`: Corresponds to the 'Event Blueprint Post Evaluate Animation' node.\\n"
		"**CRITICAL:** Inside any of these animation events, you MUST call the function `K2_GetPawnOwner()` to get the owning Actor or Pawn. You can then cast this to your specific character class to access its variables and components.\\n\\n"
		"**EXAMPLE (Get Speed on Update):**\\n"
		"```cpp\\n"
		"void OnUpdateAnimation() {\\n"
		"    APawn* OwningPawn = K2_GetPawnOwner();\\n"
		"    if (BP_MyCharacter* MyCharacter = Cast<BP_MyCharacter>(OwningPawn)) {\\n"
		"        FVector Velocity = MyCharacter->GetVelocity();\\n"
		"        float Speed = UKismetMathLibrary::VSize(Velocity);\\n"
		"        // 'MySpeed' is a variable on the Anim BP that is being set\\n"
		"        MySpeed = Speed; \\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 31: GETTING A SINGLE ACTOR BY CLASS ---\\n"
		"To get a single actor of a specific class from the game world, you MUST call the function `UGameplayStatics::GetActorOfClass`. This is useful for finding unique actors placed in the level, like a manager or a specific interactive object.\\n"
		"The function takes the class of the actor you want to find as a parameter, which MUST be in the format `BlueprintName::StaticClass()`.\\n\\n"
		"**EXAMPLE (Find a specific trigger box in the world):**\\n"
		"```cpp\\n"
		"// User Prompt: \"On begin play, find the BP_SpecialTriggerBox actor\"\\n"
		"void OnBeginPlay() {\\n"
		"    // This finds the first instance of BP_SpecialTriggerBox placed in the world.\\n"
		"    AActor* MyTriggerBox = UGameplayStatics::GetActorOfClass(BP_SpecialTriggerBox::StaticClass());\\n"
		"    if (MyTriggerBox != nullptr) {\\n"
		"        // Now you can do things with the found trigger box.\\n"
		"        UKismetSystemLibrary::PrintString(\"Found the trigger box!\");\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 32: CHECKING FOR VALID OBJECTS (NULL CHECKING) ---\\n"
		"**CRITICAL MASTER RULE:** You MUST NOT use `!= nullptr` to check if an object is valid. You MUST use the Blueprint function `UKismetSystemLibrary::IsValid(MyObject)`. This function returns a boolean that should be used in a Branch (`if` statement).\\n\\n"
		"This is the preferred, modern, and safe way to perform null checks in Blueprints.\\n\\n"
		"**INCORRECT (will work, but is not the best practice):**\\n"
		"```cpp\\n"
		"void OnBeginPlay() {\\n"
		"    AActor* MyActor = UGameplayStatics::GetActorOfClass(BP_Actor::StaticClass());\\n"
		"    // DO NOT DO THIS. It is a C++ idiom, not a Blueprint one.\\n"
		"    if (MyActor != nullptr) {\\n"
		"        UKismetSystemLibrary::PrintString(\"Actor is valid!\");\\n"
		"    }\\n"
		"}\\n"
		"```\\n\\n"
		"**PERFECT (this generates the correct 'Is Valid' node):**\\n"
		"```cpp\\n"
		"void OnBeginPlay() {\\n"
		"    AActor* MyActor = UGameplayStatics::GetActorOfClass(BP_Actor::StaticClass());\\n"
		"    // ALWAYS use this function for object validity checks.\\n"
		"    if (UKismetSystemLibrary::IsValid(MyActor)) {\\n"
		"        UKismetSystemLibrary::PrintString(\"Actor is valid!\");\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 33: GETTING ALL ACTORS OF A CLASS (ARRAYS) ---\\n"
		"To get ALL actors of a specific class from the world, you MUST call `UGameplayStatics::GetAllActorsOfClass`. This returns an array of actors, which you MUST declare with `TArray<AActor*>`.\\n"
		"To process this array, you MUST use a ranged `for` loop. The syntax for the loop MUST be `for (AActor* ElementName : ArrayName)`.\\n\\n"
		"**EXAMPLE (Damage all enemies):**\\n"
		"```cpp\\n"
		"// User Prompt: \"On begin play, find all BP_Enemy actors and make them take 10 damage\"\\n"
		"void OnBeginPlay() {\\n"
		"    // 1. Get the array of all actors of the specified class.\\n"
		"    TArray<AActor*> FoundEnemies = UGameplayStatics::GetAllActorsOfClass(BP_Enemy::StaticClass());\\n\\n"
		"    // 2. Loop through the array.\\n"
		"    for (AActor* EnemyActor : FoundEnemies) {\\n"
		"        // 3. Cast the generic actor to the specific Blueprint to access its functions.\\n"
		"        if (BP_Enemy* MyEnemy = Cast<BP_Enemy>(EnemyActor)) {\\n"
		"            // 4. Call the function on the casted enemy.\\n"
		"            MyEnemy->TakeDamage(10.0f);\\n"
		"        }\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 34: GETTING ACTORS BY CLASS AND TAG ---\\n"
		"To find all actors that have a specific class AND a specific component tag, you MUST call `UGameplayStatics::GetAllActorsOfClassWithTag`.\\n"
		"This function takes the class (using `::StaticClass()`) and the tag (as an FName string literal) as parameters.\\n\\n"
		"**EXAMPLE (Find all enemies with the 'Heavy' tag):**\\n"
		"```cpp\\n"
		"// User Prompt: \"on begin play, find all BP_Enemy actors with the tag Heavy\"\\n"
		"void OnBeginPlay() {\\n"
		"    TArray<AActor*> HeavyEnemies = UGameplayStatics::GetAllActorsOfClassWithTag(BP_Enemy::StaticClass(), \"Heavy\");\\n\\n"
		"    // You can now loop through the 'HeavyEnemies' array.\\n"
		"    for (AActor* EnemyActor : HeavyEnemies) {\\n"
		"        if (UKismetSystemLibrary::IsValid(EnemyActor)) {\\n"
		"            // ... do something with the heavy enemies ...\\n"
		"        }\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 35: GETTING ACTORS BY INTERFACE ---\\n"
		"To find all actors that implement a specific Blueprint Interface, you MUST call `UGameplayStatics::GetAllActorsWithInterface`.\\n"
		"This function takes the Interface class as a parameter, which MUST use the `::StaticClass()` syntax (e.g., `BPI_Interact::StaticClass()`). Blueprint Interfaces are often prefixed with `BPI_`.\\n\\n"
		"**EXAMPLE (Find all interactable objects):**\\n"
		"```cpp\\n"
		"// User Prompt: \"on begin play, find all actors with the BPI_Interact interface and call the Notify event\"\\n"
		"void OnBeginPlay() {\\n"
		"    TArray<AActor*> InteractableActors = UGameplayStatics::GetAllActorsWithInterface(BPI_Interact::StaticClass());\\n\\n"
		"    for (AActor* Interactable : InteractableActors) {\\n"
		"        // Since we are calling an interface function, we do not need to cast.\\n"
		"        // We can call the function directly on the Actor reference.\\n"
		"        if (UKismetSystemLibrary::IsValid(Interactable)) {\\n"
		"            Interactable->Notify();\\n"
		"        }\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 36: GETTING A SINGLE COMPONENT BY CLASS ---\\n"
		"To get a single component of a specific class from an actor, you MUST call the member function `GetComponentByClass`.\\n"
		"The function takes the component class you want to find as a parameter, which MUST be in the format `ComponentName::StaticClass()`. Component classes often start with 'U', like `UPawnSensingComponent`.\\n"
		"**CRITICAL:** The result of `GetComponentByClass` is a generic `ActorComponent`. You MUST cast this result to the specific component type you need before you can use its unique functions or variables.\\n\\n"
		"**EXAMPLE (Get a custom component and use it):**\\n"
		"```cpp\\n"
		"// User Prompt: \"On begin play, get my BP_MyComponent and call its CustomFunction\"\\n"
		"void OnBeginPlay() {\\n"
		"    // 1. Call the function on 'SelfActor', passing the component class AS A PARAMETER.\\n"
		"    UActorComponent* FoundComponent = SelfActor->GetComponentByClass(BP_MyComponent::StaticClass());\\n\\n"
		"    // 2. Check if the component was found.\\n"
		"    if (UKismetSystemLibrary::IsValid(FoundComponent)) {\\n"
		"        // 3. Cast the generic component to the specific Blueprint type.\\n"
		"        if (BP_MyComponent* MyComponent = Cast<BP_MyComponent>(FoundComponent)) {\\n"
		"            // 4. Now you can use the specific MyComponent variable.\\n"
		"            MyComponent->CustomFunction();\\n"
		"        }\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 37: GETTING ALL COMPONENTS BY CLASS (ARRAYS) ---\\n"
		"To get ALL components of a specific class from an actor, you MUST call the member function `GetComponentsByClass`. This returns an array of components, which you MUST declare with `TArray<UActorComponent*>`.\\n"
		"The function takes the component class as a parameter, which MUST use the `::StaticClass()` syntax.\\n"
		"You MUST use a `for` loop to process the resulting array.\\n\\n"
		"**EXAMPLE (Hide all static meshes on an actor):**\\n"
		"```cpp\\n"
		"// User Prompt: \"On begin play, get all of my static mesh components and hide them\"\\n"
		"void OnBeginPlay() {\\n"
		"    // 1. Get the array of components from SelfActor.\\n"
		"    TArray<UActorComponent*> MeshComponents = SelfActor->GetComponentsByClass(UStaticMeshComponent::StaticClass());\\n\\n"
		"    // 2. Loop through the array.\\n"
		"    for (UActorComponent* GenericComponent : MeshComponents) {\\n"
		"        // 3. Cast the generic component to the specific type to access its functions.\\n"
		"        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(GenericComponent)) {\\n"
		"            // 4. Call a function on the specific component.\\n"
		"            Mesh->SetVisibility(false);\\n"
		"        }\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 38: WIDGET BLUEPRINT SCRIPTING ---\\n"
		"This pattern covers all common User Widget operations, both from within a Widget Blueprint and from a standard Actor Blueprint.\\n\\n"
		"**RULE 1: WIDGET-SPECIFIC EVENTS**\\n"
		"The 'BeginPlay' for a widget is `void OnConstruct()`. To handle events from a widget's components (like a button), you MUST use the syntax `WidgetVariableName::EventName()`.\\n\\n"
		"**RULE 2: CREATING AND DISPLAYING WIDGETS**\\n"
		"To create a widget, you MUST use `UWidgetBlueprintLibrary::Create`. To display it, you MUST call `->AddToViewport()` on the new widget variable. The `Create` function REQUIRES an Owning Player Controller, which you get from `UGameplayStatics::GetPlayerController`.\\n\\n"
		"**EXAMPLE 1 (Scripting within a Widget Blueprint):**\\n"
		"```cpp\\n"
		"// User Prompt: \"When the ConfirmButton is clicked, create a WBP_Popup and add it to the screen, then close this widget.\""
		"// This creates the 'OnClicked (ConfirmButton)' event in the WIDGET'S event graph.\\n"
		"ConfirmButton::OnClicked() {\\n"
		"    // Get the player controller to own the new widget.\\n"
		"    APlayerController* PC = UGameplayStatics::GetPlayerController(SelfActor, 0);\\n"
		"    // Create the popup widget instance.\\n"
		"    UUserWidget* Popup = UWidgetBlueprintLibrary::Create(WBP_Popup::StaticClass(), PC);\\n\n"
		"    // Add the new widget to the screen.\\n"
		"    if (UKismetSystemLibrary::IsValid(Popup)) {\\n"
		"        Popup->AddToViewport();\\n"
		"    }\\n\n"
		"    // 'SelfActor' in a widget context refers to the widget itself. Remove it from the screen.\\n"
		"    UWidgetBlueprintLibrary::RemoveFromParent(SelfActor);\\n"
		"}\\n"
		"```\\n\n"
		"**EXAMPLE 2 (Creating a widget from an Actor Blueprint):**\\n"
		"```cpp\\n"
		"// User Prompt: \"on begin play, create my WBP_HUD and add it to the screen\"\\n"
		"// This is in a normal Actor's event graph.\\n"
		"void OnBeginPlay() {\\n"
		"    // Get the player controller to own the new widget.\\n"
		"    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(SelfActor, 0);\\n\n"
		"    // Create the widget instance.\\n"
		"    UUserWidget* HudWidget = UWidgetBlueprintLibrary::Create(WBP_HUD::StaticClass(), PlayerController);\\n\n"
		"    // Add the created widget to the viewport.\\n"
		"    if (UKismetSystemLibrary::IsValid(HudWidget)) {\\n"
		"        HudWidget->AddToViewport();\\n"
		"    }\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 39: WHILE LOOPS ---\\n"
		"CRITICAL RULE: You MUST NOT use C-style loops (`for (int i = 0; i < ...)`). For loops that require an index, you MUST use the ranged-for syntax from PATTERN 4. For loops that run based on a condition, you MUST use this `while` loop pattern.\\n\\n"
		"To create a While Loop, you MUST use the standard `while (condition) { ... }` syntax. The compiler will create a 'WhileLoop' macro node.\\n"
		"CRITICAL: The condition inside the `while()` parentheses MUST use a `UKismetMathLibrary` function for comparison. Direct C++ operators like `<` or `!=` are FORBIDDEN in the condition.\\n"
		"CRITICAL: Any counter variables MUST be incremented inside the loop using a `UKismetMathLibrary` function (e.g., `Add_IntInt`). Manual C-style incrementing like `i++` is FORBIDDEN.\\n\\n"
		"**EXAMPLE (Password Generator):**\\n"
		"// User Prompt: \"Create a function that generates a random password of a given length\"\\n"
		"```cpp\\n"
		"FString GeneratePassword(int Length) {\\n"
		"    // 1. Define the character set and initialize variables before the loop.\\n"
		"    const FString Characters = \"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*\";\\n"
		"    FString Password = \"\";\\n"
		"    int i = 0;\\n\\n"
		"    // 2. The 'while' condition MUST use a library function for comparison.\\n"
		"    while (UKismetMathLibrary::Less_IntInt(i, Length)) {\\n"
		"        // 3. Logic inside the loop body.\\n"
		"        int CharIndex = UKismetMathLibrary::RandomIntegerInRange(0, UKismetStringLibrary::Len(Characters) - 1);\\n"
		"        FString Char = UKismetStringLibrary::GetSubstring(Characters, CharIndex, 1);\\n"
		"        Password = UKismetStringLibrary::Concat_StrStr(Password, Char);\\n\\n"
		"        // 4. The counter MUST be incremented using a library function.\\n"
		"        i = UKismetMathLibrary::Add_IntInt(i, 1);\\n"
		"    }\\n\\n"
		"    // 5. Return the final result after the loop is completed.\\n"
		"    return Password;\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 41: ENHANCED INPUT SYSTEM (UE5) ---\\n"
		"CRITICAL: These patterns are for Blueprint code generation with Enhanced Input. The subsystem is accessed via special Blueprint nodes.\\n\\n"
		"**KEY RULES:**\\n"
		"1. NEVER create variables for intermediate values - Chain calls directly inside the if block\\n"
		"2. NEVER try to call `GetEnhancedInputLocalPlayerSubsystem` directly - It's a special Blueprint node\\n"
		"3. For AddMappingContext - Just call it on the subsystem directly, the compiler handles node creation\\n"
		"4. NEVER leave if blocks empty - Always include the action inside the block\\n"
		"5. Use `UGameplayStatics::GetWorldDeltaSeconds(this)` for delta time - this is the ONLY correct way\\n"
		"6. NEVER cast `this` to the class you're already in - If in PlayerController, `this` IS a PlayerController\\n"
		"7. AddMappingContext ONLY ONCE - Choose EITHER PlayerController OR Character, NEVER both\\n\\n"
		"**AddMappingContext Pattern (WORKS):**\\n"
		"```cpp\\n"
		"void OnBeginPlay() {\\n"
		"    Super::OnBeginPlay();\\n"
		"    AddMappingContext(InputMappingContext, 0);\\n"
		"}\\n"
		"```\\n\\n"
		"**Getting Delta Time (ONLY CORRECT WAY):**\\n"
		"```cpp\\n"
		"float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);\\n"
		"```\\n\\n"
		"**Movement Input Function:**\\n"
		"```cpp\\n"
		"void Move(const FInputActionValue& Value) {\\n"
		"    FVector2D MovementVector = Value.Get<FVector2D>();\\n"
		"    FRotator ControlRotation = GetControlRotation();\\n"
		"    FRotator YawRotation = UKismetMathLibrary::MakeRotator(0.0, 0.0, ControlRotation.Yaw);\\n"
		"    FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(YawRotation);\\n"
		"    FVector RightDirection = UKismetMathLibrary::GetRightVector(YawRotation);\\n"
		"    AddMovementInput(ForwardDirection, MovementVector.X);\\n"
		"    AddMovementInput(RightDirection, MovementVector.Y);\\n"
		"}\\n"
		"```\\n\\n"
		"**Look Input Function (BREAK COMPLEX EXPRESSIONS):**\\n"
		"```cpp\\n"
		"void Look(const FInputActionValue& Value) {\\n"
		"    FVector2D LookAxisVector = Value.Get<FVector2D>();\\n"
		"    float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(this);\\n"
		"    float YawRate = UKismetMathLibrary::Multiply_DoubleDouble(LookAxisVector.X, LookSensitivity);\\n"
		"    YawRate = UKismetMathLibrary::Multiply_DoubleDouble(YawRate, DeltaTime);\\n"
		"    float PitchRate = UKismetMathLibrary::Multiply_DoubleDouble(LookAxisVector.Y, LookSensitivity);\\n"
		"    PitchRate = UKismetMathLibrary::Multiply_DoubleDouble(PitchRate, DeltaTime);\\n"
		"    AddControllerYawInput(YawRate);\\n"
		"    AddControllerPitchInput(PitchRate);\\n"
		"}\\n"
		"```\\n\\n"
		"**FInputActionValue Access:**\\n"
		"```cpp\\n"
		"// For Vector2D (stick input)\\n"
		"FVector2D MovementVector = Value.Get<FVector2D>();\\n"
		"// For float (trigger/axis)\\n"
		"float TriggerValue = Value.Get<float>();\\n"
		"// For bool (button press)\\n"
		"bool bPressed = Value.Get<bool>();\\n"
		"```\\n\\n"
		"**Jump Functions:**\\n"
		"```cpp\\n"
		"void JumpStarted(const FInputActionValue& Value) { Jump(); }\\n"
		"void JumpStopped(const FInputActionValue& Value) { StopJumping(); }\\n"
		"```\\n\\n"
		"**Variable Guidelines:**\\n"
		"- DO NOT create variables for intermediate function results\\n"
		"- DO create: InputMappingContext (InputMappingContext), MoveAction (InputAction), LookAction (InputAction), LookSensitivity (float, default 45.0)\\n\\n"
		"**Expression Breaking Rule:**\\n"
		"WRONG: AddControllerYawInput(LookAxisVector.X * 45.0 * DeltaTime);\\n"
		"CORRECT: Break into separate Multiply_DoubleDouble statements as shown in Look example above.\\n"
		"```\\n"
		"\\n\\n--- PATTERN 40: THE CONSTRUCTION SCRIPT ---\\n"
		"CRITICAL RULE: To add logic to a Blueprint's Construction Script, you MUST define a function with the exact signature `void ConstructionScript()`.\\n"
		"The Construction Script runs in the editor every time an actor is moved, rotated, scaled, or has one of its variables changed.\\n\\n"
		"**EXAMPLE (Dynamic Material):**\\n"
		"```cpp\\n"
		"// User Prompt: \\\"in the construction script, create a dynamic material instance from the mesh and set its color based on the 'bIsActive' variable\\\"\\n"
		"void ConstructionScript() {\\n"
		"    // Create a dynamic material from the first material slot of the 'Mesh' component.\\n"
		"    UMaterialInstanceDynamic* DynMat = Mesh->CreateDynamicMaterialInstance(0);\\n\\n"
		"    // Use a Select node to choose a color based on the boolean variable.\\n"
		"    FLinearColor ChosenColor = UKismetMathLibrary::SelectColor(bIsActive, FLinearColor(0.0, 1.0, 0.0, 1.0), FLinearColor(1.0, 0.0, 0.0, 1.0));\\n\\n"
		"    // Set the 'BaseColor' parameter on the dynamic material.\\n"
		"    DynMat->SetVectorParameterValue(\\\"BaseColor\\\", ChosenColor);\\n"
		"}\\n"
		"```\\n"
		"\\n\\n--- PATTERN 42: WIDGET CREATION WORKFLOW ---\\n"
		"CRITICAL: Widget creation follows a specific workflow. Follow these rules to avoid errors.\\n\\n"
		"**WORKFLOW:**\\n"
		"0. ALWAYS call get_current_folder FIRST to get the save path!\\n"
		"1. Create ONCE: Call create_blueprint with parent_class=\\\"UserWidget\\\" ONE TIME ONLY\\n"
		"2. Note the path: Tool returns asset_path like \\\"/Game/WBP_MyWidget.WBP_MyWidget\\\"\\n"
		"3. Use the path: For subsequent operations, use the path (remove .WidgetName suffix)\\n"
		"4. NEVER recreate: If you see 'Asset already exists', use the existing path\\n\\n"
		"**Widget Design Size Mode:**\\n"
		"- \\\"fill_screen\\\": Widget fills entire viewport (full-screen HUDs, main menus)\\n"
		"- \\\"desired\\\": Widget sizes to its content (buttons, health bars, minimap)\\n\\n"
		"**CRITICAL: create_widget_from_layout FORMAT**\\n"
		"The layout MUST be an ARRAY of widget objects, each with these fields:\\n"
		"- type: Widget class (CanvasPanel, VerticalBox, TextBlock, etc.)\\n"
		"- name: Widget name (string)\\n"
		"- parent_name: Parent widget name (empty string for root)\\n"
		"- properties: Array of {key, value} pairs (optional)\\n"
		"- children: Array of child widgets (optional, for nested hierarchy)\\n\\n"
		"**CORRECT EXAMPLE:**\\n"
		"```json\\n"
		"{\\\"tool\\\": \\\"create_widget_from_layout\\\", \\\"arguments\\\": {\\n"
		"  \\\"widget_path\\\": \\\"/Game/WBP_HUD\\\",\\n"
		"  \\\"layout\\\": [\\n"
		"    {\\\"type\\\": \\\"CanvasPanel\\\", \\\"name\\\": \\\"RootCanvas\\\", \\\"parent_name\\\": \\\"\\\",\\n"
		"      \\\"children\\\": [\\n"
		"        {\\\"type\\\": \\\"TextBlock\\\", \\\"name\\\": \\\"Title\\\", \\\"parent_name\\\": \\\"RootCanvas\\\",\\n"
		"          \\\"properties\\\": [{\\\"key\\\": \\\"Text\\\", \\\"value\\\": \\\"Hello\\\"}]},\\n"
		"        {\\\"type\\\": \\\"ProgressBar\\\", \\\"name\\\": \\\"HPBar\\\", \\\"parent_name\\\": \\\"RootCanvas\\\",\\n"
		"          \\\"properties\\\": [{\\\"key\\\": \\\"Slot.Position\\\", \\\"value\\\": \\\"(50,50)\\\"}, {\\\"key\\\": \\\"Slot.Size\\\", \\\"value\\\": \\\"(200,20)\\\"}]}\\n"
		"      ]}\\n"
		"  ]}}\\n"
		"```\\n\\n"
		"**WRONG - DO NOT USE:**\\n"
		"- DO NOT use widget name as key: {\\\"RootCanvas\\\": {...}} - WRONG!\\n"
		"- DO NOT nest children as object: {\\\"children\\\": {\\\"HPBar\\\": {...}}} - WRONG!\\n"
		"- ALWAYS use arrays: {\\\"children\\\": [{\\\"name\\\": \\\"HPBar\\\", ...}]} - CORRECT!\\n\\n"
		"**Anchor Presets (EASY WAY):**\\n"
		"Instead of numeric anchors, use preset names in Slot.Anchors property:\\n"
		"- \\\"fill\\\" or \\\"fill_screen\\\" -> Widget fills entire parent\\n"
		"- \\\"center\\\" -> Centered at parent center\\n"
		"- \\\"top_left\\\" -> Anchored to top-left corner\\n"
		"- \\\"top_center\\\" -> Centered horizontally at top\\n"
		"- \\\"top_right\\\" -> Anchored to top-right corner\\n"
		"- \\\"bottom_left\\\" -> Anchored to bottom-left corner\\n"
		"- \\\"bottom_right\\\" -> Anchored to bottom-right corner\\n\\n"
		"**CRITICAL: Position is ALWAYS relative to anchor point, NOT absolute screen coordinates.**\\n"
		"For a 1920x1080 viewport:\\n"
		"- Top-left anchor (0,0): Position=(50,50) -> screen position (50,50)\\n"
		"- Center anchor (0.5,0.5): Position=(0,0) -> screen position (960,540)\\n"
		"- Top-right anchor (1,0): Position=(-50,50) -> screen position (1870,50)\\n\\n"
		"**Safe Area:** NEVER place widgets closer than 50px to viewport edges.\\n\\n"
		"**Property Formats:**\\n"
		"- Slot.Position: \\\"(X,Y)\\\" -> Example: \\\"(50,50)\\\"\\n"
		"- Slot.Size: \\\"(Width,Height)\\\" -> Example: \\\"(200,50)\\\"\\n"
		"- Slot.Anchors: \\\"top_left\\\" or \\\"bottom_right\\\" (preset names)\\n"
		"- ColorAndOpacity: \\\"(R,G,B,A)\\\" -> Example: \\\"(1,0.8,0,1)\\\"\\n"
	);
	return Ret;
}
void SBpGeneratorUltimateWidget::InjectPatternsAsSystemMessage()
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== PATTERN INJECTION TRIGGERED ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Previous state: PatternsInjected=%d"), bPatternsInjectedForSession);
	bPatternsInjectedForSession = true;
	FString Patterns = GetCachedPatternsSection();
	int32 PatternTokens = Patterns.Len() / 4;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Patterns injected: ~%d tokens (%d chars)"), PatternTokens, Patterns.Len());
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Next API request will include patterns in system prompt"));
	RefreshArchitectChatView();
}
int32 SBpGeneratorUltimateWidget::EstimateFullRequestTokens(const TArray<TSharedPtr<FJsonValue>>& Messages)
{
	int32 TotalChars = 0;
	int32 StaticChars = 0;
	int32 PatternChars = 0;
	int32 HistoryChars = 0;
	FString StaticPrompt = GetCachedStaticSystemPrompt();
	StaticChars = StaticPrompt.Len();
	TotalChars += StaticChars;
	if (bPatternsInjectedForSession && !CachedPatternsSection.IsEmpty())
	{
		PatternChars = CachedPatternsSection.Len();
		TotalChars += PatternChars;
	}
	for (const TSharedPtr<FJsonValue>& MsgValue : Messages)
	{
		const TSharedPtr<FJsonObject>& Msg = MsgValue->AsObject();
		FString Role, Content;
		if (Msg->TryGetStringField(TEXT("role"), Role))
		{
			HistoryChars += Role.Len();
		}
		if (Msg->TryGetStringField(TEXT("content"), Content))
		{
			HistoryChars += Content.Len();
		}
		else
		{
			const TArray<TSharedPtr<FJsonValue>>* Parts;
			if (Msg->TryGetArrayField(TEXT("parts"), Parts))
			{
				for (const TSharedPtr<FJsonValue>& Part : *Parts)
				{
					const TSharedPtr<FJsonObject>& PartObj = Part->AsObject();
					FString Text;
					if (PartObj->TryGetStringField(TEXT("text"), Text))
					{
						HistoryChars += Text.Len();
					}
				}
			}
		}
	}
	TotalChars = StaticChars + PatternChars + HistoryChars;
	int32 TotalTokens = TotalChars / 4;
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("=== TOKEN ESTIMATION ==="));
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Static: ~%d | Patterns: ~%d | History: ~%d"),
		StaticChars / 4, PatternChars / 4, HistoryChars / 4);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  Messages in history: %d"), Messages.Num());
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("  TOTAL REQUEST: ~%d tokens"), TotalTokens);
	return TotalTokens;
}
int32 SBpGeneratorUltimateWidget::FindMatchingClosingBrace(const FString& Haystack, int32 StartIndex)
{
	if (StartIndex >= Haystack.Len() || Haystack[StartIndex] != '{')
	{
		return INDEX_NONE;
	}
	int32 BraceCount = 0;
	bool bInString = false;
	bool bEscaped = false;
	for (int32 i = StartIndex; i < Haystack.Len(); ++i)
	{
		TCHAR Char = Haystack[i];
		if (bEscaped)
		{
			bEscaped = false;
			continue;
		}
		if (Char == '\\')
		{
			bEscaped = true;
			continue;
		}
		if (Char == '"')
		{
			bInString = !bInString;
			continue;
		}
		if (!bInString)
		{
			if (Char == '{')
			{
				BraceCount++;
			}
			else if (Char == '}')
			{
				BraceCount--;
				if (BraceCount == 0)
				{
					return i;
				}
			}
		}
	}
	return INDEX_NONE;
}
bool SBpGeneratorUltimateWidget::TryParseToolCallJson(const FString& JsonString, FToolCallExtractionResult& OutResult)
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: JSON length=%d chars"), JsonString.Len());
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: First 200 chars: %s"), *JsonString.Left(200));
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("  TryParseToolCallJson: FAILED to deserialize JSON!"));
		return false;
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: JSON deserialized OK"));
	FString ToolName;
	if (!JsonObject->TryGetStringField(TEXT("tool_name"), ToolName))
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: No 'tool_name' field found!"));
		TArray<FString> FieldNames;
		for (auto& Pair : JsonObject->Values)
		{
			FieldNames.Add(Pair.Key);
		}
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: Fields present: %s"), *FString::Join(FieldNames, TEXT(", ")));
		return false;
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  TryParseToolCallJson: Found tool_name='%s'"), *ToolName);
	OutResult.bHasToolCall = true;
	OutResult.ToolName = ToolName;
	const TSharedPtr<FJsonObject>* ArgumentsPtr = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("arguments"), ArgumentsPtr) && ArgumentsPtr)
	{
		OutResult.Arguments = *ArgumentsPtr;
	}
	else
	{
		OutResult.Arguments = MakeShareable(new FJsonObject);
	}
	return true;
}
FToolCallExtractionResult SBpGeneratorUltimateWidget::ExtractToolCallFromResponse(const FString& AiResponse)
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ExtractToolCallFromResponse ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  AiResponse length: %d chars"), AiResponse.Len());
	FToolCallExtractionResult Result;
	Result.ConversationalText = AiResponse;
	const FString JsonBlockStart = TEXT("```json");
	const FString JsonBlockEnd = TEXT("```");
	int32 JsonBlockStartPos = AiResponse.Find(JsonBlockStart, ESearchCase::IgnoreCase);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Strategy 1 (```json block): StartPos=%d"), JsonBlockStartPos);
	if (JsonBlockStartPos != INDEX_NONE)
	{
		int32 ContentStart = JsonBlockStartPos + JsonBlockStart.Len();
		int32 JsonBlockEndPos = AiResponse.Find(JsonBlockEnd, ESearchCase::CaseSensitive, ESearchDir::FromStart, ContentStart);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    ContentStart=%d, EndPos=%d"), ContentStart, JsonBlockEndPos);
		if (JsonBlockEndPos != INDEX_NONE)
		{
			FString JsonContent = AiResponse.Mid(ContentStart, JsonBlockEndPos - ContentStart).TrimStartAndEnd();
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Extracted JSON content length: %d"), JsonContent.Len());
			if (TryParseToolCallJson(JsonContent, Result))
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Strategy 1 SUCCESS! Tool: %s"), *Result.ToolName);
				FString TextBefore = AiResponse.Left(JsonBlockStartPos).TrimEnd();
				FString TextAfter = AiResponse.Mid(JsonBlockEndPos + JsonBlockEnd.Len()).TrimStart();
				Result.ConversationalText = TextBefore;
				if (!TextAfter.IsEmpty())
				{
					if (!Result.ConversationalText.IsEmpty())
					{
						Result.ConversationalText += TEXT("\n");
					}
					Result.ConversationalText += TextAfter;
				}
				return Result;
			}
			else
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Strategy 1 TryParseToolCallJson FAILED"));
			}
		}
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Strategy 2 (inline JSON):"));
	int32 BraceStart = AiResponse.Find(TEXT("{"), ESearchCase::CaseSensitive);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    First brace at: %d"), BraceStart);
	int32 AttemptCount = 0;
	while (BraceStart != INDEX_NONE)
	{
		AttemptCount++;
		int32 BraceEnd = FindMatchingClosingBrace(AiResponse, BraceStart);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Attempt %d: BraceStart=%d, BraceEnd=%d"), AttemptCount, BraceStart, BraceEnd);
		if (BraceEnd != INDEX_NONE)
		{
			FString JsonContent = AiResponse.Mid(BraceStart, BraceEnd - BraceStart + 1);
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("      JSON segment length: %d"), JsonContent.Len());
			if (TryParseToolCallJson(JsonContent, Result))
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("      Strategy 2 SUCCESS! Tool: %s"), *Result.ToolName);
				FString TextBefore = AiResponse.Left(BraceStart).TrimEnd();
				FString TextAfter = AiResponse.Mid(BraceEnd + 1).TrimStart();
				Result.ConversationalText = TextBefore;
				if (!TextAfter.IsEmpty())
				{
					if (!Result.ConversationalText.IsEmpty())
					{
						Result.ConversationalText += TEXT("\n");
					}
					Result.ConversationalText += TextAfter;
				}
				return Result;
			}
			else
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("      TryParseToolCallJson failed for this segment"));
			}
		}
		BraceStart = AiResponse.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, BraceStart + 1);
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  NO TOOL CALL FOUND after %d attempts"), AttemptCount);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Strategy 3: Regex-like extraction for malformed JSON..."));
	int32 ToolNamePos = AiResponse.Find(TEXT("\"tool_name\""), ESearchCase::CaseSensitive);
	if (ToolNamePos != INDEX_NONE)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Found 'tool_name' at position %d"), ToolNamePos);
		int32 ColonPos = AiResponse.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ToolNamePos + 11);
		if (ColonPos != INDEX_NONE)
		{
			int32 QuoteStart = AiResponse.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, ColonPos + 1);
			if (QuoteStart != INDEX_NONE)
			{
				int32 QuoteEnd = AiResponse.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, QuoteStart + 1);
				if (QuoteEnd != INDEX_NONE)
				{
					FString ToolName = AiResponse.Mid(QuoteStart + 1, QuoteEnd - QuoteStart - 1);
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Extracted tool_name: '%s'"), *ToolName);
					Result.bHasToolCall = true;
					Result.ToolName = ToolName;
					Result.Arguments = MakeShareable(new FJsonObject);
					int32 ArgsPos = AiResponse.Find(TEXT("\"arguments\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, QuoteEnd);
					if (ArgsPos != INDEX_NONE)
					{
						int32 ArgsColonPos = AiResponse.Find(TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ArgsPos + 10);
						if (ArgsColonPos != INDEX_NONE)
						{
							int32 ArgsBraceStart = AiResponse.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ArgsColonPos + 1);
							if (ArgsBraceStart != INDEX_NONE)
							{
								int32 ArgsBraceEnd = FindMatchingClosingBrace(AiResponse, ArgsBraceStart);
								if (ArgsBraceEnd != INDEX_NONE)
								{
									FString ArgsJson = AiResponse.Mid(ArgsBraceStart, ArgsBraceEnd - ArgsBraceStart + 1);
									UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Arguments JSON length: %d"), ArgsJson.Len());
									TSharedPtr<FJsonObject> ArgsObject;
									TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ArgsJson);
									if (FJsonSerializer::Deserialize(Reader, ArgsObject) && ArgsObject.IsValid())
									{
										Result.Arguments = ArgsObject;
										UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Strategy 3 SUCCESS! Parsed arguments"));
									}
									else
									{
										UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("    Arguments JSON parse failed, using empty object"));
									}
								}
							}
						}
					}
					return Result;
				}
			}
		}
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  ALL STRATEGIES FAILED - no tool call found"));
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::DispatchToolCall(const FString& ToolName, const TSharedPtr<FJsonObject>& Arguments)
{
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Dispatching tool call: %s"), *ToolName);
	if (ArchitectInteractionMode == EAIInteractionMode::JustChat)
	{
		if (!IsReadOnlyTool(ToolName))
		{
			FToolExecutionResult Result;
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(
				TEXT("Tool '%s' was blocked because Just Chat mode is active. "),
				TEXT("In Just Chat mode, only read-only tools are allowed. "),
				TEXT("Switch to Auto Edit mode to use this tool."),
				*ToolName
			);
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Tool blocked in Just Chat mode: %s"), *ToolName);
			return Result;
		}
	}
	if (ToolName == TEXT("classify_intent"))
	{
		return ExecuteTool_ClassifyIntent(Arguments);
	}
	else if (ToolName == TEXT("find_asset_by_name"))
	{
		return ExecuteTool_FindAssetByName(Arguments);
	}
	else if (ToolName == TEXT("generate_blueprint_logic"))
	{
		return ExecuteTool_GenerateBlueprintLogic(Arguments);
	}
	else if (ToolName == TEXT("get_current_folder"))
	{
		return ExecuteTool_GetCurrentFolder(Arguments);
	}
	else if (ToolName == TEXT("list_assets_in_folder"))
	{
		return ExecuteTool_ListAssetsInFolder(Arguments);
	}
	else if (ToolName == TEXT("create_blueprint"))
	{
		return ExecuteTool_CreateBlueprint(Arguments);
	}
	else if (ToolName == TEXT("add_component"))
	{
		return ExecuteTool_AddComponent(Arguments);
	}
	else if (ToolName == TEXT("add_variable"))
	{
		return ExecuteTool_AddVariable(Arguments);
	}
	else if (ToolName == TEXT("get_asset_summary"))
	{
		return ExecuteTool_GetAssetSummary(Arguments);
	}
	else if (ToolName == TEXT("get_selected_assets"))
	{
		return ExecuteTool_GetSelectedAssets(Arguments);
	}
	else if (ToolName == TEXT("spawn_actors"))
	{
		return ExecuteTool_SpawnActors(Arguments);
	}
	else if (ToolName == TEXT("get_all_scene_actors"))
	{
		return ExecuteTool_GetAllSceneActors(Arguments);
	}
	else if (ToolName == TEXT("create_material"))
	{
		return ExecuteTool_CreateMaterial(Arguments);
	}
	else if (ToolName == TEXT("generate_pbr_material"))
	{
		return ExecuteTool_GeneratePBRMaterial(Arguments);
	}
	else if (ToolName == TEXT("generate_texture"))
	{
		return ExecuteTool_GenerateTexture(Arguments);
	}
	else if (ToolName == TEXT("create_material_from_textures"))
	{
		return ExecuteTool_CreateMaterialFromTextures(Arguments);
	}
	else if (ToolName == TEXT("set_actor_materials"))
	{
		return ExecuteTool_SetActorMaterials(Arguments);
	}
	else if (ToolName == TEXT("get_api_context"))
	{
		return ExecuteTool_GetApiContext(Arguments);
	}
	else if (ToolName == TEXT("get_selected_nodes"))
	{
		return ExecuteTool_GetSelectedNodes(Arguments);
	}
	else if (ToolName == TEXT("create_struct"))
	{
		return ExecuteTool_CreateStruct(Arguments);
	}
	else if (ToolName == TEXT("create_enum"))
	{
		return ExecuteTool_CreateEnum(Arguments);
	}
	else if (ToolName == TEXT("get_data_asset_details"))
	{
		return ExecuteTool_GetDataAssetDetails(Arguments);
	}
	else if (ToolName == TEXT("edit_component_property"))
	{
		return ExecuteTool_EditComponentProperty(Arguments);
	}
	else if (ToolName == TEXT("select_actors"))
	{
		return ExecuteTool_SelectActors(Arguments);
	}
	else if (ToolName == TEXT("create_project_folder"))
	{
		return ExecuteTool_CreateProjectFolder(Arguments);
	}
	else if (ToolName == TEXT("move_asset"))
	{
		return ExecuteTool_MoveAsset(Arguments);
	}
	else if (ToolName == TEXT("move_assets"))
	{
		return ExecuteTool_MoveAssets(Arguments);
	}
	else if (ToolName == TEXT("get_detailed_blueprint_summary"))
	{
		return ExecuteTool_GetDetailedBlueprintSummary(Arguments);
	}
	else if (ToolName == TEXT("get_material_graph_summary"))
	{
		return ExecuteTool_GetMaterialGraphSummary(Arguments);
	}
	else if (ToolName == TEXT("insert_code"))
	{
		return ExecuteTool_InsertCode(Arguments);
	}
	else if (ToolName == TEXT("get_project_root_path"))
	{
		return ExecuteTool_GetProjectRootPath(Arguments);
	}
	else if (ToolName == TEXT("add_widget_to_user_widget"))
	{
		return ExecuteTool_AddWidgetToUserWidget(Arguments);
	}
	else if (ToolName == TEXT("edit_widget_property"))
	{
		return ExecuteTool_EditWidgetProperty(Arguments);
	}
	else if (ToolName == TEXT("get_batch_widget_properties"))
	{
		return ExecuteTool_GetBatchWidgetProperties(Arguments);
	}
	else if (ToolName == TEXT("add_widgets_to_layout"))
	{
		return ExecuteTool_AddWidgetsToLayout(Arguments);
	}
	else if (ToolName == TEXT("create_widget_from_layout"))
	{
		return ExecuteTool_CreateWidgetFromLayout(Arguments);
	}
	else if (ToolName == TEXT("get_behavior_tree_summary"))
	{
		return ExecuteTool_GetBehaviorTreeSummary(Arguments);
	}
	else if (ToolName == TEXT("edit_data_asset_defaults"))
	{
		return ExecuteTool_EditDataAssetDefaults(Arguments);
	}
	else if (ToolName == TEXT("delete_variable"))
	{
		return ExecuteTool_DeleteVariable(Arguments);
	}
	else if (ToolName == TEXT("categorize_variables"))
	{
		return ExecuteTool_CategorizeVariables(Arguments);
	}
	else if (ToolName == TEXT("delete_component"))
	{
		return ExecuteTool_DeleteComponent(Arguments);
	}
	else if (ToolName == TEXT("delete_widget"))
	{
		return ExecuteTool_DeleteWidget(Arguments);
	}
	else if (ToolName == TEXT("delete_nodes"))
	{
		return ExecuteTool_DeleteNodes(Arguments);
	}
	else if (ToolName == TEXT("delete_function"))
	{
		return ExecuteTool_DeleteFunction(Arguments);
	}
	else if (ToolName == TEXT("undo_last_generated"))
	{
		return ExecuteTool_UndoLastGenerated(Arguments);
	}
	else if (ToolName == TEXT("check_project_source"))
	{
		return ExecuteTool_CheckProjectSource(Arguments);
	}
	else if (ToolName == TEXT("select_folder"))
	{
		return ExecuteTool_SelectFolder(Arguments);
	}
	else if (ToolName == TEXT("scan_directory"))
	{
		return ExecuteTool_ScanDirectory(Arguments);
	}
	else if (ToolName == TEXT("read_cpp_file"))
	{
		return ExecuteTool_ReadCppFile(Arguments);
	}
	else if (ToolName == TEXT("write_cpp_file"))
	{
		return ExecuteTool_WriteCppFile(Arguments);
	}
	else if (ToolName == TEXT("create_model_3d"))
	{
		return ExecuteTool_CreateModel3D(Arguments);
	}
	else if (ToolName == TEXT("refine_model_3d"))
	{
		return ExecuteTool_RefineModel3D(Arguments);
	}
	else if (ToolName == TEXT("image_to_model_3d"))
	{
		return ExecuteTool_ImageToModel3D(Arguments);
	}
	else if (ToolName == TEXT("create_data_table"))
	{
		return ExecuteTool_CreateDataTable(Arguments);
	}
	else if (ToolName == TEXT("create_input_action"))
	{
		return ExecuteTool_CreateInputAction(Arguments);
	}
	else if (ToolName == TEXT("create_input_mapping_context"))
	{
		return ExecuteTool_CreateInputMappingContext(Arguments);
	}
	else if (ToolName == TEXT("add_input_mapping"))
	{
		return ExecuteTool_AddInputMapping(Arguments);
	}
	else if (ToolName == TEXT("add_data_table_rows"))
	{
		return ExecuteTool_AddDataTableRows(Arguments);
	}
	else
	{
		FToolExecutionResult Result;
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Unknown tool: %s"), *ToolName);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("%s"), *Result.ErrorMessage);
		return Result;
	}
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ClassifyIntent(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString UserMessage;
	if (!Args->TryGetStringField(TEXT("user_message"), UserMessage))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: user_message");
		return Result;
	}
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		Result.bSuccess = true;
		Result.ResultJson = TEXT("{\"intent\": \"CHAT\", \"needs_patterns\": false, \"message\": \"No API key configured\"}");
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classified as: CHAT (no API key)"));
		return Result;
	}
	FString IntentPrompt = FString::Printf(
		TEXT("Classify the user's intent. Reply with ONLY ONE of these words: CODE, ASSET, MODEL, TEXTURE, CHAT\n\n")
		TEXT("CODE = User wants to generate Blueprint code, logic, functions, variables, or modify existing Blueprint graphs\n")
		TEXT("ASSET = User wants to CREATE new assets like Input Actions, Input Mapping Contexts, Actors, Widget Blueprints, Data Tables, Enums, Structs\n")
		TEXT("MODEL = User wants to generate or create a 3D model, mesh, or 3D asset\n")
		TEXT("TEXTURE = User wants to generate or create textures, materials, PBR materials, or images\n")
		TEXT("CHAT = User is asking a question, having a conversation, or requesting information\n\n")
		TEXT("User message: \"%s\"\n\n")
		TEXT("Reply with ONLY one word:"),
		*UserMessage
	);
	FString CapturedProvider = ProviderStr;
	FString CapturedBaseURL = BaseURL;
	FString CapturedModelName = ModelName;
	FString CapturedApiKey = ApiKeyToUse;
	FString CapturedOpenAIModel = FApiKeyManager::Get().GetActiveOpenAIModel();
	FString CapturedClaudeModel = FApiKeyManager::Get().GetActiveClaudeModel();
	FString CapturedGeminiModel = FApiKeyManager::Get().GetActiveGeminiModel();
	TWeakPtr<SBpGeneratorUltimateWidget> WeakWidget = SharedThis(this);
	TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetTimeout(30.0f);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString RequestBody;
	if (CapturedProvider == TEXT("Custom"))
	{
		FString CustomURL = CapturedBaseURL.TrimStartAndEnd();
		if (CustomURL.IsEmpty()) CustomURL = TEXT("https://api.openai.com/v1/chat/completions");
		Request->SetURL(CustomURL);
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *CapturedApiKey));
		FString CustomModel = CapturedModelName.TrimStartAndEnd();
		if (CustomModel.IsEmpty()) CustomModel = TEXT("gpt-3.5-turbo");
		JsonPayload->SetStringField(TEXT("model"), CustomModel);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (CapturedProvider == TEXT("OpenAI"))
	{
		Request->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *CapturedApiKey));
		JsonPayload->SetStringField(TEXT("model"), CapturedOpenAIModel);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (CapturedProvider == TEXT("Claude"))
	{
		Request->SetURL(TEXT("https://api.anthropic.com/v1/messages"));
		Request->SetHeader(TEXT("x-api-key"), CapturedApiKey);
		Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
		JsonPayload->SetStringField(TEXT("model"), CapturedClaudeModel);
		JsonPayload->SetNumberField(TEXT("max_tokens"), 100);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (CapturedProvider == TEXT("Gemini"))
	{
		Request->SetURL(FString::Printf(TEXT("https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s"), *CapturedGeminiModel, *CapturedApiKey));
		TSharedPtr<FJsonObject> Contents = MakeShareable(new FJsonObject);
		TArray<TSharedPtr<FJsonValue>> Parts;
		TSharedPtr<FJsonObject> Part = MakeShareable(new FJsonObject);
		Part->SetStringField(TEXT("text"), IntentPrompt);
		Parts.Add(MakeShareable(new FJsonValueObject(Part)));
		Contents->SetArrayField(TEXT("parts"), Parts);
		TArray<TSharedPtr<FJsonValue>> ContentsArray;
		ContentsArray.Add(MakeShareable(new FJsonValueObject(Contents)));
		JsonPayload->SetArrayField(TEXT("contents"), ContentsArray);
	}
	else
	{
		Result.bSuccess = true;
		Result.ResultJson = FString::Printf(TEXT("{\"intent\": \"CHAT\", \"needs_patterns\": false, \"message\": \"Unsupported provider: %s\"}"), *CapturedProvider);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Unsupported provider for classification: %s"), *CapturedProvider);
		return Result;
	}
	TSharedRef<TJsonWriter<>> BodyWriter = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), BodyWriter);
	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindLambda([WeakWidget](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		FToolExecutionResult AsyncResult;
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			FString ResponseText = Response->GetContentAsString();
			FString IntentResult = ResponseText.TrimStartAndEnd().ToUpper();
			FString FinalIntent;
			bool bNeedsPatterns = false;
			if (IntentResult.Contains(TEXT("CODE")))
			{
				FinalIntent = TEXT("CODE");
				bNeedsPatterns = true;
			}
			else if (IntentResult.Contains(TEXT("ASSET")))
			{
				FinalIntent = TEXT("ASSET");
			}
			else if (IntentResult.Contains(TEXT("MODEL")))
			{
				FinalIntent = TEXT("MODEL");
			}
			else if (IntentResult.Contains(TEXT("TEXTURE")))
			{
				FinalIntent = TEXT("TEXTURE");
			}
			else
			{
				FinalIntent = TEXT("CHAT");
			}
			TSharedPtr<FJsonObject> ResultObj = MakeShareable(new FJsonObject);
			ResultObj->SetBoolField(TEXT("success"), true);
			ResultObj->SetStringField(TEXT("intent"), FinalIntent);
			ResultObj->SetBoolField(TEXT("needs_patterns"), bNeedsPatterns);
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&AsyncResult.ResultJson);
			FJsonSerializer::Serialize(ResultObj.ToSharedRef(), Writer);
			AsyncResult.bSuccess = true;
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classified by AI as: %s"), *FinalIntent);
		}
		else
		{
			AsyncResult.bSuccess = true;
			AsyncResult.ResultJson = TEXT("{\"intent\": \"CHAT\", \"needs_patterns\": false, \"message\": \"AI classification request failed\"}");
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification request failed, defaulting to CHAT"));
		}
		AsyncTask(ENamedThreads::GameThread, [WeakWidget, AsyncResult]()
		{
			TSharedPtr<SBpGeneratorUltimateWidget> PinnedWidget = WeakWidget.Pin();
			if (PinnedWidget.IsValid())
			{
				PinnedWidget->ContinueConversationWithToolResult(TEXT("classify_intent"), AsyncResult);
			}
		});
	});
	if (!Request->ProcessRequest())
	{
		Result.bSuccess = true;
		Result.ResultJson = TEXT("{\"intent\": \"CHAT\", \"needs_patterns\": false, \"message\": \"Failed to start HTTP request\"}");
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Failed to start classification request, defaulting to CHAT"));
		return Result;
	}
	Result.bIsPending = true;
	Result.bSuccess = true;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Intent classification request started, returning pending..."));
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_FindAssetByName(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString NamePattern;
	if (!Args->TryGetStringField(TEXT("name_pattern"), NamePattern))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name_pattern");
		return Result;
	}
	FString AssetType;
	Args->TryGetStringField(TEXT("asset_type"), AssetType);
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	TArray<FAssetData> FoundAssets;
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	if (!AssetType.IsEmpty())
	{
		if (AssetType.Equals(TEXT("Blueprint"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		}
		else if (AssetType.Equals(TEXT("Material"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
		}
		else if (AssetType.Equals(TEXT("Texture"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(UTexture::StaticClass()->GetClassPathName());
		}
	}
	AssetRegistry.GetAssets(Filter, FoundAssets);
	TArray<FAssetData> MatchingAssets;
	for (const FAssetData& Asset : FoundAssets)
	{
		if (Asset.AssetName.ToString().Contains(NamePattern, ESearchCase::IgnoreCase))
		{
			MatchingAssets.Add(Asset);
			if (MatchingAssets.Num() >= 20)
			{
				break;
			}
		}
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FAssetData& Asset : MatchingAssets)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShareable(new FJsonObject);
		AssetObject->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		AssetObject->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		AssetObject->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
		AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetObject)));
	}
	ResultObject->SetArrayField(TEXT("assets"), AssetsArray);
	ResultObject->SetNumberField(TEXT("count"), MatchingAssets.Num());
	ResultObject->SetBoolField(TEXT("truncated"), FoundAssets.Num() > 20);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.bSuccess = true;
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GenerateBlueprintLogic(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString Code;
	if (!Args->TryGetStringField(TEXT("code"), Code))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: code");
		return Result;
	}
	FBpGeneratorUltimateModule& BpGeneratorModule = FModuleManager::LoadModuleChecked<FBpGeneratorUltimateModule>("BpGeneratorUltimate");
	UBlueprint* TargetBlueprint = BpGeneratorModule.GetTargetBlueprint();
	FString BlueprintPath;
	if (Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath) && !BlueprintPath.IsEmpty())
	{
		UBlueprint* SpecifiedBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
		if (SpecifiedBP)
		{
			TargetBlueprint = SpecifiedBP;
		}
	}
	if (!TargetBlueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No target Blueprint selected or found. Please select a Blueprint in the Content Browser or specify a valid blueprint_path.");
		return Result;
	}
	FBpGeneratorCompiler Compiler(TargetBlueprint);
	bool bCompileSuccess = Compiler.Compile(Code.TrimStartAndEnd());
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), bCompileSuccess);
	ResultObject->SetStringField(TEXT("blueprint_name"), TargetBlueprint->GetName());
	if (!bCompileSuccess)
	{
		ResultObject->SetStringField(TEXT("error"), TEXT("Compilation failed. Check the Output Log for details."));
	}
	else
	{
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully generated Blueprint logic in '%s'"), *TargetBlueprint->GetName()));
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.bSuccess = bCompileSuccess;
	Result.ResultJson = ResultString;
	if (!bCompileSuccess)
	{
		Result.ErrorMessage = TEXT("Blueprint compilation failed.");
	}
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetCurrentFolder(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	FString CurrentPath;
	FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
	if (!CurrentPathItem.GetVirtualPathName().IsNone())
	{
		CurrentPath = CurrentPathItem.GetInternalPathString();
	}
	if (CurrentPath.IsEmpty())
	{
		TArray<FString> SelectedFolders;
		ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
		if (SelectedFolders.Num() > 0)
		{
			CurrentPath = SelectedFolders[0];
		}
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (CurrentPath.IsEmpty())
	{
		CurrentPath = TEXT("/Game/");
		ResultObject->SetStringField(TEXT("folder_path"), CurrentPath);
		ResultObject->SetBoolField(TEXT("is_default"), true);
		ResultObject->SetStringField(TEXT("note"), TEXT("Could not determine current folder. Using /Game/ as default. Please click on a folder in the Content Browser."));
	}
	else
	{
		ResultObject->SetStringField(TEXT("folder_path"), CurrentPath);
		ResultObject->SetBoolField(TEXT("is_default"), false);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.bSuccess = true;
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ListAssetsInFolder(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString FolderPath;
	if (!Args->TryGetStringField(TEXT("folder_path"), FolderPath))
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
		if (!CurrentPathItem.GetVirtualPathName().IsNone())
		{
			FolderPath = CurrentPathItem.GetInternalPathString();
		}
		if (FolderPath.IsEmpty())
		{
			TArray<FString> SelectedFolders;
			ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
			if (SelectedFolders.Num() > 0)
			{
				FolderPath = SelectedFolders[0];
			}
		}
		if (FolderPath.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("No folder_path provided and could not determine current folder. Please specify a folder_path or click on a folder in the Content Browser.");
			return Result;
		}
	}
	FString AssetTypeFilter;
	Args->TryGetStringField(TEXT("asset_type"), AssetTypeFilter);
	FString NamePattern;
	Args->TryGetStringField(TEXT("name_pattern"), NamePattern);
	if (!FolderPath.StartsWith(TEXT("/")) && !FolderPath.Contains(TEXT(":")))
	{
		FolderPath = TEXT("/") + FolderPath;
	}
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	bool bRecursive = false;
	Args->TryGetBoolField(TEXT("recursive"), bRecursive);
	AssetRegistryModule.Get().GetAssetsByPath(FName(*FolderPath), AssetData, bRecursive);
	TArray<TSharedPtr<FJsonValue>> AssetJsonArray;
	for (const FAssetData& Data : AssetData)
	{
		if (!AssetTypeFilter.IsEmpty())
		{
			FString AssetClass = Data.AssetClassPath.GetAssetName().ToString();
			if (!AssetClass.Contains(AssetTypeFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		if (!NamePattern.IsEmpty())
		{
			if (!Data.AssetName.ToString().Contains(NamePattern, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}
		TSharedPtr<FJsonObject> AssetObject = MakeShareable(new FJsonObject);
		AssetObject->SetStringField(TEXT("name"), Data.AssetName.ToString());
		AssetObject->SetStringField(TEXT("path"), Data.GetObjectPathString());
		AssetObject->SetStringField(TEXT("class"), Data.AssetClassPath.GetAssetName().ToString());
		AssetJsonArray.Add(MakeShareable(new FJsonValueObject(AssetObject)));
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetStringField(TEXT("folder_path"), FolderPath);
	ResultObject->SetArrayField(TEXT("assets"), AssetJsonArray);
	ResultObject->SetNumberField(TEXT("count"), AssetJsonArray.Num());
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.bSuccess = true;
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateBlueprint(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString BpName;
	if (!Args->TryGetStringField(TEXT("name"), BpName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		return Result;
	}
	FString ParentClassName = TEXT("Actor");
	Args->TryGetStringField(TEXT("parent_class"), ParentClassName);
	FString SavePath;
	if (!Args->TryGetStringField(TEXT("save_path"), SavePath) || SavePath.IsEmpty())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
		if (!CurrentPathItem.GetVirtualPathName().IsNone())
		{
			SavePath = CurrentPathItem.GetInternalPathString();
		}
		if (SavePath.IsEmpty())
		{
			TArray<FString> SelectedFolders;
			ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
			if (SelectedFolders.Num() > 0)
			{
				SavePath = SelectedFolders[0];
			}
		}
		if (SavePath.IsEmpty())
		{
			SavePath = TEXT("/Game");
		}
	}
	UClass* FoundParentClass = FindFirstObjectSafe<UClass>(*ParentClassName);
	if (!FoundParentClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAllAssets(AssetData);
		for (const FAssetData& Data : AssetData)
		{
			if (Data.AssetName.ToString() == ParentClassName)
			{
				UBlueprint* BpAsset = Cast<UBlueprint>(Data.GetAsset());
				if (BpAsset && BpAsset->GeneratedClass)
				{
					FoundParentClass = BpAsset->GeneratedClass;
					break;
				}
			}
		}
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!FoundParentClass)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Parent class '%s' could not be found."), *ParentClassName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Parent class '%s' could not be found."), *ParentClassName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString PackagePath = SavePath;
	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}
	PackagePath += BpName;
	if (FPackageName::DoesPackageExist(PackagePath))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Asset already exists at path '%s'."), *PackagePath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Asset already exists at path '%s'."), *PackagePath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	bool bIsWidgetBlueprint = FoundParentClass->IsChildOf(UUserWidget::StaticClass());
	UBlueprint* NewBlueprint = nullptr;
	if (bIsWidgetBlueprint)
	{
		NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
			FoundParentClass,
			CreatePackage(*PackagePath),
			FName(*BpName),
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass()
		);
	}
	else
	{
		NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
			FoundParentClass,
			CreatePackage(*PackagePath),
			FName(*BpName),
			BPTYPE_Normal,
			UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass()
		);
	}
	if (NewBlueprint)
	{
		FAssetRegistryModule::AssetCreated(NewBlueprint);
		NewBlueprint->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		UPackage::SavePackage(NewBlueprint->GetPackage(), NewBlueprint, *PackageFileName, SaveArgs);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_blueprint] Saved package to: %s"), *PackageFileName);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FString> PathsToScan;
		PathsToScan.Add(PackagePath);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_blueprint] Scanning path: %s"), *PackagePath);
		AssetRegistryModule.Get().ScanPathsSynchronous(PathsToScan, true);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_blueprint] Scan complete"));
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("asset_path"), NewBlueprint->GetPathName());
		ResultObject->SetStringField(TEXT("name"), BpName);
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created Blueprint '%s' at '%s'"), *BpName, *NewBlueprint->GetPathName()));
		Result.bSuccess = true;
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Failed to create Blueprint."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create Blueprint.");
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddComponent(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		return Result;
	}
	FString ComponentClassName;
	if (!Args->TryGetStringField(TEXT("component_class"), ComponentClassName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: component_class");
		return Result;
	}
	FString ComponentName;
	if (!Args->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		ComponentName = ComponentClassName + TEXT("_0");
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UClass* FoundComponentClass = FindFirstObjectSafe<UClass>(*ComponentClassName);
	if (!FoundComponentClass)
	{
		FoundComponentClass = FindFirstObjectSafe<UClass>(*(ComponentClassName + TEXT("Component")));
	}
	if (!FoundComponentClass || !FoundComponentClass->IsChildOf(UActorComponent::StaticClass()))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Component class '%s' could not be found or is not a valid Actor Component."), *ComponentClassName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Component class '%s' could not be found or is not a valid Actor Component."), *ComponentClassName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UActorComponent* NewComponentInstance = NewObject<UActorComponent>(GetTransientPackage(), FoundComponentClass, FName(*ComponentName));
	if (!NewComponentInstance)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to create new component instance for '%s'."), *ComponentName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create new component instance for '%s'."), *ComponentName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<UActorComponent*> ComponentsToAdd;
	ComponentsToAdd.Add(NewComponentInstance);
	FKismetEditorUtilities::FAddComponentsToBlueprintParams Params;
	FKismetEditorUtilities::AddComponentsToBlueprint(TargetBlueprint, ComponentsToAdd, Params);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("component_name"), ComponentName);
	ResultObject->SetStringField(TEXT("component_class"), ComponentClassName);
	ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added %s component '%s' to Blueprint"), *ComponentClassName, *ComponentName));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddVariable(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		return Result;
	}
	FString VarName;
	if (!Args->TryGetStringField(TEXT("var_name"), VarName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: var_name");
		return Result;
	}
	FString VarType;
	if (!Args->TryGetStringField(TEXT("var_type"), VarType))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: var_type");
		return Result;
	}
	FString DefaultValue;
	Args->TryGetStringField(TEXT("default_value"), DefaultValue);
	FString Category;
	Args->TryGetStringField(TEXT("category"), Category);
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FGuid ExistingVarGuid = FBlueprintEditorUtils::FindMemberVariableGuidByName(TargetBlueprint, FName(*VarName));
	if (ExistingVarGuid.IsValid())
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Variable '%s' already exists. No changes made."), *VarName));
		Result.bSuccess = true;
		Result.ResultJson = TEXT("{\"success\": true}");
		return Result;
	}
	FEdGraphPinType PinType;
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	auto ParseTypeString = [&](const FString& TypeStr, FEdGraphPinType& OutPinType) -> bool
	{
		FString InnerType = TypeStr;
		if (TypeStr.StartsWith(TEXT("TArray<")) && TypeStr.EndsWith(TEXT(">")))
		{
			InnerType = TypeStr.Mid(7, TypeStr.Len() - 8).TrimStartAndEnd();
			OutPinType.ContainerType = EPinContainerType::Array;
		}
		else if (TypeStr.StartsWith(TEXT("Array<")) && TypeStr.EndsWith(TEXT(">")))
		{
			InnerType = TypeStr.Mid(6, TypeStr.Len() - 7).TrimStartAndEnd();
			OutPinType.ContainerType = EPinContainerType::Array;
		}
		if (InnerType.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Boolean;
		}
		else if (InnerType.Equals(TEXT("byte"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Byte;
		}
		else if (InnerType.Equals(TEXT("int"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Int;
		}
		else if (InnerType.Equals(TEXT("int64"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Int64;
		}
		else if (InnerType.Equals(TEXT("float"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Real;
			OutPinType.PinSubCategory = K2Schema->PC_Double;
		}
		else if (InnerType.Equals(TEXT("double"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Real;
			OutPinType.PinSubCategory = K2Schema->PC_Double;
		}
		else if (InnerType.Equals(TEXT("string"), ESearchCase::IgnoreCase) ||
				 InnerType.Equals(TEXT("FString"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_String;
		}
		else if (InnerType.Equals(TEXT("text"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Text;
		}
		else if (InnerType.Equals(TEXT("name"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Name;
		}
		else if (InnerType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
		}
		else if (InnerType.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
		}
		else if (InnerType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
		}
		else if (InnerType.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FColor>::Get();
		}
		else if (InnerType.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get();
		}
		else if (InnerType.Equals(TEXT("Vector2D"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get();
		}
		else if (InnerType.Equals(TEXT("Quat"), ESearchCase::IgnoreCase))
		{
			OutPinType.PinCategory = K2Schema->PC_Struct;
			OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get();
		}
		else if (InnerType.StartsWith(TEXT("TSubclassOf<")) && InnerType.EndsWith(TEXT(">")))
		{
			FString ClassName = InnerType.Mid(12, InnerType.Len() - 13).TrimStartAndEnd();
			UClass* FoundClass = nullptr;
			FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
			if (!FoundClass)
			{
				FoundClass = UClass::TryFindTypeSlow<UClass>(ClassName);
			}
			if (!FoundClass)
			{
				FString ScriptPath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
				FoundClass = LoadObject<UClass>(nullptr, *ScriptPath);
			}
			if (!FoundClass)
			{
				FString UMGPath = FString::Printf(TEXT("/Script/UMGEditor.%s"), *ClassName);
				FoundClass = LoadObject<UClass>(nullptr, *UMGPath);
			}
			if (!FoundClass)
			{
				FString UMGPath2 = FString::Printf(TEXT("/Script/UMG.%s"), *ClassName);
				FoundClass = LoadObject<UClass>(nullptr, *UMGPath2);
			}
			if (!FoundClass)
			{
				for (TObjectIterator<UClass> It; It; ++It)
				{
					if (It->GetName().Equals(ClassName, ESearchCase::IgnoreCase) ||
						It->GetFName().ToString().Equals(ClassName, ESearchCase::IgnoreCase))
					{
						FoundClass = *It;
						break;
					}
				}
			}
			if (!FoundClass)
			{
				TArray<FAssetData> Assets;
				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				AssetRegistryModule.Get().GetAssetsByPath(FName(TEXT("/Game")), Assets, true);
				for (const FAssetData& AssetData : Assets)
				{
					if (AssetData.AssetName.ToString().Equals(ClassName, ESearchCase::IgnoreCase))
					{
						if (UObject* Asset = AssetData.GetAsset())
						{
							if (UBlueprint* BP = Cast<UBlueprint>(Asset))
							{
								if (BP->GeneratedClass)
								{
									FoundClass = BP->GeneratedClass;
									break;
								}
							}
							else if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Asset))
							{
								if (WidgetBP->GeneratedClass)
								{
									FoundClass = WidgetBP->GeneratedClass;
									break;
								}
							}
						}
					}
				}
			}
			if (FoundClass)
			{
				OutPinType.PinCategory = K2Schema->PC_Class;
				OutPinType.PinSubCategoryObject = FoundClass;
			}
			else
			{
				return false;
			}
		}
		else if (InnerType.StartsWith(TEXT("TSoftObjectPtr<")) && InnerType.EndsWith(TEXT(">")))
		{
			FString AssetPath = InnerType.Mid(15, InnerType.Len() - 16).TrimStartAndEnd();
			UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
			if (UClass* AssetClass = Cast<UClass>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_SoftObject;
				OutPinType.PinSubCategoryObject = AssetClass;
			}
			else if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_SoftObject;
				OutPinType.PinSubCategoryObject = BP->GeneratedClass;
			}
			else
			{
				return false;
			}
		}
		else if (InnerType.StartsWith(TEXT("TSoftClassPtr<")) && InnerType.EndsWith(TEXT(">")))
		{
			FString AssetPath = InnerType.Mid(14, InnerType.Len() - 15).TrimStartAndEnd();
			UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
			if (UClass* AssetClass = Cast<UClass>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_SoftClass;
				OutPinType.PinSubCategoryObject = AssetClass;
			}
			else if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_SoftClass;
				OutPinType.PinSubCategoryObject = BP->GeneratedClass;
			}
			else
			{
				return false;
			}
		}
		else if (InnerType.StartsWith(TEXT("/Game/")) || InnerType.StartsWith(TEXT("/Engine/")))
		{
			UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(InnerType);
			if (!LoadedAsset)
			{
				LoadedAsset = LoadObject<UObject>(nullptr, *InnerType);
			}
			if (UBlueprint* BP = Cast<UBlueprint>(LoadedAsset))
			{
				if (BP->GeneratedClass)
				{
					OutPinType.PinCategory = K2Schema->PC_Object;
					OutPinType.PinSubCategoryObject = BP->GeneratedClass;
				}
				else
				{
					OutPinType.PinCategory = K2Schema->PC_Class;
					OutPinType.PinSubCategoryObject = BP->GetClass();
				}
			}
			else if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedAsset))
			{
				if (WidgetBP->GeneratedClass)
				{
					OutPinType.PinCategory = K2Schema->PC_Object;
					OutPinType.PinSubCategoryObject = WidgetBP->GeneratedClass;
				}
				else
				{
					return false;
				}
			}
			else if (UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_Struct;
				OutPinType.PinSubCategoryObject = UserStruct;
			}
			else if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_Struct;
				OutPinType.PinSubCategoryObject = ScriptStruct;
			}
			else if (UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = UserEnum;
			}
			else if (UEnum* Enum = Cast<UEnum>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = Enum;
			}
			else if (UClass* Class = Cast<UClass>(LoadedAsset))
			{
				OutPinType.PinCategory = K2Schema->PC_Object;
				OutPinType.PinSubCategoryObject = Class;
			}
			else
			{
				return false;
			}
		}
		else
		{
			FString CleanedInnerType = InnerType;
			if (CleanedInnerType.EndsWith(TEXT("*")))
			{
				CleanedInnerType = CleanedInnerType.LeftChop(1).TrimStartAndEnd();
			}
			UObject* FoundType = LoadObject<UObject>(nullptr, *CleanedInnerType);
			if (!FoundType)
			{
				FoundType = FindFirstObjectSafe<UObject>(*CleanedInnerType);
			}
			if (!FoundType)
			{
				FString ScriptPath = FString::Printf(TEXT("/Script/Engine.%s"), *CleanedInnerType);
				FoundType = StaticFindObject(UClass::StaticClass(), nullptr, *ScriptPath);
			}
			if (!FoundType && (CleanedInnerType.StartsWith(TEXT("A")) || CleanedInnerType.StartsWith(TEXT("U"))))
			{
				FString ShortName = CleanedInnerType.Mid(1);
				FString ScriptPath = FString::Printf(TEXT("/Script/Engine.%s"), *ShortName);
				FoundType = StaticFindObject(UClass::StaticClass(), nullptr, *ScriptPath);
			}
			if (!FoundType)
			{
				for (TObjectIterator<UClass> It; It; ++It)
				{
					FString ClassName = It->GetName();
					if (ClassName.Equals(CleanedInnerType, ESearchCase::IgnoreCase) ||
						(CleanedInnerType.Len() > 1 && ClassName.Mid(1).Equals(CleanedInnerType.Mid(1), ESearchCase::IgnoreCase)))
					{
						FoundType = *It;
						break;
					}
				}
			}
			if (UClass* FoundClass = Cast<UClass>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Object;
				OutPinType.PinSubCategoryObject = FoundClass;
			}
			else if (UUserDefinedStruct* UserStruct = Cast<UUserDefinedStruct>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Struct;
				OutPinType.PinSubCategoryObject = UserStruct;
			}
			else if (UScriptStruct* FoundStruct = Cast<UScriptStruct>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Struct;
				OutPinType.PinSubCategoryObject = FoundStruct;
			}
			else if (UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = UserEnum;
			}
			else if (UEnum* FoundEnum = Cast<UEnum>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = FoundEnum;
			}
			else
			{
				TArray<FAssetData> Assets;
				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				AssetRegistryModule.Get().GetAssetsByPath(FName(TEXT("/Game")), Assets, true);
				for (const FAssetData& AssetData : Assets)
				{
					if (AssetData.AssetName.ToString().Equals(CleanedInnerType, ESearchCase::IgnoreCase))
					{
						if (UObject* Asset = AssetData.GetAsset())
						{
							if (UBlueprint* BP = Cast<UBlueprint>(Asset))
							{
								if (BP->GeneratedClass)
								{
									OutPinType.PinCategory = K2Schema->PC_Object;
									OutPinType.PinSubCategoryObject = BP->GeneratedClass;
									return true;
								}
							}
							else if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Asset))
							{
								if (WidgetBP->GeneratedClass)
								{
									OutPinType.PinCategory = K2Schema->PC_Object;
									OutPinType.PinSubCategoryObject = WidgetBP->GeneratedClass;
									return true;
								}
							}
							else if (UUserDefinedStruct* FoundUserStruct = Cast<UUserDefinedStruct>(Asset))
							{
								OutPinType.PinCategory = K2Schema->PC_Struct;
								OutPinType.PinSubCategoryObject = FoundUserStruct;
								return true;
							}
							else if (UUserDefinedEnum* FoundUserEnum = Cast<UUserDefinedEnum>(Asset))
							{
								OutPinType.PinCategory = K2Schema->PC_Byte;
								OutPinType.PinSubCategoryObject = FoundUserEnum;
								return true;
							}
						}
					}
				}
				return false;
			}
		}
		return true;
	};
	if (!ParseTypeString(VarType, PinType))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown or unsupported variable type: %s"), *VarType));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Unknown or unsupported variable type: %s"), *VarType);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditorUtils::AddMemberVariable(TargetBlueprint, FName(*VarName), PinType, DefaultValue);
	if (!Category.IsEmpty())
	{
		for (FBPVariableDescription& VarDesc : TargetBlueprint->NewVariables)
		{
			if (VarDesc.VarName == FName(*VarName))
			{
				VarDesc.Category = FText::FromString(Category);
				break;
			}
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("var_name"), VarName);
	ResultObject->SetStringField(TEXT("var_type"), VarType);
	ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added variable '%s' of type '%s' to Blueprint"), *VarName, *VarType));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteVariable(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		return Result;
	}
	FString VarName;
	if (!Args->TryGetStringField(TEXT("var_name"), VarName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: var_name");
		return Result;
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FGuid VarGuid = FBlueprintEditorUtils::FindMemberVariableGuidByName(TargetBlueprint, FName(*VarName));
	if (!VarGuid.IsValid())
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Variable '%s' not found in Blueprint."), *VarName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Variable '%s' not found in Blueprint."), *VarName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Variable")));
	TargetBlueprint->Modify();
	FBlueprintEditorUtils::RemoveMemberVariable(TargetBlueprint, FName(*VarName));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("var_name"), VarName);
	ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted variable '%s' from Blueprint."), *VarName));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CategorizeVariables(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		TArray<FAssetData> SelectedAssetsData;
		ContentBrowserSingleton.GetSelectedAssets(SelectedAssetsData);
		if (SelectedAssetsData.Num() > 0)
		{
			for (const FAssetData& AssetData : SelectedAssetsData)
			{
				if (AssetData.AssetClassPath.GetAssetName() == TEXT("Blueprint"))
				{
					BlueprintPath = AssetData.GetObjectPathString();
					break;
				}
			}
		}
		if (BlueprintPath.IsEmpty())
		{
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), TEXT("No blueprint_path provided and no Blueprint selected. Please select a Blueprint in the Content Browser."));
			Result.bSuccess = false;
			Result.ErrorMessage = ResultObject->GetStringField(TEXT("error"));
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath));
		Result.bSuccess = false;
		Result.ErrorMessage = ResultObject->GetStringField(TEXT("error"));
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TSharedPtr<FJsonObject>* CategoriesObj = nullptr;
	bool bApplyCategories = Args->TryGetObjectField(TEXT("categories"), CategoriesObj);
	if (bApplyCategories)
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("Categorize Variables")));
		TargetBlueprint->Modify();
		int32 UpdatedCount = 0;
		int32 SkippedCount = 0;
		for (FBPVariableDescription& VarDesc : TargetBlueprint->NewVariables)
		{
			FString VarName = VarDesc.VarName.ToString();
			if ((*CategoriesObj)->HasField(VarName))
			{
				if (VarDesc.Category.ToString() == TEXT("Default") || VarDesc.Category.IsEmpty())
				{
					FString NewCategory = (*CategoriesObj)->GetStringField(VarName);
					VarDesc.Category = FText::FromString(NewCategory);
					UpdatedCount++;
				}
				else
				{
					SkippedCount++;
				}
			}
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetNumberField(TEXT("updated_count"), UpdatedCount);
		ResultObject->SetNumberField(TEXT("skipped_count"), SkippedCount);
		ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
		if (SkippedCount > 0)
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully categorized %d variables. Skipped %d variables that were already in custom categories."), UpdatedCount, SkippedCount));
		}
		else
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully categorized %d variables."), UpdatedCount));
		}
		Result.bSuccess = true;
	}
	else
	{
		TArray<TSharedPtr<FJsonValue>> VariablesArray;
		for (const FBPVariableDescription& VarDesc : TargetBlueprint->NewVariables)
		{
			if (VarDesc.Category.ToString() == TEXT("Default") || VarDesc.Category.IsEmpty())
			{
				TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject);
				VarObj->SetStringField(TEXT("name"), VarDesc.VarName.ToString());
				VarObj->SetStringField(TEXT("type"), VarDesc.VarType.PinCategory.ToString());
				VarObj->SetStringField(TEXT("category"), VarDesc.Category.ToString());
				VariablesArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
			}
		}
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetArrayField(TEXT("variables"), VariablesArray);
		ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
		ResultObject->SetStringField(TEXT("blueprint_name"), TargetBlueprint->GetName());
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d uncategorized variables. Provide a 'categories' object to organize them. Already-categorized variables will be preserved."), VariablesArray.Num()));
		Result.bSuccess = true;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetAssetSummary(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString AssetPath;
	if (!Args->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: asset_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!LoadedAsset)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to load asset at path: %s"), *AssetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Summary;
	if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset))
	{
		Summary = FBpSummarizerr::Summarize(Blueprint);
	}
	else if (UMaterial* Material = Cast<UMaterial>(LoadedAsset))
	{
		FMaterialGraphDescriber Describer;
		Summary = Describer.Describe(Material);
	}
	else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(LoadedAsset))
	{
		FBtGraphDescriber Describer;
		Summary = Describer.Describe(BehaviorTree);
	}
	else
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Asset '%s' is not a supported type (Blueprint, Material, or Behavior Tree)."), *LoadedAsset->GetClass()->GetName());
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObject->SetStringField(TEXT("asset_type"), LoadedAsset->GetClass()->GetName());
	ResultObject->SetStringField(TEXT("summary"), Summary);
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_SpawnActors(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	const TArray<TSharedPtr<FJsonValue>>* ActorsArray;
	if (!Args->TryGetArrayField(TEXT("actors"), ActorsArray) || ActorsArray->Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing or empty required parameter: actors (array)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not get editor world.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	int32 SuccessCount = 0;
	int32 FailCount = 0;
	TArray<TSharedPtr<FJsonValue>> SpawnedActorsArray;
	for (const TSharedPtr<FJsonValue>& ActorValue : *ActorsArray)
	{
		const TSharedPtr<FJsonObject>* ActorObj;
		if (!ActorValue->TryGetObject(ActorObj))
		{
			FailCount++;
			continue;
		}
		FString ActorClass;
		if (!(*ActorObj)->TryGetStringField(TEXT("actor_class"), ActorClass))
		{
			FailCount++;
			continue;
		}
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
		FString ActorLabel;
		const TArray<TSharedPtr<FJsonValue>>* LocationArray;
		if ((*ActorObj)->TryGetArrayField(TEXT("location"), LocationArray) && LocationArray->Num() >= 3)
		{
			Location.X = (*LocationArray)[0]->AsNumber();
			Location.Y = (*LocationArray)[1]->AsNumber();
			Location.Z = (*LocationArray)[2]->AsNumber();
		}
		const TArray<TSharedPtr<FJsonValue>>* RotationArray;
		if ((*ActorObj)->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray->Num() >= 3)
		{
			Rotation.Pitch = (*RotationArray)[0]->AsNumber();
			Rotation.Yaw = (*RotationArray)[1]->AsNumber();
			Rotation.Roll = (*RotationArray)[2]->AsNumber();
		}
		const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
		if ((*ActorObj)->TryGetArrayField(TEXT("scale"), ScaleArray) && ScaleArray->Num() >= 3)
		{
			Scale.X = (*ScaleArray)[0]->AsNumber();
			Scale.Y = (*ScaleArray)[1]->AsNumber();
			Scale.Z = (*ScaleArray)[2]->AsNumber();
		}
		(*ActorObj)->TryGetStringField(TEXT("label"), ActorLabel);
		AActor* SpawnedActor = nullptr;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ActorClass.Equals(TEXT("Cube"), ESearchCase::IgnoreCase) ||
			ActorClass.Equals(TEXT("Sphere"), ESearchCase::IgnoreCase) ||
			ActorClass.Equals(TEXT("Cylinder"), ESearchCase::IgnoreCase) ||
			ActorClass.Equals(TEXT("Cone"), ESearchCase::IgnoreCase))
		{
			FString MeshPath;
			if (ActorClass.Equals(TEXT("Cube"), ESearchCase::IgnoreCase))
				MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
			else if (ActorClass.Equals(TEXT("Sphere"), ESearchCase::IgnoreCase))
				MeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
			else if (ActorClass.Equals(TEXT("Cylinder"), ESearchCase::IgnoreCase))
				MeshPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
			else if (ActorClass.Equals(TEXT("Cone"), ESearchCase::IgnoreCase))
				MeshPath = TEXT("/Engine/BasicShapes/Cone.Cone");
			UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *MeshPath));
			if (Mesh)
			{
				SpawnedActor = EditorWorld->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParams);
				if (AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(SpawnedActor))
				{
					SMActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
					SMActor->SetActorScale3D(Scale);
				}
			}
		}
		else if (ActorClass.StartsWith(TEXT("/Game/")) || ActorClass.StartsWith(TEXT("/Script/")))
		{
			UClass* ClassToSpawn = nullptr;
			UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(ActorClass));
			if (Blueprint && Blueprint->GeneratedClass)
			{
				ClassToSpawn = Blueprint->GeneratedClass;
			}
			else
			{
				ClassToSpawn = FindObject<UClass>(nullptr, *ActorClass);
			}
			if (ClassToSpawn && ClassToSpawn->IsChildOf(AActor::StaticClass()))
			{
				SpawnedActor = EditorWorld->SpawnActor<AActor>(ClassToSpawn, Location, Rotation, SpawnParams);
				if (SpawnedActor)
				{
					SpawnedActor->SetActorScale3D(Scale);
				}
			}
		}
		else
		{
			FString FullClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ActorClass);
			UClass* ClassToSpawn = FindObject<UClass>(nullptr, *FullClassName);
			if (!ClassToSpawn)
			{
				FullClassName = FString::Printf(TEXT("/Script/Engine.A%s"), *ActorClass);
				ClassToSpawn = FindObject<UClass>(nullptr, *FullClassName);
			}
			if (ClassToSpawn && ClassToSpawn->IsChildOf(AActor::StaticClass()))
			{
				SpawnedActor = EditorWorld->SpawnActor<AActor>(ClassToSpawn, Location, Rotation, SpawnParams);
				if (SpawnedActor)
				{
					SpawnedActor->SetActorScale3D(Scale);
				}
			}
		}
		if (SpawnedActor)
		{
			if (!ActorLabel.IsEmpty())
			{
				SpawnedActor->SetActorLabel(ActorLabel);
			}
			SuccessCount++;
			TSharedPtr<FJsonObject> SpawnedObj = MakeShareable(new FJsonObject);
			SpawnedObj->SetStringField(TEXT("label"), SpawnedActor->GetActorLabel());
			SpawnedObj->SetStringField(TEXT("class"), ActorClass);
			SpawnedActorsArray.Add(MakeShareable(new FJsonValueObject(SpawnedObj)));
		}
		else
		{
			FailCount++;
		}
	}
	ResultObject->SetBoolField(TEXT("success"), SuccessCount > 0);
	ResultObject->SetNumberField(TEXT("spawned_count"), SuccessCount);
	ResultObject->SetNumberField(TEXT("failed_count"), FailCount);
	ResultObject->SetArrayField(TEXT("spawned_actors"), SpawnedActorsArray);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Spawned %d actors successfully (%d failed)"), SuccessCount, FailCount));
	Result.bSuccess = SuccessCount > 0;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetAllSceneActors(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not get editor world.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> ActorsArray;
	for (TActorIterator<AActor> It(EditorWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsHidden()) continue;
		TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
		ActorObj->SetStringField(TEXT("label"), Actor->GetActorLabel());
		ActorObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		FVector Loc = Actor->GetActorLocation();
		TSharedPtr<FJsonObject> LocationObj = MakeShareable(new FJsonObject);
		LocationObj->SetNumberField(TEXT("x"), Loc.X);
		LocationObj->SetNumberField(TEXT("y"), Loc.Y);
		LocationObj->SetNumberField(TEXT("z"), Loc.Z);
		ActorObj->SetObjectField(TEXT("location"), LocationObj);
		ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetArrayField(TEXT("actors"), ActorsArray);
	ResultObject->SetNumberField(TEXT("count"), ActorsArray.Num());
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateMaterial(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString MaterialName;
	if (!Args->TryGetStringField(TEXT("name"), MaterialName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString SavePath;
	if (!Args->TryGetStringField(TEXT("save_path"), SavePath) || SavePath.IsEmpty())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
		if (!CurrentPathItem.GetVirtualPathName().IsNone())
		{
			SavePath = CurrentPathItem.GetInternalPathString();
		}
		if (SavePath.IsEmpty())
		{
			TArray<FString> SelectedFolders;
			ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
			if (SelectedFolders.Num() > 0)
			{
				SavePath = SelectedFolders[0];
			}
		}
		if (SavePath.IsEmpty())
		{
			SavePath = TEXT("/Game");
		}
	}
	FLinearColor Color = FLinearColor::White;
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Args->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		Color.R = (*ColorArray)[0]->AsNumber();
		Color.G = (*ColorArray)[1]->AsNumber();
		Color.B = (*ColorArray)[2]->AsNumber();
		Color.A = ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f;
	}
	FString MaterialPath = SavePath;
	if (!MaterialPath.EndsWith(TEXT("/")))
	{
		MaterialPath += TEXT("/");
	}
	MaterialPath += MaterialName;
	UPackage* Package = CreatePackage(*MaterialPath);
	if (!Package)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to create package at path: %s"), *MaterialPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(),
		Package,
		FName(*MaterialName),
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));
	if (!NewMaterial)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create material asset.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UMaterialExpressionConstant3Vector* ColorExpression = NewObject<UMaterialExpressionConstant3Vector>(NewMaterial);
	ColorExpression->Constant = FLinearColor(Color.R, Color.G, Color.B);
	NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(ColorExpression);
	NewMaterial->GetEditorOnlyData()->BaseColor.Expression = ColorExpression;
	NewMaterial->PreEditChange(nullptr);
	NewMaterial->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewMaterial);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("material_path"), MaterialPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created material at '%s' with color (%.2f, %.2f, %.2f)"), *MaterialPath, Color.R, Color.G, Color.B));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GeneratePBRMaterial(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (ActivePBRMaterialGen.bIsGenerating)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("A PBR material is already being generated. Please wait for it to complete.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString MaterialName;
	if (!Args->TryGetStringField(TEXT("name"), MaterialName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Description;
	if (!Args->TryGetStringField(TEXT("description"), Description))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: description");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	ActivePBRMaterialGen.MaterialName = MaterialName;
	ActivePBRMaterialGen.Description = Description;
	ActivePBRMaterialGen.SavePath = TEXT("/Game/GeneratedMaterials");
	ActivePBRMaterialGen.bIsMetallic = false;
	ActivePBRMaterialGen.bGenerateAO = true;
	ActivePBRMaterialGen.CurrentStep = 0;
	ActivePBRMaterialGen.TotalSteps = 5;
	ActivePBRMaterialGen.BaseColorPath.Empty();
	ActivePBRMaterialGen.NormalPath.Empty();
	ActivePBRMaterialGen.RoughnessPath.Empty();
	ActivePBRMaterialGen.MetallicPath.Empty();
	ActivePBRMaterialGen.AOPath.Empty();
	ActivePBRMaterialGen.bIsGenerating = true;
	if (!Args->TryGetStringField(TEXT("save_path"), ActivePBRMaterialGen.SavePath) || ActivePBRMaterialGen.SavePath.IsEmpty())
	{
		ActivePBRMaterialGen.SavePath = TEXT("/Game/GeneratedMaterials");
	}
	Args->TryGetBoolField(TEXT("is_metallic"), ActivePBRMaterialGen.bIsMetallic);
	Args->TryGetBoolField(TEXT("generate_ao"), ActivePBRMaterialGen.bGenerateAO);
	if (!ActivePBRMaterialGen.bGenerateAO)
	{
		ActivePBRMaterialGen.TotalSteps = 4;
	}
	ProcessPBRMaterialGenerationStep();
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Started generating PBR material '%s' - this will take several minutes..."), *MaterialName));
	ResultObject->SetNumberField(TEXT("total_textures"), ActivePBRMaterialGen.TotalSteps);
	ResultObject->SetStringField(TEXT("material_name"), MaterialName);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
void SBpGeneratorUltimateWidget::ProcessPBRMaterialGenerationStep()
{
	if (ActivePBRMaterialGen.CurrentStep >= ActivePBRMaterialGen.TotalSteps)
	{
		CreatePBRMaterialFromGeneratedTextures();
		return;
	}
	FString Prompt;
	FString AssetName = ActivePBRMaterialGen.MaterialName;
	FString Suffix;
	switch (ActivePBRMaterialGen.CurrentStep)
	{
	case 0:
		Prompt = FString::Printf(TEXT("photorealistic %s texture, seamless, high quality, detailed"), *ActivePBRMaterialGen.Description);
		Suffix = TEXT("_BC");
		break;
	case 1:
		Prompt = FString::Printf(TEXT("normal map for %s, surface detail, bump information, seamless"), *ActivePBRMaterialGen.Description);
		Suffix = TEXT("_N");
		break;
	case 2:
		Prompt = FString::Printf(TEXT("roughness map for %s, white=rough black=smooth, grayscale, seamless"), *ActivePBRMaterialGen.Description);
		Suffix = TEXT("_R");
		break;
	case 3:
		if (ActivePBRMaterialGen.bIsMetallic)
		{
			Prompt = FString::Printf(TEXT("metallic map for %s, white=metal black=non-metal, grayscale, seamless"), *ActivePBRMaterialGen.Description);
		}
		else
		{
			Prompt = FString::Printf(TEXT("non-metallic %s, black (no metal), grayscale, seamless"), *ActivePBRMaterialGen.Description);
		}
		Suffix = TEXT("_M");
		break;
	case 4:
		Prompt = FString::Printf(TEXT("ambient occlusion map for %s, cavities are dark, grayscale, seamless"), *ActivePBRMaterialGen.Description);
		Suffix = TEXT("_AO");
		break;
	}
	AssetName = TEXT("T_") + ActivePBRMaterialGen.MaterialName + Suffix;
	FTextureGenRequest Request;
	Request.Prompt = Prompt;
	Request.AspectRatio = TEXT("1:1");
	Request.CustomAssetName = AssetName;
	Request.SavePath = ActivePBRMaterialGen.SavePath.IsEmpty() ? TEXT("/Game/GeneratedTextures") : ActivePBRMaterialGen.SavePath;
	FString ApiKey = FApiKeyManager::Get().GetActiveApiKey();
	FTextureGenManager::Get().GenerateTexture(Request, ApiKey,
		[this](const FTextureGenResult& TexResult)
		{
			OnPBRTextureGenerated(TexResult);
		}
	);
}
void SBpGeneratorUltimateWidget::OnPBRTextureGenerated(const FTextureGenResult& Result)
{
	if (!Result.bSuccess)
	{
		ActivePBRMaterialGen.bIsGenerating = false;
		UE_LOG(LogTemp, Error, TEXT("PBR Material Generation Error: %s"), *Result.ErrorMessage);
		return;
	}
	switch (ActivePBRMaterialGen.CurrentStep)
	{
	case 0:
		ActivePBRMaterialGen.BaseColorPath = Result.AssetPath;
		break;
	case 1:
		ActivePBRMaterialGen.NormalPath = Result.AssetPath;
		break;
	case 2:
		ActivePBRMaterialGen.RoughnessPath = Result.AssetPath;
		break;
	case 3:
		ActivePBRMaterialGen.MetallicPath = Result.AssetPath;
		break;
	case 4:
		ActivePBRMaterialGen.AOPath = Result.AssetPath;
		break;
	}
	ActivePBRMaterialGen.CurrentStep++;
	ProcessPBRMaterialGenerationStep();
}
void SBpGeneratorUltimateWidget::CreatePBRMaterialFromGeneratedTextures()
{
	FString MaterialName = ActivePBRMaterialGen.MaterialName;
	FString SavePath = ActivePBRMaterialGen.SavePath.IsEmpty() ? TEXT("/Game/GeneratedMaterials") : ActivePBRMaterialGen.SavePath;
	FString MaterialPath = FString::Printf(TEXT("%s/%s"), *SavePath, *MaterialName);
	UPackage* Package = CreatePackage(*MaterialPath);
	if (!Package)
	{
		ActivePBRMaterialGen.bIsGenerating = false;
		UE_LOG(LogTemp, Error, TEXT("Failed to create package at path: %s"), *MaterialPath);
		return;
	}
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(),
		Package,
		FName(*MaterialName),
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));
	if (!NewMaterial)
	{
		ActivePBRMaterialGen.bIsGenerating = false;
		UE_LOG(LogTemp, Error, TEXT("Failed to create material asset."));
		return;
	}
	NewMaterial->PreEditChange(nullptr);
	UTexture2D* BaseColorTex = LoadObject<UTexture2D>(nullptr, *ActivePBRMaterialGen.BaseColorPath);
	UTexture2D* NormalTex = LoadObject<UTexture2D>(nullptr, *ActivePBRMaterialGen.NormalPath);
	UTexture2D* RoughnessTex = LoadObject<UTexture2D>(nullptr, *ActivePBRMaterialGen.RoughnessPath);
	UTexture2D* MetallicTex = LoadObject<UTexture2D>(nullptr, *ActivePBRMaterialGen.MetallicPath);
	UTexture2D* AOTex = ActivePBRMaterialGen.bGenerateAO ? LoadObject<UTexture2D>(nullptr, *ActivePBRMaterialGen.AOPath) : nullptr;
	int32 NodeX = -600;
	int32 NodeY = 0;
	int32 NodeSpacing = 400;
	if (BaseColorTex)
	{
		UMaterialExpressionTextureSample* BCExpression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		BCExpression->Texture = BaseColorTex;
		BCExpression->SamplerType = SAMPLERTYPE_Color;
		BCExpression->MaterialExpressionEditorX = NodeX;
		BCExpression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(BCExpression);
		NewMaterial->GetEditorOnlyData()->BaseColor.Expression = BCExpression;
		NodeY += NodeSpacing;
	}
	if (NormalTex)
	{
		UMaterialExpressionTextureSample* NExpression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		NExpression->Texture = NormalTex;
		NExpression->SamplerType = SAMPLERTYPE_Color;
		NExpression->MaterialExpressionEditorX = NodeX;
		NExpression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(NExpression);
		NewMaterial->GetEditorOnlyData()->Normal.Expression = NExpression;
		NodeY += NodeSpacing;
	}
	if (RoughnessTex)
	{
		UMaterialExpressionTextureSample* RExpression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		RExpression->Texture = RoughnessTex;
		RExpression->SamplerType = SAMPLERTYPE_Color;
		RExpression->MaterialExpressionEditorX = NodeX;
		RExpression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(RExpression);
		NewMaterial->GetEditorOnlyData()->Roughness.Expression = RExpression;
		NodeY += NodeSpacing;
	}
	if (MetallicTex)
	{
		UMaterialExpressionTextureSample* MExpression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		MExpression->Texture = MetallicTex;
		MExpression->SamplerType = SAMPLERTYPE_Color;
		MExpression->MaterialExpressionEditorX = NodeX;
		MExpression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(MExpression);
		NewMaterial->GetEditorOnlyData()->Metallic.Expression = MExpression;
		NodeY += NodeSpacing;
	}
	if (AOTex)
	{
		UMaterialExpressionTextureSample* AOExpression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		AOExpression->Texture = AOTex;
		AOExpression->SamplerType = SAMPLERTYPE_Color;
		AOExpression->MaterialExpressionEditorX = NodeX;
		AOExpression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(AOExpression);
		NewMaterial->GetEditorOnlyData()->AmbientOcclusion.Expression = AOExpression;
	}
	NewMaterial->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.AssetCreated(NewMaterial);
	UE_LOG(LogTemp, Log, TEXT("Successfully generated PBR material '%s' at '%s' with %d texture maps"), *MaterialName, *MaterialPath, ActivePBRMaterialGen.TotalSteps);
	ActivePBRMaterialGen.bIsGenerating = false;
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ PBR Material Generated: %s (%d textures)"), *MaterialName, ActivePBRMaterialGen.TotalSteps)));
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = true;
	FSlateNotificationManager::Get().AddNotification(Info);
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GenerateTexture(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString Prompt;
	if (!Args->TryGetStringField(TEXT("prompt"), Prompt) || Prompt.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: prompt");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString TextureName;
	Args->TryGetStringField(TEXT("texture_name"), TextureName);
	FString SavePath = TEXT("/Game/GeneratedTextures");
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	FString AspectRatio = TEXT("1024x1024");
	Args->TryGetStringField(TEXT("aspect_ratio"), AspectRatio);
	FString ApiKey = FApiKeyManager::Get().GetActiveApiKey();
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Texture Generation Failed: No API key configured"));
		UE_LOG(LogTemp, Error, TEXT("Please configure an API key in the plugin settings."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No API key configured. Please configure an API key in the plugin settings.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UE_LOG(LogTemp, Log, TEXT("Texture Generation: Using API key (first 8 chars): %s..."), *ApiKey.Left(8));
	UE_LOG(LogTemp, Log, TEXT("Texture Gen Endpoint: %s"), *FApiKeyManager::Get().GetActiveTextureGenEndpoint());
	UE_LOG(LogTemp, Log, TEXT("Texture Gen Model: %s"), *FApiKeyManager::Get().GetActiveTextureGenModel());
	FTextureGenRequest Request;
	Request.Prompt = Prompt;
	Request.CustomAssetName = TextureName;
	Request.AspectRatio = AspectRatio;
	Request.SavePath = SavePath;
	FTextureGenManager::Get().GenerateTexture(Request, ApiKey,
		[this, ResultObject, Prompt, SavePath, TextureName](const FTextureGenResult& GenResult)
		{
			UE_LOG(LogTemp, Log, TEXT("Texture Generation Callback - Success: %s, Error: %s, AssetPath: %s"),
				GenResult.bSuccess ? TEXT("true") : TEXT("false"), *GenResult.ErrorMessage, *GenResult.AssetPath);
			if (GenResult.bSuccess)
			{
				ResultObject->SetBoolField(TEXT("success"), true);
				ResultObject->SetStringField(TEXT("texture_path"), GenResult.AssetPath);
				ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Texture generated successfully: %s"), *GenResult.AssetPath));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ Texture Generated: %s"), *GenResult.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), GenResult.ErrorMessage);
				UE_LOG(LogTemp, Error, TEXT("Texture Generation Failed: %s"), *GenResult.ErrorMessage);
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Texture Generation Failed: %s"), *GenResult.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		});
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("Texture generation started..."));
	ResultObject->SetStringField(TEXT("prompt"), Prompt);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateModel3D(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString Prompt;
	if (!Args->TryGetStringField(TEXT("prompt"), Prompt) || Prompt.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: prompt");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ModelName;
	Args->TryGetStringField(TEXT("model_name"), ModelName);
	FString SavePath = TEXT("/Game/GeneratedModels");
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	bool bAutoRefine = FApiKeyManager::Get().GetActiveMeshGenAutoRefine();
	Args->TryGetBoolField(TEXT("auto_refine"), bAutoRefine);
	int32 Seed = -1;
	Args->TryGetNumberField(TEXT("seed"), Seed);
	FString ApiKey = MeshyApiKey;
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("3D Model Generation Failed: No Meshy API key configured"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No Meshy API key configured. Please configure it in the 3D Model Generator view.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	E3DModelType SelectedModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
	EMeshQuality SelectedQuality = FApiKeyManager::Get().GetActiveSelectedQuality();
	FString CustomEndpoint;
	FString CustomModel;
	FString CustomFormat;
	if (SelectedModelType == E3DModelType::custom || SelectedModelType == E3DModelType::others)
	{
		CustomEndpoint = FApiKeyManager::Get().GetActiveMeshGenEndpoint();
		CustomModel = FApiKeyManager::Get().GetActiveMeshGenModel();
		CustomFormat = FApiKeyManager::Get().GetActiveMeshGenFormat();
	}
	FMeshCreationRequest Request;
	Request.Prompt = Prompt;
	Request.CustomAssetName = ModelName;
	Request.SavePath = SavePath;
	Request.bAutoRefine = bAutoRefine;
	Request.Seed = Seed;
	Request.ModelType = SelectedModelType;
	Request.Quality = SelectedQuality;
	Request.CustomEndpoint = CustomEndpoint;
	Request.CustomModelName = CustomModel;
	Request.ModelFormat = CustomFormat.IsEmpty() ? TEXT("glb") : CustomFormat;
	FMeshAssetManager::Get().CreateMeshFromText(Request, ApiKey,
		[this, ResultObject, Prompt, SavePath, ModelName](const FMeshCreationResult& GenResult)
		{
			if (GenResult.bSuccess)
			{
				ResultObject->SetBoolField(TEXT("success"), true);
				ResultObject->SetStringField(TEXT("model_path"), GenResult.AssetPath);
				ResultObject->SetStringField(TEXT("task_id"), GenResult.TaskId);
				ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("3D model generated successfully: %s"), *GenResult.AssetPath));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ 3D Model Generated: %s"), *GenResult.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), GenResult.ErrorMessage);
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Model Generation Failed: %s"), *GenResult.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		});
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("3D model generation started..."));
	ResultObject->SetStringField(TEXT("prompt"), Prompt);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_RefineModel3D(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString TaskId;
	if (!Args->TryGetStringField(TEXT("task_id"), TaskId) || TaskId.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: task_id");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ModelName;
	Args->TryGetStringField(TEXT("model_name"), ModelName);
	FString SavePath = TEXT("/Game/GeneratedModels");
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	FString ApiKey = MeshyApiKey;
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("3D Model Refinement Failed: No Meshy API key configured"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No Meshy API key configured. Please configure it in the 3D Model Generator view.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FMeshCreationRequest Request;
	Request.PreviewTaskId = TaskId;
	Request.CustomAssetName = ModelName;
	Request.SavePath = SavePath;
	FMeshAssetManager::Get().RefineMeshPreview(Request, ApiKey,
		[this, ResultObject, TaskId, SavePath, ModelName](const FMeshCreationResult& GenResult)
		{
			if (GenResult.bSuccess)
			{
				ResultObject->SetBoolField(TEXT("success"), true);
				ResultObject->SetStringField(TEXT("model_path"), GenResult.AssetPath);
				ResultObject->SetStringField(TEXT("task_id"), GenResult.TaskId);
				ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("3D model refined successfully: %s"), *GenResult.AssetPath));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ 3D Model Refined: %s"), *GenResult.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), GenResult.ErrorMessage);
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Model Refinement Failed: %s"), *GenResult.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		});
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("3D model refinement started..."));
	ResultObject->SetStringField(TEXT("task_id"), TaskId);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ImageToModel3D(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ImagePath;
	if (!Args->TryGetStringField(TEXT("image_path"), ImagePath) || ImagePath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: image_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Prompt;
	Args->TryGetStringField(TEXT("prompt"), Prompt);
	FString ModelName;
	Args->TryGetStringField(TEXT("model_name"), ModelName);
	FString SavePath = TEXT("/Game/GeneratedModels");
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	FString ApiKey = MeshyApiKey;
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Image-to-3D Failed: No Meshy API key configured"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No Meshy API key configured. Please configure it in the 3D Model Generator view.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	E3DModelType SelectedModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
	EMeshQuality SelectedQuality = FApiKeyManager::Get().GetActiveSelectedQuality();
	FString CustomEndpoint;
	FString CustomModel;
	FString CustomFormat;
	if (SelectedModelType == E3DModelType::custom || SelectedModelType == E3DModelType::others)
	{
		CustomEndpoint = FApiKeyManager::Get().GetActiveMeshGenEndpoint();
		CustomModel = FApiKeyManager::Get().GetActiveMeshGenModel();
		CustomFormat = FApiKeyManager::Get().GetActiveMeshGenFormat();
	}
	FMeshCreationRequest Request;
	Request.ImagePath = ImagePath;
	Request.Prompt = Prompt;
	Request.CustomAssetName = ModelName;
	Request.SavePath = SavePath;
	Request.ModelType = SelectedModelType;
	Request.Quality = SelectedQuality;
	Request.CustomEndpoint = CustomEndpoint;
	Request.CustomModelName = CustomModel;
	Request.ModelFormat = CustomFormat.IsEmpty() ? TEXT("glb") : CustomFormat;
	FMeshAssetManager::Get().CreateMeshFromImage(Request, ApiKey,
		[this, ResultObject, ImagePath, Prompt, SavePath, ModelName](const FMeshCreationResult& GenResult)
		{
			if (GenResult.bSuccess)
			{
				ResultObject->SetBoolField(TEXT("success"), true);
				ResultObject->SetStringField(TEXT("model_path"), GenResult.AssetPath);
				ResultObject->SetStringField(TEXT("task_id"), GenResult.TaskId);
				ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("3D model generated from image: %s"), *GenResult.AssetPath));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ 3D Model Generated: %s"), *GenResult.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), GenResult.ErrorMessage);
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Image-to-3D Failed: %s"), *GenResult.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		});
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("Image-to-3D conversion started..."));
	ResultObject->SetStringField(TEXT("image_path"), ImagePath);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
enum class ETextureType : uint8
{
	None,
	BaseColor,
	Normal,
	Roughness,
	Metallic,
	AmbientOcclusion,
	Emissive,
	Opacity
};
static ETextureType DetectTextureTypeFromName(const FString& TextureName)
{
	FString LowerName = TextureName.ToLower();
	if (LowerName.Contains(TEXT("basecolor")) || LowerName.Contains(TEXT("base_color")) ||
		LowerName.Contains(TEXT("albedo")) || LowerName.Contains(TEXT("diffuse")) ||
		(LowerName.Contains(TEXT("color")) && !LowerName.Contains(TEXT("emissive"))) ||
		LowerName.EndsWith(TEXT("_bc")) || LowerName.Contains(TEXT("_bc.")) ||
		LowerName.EndsWith(TEXT("_d")) || LowerName.Contains(TEXT("_d.")) ||
		(LowerName.EndsWith(TEXT("_c")) && !LowerName.Contains(TEXT("_n."))))
	{
		return ETextureType::BaseColor;
	}
	if (LowerName.Contains(TEXT("normal")) || LowerName.Contains(TEXT("n.")) ||
		LowerName.Contains(TEXT("_n_")) || LowerName.EndsWith(TEXT("_n")) ||
		LowerName.Contains(TEXT("_n.")) || LowerName.EndsWith(TEXT("_n.")))
	{
		return ETextureType::Normal;
	}
	if (LowerName.Contains(TEXT("roughness")) || LowerName.Contains(TEXT("rough")) ||
		LowerName.EndsWith(TEXT("_r")) || LowerName.Contains(TEXT("_r.")))
	{
		return ETextureType::Roughness;
	}
	if (LowerName.Contains(TEXT("metallic")) || LowerName.Contains(TEXT("metal")) ||
		(LowerName.EndsWith(TEXT("_m")) && !LowerName.Contains(TEXT("norm"))) ||
		LowerName.Contains(TEXT("_m.")))
	{
		return ETextureType::Metallic;
	}
	if (LowerName.Contains(TEXT("ao")) || LowerName.Contains(TEXT("ambient")) ||
		LowerName.Contains(TEXT("occlusion")) || LowerName.Contains(TEXT("_ao")) ||
		LowerName.Contains(TEXT("_ao.")))
	{
		return ETextureType::AmbientOcclusion;
	}
	if (LowerName.Contains(TEXT("emissive")) || LowerName.Contains(TEXT("emit")) ||
		LowerName.Contains(TEXT("emission")) || LowerName.Contains(TEXT("_e")) ||
		LowerName.Contains(TEXT("_e.")))
	{
		return ETextureType::Emissive;
	}
	if (LowerName.Contains(TEXT("opacity")) || LowerName.Contains(TEXT("alpha")))
	{
		return ETextureType::Opacity;
	}
	return ETextureType::None;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateMaterialFromTextures(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString MaterialName;
	if (!Args->TryGetStringField(TEXT("material_name"), MaterialName) || MaterialName.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: material_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString SavePath = TEXT("/Game/GeneratedMaterials");
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	FString Source = TEXT("selected");
	Args->TryGetStringField(TEXT("source"), Source);
	TArray<FAssetData> TextureAssets;
	if (Source == TEXT("folder"))
	{
		FString FolderPath;
		if (!Args->TryGetStringField(TEXT("folder_path"), FolderPath) || FolderPath.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("Missing required parameter: folder_path (required when source='folder')");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> FolderAssets;
		FARFilter Filter;
		Filter.PackagePaths.Add(*FolderPath);
		Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		AssetRegistryModule.Get().GetAssets(Filter, FolderAssets);
		for (const FAssetData& Asset : FolderAssets)
		{
			TextureAssets.Add(Asset);
		}
	}
	else
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> SelectedAssets;
		ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);
		for (const FAssetData& Asset : SelectedAssets)
		{
			if (Asset.IsInstanceOf<UTexture2D>())
			{
				TextureAssets.Add(Asset);
			}
		}
		if (TextureAssets.Num() == 0)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("No texture assets selected. Please select textures in the Content Browser.");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
	}
	TMap<ETextureType, FString> TextureMap;
	TArray<FString> DetectedTextures;
	for (const FAssetData& Asset : TextureAssets)
	{
		FString AssetPath = Asset.GetObjectPathString();
		FString AssetName = Asset.AssetName.ToString();
		ETextureType Type = DetectTextureTypeFromName(AssetName);
		if (Type != ETextureType::None)
		{
			if (!TextureMap.Contains(Type))
			{
				TextureMap.Add(Type, AssetPath);
				FString TypeString;
				switch (Type)
				{
				case ETextureType::BaseColor: TypeString = TEXT("BaseColor"); break;
				case ETextureType::Normal: TypeString = TEXT("Normal"); break;
				case ETextureType::Roughness: TypeString = TEXT("Roughness"); break;
				case ETextureType::Metallic: TypeString = TEXT("Metallic"); break;
				case ETextureType::AmbientOcclusion: TypeString = TEXT("AmbientOcclusion"); break;
				case ETextureType::Emissive: TypeString = TEXT("Emissive"); break;
				case ETextureType::Opacity: TypeString = TEXT("Opacity"); break;
				default: TypeString = TEXT("Unknown"); break;
				}
				DetectedTextures.Add(FString::Printf(TEXT("%s -> %s"), *AssetName, *TypeString));
			}
		}
	}
	if (TextureMap.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not detect any valid texture types from selected assets. Ensure textures are named with patterns like 'BaseColor', 'Normal', 'Roughness', 'Metallic', 'AO', 'Emissive'.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString FullMaterialPath = FString::Printf(TEXT("%s/%s"), *SavePath, *MaterialName);
	if (UEditorAssetLibrary::DoesAssetExist(FullMaterialPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Material already exists at: %s"), *FullMaterialPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UPackage* Package = CreatePackage(*FullMaterialPath);
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(), Package, FName(*MaterialName),
		RF_Public | RF_Standalone, nullptr, GWarn));
	if (!NewMaterial)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create material");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FString> ConnectedTextures;
	int32 NodeX = -600;
	int32 NodeY = 0;
	int32 NodeSpacing = 400;
	for (const auto& Pair : TextureMap)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Pair.Value);
		if (!Texture) continue;
		UMaterialExpressionTextureSample* Expression = NewObject<UMaterialExpressionTextureSample>(NewMaterial);
		Expression->Texture = Texture;
		Expression->SamplerType = SAMPLERTYPE_Color;
		Expression->MaterialExpressionEditorX = NodeX;
		Expression->MaterialExpressionEditorY = NodeY;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
		NodeY += NodeSpacing;
		switch (Pair.Key)
		{
		case ETextureType::BaseColor:
			NewMaterial->GetEditorOnlyData()->BaseColor.Expression = Expression;
			ConnectedTextures.Add(TEXT("BaseColor"));
			break;
		case ETextureType::Normal:
			NewMaterial->GetEditorOnlyData()->Normal.Expression = Expression;
			ConnectedTextures.Add(TEXT("Normal"));
			break;
		case ETextureType::Roughness:
			NewMaterial->GetEditorOnlyData()->Roughness.Expression = Expression;
			ConnectedTextures.Add(TEXT("Roughness"));
			break;
		case ETextureType::Metallic:
			NewMaterial->GetEditorOnlyData()->Metallic.Expression = Expression;
			ConnectedTextures.Add(TEXT("Metallic"));
			break;
		case ETextureType::AmbientOcclusion:
			NewMaterial->GetEditorOnlyData()->AmbientOcclusion.Expression = Expression;
			ConnectedTextures.Add(TEXT("AmbientOcclusion"));
			break;
		case ETextureType::Emissive:
			NewMaterial->GetEditorOnlyData()->EmissiveColor.Expression = Expression;
			ConnectedTextures.Add(TEXT("Emissive"));
			break;
		case ETextureType::Opacity:
			NewMaterial->GetEditorOnlyData()->Opacity.Expression = Expression;
			ConnectedTextures.Add(TEXT("Opacity"));
			break;
		}
	}
	NewMaterial->PreEditChange(nullptr);
	NewMaterial->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.AssetCreated(NewMaterial);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("material_path"), FullMaterialPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Material created with %d texture maps: %s"),
		ConnectedTextures.Num(), *FString::Join(ConnectedTextures, TEXT(", "))));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	Result.bSuccess = true;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_SetActorMaterials(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString MaterialPath;
	if (!Args->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: material_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* ActorLabelsArray;
	if (!Args->TryGetArrayField(TEXT("actor_labels"), ActorLabelsArray) || ActorLabelsArray->Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing or empty required parameter: actor_labels (array)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
	if (!Material)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load material at path: %s"), *MaterialPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!EditorWorld)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not get editor world.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TMap<FString, AActor*> ActorsByLabel;
	for (TActorIterator<AActor> It(EditorWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor)
		{
			ActorsByLabel.Add(Actor->GetActorLabel(), Actor);
		}
	}
	int32 SuccessCount = 0;
	int32 FailCount = 0;
	int32 TotalMaterialsApplied = 0;
	for (const TSharedPtr<FJsonValue>& LabelValue : *ActorLabelsArray)
	{
		FString ActorLabel = LabelValue->AsString();
		if (ActorLabel.IsEmpty())
		{
			FailCount++;
			continue;
		}
		AActor** FoundActorPtr = ActorsByLabel.Find(ActorLabel);
		if (!FoundActorPtr || !(*FoundActorPtr))
		{
			FailCount++;
			continue;
		}
		AActor* FoundActor = *FoundActorPtr;
		TArray<UMeshComponent*> MeshComponents;
		FoundActor->GetComponents<UMeshComponent>(MeshComponents);
		int32 MaterialsApplied = 0;
		for (UMeshComponent* MeshComp : MeshComponents)
		{
			int32 NumMaterials = MeshComp->GetNumMaterials();
			for (int32 i = 0; i < NumMaterials; i++)
			{
				MeshComp->SetMaterial(i, Material);
				MaterialsApplied++;
			}
		}
		if (MaterialsApplied > 0)
		{
			SuccessCount++;
			TotalMaterialsApplied += MaterialsApplied;
		}
		else
		{
			FailCount++;
		}
	}
	ResultObject->SetBoolField(TEXT("success"), SuccessCount > 0);
	ResultObject->SetStringField(TEXT("material_path"), MaterialPath);
	ResultObject->SetNumberField(TEXT("actors_updated"), SuccessCount);
	ResultObject->SetNumberField(TEXT("actors_failed"), FailCount);
	ResultObject->SetNumberField(TEXT("total_material_slots_updated"), TotalMaterialsApplied);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Applied material '%s' to %d actors (%d failed, %d total material slots)"), *MaterialPath, SuccessCount, FailCount, TotalMaterialsApplied));
	Result.bSuccess = SuccessCount > 0;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetApiContext(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString Query;
	if (!Args->TryGetStringField(TEXT("query"), Query))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: query");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BpGeneratorUltimate"));
	if (!Plugin.IsValid())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not find BpGeneratorUltimate plugin.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString CheatsheetPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content/Python/api_cheatsheet.json"));
	if (!FPaths::FileExists(CheatsheetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not find api_cheatsheet.json file.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *CheatsheetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to load api_cheatsheet.json file.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> ApiArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, ApiArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to parse api_cheatsheet.json.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FString> Keywords;
	Query.ToLower().ParseIntoArray(Keywords, TEXT(" "), true);
	struct FScoredFunction
	{
		int32 Score;
		TSharedPtr<FJsonObject> Data;
	};
	TArray<FScoredFunction> ScoredFunctions;
	for (const TSharedPtr<FJsonValue>& Value : ApiArray)
	{
		TSharedPtr<FJsonObject> FuncObj = Value->AsObject();
		if (!FuncObj.IsValid()) continue;
		FString FullSignature, FunctionName, ReturnType;
		FuncObj->TryGetStringField(TEXT("full_signature"), FullSignature);
		FuncObj->TryGetStringField(TEXT("function_name"), FunctionName);
		FuncObj->TryGetStringField(TEXT("return_type"), ReturnType);
		FString SearchText = (FullSignature + TEXT(" ") + ReturnType).ToLower();
		int32 Score = 0;
		for (const FString& Keyword : Keywords)
		{
			if (SearchText.Contains(Keyword))
			{
				Score += 1;
				if (FunctionName.ToLower().Contains(Keyword))
				{
					Score += 2;
				}
			}
		}
		if (Score > 0)
		{
			ScoredFunctions.Add({ Score, FuncObj });
		}
	}
	ScoredFunctions.Sort([](const FScoredFunction& A, const FScoredFunction& B) {
		return A.Score > B.Score;
	});
	FString Context = TEXT("--- RELEVANT API CONTEXT ---\n\n");
	int32 Count = FMath::Min(5, ScoredFunctions.Num());
	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	for (int32 i = 0; i < Count; i++)
	{
		TSharedPtr<FJsonObject> FuncObj = ScoredFunctions[i].Data;
		FString FullSignature, ReturnType;
		FuncObj->TryGetStringField(TEXT("full_signature"), FullSignature);
		FuncObj->TryGetStringField(TEXT("return_type"), ReturnType);
		Context += FString::Printf(TEXT("Function: %s\n"), *FullSignature);
		Context += FString::Printf(TEXT("Returns: %s\n"), *ReturnType);
		const TArray<TSharedPtr<FJsonValue>>* ParamsArray;
		if (FuncObj->TryGetArrayField(TEXT("parameters"), ParamsArray) && ParamsArray->Num() > 0)
		{
			Context += TEXT("Parameters:\n");
			for (const TSharedPtr<FJsonValue>& ParamVal : *ParamsArray)
			{
				TSharedPtr<FJsonObject> ParamObj = ParamVal->AsObject();
				if (ParamObj.IsValid())
				{
					FString ParamType, ParamName;
					ParamObj->TryGetStringField(TEXT("type"), ParamType);
					ParamObj->TryGetStringField(TEXT("name"), ParamName);
					Context += FString::Printf(TEXT("  - %s %s\n"), *ParamType, *ParamName);
				}
			}
		}
		else
		{
			Context += TEXT("Parameters: None\n");
		}
		Context += TEXT("\n");
		ResultsArray.Add(MakeShareable(new FJsonValueObject(FuncObj)));
	}
	if (Count == 0)
	{
		Context = TEXT("No relevant API functions found for that query.");
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("query"), Query);
	ResultObject->SetNumberField(TEXT("results_count"), Count);
	ResultObject->SetStringField(TEXT("context"), Context);
	ResultObject->SetArrayField(TEXT("functions"), ResultsArray);
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetSelectedAssets(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	TArray<FAssetData> SelectedAssets;
	ContentBrowserSingleton.GetSelectedAssets(SelectedAssets);
	if (SelectedAssets.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No assets are currently selected in the Content Browser.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FAssetData& Data : SelectedAssets)
	{
		TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject);
		AssetObj->SetStringField(TEXT("name"), Data.AssetName.ToString());
		AssetObj->SetStringField(TEXT("path"), Data.GetObjectPathString());
		AssetObj->SetStringField(TEXT("class"), Data.AssetClassPath.GetAssetName().ToString());
		AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetArrayField(TEXT("assets"), AssetsArray);
	ResultObject->SetNumberField(TEXT("count"), AssetsArray.Num());
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d selected asset(s) in Content Browser"), AssetsArray.Num()));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetSelectedNodes(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not get the Asset Editor Subsystem.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	IAssetEditorInstance* ActiveEditor = nullptr;
	double LastActivationTime = 0.0;
	TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();
	for (UObject* Asset : EditedAssets)
	{
		TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(Asset);
		for (IAssetEditorInstance* Editor : Editors)
		{
			if (Editor && Editor->GetLastActivationTime() > LastActivationTime)
			{
				LastActivationTime = Editor->GetLastActivationTime();
				ActiveEditor = Editor;
			}
		}
	}
	if (!ActiveEditor)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No active asset editor found. Please open a Blueprint, Material, or Behavior Tree editor.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FName EditorName = ActiveEditor->GetEditorName();
	TSet<UObject*> SelectedNodes;
	FString NodesDescription;
	if (EditorName == FName(TEXT("BlueprintEditor")) || EditorName == FName(TEXT("AnimationBlueprintEditor")))
	{
		FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
		SelectedNodes = BlueprintEditor->GetSelectedNodes();
		FBpGraphDescriber Describer;
		NodesDescription = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("WidgetBlueprintEditor")))
	{
		FWidgetBlueprintEditor* WidgetEditor = static_cast<FWidgetBlueprintEditor*>(ActiveEditor);
		SelectedNodes = WidgetEditor->GetSelectedNodes();
		FBpGraphDescriber Describer;
		NodesDescription = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("MaterialEditor")))
	{
		FMaterialEditor* MaterialEditor = static_cast<FMaterialEditor*>(ActiveEditor);
		SelectedNodes = MaterialEditor->GetSelectedNodes();
		FMaterialNodeDescriber Describer;
		NodesDescription = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("Behavior Tree")))
	{
		FBehaviorTreeEditor* BehaviorTreeEditor = static_cast<FBehaviorTreeEditor*>(ActiveEditor);
		SelectedNodes = BehaviorTreeEditor->GetSelectedNodes();
		FBtGraphDescriber Describer;
		NodesDescription = Describer.DescribeSelection(SelectedNodes);
	}
	else
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("The active editor ('%s') is not a supported type. Supported: Blueprint, Material, Behavior Tree."), *EditorName.ToString());
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (SelectedNodes.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No nodes are currently selected in the active graph editor. Please select some nodes first.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("node_count"), SelectedNodes.Num());
	ResultObject->SetStringField(TEXT("editor_type"), EditorName.ToString());
	ResultObject->SetStringField(TEXT("nodes_description"), NodesDescription);
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
namespace
{
	bool ConvertStringToPinType(const FString& TypeStr, FEdGraphPinType& OutPinType)
	{
		if (TypeStr.Equals(TEXT("bool"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		else if (TypeStr.Equals(TEXT("byte"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
		else if (TypeStr.Equals(TEXT("int"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		else if (TypeStr.Equals(TEXT("int32"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		else if (TypeStr.Equals(TEXT("int64"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
		else if (TypeStr.Equals(TEXT("float"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
		else if (TypeStr.Equals(TEXT("double"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double; }
		else if (TypeStr.Equals(TEXT("name"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
		else if (TypeStr.Equals(TEXT("fname"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Name;
		else if (TypeStr.Equals(TEXT("string"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
		else if (TypeStr.Equals(TEXT("fstring"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_String;
		else if (TypeStr.Equals(TEXT("text"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
		else if (TypeStr.Equals(TEXT("ftext"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Text;
		else if (TypeStr.Equals(TEXT("vector"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get(); }
		else if (TypeStr.Equals(TEXT("fvector"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FVector>::Get(); }
		else if (TypeStr.Equals(TEXT("rotator"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); }
		else if (TypeStr.Equals(TEXT("frotator"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); }
		else if (TypeStr.Equals(TEXT("transform"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get(); }
		else if (TypeStr.Equals(TEXT("ftransform"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get(); }
		else if (TypeStr.Equals(TEXT("color"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FColor>::Get(); }
		else if (TypeStr.Equals(TEXT("fcolor"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FColor>::Get(); }
		else if (TypeStr.Equals(TEXT("linearcolor"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
		else if (TypeStr.Equals(TEXT("flinearcolor"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
		else if (TypeStr.Equals(TEXT("vector2d"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get(); }
		else if (TypeStr.Equals(TEXT("fvector2d"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FVector2D>::Get(); }
		else if (TypeStr.Equals(TEXT("quaternion"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get(); }
		else if (TypeStr.Equals(TEXT("fquat"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get(); }
		else
		{
			UObject* FoundObject = FindFirstObjectSafe<UObject>(*TypeStr);
			if (!FoundObject) { FoundObject = LoadObject<UObject>(nullptr, *TypeStr); }
			if (UClass* FoundClass = Cast<UClass>(FoundObject)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Object; OutPinType.PinSubCategoryObject = FoundClass; }
			else if (UUserDefinedEnum* FoundEnum = Cast<UUserDefinedEnum>(FoundObject)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte; OutPinType.PinSubCategoryObject = FoundEnum; }
			else if (UScriptStruct* FoundStruct = Cast<UScriptStruct>(FoundObject)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = FoundStruct; }
			else { return false; }
		}
		return true;
	}
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateStruct(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString StructName;
	if (!Args->TryGetStringField(TEXT("name"), StructName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString SavePath;
	if (!Args->TryGetStringField(TEXT("save_path"), SavePath) || SavePath.IsEmpty())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
		if (!CurrentPathItem.GetVirtualPathName().IsNone())
		{
			SavePath = CurrentPathItem.GetInternalPathString();
		}
		if (SavePath.IsEmpty())
		{
			SavePath = TEXT("/Game");
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* VariablesArray;
	TArray<TSharedPtr<FJsonValue>> EmptyArray;
	if (!Args->TryGetArrayField(TEXT("variables"), VariablesArray))
	{
		VariablesArray = &EmptyArray;
	}
	FString OutAssetPath, OutError;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(StructName, SavePath, UUserDefinedStruct::StaticClass(), nullptr);
	if (UUserDefinedStruct* NewStruct = Cast<UUserDefinedStruct>(NewAsset))
	{
		NewStruct->EditorData = NewObject<UUserDefinedStructEditorData>(NewStruct);
		FStructureEditorUtils::ModifyStructData(NewStruct);
		for (const TSharedPtr<FJsonValue>& VarValue : *VariablesArray)
		{
			const TSharedPtr<FJsonObject>* VarObject;
			if (VarValue->TryGetObject(VarObject))
			{
				FString VarName, VarType;
				if (!(*VarObject)->TryGetStringField(TEXT("name"), VarName))
				{
					(*VarObject)->TryGetStringField(TEXT("var_name"), VarName);
				}
				if (!(*VarObject)->TryGetStringField(TEXT("type"), VarType))
				{
					(*VarObject)->TryGetStringField(TEXT("var_type"), VarType);
				}
				if (VarName.IsEmpty() || VarType.IsEmpty())
				{
					continue;
				}
				FEdGraphPinType PinType;
				if (ConvertStringToPinType(VarType, PinType))
				{
					FStructureEditorUtils::AddVariable(NewStruct, PinType);
					TArray<FStructVariableDescription>& VarDescriptions = FStructureEditorUtils::GetVarDesc(NewStruct);
					const FGuid VarGuid = VarDescriptions.Last().VarGuid;
					FStructureEditorUtils::RenameVariable(NewStruct, VarGuid, VarName);
				}
			}
		}
		FStructureEditorUtils::OnStructureChanged(NewStruct);
		NewStruct->MarkPackageDirty();
		OutAssetPath = NewStruct->GetPathName();
	}
	else
	{
		OutError = TEXT("Failed to create User Defined Struct asset. The asset may already exist or the path is invalid.");
	}
	if (OutError.IsEmpty())
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("asset_path"), OutAssetPath);
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created struct '%s' at '%s'"), *StructName, *OutAssetPath));
		Result.bSuccess = true;
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		Result.bSuccess = false;
		Result.ErrorMessage = OutError;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateEnum(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString EnumName;
	if (!Args->TryGetStringField(TEXT("name"), EnumName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString SavePath;
	if (!Args->TryGetStringField(TEXT("save_path"), SavePath) || SavePath.IsEmpty())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
		FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
		if (!CurrentPathItem.GetVirtualPathName().IsNone())
		{
			SavePath = CurrentPathItem.GetInternalPathString();
		}
		if (SavePath.IsEmpty())
		{
			SavePath = TEXT("/Game");
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* EnumeratorsArray;
	if (!Args->TryGetArrayField(TEXT("enumerators"), EnumeratorsArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: enumerators (array of strings)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FString> Enumerators;
	for (const TSharedPtr<FJsonValue>& Value : *EnumeratorsArray)
	{
		Enumerators.Add(Value->AsString());
	}
	FString OutAssetPath, OutError;
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(EnumName, SavePath, UUserDefinedEnum::StaticClass(), nullptr);
	if (UUserDefinedEnum* NewEnum = Cast<UUserDefinedEnum>(NewAsset))
	{
		TArray<TPair<FName, int64>> EnumNameValuePairs;
		for (int32 i = 0; i < Enumerators.Num(); ++i)
		{
			FName InternalName = FName(*FString::Printf(TEXT("%s::%s"), *NewEnum->GetName(), *Enumerators[i]));
			EnumNameValuePairs.Add(TPair<FName, int64>(InternalName, i));
		}
		if (NewEnum->SetEnums(EnumNameValuePairs, UEnum::ECppForm::Namespaced))
		{
			for (int32 i = 0; i < Enumerators.Num(); ++i)
			{
				NewEnum->DisplayNameMap.Add(NewEnum->GetNameByIndex(i), FText::FromString(Enumerators[i]));
			}
		}
		NewEnum->MarkPackageDirty();
		OutAssetPath = NewEnum->GetPathName();
	}
	else
	{
		OutError = TEXT("Failed to create User Defined Enum asset. The asset may already exist or the path is invalid.");
	}
	if (OutError.IsEmpty())
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("asset_path"), OutAssetPath);
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created enum '%s' at '%s'"), *EnumName, *OutAssetPath));
		Result.bSuccess = true;
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		Result.bSuccess = false;
		Result.ErrorMessage = OutError;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetDataAssetDetails(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString AssetPath;
	if (!Args->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: asset_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString OutError;
	UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Asset)
	{
		OutError = FString::Printf(TEXT("Could not load asset at path: %s"), *AssetPath);
	}
	else
	{
		TSharedPtr<FJsonObject> DetailsObject = MakeShareable(new FJsonObject);
		if (UUserDefinedEnum* Enum = Cast<UUserDefinedEnum>(Asset))
		{
			DetailsObject->SetStringField("type", "Enum");
			TArray<TSharedPtr<FJsonValue>> EnumValues;
			for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
			{
				FString DisplayName = Enum->GetDisplayNameTextByIndex(i).ToString();
				EnumValues.Add(MakeShareable(new FJsonValueString(DisplayName)));
			}
			DetailsObject->SetArrayField("values", EnumValues);
		}
		else if (UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Asset))
		{
			DetailsObject->SetStringField("type", "Struct");
			TArray<TSharedPtr<FJsonValue>> StructMembers;
			const TArray<FStructVariableDescription>& VarDescriptions = FStructureEditorUtils::GetVarDesc(Struct);
			for (const FStructVariableDescription& VarDesc : VarDescriptions)
			{
				TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject);
				MemberObject->SetStringField("name", VarDesc.VarName.ToString());
				FEdGraphPinType PinType = VarDesc.ToPinType();
				if (PinType.PinSubCategoryObject.IsValid())
				{
					MemberObject->SetStringField("type", PinType.PinSubCategoryObject->GetName());
				}
				else
				{
					MemberObject->SetStringField("type", PinType.PinCategory.ToString());
				}
				StructMembers.Add(MakeShareable(new FJsonValueObject(MemberObject)));
			}
			DetailsObject->SetArrayField("members", StructMembers);
		}
		else if (Asset->IsA<UBlueprint>())
		{
			UClass* BpClass = Cast<UBlueprint>(Asset)->GeneratedClass;
			if (BpClass)
			{
				DetailsObject->SetStringField("type", "Blueprint");
				TArray<TSharedPtr<FJsonValue>> BpMembers;
				for (TFieldIterator<FProperty> PropIt(BpClass); PropIt; ++PropIt)
				{
					FProperty* Property = *PropIt;
					if (Property && Property->HasAnyPropertyFlags(CPF_Edit) && Property->GetOwnerClass() == BpClass)
					{
						TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject);
						MemberObject->SetStringField("name", Property->GetName());
						MemberObject->SetStringField("type", Property->GetClass()->GetName());
						BpMembers.Add(MakeShareable(new FJsonValueObject(MemberObject)));
					}
				}
				DetailsObject->SetArrayField("members", BpMembers);
			}
		}
		else
		{
			DetailsObject->SetStringField("type", "DataObject");
			TArray<TSharedPtr<FJsonValue>> Members;
			for (TFieldIterator<FProperty> PropIt(Asset->GetClass()); PropIt; ++PropIt)
			{
				FProperty* Property = *PropIt;
				if (Property && Property->HasAnyPropertyFlags(CPF_Edit))
				{
					TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject);
					MemberObject->SetStringField("name", Property->GetName());
					MemberObject->SetStringField("type", Property->GetClass()->GetName());
					Members.Add(MakeShareable(new FJsonValueObject(MemberObject)));
				}
			}
			DetailsObject->SetArrayField("members", Members);
		}
		FString DetailsString;
		TSharedRef<TJsonWriter<>> DetailsWriter = TJsonWriterFactory<>::Create(&DetailsString);
		FJsonSerializer::Serialize(DetailsObject.ToSharedRef(), DetailsWriter);
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("asset_path"), AssetPath);
		ResultObject->SetStringField(TEXT("details"), DetailsString);
		Result.bSuccess = true;
	}
	if (!OutError.IsEmpty())
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		Result.bSuccess = false;
		Result.ErrorMessage = OutError;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_EditComponentProperty(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BlueprintPath, ComponentName, PropertyName, PropertyValue;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!Args->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: component_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!Args->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: property_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!Args->TryGetStringField(TEXT("value"), PropertyValue))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: value");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!TargetBlueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!TargetBlueprint->SimpleConstructionScript)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Blueprint '%s' has no SimpleConstructionScript."), *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	USCS_Node* TargetNode = TargetBlueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
	if (!TargetNode)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Component '%s' not found in Blueprint '%s'"), *ComponentName, *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UActorComponent* ComponentTemplate = TargetNode->ComponentTemplate;
	if (!ComponentTemplate)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Component template for '%s' is invalid."), *ComponentName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FProperty* Property = ComponentTemplate->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (!Property)
	{
		TArray<FString> Suggestions;
		for (TFieldIterator<FProperty> PropIt(ComponentTemplate->GetClass()); PropIt; ++PropIt)
		{
			FString PropName = PropIt->GetName();
			if (PropName.Contains(PropertyName, ESearchCase::IgnoreCase)) { Suggestions.Add(PropName); }
		}
		FString SuggestionStr = FString::Join(Suggestions, TEXT(", "));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Property '%s' not found on component '%s'"), *PropertyName, *ComponentName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		ResultObject->SetStringField(TEXT("suggestions"), SuggestionStr);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	bool bPropertySetSuccess = false;
	void* PropertyData = Property->ContainerPtrToValuePtr<void>(ComponentTemplate);
	if (PropertyData)
	{
		if (Property->ImportText_Direct(*PropertyValue, PropertyData, nullptr, PPF_None))
		{
			bPropertySetSuccess = true;
		}
	}
	if (!bPropertySetSuccess)
	{
		if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				FVector VectorValue;
				if (VectorValue.InitFromString(PropertyValue))
				{
					*static_cast<FVector*>(PropertyData) = VectorValue;
					bPropertySetSuccess = true;
				}
			}
			else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
			{
				FRotator RotatorValue;
				if (RotatorValue.InitFromString(PropertyValue))
				{
					*static_cast<FRotator*>(PropertyData) = RotatorValue;
					bPropertySetSuccess = true;
				}
			}
		}
	}
	if (!bPropertySetSuccess)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to set property '%s' to value '%s'. The value format may be incorrect for the property type."), *PropertyName, *PropertyValue);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Set property '%s' on component '%s' to '%s'"), *PropertyName, *ComponentName, *PropertyValue));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteComponent(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ComponentName;
	if (!Args->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: component_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!TargetBlueprint)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!TargetBlueprint->SimpleConstructionScript)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Blueprint '%s' has no SimpleConstructionScript."), *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	USCS_Node* TargetNode = TargetBlueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
	if (!TargetNode)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Component '%s' not found in Blueprint '%s'"), *ComponentName, *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (TargetNode->IsNative())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Cannot delete native/root component '%s'."), *ComponentName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Component")));
	TargetBlueprint->Modify();
	TargetBlueprint->SimpleConstructionScript->RemoveNode(TargetNode);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted component '%s' from Blueprint."), *ComponentName));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_SelectActors(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	const TArray<TSharedPtr<FJsonValue>>* ActorLabelsArray;
	if (!Args->TryGetArrayField(TEXT("actor_labels"), ActorLabelsArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: actor_labels (array)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FString> ActorLabels;
	for (const TSharedPtr<FJsonValue>& Value : *ActorLabelsArray)
	{
		ActorLabels.Add(Value->AsString());
	}
	FString OutError;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutError = TEXT("Failed to get a valid editor world.");
	}
	else
	{
		TArray<AActor*> ActorsToSelect;
		for (const FString& Label : ActorLabels)
		{
			bool bFoundActor = false;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (*It && (*It)->GetActorLabel() == Label)
				{
					ActorsToSelect.Add(*It);
					bFoundActor = true;
					break;
				}
			}
			if (!bFoundActor)
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Could not find an actor with label '%s' to select."), *Label);
			}
		}
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (EditorActorSubsystem)
		{
			EditorActorSubsystem->SetSelectedLevelActors(ActorsToSelect);
		}
	}
	if (OutError.IsEmpty())
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetNumberField(TEXT("selected_count"), ActorLabels.Num());
		ResultObject->SetStringField(TEXT("message"), ActorLabels.Num() > 0
			? FString::Printf(TEXT("Selected %d actor(s) in the level."), ActorLabels.Num())
			: TEXT("Cleared actor selection."));
		Result.bSuccess = true;
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		Result.bSuccess = false;
		Result.ErrorMessage = OutError;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateProjectFolder(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString FolderPath;
	if (!Args->TryGetStringField(TEXT("folder_path"), FolderPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: folder_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString OutError;
	bool bAlreadyExists = false;
	if (FolderPath.IsEmpty() || !FolderPath.StartsWith(TEXT("/Game/")))
	{
		OutError = TEXT("Invalid folder path. Path must start with /Game/.");
	}
	else if (UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
	{
		bAlreadyExists = true;
	}
	else if (!UEditorAssetLibrary::MakeDirectory(FolderPath))
	{
		OutError = FString::Printf(TEXT("Failed to create folder at '%s'."), *FolderPath);
	}
	else
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().ScanPathsSynchronous({ FolderPath }, true);
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);
	}
	if (OutError.IsEmpty())
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("folder_path"), FolderPath);
		if (bAlreadyExists)
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Folder already exists at '%s'"), *FolderPath));
		}
		else
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created folder at '%s'"), *FolderPath));
		}
		Result.bSuccess = true;
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		Result.bSuccess = false;
		Result.ErrorMessage = OutError;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_MoveAsset(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString AssetPath;
	if (!Args->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: asset_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString DestinationFolder;
	if (!Args->TryGetStringField(TEXT("destination_folder"), DestinationFolder))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: destination_folder");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationFolder))
	{
		UEditorAssetLibrary::MakeDirectory(DestinationFolder);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().ScanPathsSynchronous({ DestinationFolder }, true);
	}
	FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
	FString NewAssetPath = DestinationFolder / AssetName;
	bool bMoved = UEditorAssetLibrary::RenameAsset(AssetPath, NewAssetPath);
	if (bMoved)
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("old_path"), AssetPath);
		ResultObject->SetStringField(TEXT("new_path"), NewAssetPath);
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully moved asset to '%s'"), *NewAssetPath));
		Result.bSuccess = true;
	}
	else
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to move asset '%s' to '%s'"), *AssetPath, *DestinationFolder);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_MoveAssets(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	const TArray<TSharedPtr<FJsonValue>>* AssetPathsJson;
	if (!Args->TryGetArrayField(TEXT("asset_paths"), AssetPathsJson) || AssetPathsJson->Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing or empty required parameter: asset_paths (array of asset paths)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString DestinationFolder;
	if (!Args->TryGetStringField(TEXT("destination_folder"), DestinationFolder))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: destination_folder");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FString> AssetPaths;
	for (const TSharedPtr<FJsonValue>& PathValue : *AssetPathsJson)
	{
		AssetPaths.Add(PathValue->AsString());
	}
	if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationFolder))
	{
		UEditorAssetLibrary::MakeDirectory(DestinationFolder);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().ScanPathsSynchronous({ DestinationFolder }, true);
	}
	int32 SuccessCount = 0;
	int32 FailCount = 0;
	TArray<TSharedPtr<FJsonValue>> MovedAssetsJson;
	TArray<TSharedPtr<FJsonValue>> FailedAssetsJson;
	for (const FString& AssetPath : AssetPaths)
	{
		if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			FailCount++;
			TSharedPtr<FJsonObject> FailedEntry = MakeShareable(new FJsonObject);
			FailedEntry->SetStringField(TEXT("asset_path"), AssetPath);
			FailedEntry->SetStringField(TEXT("reason"), TEXT("Asset not found"));
			FailedAssetsJson.Add(MakeShareable(new FJsonValueObject(FailedEntry)));
			continue;
		}
		FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
		FString NewAssetPath = DestinationFolder / AssetName;
		bool bMoved = UEditorAssetLibrary::RenameAsset(AssetPath, NewAssetPath);
		if (bMoved)
		{
			SuccessCount++;
			TSharedPtr<FJsonObject> MovedEntry = MakeShareable(new FJsonObject);
			MovedEntry->SetStringField(TEXT("old_path"), AssetPath);
			MovedEntry->SetStringField(TEXT("new_path"), NewAssetPath);
			MovedAssetsJson.Add(MakeShareable(new FJsonValueObject(MovedEntry)));
		}
		else
		{
			FailCount++;
			TSharedPtr<FJsonObject> FailedEntry = MakeShareable(new FJsonObject);
			FailedEntry->SetStringField(TEXT("asset_path"), AssetPath);
			FailedEntry->SetStringField(TEXT("reason"), TEXT("RenameAsset failed"));
			FailedAssetsJson.Add(MakeShareable(new FJsonValueObject(FailedEntry)));
		}
	}
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);
	ResultObject->SetBoolField(TEXT("success"), FailCount == 0);
	ResultObject->SetNumberField(TEXT("total_assets"), AssetPaths.Num());
	ResultObject->SetNumberField(TEXT("moved_count"), SuccessCount);
	ResultObject->SetNumberField(TEXT("failed_count"), FailCount);
	ResultObject->SetArrayField(TEXT("moved_assets"), MovedAssetsJson);
	if (FailedAssetsJson.Num() > 0)
	{
		ResultObject->SetArrayField(TEXT("failed_assets"), FailedAssetsJson);
	}
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Moved %d assets to '%s' (%d failed)"), SuccessCount, *DestinationFolder, FailCount));
	Result.bSuccess = (SuccessCount > 0);
	if (FailCount > 0 && SuccessCount == 0)
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to move all %d assets"), FailCount);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_EditDataAssetDefaults(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString AssetPath;
	if (!Args->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: asset_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* EditsArray = nullptr;
	if (!Args->TryGetArrayField(TEXT("edits"), EditsArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: edits (array of edit objects)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Asset)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load asset at path: %s"), *AssetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UObject* TargetObject = nullptr;
	if (UBlueprint* BlueprintAsset = Cast<UBlueprint>(Asset))
	{
		if (BlueprintAsset->GeneratedClass)
		{
			TargetObject = BlueprintAsset->GeneratedClass->GetDefaultObject();
		}
	}
	else
	{
		TargetObject = Asset;
	}
	if (!TargetObject)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not resolve a valid object to modify for asset: %s"), *AssetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	int32 SuccessfulEdits = 0;
	int32 FailedEdits = 0;
	FString AllErrors;
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Edit Asset Defaults")));
	TargetObject->Modify();
	for (const TSharedPtr<FJsonValue>& EditValue : *EditsArray)
	{
		const TSharedPtr<FJsonObject>& EditObject = EditValue->AsObject();
		if (!EditObject.IsValid()) continue;
		FString PropertyName, PropValStr;
		if (!EditObject->TryGetStringField(TEXT("property_name"), PropertyName) ||
			!EditObject->TryGetStringField(TEXT("value"), PropValStr))
		{
			AllErrors += TEXT("Invalid edit operation format. ");
			FailedEdits++;
			continue;
		}
		FProperty* FinalProperty = nullptr;
		void* FinalContainerPtr = TargetObject;
		UStruct* CurrentStruct = TargetObject->GetClass();
		TArray<FString> PropertyPathParts;
		if (PropertyName.Contains(TEXT(".")))
		{
			PropertyName.ParseIntoArray(PropertyPathParts, TEXT("."));
			for (int32 i = 0; i < PropertyPathParts.Num(); ++i)
			{
				FProperty* CurrentProp = CurrentStruct->FindPropertyByName(FName(*PropertyPathParts[i]));
				if (!CurrentProp)
				{
					AllErrors += FString::Printf(TEXT("Property '%s' not found on '%s'. "), *PropertyPathParts[i], *CurrentStruct->GetName());
					FailedEdits++;
					break;
				}
				if (i == PropertyPathParts.Num() - 1)
				{
					FinalProperty = CurrentProp;
				}
				else
				{
					if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp))
					{
						CurrentStruct = StructProp->Struct;
						FinalContainerPtr = StructProp->ContainerPtrToValuePtr<void>(FinalContainerPtr);
						if (!FinalContainerPtr)
						{
							AllErrors += FString::Printf(TEXT("Container pointer became null while traversing path at '%s'. "), *PropertyPathParts[i]);
							FailedEdits++;
							break;
						}
					}
					else
					{
						AllErrors += FString::Printf(TEXT("Property '%s' is not a struct and cannot be traversed. "), *PropertyPathParts[i]);
						FailedEdits++;
						break;
					}
				}
			}
			if (!FinalProperty || FailedEdits > 0)
			{
				continue;
			}
		}
		else
		{
			FinalProperty = TargetObject->GetClass()->FindPropertyByName(FName(*PropertyName));
			FinalContainerPtr = TargetObject;
		}
		if (!FinalProperty)
		{
			AllErrors += FString::Printf(TEXT("Property '%s' not found. "), *PropertyName);
			FailedEdits++;
			continue;
		}
		void* PropertyData = FinalProperty->ContainerPtrToValuePtr<void>(FinalContainerPtr);
		FString FinalPropValStr = PropValStr;
		if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'")))
		{
			FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr);
		}
		if (FinalProperty->ImportText_Direct(*FinalPropValStr, PropertyData, nullptr, PPF_None) != nullptr)
		{
			SuccessfulEdits++;
		}
		else
		{
			AllErrors += FString::Printf(TEXT("Failed to set '%s'. "), *PropertyName);
			FailedEdits++;
		}
	}
	Asset->MarkPackageDirty();
	ResultObject->SetBoolField(TEXT("success"), FailedEdits == 0);
	ResultObject->SetNumberField(TEXT("successful_edits"), SuccessfulEdits);
	ResultObject->SetNumberField(TEXT("failed_edits"), FailedEdits);
	if (!AllErrors.IsEmpty())
	{
		ResultObject->SetStringField(TEXT("errors"), AllErrors);
	}
	Result.bSuccess = FailedEdits == 0;
	if (FailedEdits > 0)
	{
		Result.ErrorMessage = AllErrors;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
class FBpDetailedSummarizer
{
private:
	template<int32 BufferSize>
	void DescribeWidgetHierarchy(UWidget* Widget, TStringBuilder<BufferSize>& Report, int32 Indent)
	{
		if (!Widget) return;
		Report.Append(FString::ChrN(Indent * 2, ' '));
		Report.Appendf(TEXT("- %s (Class: %s)\n"), *Widget->GetFName().ToString(), *Widget->GetClass()->GetName());
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
			{
				DescribeWidgetHierarchy(Panel->GetChildAt(i), Report, Indent + 1);
			}
		}
	}
	FString PinTypeToString(const FEdGraphPinType& PinType)
	{
		if (PinType.PinSubCategoryObject.IsValid())
		{
			return PinType.PinSubCategoryObject->GetName();
		}
		return PinType.PinCategory.ToString();
	}
public:
	FString Summarize(UBlueprint* Blueprint)
	{
		if (!Blueprint) return TEXT("Invalid Blueprint provided.");
		TStringBuilder<8192> Report;
		TArray<FString> PerformanceIssues;
		bool bTickIsUsed = false;
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(Blueprint));
		Report.Appendf(TEXT("--- BLUEPRINT SUMMARY: %s ---\n\n"), *Blueprint->GetName());
		Report.Append(TEXT("--- ASSET ANALYSIS ---\n"));
		TArray<FAssetIdentifier> Referencers;
		AssetRegistryModule.Get().GetReferencers(AssetData.PackageName, Referencers);
		TArray<FAssetIdentifier> Dependencies;
		AssetRegistryModule.Get().GetDependencies(AssetData.PackageName, Dependencies);
		const FString FilePath = FPackageName::LongPackageNameToFilename(AssetData.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
		const int64 FileSize = IFileManager::Get().FileSize(*FilePath);
		Report.Appendf(TEXT("On-Disk Size: %.2f KB\n"), FileSize / 1024.0f);
		Report.Appendf(TEXT("Referenced By (Referencers): %d other assets\n"), Referencers.Num());
		Report.Appendf(TEXT("References (Dependencies): %d other assets\n"), Dependencies.Num());
		Report.Append(TEXT("Hard Asset References:\n"));
		int32 HardRefCount = 0;
		for (const FAssetIdentifier& Dep : Dependencies)
		{
			if (Dep.IsPackage() && Dep.PackageName != AssetData.PackageName && !Dep.PackageName.ToString().StartsWith(TEXT("/Script")))
			{
				Report.Appendf(TEXT("  - %s\n"), *Dep.PackageName.ToString());
				HardRefCount++;
			}
		}
		if (HardRefCount == 0) Report.Append(TEXT("  - None\n"));
		Report.Append(TEXT("\n"));
		Report.Appendf(TEXT("Parent Class: %s\n"), Blueprint->ParentClass ? *Blueprint->ParentClass->GetName() : TEXT("None (Invalid)"));
		if (Blueprint->ImplementedInterfaces.Num() > 0)
		{
			Report.Append(TEXT("Implemented Interfaces:\n"));
			for (const auto& Interface : Blueprint->ImplementedInterfaces)
			{
				if (Interface.Interface) Report.Appendf(TEXT("  - %s\n"), *Interface.Interface->GetName());
			}
		}
		Report.Append(TEXT("\n"));
		if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
		{
			if (WidgetBP->WidgetTree && WidgetBP->WidgetTree->RootWidget)
			{
				Report.Append(TEXT("--- WIDGET HIERARCHY ---\n"));
				DescribeWidgetHierarchy(WidgetBP->WidgetTree->RootWidget, Report, 0);
				Report.Append(TEXT("\n"));
			}
		}
		if (Blueprint->SimpleConstructionScript && Blueprint->SimpleConstructionScript->GetAllNodes().Num() > 0)
		{
			Report.Append(TEXT("--- COMPONENTS ---\n"));
			for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (Node && Node->ComponentClass) Report.Appendf(TEXT("- %s (Name: %s)\n"), *Node->ComponentClass->GetName(), *Node->GetVariableName().ToString());
			}
			Report.Append(TEXT("\n"));
		}
		if (Blueprint->NewVariables.Num() > 0)
		{
			Report.Append(TEXT("--- VARIABLES ---\n"));
			for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
			{
				FString TypeStr = PinTypeToString(VarDesc.VarType);
				FString MetaData;
				if ((VarDesc.PropertyFlags & CPF_Edit) != 0) MetaData += TEXT("(Instance Editable) ");
				if ((VarDesc.PropertyFlags & CPF_ExposeOnSpawn) != 0) MetaData += TEXT("(Expose on Spawn) ");
				Report.Appendf(TEXT("- %s %s %s\n"), *TypeStr, *VarDesc.VarName.ToString(), *MetaData);
			}
			Report.Append(TEXT("\n"));
		}
		TArray<UEdGraph*> GraphsToAnalyze;
		GraphsToAnalyze.Append(Blueprint->UbergraphPages);
		GraphsToAnalyze.Append(Blueprint->FunctionGraphs);
		FBpGraphDescriber GraphDescriber;
		for (UEdGraph* Graph : GraphsToAnalyze)
		{
			if (!Graph) continue;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
				{
					if (EventNode->EventReference.GetMemberName() == FName(TEXT("Tick")))
					{
						UEdGraphPin* ThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
						if (ThenPin && ThenPin->LinkedTo.Num() > 0)
						{
							bTickIsUsed = true;
							break;
						}
					}
				}
			}
			if (bTickIsUsed) break;
			Report.Appendf(TEXT("--- Analyzing Graph: %s ---\n"), *Graph->GetName());
			TSet<UObject*> AllNodes;
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node) AllNodes.Add(Node);
			}
			Report.Append(GraphDescriber.Describe(AllNodes));
			Report.Append(TEXT("\n"));
		}
		if (bTickIsUsed)
		{
			PerformanceIssues.AddUnique(FString::Printf(TEXT("Uses Event Tick with connected logic.")));
		}
		if (PerformanceIssues.Num() > 0)
		{
			Report.Append(TEXT("\n--- POTENTIAL PERFORMANCE ISSUES ---\n"));
			for (const FString& Issue : PerformanceIssues)
			{
				Report.Appendf(TEXT("! %s\n"), *Issue);
			}
		}
		return FString(Report);
	}
};
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetDetailedBlueprintSummary(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!TargetBP)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Summary = FBpSummarizerr::Summarize(TargetBP);
	if (Summary.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("The summarizer returned an empty string. The Blueprint may be empty or corrupted.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	else
	{
		Result.bSuccess = true;
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("summary"), Summary);
		ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetMaterialGraphSummary(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString MaterialPath;
	if (!Args->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: material_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(MaterialPath);
	if (!LoadedAsset)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to load any asset at path: %s."), *MaterialPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (LoadedAsset->IsA(UMaterialInstance::StaticClass()))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("The selected asset is a Material Instance, which does not have a node graph.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UMaterial* TargetMaterial = Cast<UMaterial>(LoadedAsset);
	if (!TargetMaterial)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("The asset at path '%s' is not a base Material."), *MaterialPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FMaterialGraphDescriber Describer;
	FString Summary = Describer.Describe(TargetMaterial);
	if (Summary.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("The summarizer returned an empty string or an error occurred.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	else
	{
		Result.bSuccess = true;
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("summary"), Summary);
		ResultObject->SetStringField(TEXT("material_path"), MaterialPath);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_InsertCode(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BpPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BpPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString AiCode;
	if (!Args->TryGetStringField(TEXT("code"), AiCode))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: code");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBP)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(TargetBP, true);
	if (!AssetEditor)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not open Blueprint editor. Please open the Blueprint in the editor first.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditor);
	const TSet<UObject*> SelectedNodes = BlueprintEditor->GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("You must have exactly one Blueprint node selected (currently %d selected)."), SelectedNodes.Num());
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedNodes.begin());
	if (!Node)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("The selected object is not a valid graph node.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBpGeneratorCompiler Compiler(TargetBP);
	FString DummyFunctionCode = FString::Printf(TEXT("void InsertedLogic() { %s }"), *AiCode);
	if (!Compiler.InsertCode(Node, DummyFunctionCode))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Compiler failed to insert code at the selected node.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("Code successfully inserted at the selected node."));
	ResultObject->SetStringField(TEXT("blueprint_path"), BpPath);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteNodes(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not get the Asset Editor Subsystem.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	IAssetEditorInstance* ActiveEditor = nullptr;
	double LastActivationTime = 0.0;
	TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();
	for (UObject* Asset : EditedAssets)
	{
		TArray<IAssetEditorInstance*> Editors = AssetEditorSubsystem->FindEditorsForAsset(Asset);
		for (IAssetEditorInstance* Editor : Editors)
		{
			const FName EditorName = Editor->GetEditorName();
			if (EditorName == FName(TEXT("BlueprintEditor")) ||
				EditorName == FName(TEXT("AnimationBlueprintEditor")) ||
				EditorName == FName(TEXT("WidgetBlueprintEditor")))
			{
				if (Editor && Editor->GetLastActivationTime() > LastActivationTime)
				{
					LastActivationTime = Editor->GetLastActivationTime();
					ActiveEditor = Editor;
				}
			}
		}
	}
	if (!ActiveEditor)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No active Blueprint editor found. Please open a Blueprint editor and select the nodes you want to delete.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
	TSet<UObject*> SelectedNodes = BlueprintEditor->GetSelectedNodes();
	if (SelectedNodes.Num() == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No nodes are currently selected. Please select the nodes you want to delete.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Nodes")));
	TArray<FName> DeletedNodeNames;
	int32 DeletedCount = 0;
	for (UObject* SelectedObj : SelectedNodes)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObj))
		{
			if (Node->GetClass()->GetName() == TEXT("EdGraphCommentNode"))
			{
				continue;
			}
			FName NodeName = Node->GetFName();
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(Node);
			if (Blueprint)
			{
				Blueprint->Modify();
				Node->Modify();
				Node->DestroyNode();
				DeletedNodeNames.Add(NodeName);
				DeletedCount++;
			}
		}
	}
	if (DeletedCount == 0)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No deletable nodes were found in the selection (comment nodes are skipped).");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BlueprintEditor->GetBlueprintObj());
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("deleted_count"), DeletedCount);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted %d nodes from the graph."), DeletedCount));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetProjectRootPath(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ProjectPath = FPaths::GetProjectFilePath();
	if (ProjectPath.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not determine the project root path via FPaths::GetProjectFilePath().");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	else
	{
		Result.bSuccess = true;
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("project_path"), ProjectPath);
		ResultObject->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddWidgetToUserWidget(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString WidgetPath;
	if (!Args->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString WidgetType;
	if (!Args->TryGetStringField(TEXT("widget_type"), WidgetType))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_type");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString WidgetName;
	if (!Args->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ParentName;
	Args->TryGetStringField(TEXT("parent_name"), ParentName);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[add_widget_to_user_widget] Loading widget at: %s"), *WidgetPath);
	UObject* LoadedObj = UEditorAssetLibrary::LoadAsset(WidgetPath);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[add_widget_to_user_widget] LoadAsset result: %s"), LoadedObj ? TEXT("SUCCESS") : TEXT("FAILED"));
	if (LoadedObj)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[add_widget_to_user_widget] Loaded object class: %s, Name: %s"), *LoadedObj->GetClass()->GetName(), *LoadedObj->GetName());
	}
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObj);
	if (!WidgetBP)
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("[add_widget_to_user_widget] Cast to UWidgetBlueprint FAILED! LoadedObj class: %s"), LoadedObj ? *LoadedObj->GetClass()->GetName() : TEXT("null"));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load User Widget Blueprint at path: %s"), *WidgetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!WidgetBP->WidgetTree)
	{
		WidgetBP->WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional);
	}
	UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetType);
	if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		FString PrefixedType = TEXT("U") + WidgetType;
		WidgetClass = FindFirstObjectSafe<UClass>(*PrefixedType);
	}
	if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not find a valid UWidget class named '%s'."), *WidgetType);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FName NewWidgetFName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
	UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetFName);
	if (!NewWidget)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to construct new widget in the WidgetTree.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (UTextBlock* TextBlock = Cast<UTextBlock>(NewWidget))
	{
		TextBlock->SetAutoWrapText(true);
	}
	FString CreatedWidgetName = NewWidgetFName.ToString();
	FString OutError;
	if (WidgetBP->WidgetTree->RootWidget == nullptr)
	{
		if (!ParentName.IsEmpty())
		{
			WidgetBP->WidgetTree->RemoveWidget(NewWidget);
			OutError = FString::Printf(TEXT("The WidgetTree is empty, so a parent named '%s' cannot exist. The first widget added becomes the root."), *ParentName);
			Result.bSuccess = false;
			Result.ErrorMessage = OutError;
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), OutError);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		WidgetBP->WidgetTree->RootWidget = NewWidget;
	}
	else
	{
		UWidget* ParentWidget = nullptr;
		if (!ParentName.IsEmpty())
		{
			ParentWidget = WidgetBP->WidgetTree->FindWidget(FName(*ParentName));
			if (!ParentWidget)
			{
				WidgetBP->WidgetTree->RemoveWidget(NewWidget);
				OutError = FString::Printf(TEXT("Could not find the specified parent widget named '%s'."), *ParentName);
				Result.bSuccess = false;
				Result.ErrorMessage = OutError;
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), OutError);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
		}
		else
		{
			ParentWidget = WidgetBP->WidgetTree->RootWidget;
		}
		UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
		if (!ParentPanel)
		{
			WidgetBP->WidgetTree->RemoveWidget(NewWidget);
			OutError = FString::Printf(TEXT("The target parent '%s' is not a Panel Widget (e.g., CanvasPanel, VerticalBox) and cannot have children."), *ParentWidget->GetName());
			Result.bSuccess = false;
			Result.ErrorMessage = OutError;
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), OutError);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ParentPanel))
		{
			UCanvasPanelSlot* CanvasSlot = CanvasPanel->AddChildToCanvas(NewWidget);
			if (CanvasSlot)
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f));
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetSize(FVector2D(100.0f, 30.0f));
			}
		}
		else
		{
			ParentPanel->AddChild(NewWidget);
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("widget_name"), CreatedWidgetName);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added widget '%s' to '%s'."), *CreatedWidgetName, *WidgetPath));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_EditWidgetProperty(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString WidgetPath;
	if (!Args->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString WidgetName;
	if (!Args->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString PropertyName;
	if (!Args->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: property_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Value;
	if (!Args->TryGetStringField(TEXT("value"), Value))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: value");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(WidgetPath));
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load a valid User Widget Blueprint at path: %s"), *WidgetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!TargetWidget)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not find a widget named '%s' in the hierarchy."), *WidgetName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TargetWidget->Modify();
	void* ContainerPtr = TargetWidget;
	UStruct* ContainerStruct = TargetWidget->GetClass();
	if (PropertyName.StartsWith(TEXT("Slot.")))
	{
		if (TargetWidget->Slot)
		{
			TargetWidget->Slot->Modify();
			ContainerPtr = TargetWidget->Slot;
			ContainerStruct = TargetWidget->Slot->GetClass();
			PropertyName.RightChopInline(5);
		}
		else
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' does not have a Slot to modify."), *WidgetName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
	}
	TArray<FString> PathParts;
	PropertyName.ParseIntoArray(PathParts, TEXT("."));
	FProperty* FinalProperty = nullptr;
	for (int32 i = 0; i < PathParts.Num(); ++i)
	{
		FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
		if (!CurrentProp)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Property '%s' not found on '%s'"), *PathParts[i], *ContainerStruct->GetName());
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		if (i == PathParts.Num() - 1)
		{
			FinalProperty = CurrentProp;
		}
		else
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp))
			{
				ContainerStruct = StructProp->Struct;
				ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr);
				if (!ContainerPtr)
				{
					Result.bSuccess = false;
					Result.ErrorMessage = FString::Printf(TEXT("Container pointer became null while traversing path at '%s'."), *PathParts[i]);
					ResultObject->SetBoolField(TEXT("success"), false);
					ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
					FString ResultString;
					TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
					FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
					Result.ResultJson = ResultString;
					return Result;
				}
			}
			else
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Property '%s' is not a struct and cannot be traversed."), *PathParts[i]);
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
		}
	}
	if (FinalProperty && ContainerPtr)
	{
		FString FinalPropValStr = Value;
		if (FinalProperty->IsA<FTextProperty>() && !Value.StartsWith(TEXT("\"")) && !Value.StartsWith(TEXT("'")))
		{
			FinalPropValStr = FString::Printf(TEXT("\"%s\""), *Value);
		}
		bool bColorHandled = false;
		if (FStructProperty* StructProp = CastField<FStructProperty>(FinalProperty))
		{
			UStruct* Struct = StructProp->Struct;
			if (Struct)
			{
				FString StructName = Struct->GetName();
				if (StructName.Contains(TEXT("SlateColor")))
				{
					FLinearColor ParsedColor = ParseColor(Value);
					FSlateColor SlateColor(ParsedColor);
					void* DestPtr = StructProp->ContainerPtrToValuePtr<void>(ContainerPtr);
					*reinterpret_cast<FSlateColor*>(DestPtr) = SlateColor;
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("set_widget_property: FSlateColor '%s' = '%s' -> (R=%.2f,G=%.2f,B=%.2f,A=%.2f)"),
						*PropertyName, *Value, ParsedColor.R, ParsedColor.G, ParsedColor.B, ParsedColor.A);
					bColorHandled = true;
				}
				else if (StructName.Contains(TEXT("LinearColor")))
				{
					FLinearColor ParsedColor = ParseColor(Value);
					void* DestPtr = StructProp->ContainerPtrToValuePtr<void>(ContainerPtr);
					*reinterpret_cast<FLinearColor*>(DestPtr) = ParsedColor;
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("set_widget_property: FLinearColor '%s' = '%s' -> (R=%.2f,G=%.2f,B=%.2f,A=%.2f)"),
						*PropertyName, *Value, ParsedColor.R, ParsedColor.G, ParsedColor.B, ParsedColor.A);
					bColorHandled = true;
				}
			}
		}
		if (!bColorHandled)
		{
			if (FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None) == nullptr)
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Failed to import value for '%s'."), *PropertyName);
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
		}
	}
	else
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to resolve final property for path '%s'."), *PropertyName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully set property '%s' on widget '%s'."), *PropertyName, *WidgetName));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteWidget(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString WidgetPath;
	if (!Args->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString WidgetName;
	if (!Args->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UObject* LoadedObj = UEditorAssetLibrary::LoadAsset(WidgetPath);
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObj);
	if (!WidgetBP)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Widget Blueprint at path: %s"), *WidgetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!WidgetBP->WidgetTree)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Widget Blueprint does not have a WidgetTree.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!TargetWidget)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' not found in WidgetTree."), *WidgetName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (TargetWidget == WidgetBP->WidgetTree->RootWidget)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Cannot delete the root widget. You can replace the root widget using create_widget_from_layout.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Widget")));
	WidgetBP->Modify();
	WidgetBP->WidgetTree->RemoveWidget(TargetWidget);
	TargetWidget->RemoveFromParent();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted widget '%s' from the Widget Blueprint."), *WidgetName));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
static void RecursivelyGetWidgetProperties(UStruct* InStruct, const FString& Prefix, TArray<TSharedPtr<FJsonValue>>& OutPropertiesArray, int32 Depth)
{
	if (!InStruct || Depth > 2)
	{
		return;
	}
	for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (Property && Property->HasAnyPropertyFlags(CPF_Edit))
		{
			FString PropName = Prefix + Property->GetName();
			if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
			{
				TSharedPtr<FJsonObject> StructPropObject = MakeShareable(new FJsonObject);
				StructPropObject->SetStringField("name", PropName);
				StructPropObject->SetStringField("type", StructProperty->Struct->GetName());
				OutPropertiesArray.Add(MakeShareable(new FJsonValueObject(StructPropObject)));
				RecursivelyGetWidgetProperties(StructProperty->Struct, PropName + TEXT("."), OutPropertiesArray, Depth + 1);
			}
			else
			{
				TSharedPtr<FJsonObject> PropObject = MakeShareable(new FJsonObject);
				PropObject->SetStringField("name", PropName);
				PropObject->SetStringField("type", Property->GetCPPType());
				OutPropertiesArray.Add(MakeShareable(new FJsonValueObject(PropObject)));
			}
		}
	}
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetBatchWidgetProperties(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	const TArray<TSharedPtr<FJsonValue>>* WidgetClassesArray = nullptr;
	if (!Args->TryGetArrayField(TEXT("widget_classes"), WidgetClassesArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_classes (array of class names)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TSharedPtr<FJsonObject> PropertiesResult = MakeShareable(new FJsonObject);
	for (const TSharedPtr<FJsonValue>& ClassValue : *WidgetClassesArray)
	{
		if (!ClassValue.IsValid()) continue;
		FString ClassName = ClassValue->AsString();
		if (ClassName.IsEmpty()) continue;
		UClass* FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
		if (!FoundClass) { FString PrefixedName = TEXT("U") + ClassName; FoundClass = FindFirstObjectSafe<UClass>(*PrefixedName); }
		if (!FoundClass) continue;
		TArray<TSharedPtr<FJsonValue>> PropertiesArray;
		RecursivelyGetWidgetProperties(FoundClass, TEXT(""), PropertiesArray, 0);
		PropertiesResult->SetArrayField(ClassName, PropertiesArray);
	}
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetObjectField(TEXT("properties"), PropertiesResult);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddWidgetsToLayout(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString WidgetPath;
	if (!Args->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* LayoutArray = nullptr;
	if (!Args->TryGetArrayField(TEXT("layout"), LayoutArray))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: layout (array of widget definitions)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(WidgetPath));
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Could not load a valid widget blueprint to add to.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Add Widgets to Layout")));
	WidgetBP->Modify();
	WidgetBP->WidgetTree->Modify();
	TArray<UWidget*> AllCurrentWidgets;
	WidgetBP->WidgetTree->GetAllWidgets(AllCurrentWidgets);
	TMap<FString, UWidget*> AllWidgetsMap;
	for (UWidget* Widget : AllCurrentWidgets)
	{
		if (Widget) AllWidgetsMap.Add(Widget->GetFName().ToString(), Widget);
	}
	TMap<FString, UWidget*> NewWidgetsMap;
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : *LayoutArray)
	{
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetTypeStr;
		if (!WidgetDef->TryGetStringField(TEXT("widget_type"), WidgetTypeStr))
		{
			if (!WidgetDef->TryGetStringField(TEXT("type"), WidgetTypeStr))
			{
				WidgetDef->TryGetStringField(TEXT("class"), WidgetTypeStr);
			}
		}
		FString WidgetName;
		if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName))
		{
			WidgetDef->TryGetStringField(TEXT("name"), WidgetName);
		}
		FString ParentName;
		if (!WidgetDef->TryGetStringField(TEXT("parent_name"), ParentName))
		{
			WidgetDef->TryGetStringField(TEXT("parent"), ParentName);
		}
		if (WidgetName.IsEmpty() || WidgetTypeStr.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("Widget definition missing 'name' or 'type'.");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		if (ParentName.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Widget '%s' is missing a 'parent_name'."), *WidgetName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetTypeStr);
		if (!WidgetClass) { FString PrefixedType = TEXT("U") + WidgetTypeStr; WidgetClass = FindFirstObjectSafe<UClass>(*PrefixedType); }
		if (!WidgetClass || !WidgetClass->IsChildOf<UWidget>())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Invalid widget type '%s'."), *WidgetTypeStr);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		FName NewWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
		UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetName);
		if (!NewWidget)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Failed to construct widget '%s'."), *WidgetName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(NewWidget))
		{
			TextBlock->SetAutoWrapText(true);
		}
		AllWidgetsMap.Add(NewWidgetName.ToString(), NewWidget);
		NewWidgetsMap.Add(WidgetName, NewWidget);
		UWidget** FoundParentWidgetPtr = AllWidgetsMap.Find(ParentName);
		if (!FoundParentWidgetPtr || !*FoundParentWidgetPtr)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Could not find parent '%s'."), *ParentName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		UWidget* ParentWidget = *FoundParentWidgetPtr;
		if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ParentWidget))
		{
			UCanvasPanelSlot* CanvasSlot = CanvasPanel->AddChildToCanvas(NewWidget);
			if (CanvasSlot)
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f));
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetSize(FVector2D(100.0f, 30.0f));
			}
		}
		else if (UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget))
		{
			ParentPanel->AddChild(NewWidget);
		}
		else if (UContentWidget* ParentContent = Cast<UContentWidget>(ParentWidget))
		{
			ParentContent->SetContent(NewWidget);
		}
		else
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Parent '%s' is not a container."), *ParentName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
	}
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : *LayoutArray)
	{
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetName;
		if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName))
		{
			WidgetDef->TryGetStringField(TEXT("name"), WidgetName);
		}
		UWidget* TargetWidget = *NewWidgetsMap.Find(WidgetName);
		if (!TargetWidget) continue;
		TArray<TPair<FString, FString>> ParsedProperties;
		const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr;
		const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
		if (WidgetDef->TryGetArrayField(TEXT("properties"), PropertiesArray))
		{
			for (const TSharedPtr<FJsonValue>& PropValue : *PropertiesArray)
			{
				const TSharedPtr<FJsonObject>& PropObject = PropValue->AsObject();
				if (PropObject.IsValid())
				{
					FString Key, Value;
					if (!PropObject->TryGetStringField(TEXT("key"), Key))
					{
						PropObject->TryGetStringField(TEXT("name"), Key);
					}
					PropObject->TryGetStringField(TEXT("value"), Value);
					if (!Key.IsEmpty())
					{
						ParsedProperties.Add(TPair<FString, FString>(Key, Value));
					}
				}
			}
		}
		else if (WidgetDef->TryGetObjectField(TEXT("properties"), PropertiesObject))
		{
			for (const auto& Pair : (*PropertiesObject)->Values)
			{
				ParsedProperties.Add(TPair<FString, FString>(Pair.Key, Pair.Value->AsString()));
			}
		}
		for (const auto& PropPair : ParsedProperties)
		{
			FString PropertyPath = PropPair.Key;
			FString PropValStr = PropPair.Value;
			void* ContainerPtr = TargetWidget;
			UStruct* ContainerStruct = TargetWidget->GetClass();
			if (PropertyPath.StartsWith(TEXT("Slot.")))
			{
				if (TargetWidget->Slot)
				{
					ContainerPtr = TargetWidget->Slot;
					ContainerStruct = TargetWidget->Slot->GetClass();
					PropertyPath.RightChopInline(5);
				}
				else
				{
					continue;
				}
			}
			TArray<FString> PathParts;
			PropertyPath.ParseIntoArray(PathParts, TEXT("."));
			FProperty* FinalProperty = nullptr;
			for (int32 i = 0; i < PathParts.Num(); ++i)
			{
				FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
				if (!CurrentProp)
				{
					break;
				}
				if (i == PathParts.Num() - 1)
				{
					FinalProperty = CurrentProp;
				}
				else
				{
					if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp))
					{
						ContainerStruct = StructProp->Struct;
						ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr);
						if (!ContainerPtr) break;
					}
					else
					{
						break;
					}
				}
			}
			if (FinalProperty && ContainerPtr)
			{
				FString FinalPropValStr = PropValStr;
				if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'")))
				{
					FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr);
				}
				FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None);
			}
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("widgets_added"), NewWidgetsMap.Num());
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully added %d widgets to '%s'."), NewWidgetsMap.Num(), *WidgetPath));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
static FAnchors ParseAnchorPreset(const FString& AnchorStr)
{
	FString LowerValue = AnchorStr.ToLower().TrimStartAndEnd();
	if (LowerValue == TEXT("fill") || LowerValue == TEXT("fill_screen"))
		return FAnchors(0.0f, 0.0f, 1.0f, 1.0f);
	if (LowerValue == TEXT("center"))
		return FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
	if (LowerValue == TEXT("top_left") || LowerValue == TEXT("topleft"))
		return FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
	if (LowerValue == TEXT("top_center") || LowerValue == TEXT("topcenter"))
		return FAnchors(0.5f, 0.0f, 0.5f, 0.0f);
	if (LowerValue == TEXT("top_right") || LowerValue == TEXT("topright"))
		return FAnchors(1.0f, 0.0f, 1.0f, 0.0f);
	if (LowerValue == TEXT("bottom_left") || LowerValue == TEXT("bottomleft"))
		return FAnchors(0.0f, 1.0f, 0.0f, 1.0f);
	if (LowerValue == TEXT("bottom_center") || LowerValue == TEXT("bottomcenter"))
		return FAnchors(0.5f, 1.0f, 0.5f, 1.0f);
	if (LowerValue == TEXT("bottom_right") || LowerValue == TEXT("bottomright"))
		return FAnchors(1.0f, 1.0f, 1.0f, 1.0f);
	if (LowerValue == TEXT("center_left") || LowerValue == TEXT("centerleft"))
		return FAnchors(0.0f, 0.5f, 0.0f, 0.5f);
	if (LowerValue == TEXT("center_right") || LowerValue == TEXT("centerright"))
		return FAnchors(1.0f, 0.5f, 1.0f, 0.5f);
	if (LowerValue == TEXT("horizontal_fill") || LowerValue == TEXT("hfill"))
		return FAnchors(0.0f, 0.0f, 1.0f, 0.0f);
	if (LowerValue == TEXT("vertical_fill") || LowerValue == TEXT("vfill"))
		return FAnchors(0.0f, 0.0f, 0.0f, 1.0f);
	return FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
}
static FVector2D ParseVector2D(const FString& VecStr)
{
	FString TrimmedValue = VecStr.TrimStartAndEnd();
	if (TrimmedValue.StartsWith(TEXT("(")) && TrimmedValue.EndsWith(TEXT(")")))
	{
		TrimmedValue = TrimmedValue.Mid(1, TrimmedValue.Len() - 2);
	}
	TArray<FString> Components;
	TrimmedValue.ParseIntoArray(Components, TEXT(","), true);
	if (Components.Num() >= 2)
	{
		return FVector2D(
			FCString::Atof(*Components[0].TrimStartAndEnd()),
			FCString::Atof(*Components[1].TrimStartAndEnd())
		);
	}
	else if (Components.Num() == 1)
	{
		float Val = FCString::Atof(*Components[0].TrimStartAndEnd());
		return FVector2D(Val, Val);
	}
	return FVector2D(0.0f, 0.0f);
}
static FLinearColor ParseColor(const FString& ColorStr)
{
	FString LowerValue = ColorStr.ToLower().TrimStartAndEnd();
	if (LowerValue.IsEmpty())
	{
		return FLinearColor::White;
	}
	if (LowerValue.StartsWith(TEXT("#")))
	{
		FString HexPart = LowerValue.RightChop(1);
		if (HexPart.Len() == 6)
		{
			int32 R = FParse::HexDigit(HexPart[0]) * 16 + FParse::HexDigit(HexPart[1]);
			int32 G = FParse::HexDigit(HexPart[2]) * 16 + FParse::HexDigit(HexPart[3]);
			int32 B = FParse::HexDigit(HexPart[4]) * 16 + FParse::HexDigit(HexPart[5]);
			return FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);
		}
		else if (HexPart.Len() == 8)
		{
			int32 R = FParse::HexDigit(HexPart[0]) * 16 + FParse::HexDigit(HexPart[1]);
			int32 G = FParse::HexDigit(HexPart[2]) * 16 + FParse::HexDigit(HexPart[3]);
			int32 B = FParse::HexDigit(HexPart[4]) * 16 + FParse::HexDigit(HexPart[5]);
			int32 A = FParse::HexDigit(HexPart[6]) * 16 + FParse::HexDigit(HexPart[7]);
			return FLinearColor(R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f);
		}
	}
	auto ParseUEFormat = [](const FString& Str) -> FLinearColor {
		FLinearColor Result = FLinearColor::White;
		TArray<FString> Parts;
		Str.ParseIntoArray(Parts, TEXT(","), true);
		for (const FString& Part : Parts)
		{
			FString TrimmedPart = Part.TrimStartAndEnd();
			if (TrimmedPart.StartsWith(TEXT("r=")))
				Result.R = FCString::Atof(*TrimmedPart.RightChop(2));
			else if (TrimmedPart.StartsWith(TEXT("g=")))
				Result.G = FCString::Atof(*TrimmedPart.RightChop(2));
			else if (TrimmedPart.StartsWith(TEXT("b=")))
				Result.B = FCString::Atof(*TrimmedPart.RightChop(2));
			else if (TrimmedPart.StartsWith(TEXT("a=")))
				Result.A = FCString::Atof(*TrimmedPart.RightChop(2));
		}
		return Result;
	};
	if (LowerValue.StartsWith(TEXT("(")) && LowerValue.EndsWith(TEXT(")")))
	{
		FString TupleContent = LowerValue.Mid(1, LowerValue.Len() - 2);
		if (TupleContent.Contains(TEXT("r=")) || TupleContent.Contains(TEXT("g=")) || TupleContent.Contains(TEXT("b=")))
		{
			return ParseUEFormat(TupleContent);
		}
		TArray<FString> Components;
		TupleContent.ParseIntoArray(Components, TEXT(","), true);
		if (Components.Num() >= 3)
		{
			float R = FCString::Atof(*Components[0].TrimStartAndEnd());
			float G = FCString::Atof(*Components[1].TrimStartAndEnd());
			float B = FCString::Atof(*Components[2].TrimStartAndEnd());
			float A = (Components.Num() >= 4) ? FCString::Atof(*Components[3].TrimStartAndEnd()) : 1.0f;
			return FLinearColor(R, G, B, A);
		}
	}
	if (LowerValue.Contains(TEXT("r=")) || LowerValue.Contains(TEXT("g=")) || LowerValue.Contains(TEXT("b=")))
	{
		return ParseUEFormat(LowerValue);
	}
	if (LowerValue == TEXT("red")) return FLinearColor::Red;
	if (LowerValue == TEXT("green")) return FLinearColor::Green;
	if (LowerValue == TEXT("blue")) return FLinearColor::Blue;
	if (LowerValue == TEXT("yellow")) return FLinearColor::Yellow;
	if (LowerValue == TEXT("cyan") || LowerValue == TEXT("aqua")) return FLinearColor(0.0f, 1.0f, 1.0f);
	if (LowerValue == TEXT("magenta") || LowerValue == TEXT("fuchsia")) return FLinearColor(1.0f, 0.0f, 1.0f);
	if (LowerValue == TEXT("white")) return FLinearColor::White;
	if (LowerValue == TEXT("black")) return FLinearColor::Black;
	if (LowerValue == TEXT("gray") || LowerValue == TEXT("grey")) return FLinearColor::Gray;
	if (LowerValue == TEXT("orange")) return FLinearColor(1.0f, 0.5f, 0.0f);
	if (LowerValue == TEXT("purple")) return FLinearColor(0.5f, 0.0f, 0.5f);
	if (LowerValue == TEXT("pink")) return FLinearColor(1.0f, 0.75f, 0.8f);
	if (LowerValue == TEXT("brown")) return FLinearColor(0.6f, 0.3f, 0.1f);
	if (LowerValue == TEXT("transparent") || LowerValue == TEXT("clear")) return FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	if (LowerValue == TEXT("gold")) return FLinearColor(1.0f, 0.84f, 0.0f);
	if (LowerValue == TEXT("silver")) return FLinearColor(0.75f, 0.75f, 0.75f);
	if (LowerValue == TEXT("navy")) return FLinearColor(0.0f, 0.0f, 0.5f);
	if (LowerValue == TEXT("teal")) return FLinearColor(0.0f, 0.5f, 0.5f);
	if (LowerValue == TEXT("olive")) return FLinearColor(0.5f, 0.5f, 0.0f);
	if (LowerValue == TEXT("maroon")) return FLinearColor(0.5f, 0.0f, 0.0f);
	if (LowerValue == TEXT("lime")) return FLinearColor(0.0f, 1.0f, 0.0f);
	if (LowerValue == TEXT("aqua")) return FLinearColor(0.0f, 1.0f, 1.0f);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("ParseColor: Unrecognized color '%s', defaulting to white"), *ColorStr);
	return FLinearColor::White;
}
static TSharedPtr<FJsonObject> NormalizeWidgetDef(const TSharedPtr<FJsonObject>& WidgetDef)
{
	if (!WidgetDef.IsValid()) return nullptr;
	TSet<FString> KnownFields = {
		TEXT("type"), TEXT("widget_type"), TEXT("class"),
		TEXT("name"), TEXT("widget_name"),
		TEXT("parent"), TEXT("parent_name"),
		TEXT("children"), TEXT("properties"),
		TEXT("slot"), TEXT("widget_path")
	};
	for (const auto& Pair : WidgetDef->Values)
	{
		if (KnownFields.Contains(Pair.Key))
		{
			continue;
		}
		const TSharedPtr<FJsonObject>* NestedObj = nullptr;
		if (Pair.Value->TryGetObject(NestedObj))
		{
			if ((*NestedObj)->HasField(TEXT("type")) ||
				(*NestedObj)->HasField(TEXT("widget_type")) ||
				(*NestedObj)->HasField(TEXT("children")))
			{
				TSharedPtr<FJsonObject> Normalized = MakeShareable(new FJsonObject);
				for (const auto& NestedPair : (*NestedObj)->Values)
				{
					Normalized->SetField(NestedPair.Key, NestedPair.Value);
				}
				if (!Normalized->HasField(TEXT("name")))
				{
					Normalized->SetStringField(TEXT("name"), Pair.Key);
				}
				return Normalized;
			}
		}
	}
	return WidgetDef;
}
void SBpGeneratorUltimateWidget::FlattenWidgetLayout(const TSharedPtr<FJsonObject>& WidgetDef, const FString& ParentName, TArray<TSharedPtr<FJsonObject>>& OutFlatList)
{
	if (!WidgetDef.IsValid()) return;
	TSharedPtr<FJsonObject> NormalizedDef = NormalizeWidgetDef(WidgetDef);
	TSharedPtr<FJsonObject> FlatWidget = MakeShareable(new FJsonObject);
	for (const auto& Pair : NormalizedDef->Values)
	{
		if (Pair.Key != TEXT("children"))
		{
			FlatWidget->SetField(Pair.Key, Pair.Value);
		}
	}
	if (!ParentName.IsEmpty())
	{
		FlatWidget->SetStringField(TEXT("parent_name"), ParentName);
	}
	OutFlatList.Add(FlatWidget);
	FString ThisWidgetName;
	if (!NormalizedDef->TryGetStringField(TEXT("widget_name"), ThisWidgetName))
	{
		NormalizedDef->TryGetStringField(TEXT("name"), ThisWidgetName);
	}
	const TArray<TSharedPtr<FJsonValue>>* ChildrenArray = nullptr;
	const TSharedPtr<FJsonObject>* ChildrenObject = nullptr;
	if (NormalizedDef->TryGetArrayField(TEXT("children"), ChildrenArray))
	{
		for (const TSharedPtr<FJsonValue>& ChildValue : *ChildrenArray)
		{
			const TSharedPtr<FJsonObject>& ChildDef = ChildValue->AsObject();
			if (ChildDef.IsValid())
			{
				FlattenWidgetLayout(ChildDef, ThisWidgetName, OutFlatList);
			}
		}
	}
	else if (NormalizedDef->TryGetObjectField(TEXT("children"), ChildrenObject))
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[FlattenWidgetLayout] Children is an object, converting to array"));
		for (const auto& ChildPair : (*ChildrenObject)->Values)
		{
			const TSharedPtr<FJsonObject>* ChildDef = nullptr;
			if (ChildPair.Value->TryGetObject(ChildDef))
			{
				TSharedPtr<FJsonObject> NormalizedChild = MakeShareable(new FJsonObject);
				for (const auto& Pair : (*ChildDef)->Values)
				{
					NormalizedChild->SetField(Pair.Key, Pair.Value);
				}
				if (!NormalizedChild->HasField(TEXT("name")))
				{
					NormalizedChild->SetStringField(TEXT("name"), ChildPair.Key);
				}
				FlattenWidgetLayout(NormalizedChild, ThisWidgetName, OutFlatList);
			}
		}
	}
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateWidgetFromLayout(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString WidgetPath;
	if (!Args->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: widget_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* LayoutArray = nullptr;
	const TSharedPtr<FJsonObject>* LayoutObject = nullptr;
	TArray<TSharedPtr<FJsonValue>> LocalLayoutArray;
	if (Args->TryGetArrayField(TEXT("layout"), LayoutArray))
	{
	}
	else if (Args->TryGetObjectField(TEXT("layout"), LayoutObject))
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Layout is an object, wrapping in array"));
		LocalLayoutArray.Add(MakeShareable(new FJsonValueObject(*LayoutObject)));
		LayoutArray = &LocalLayoutArray;
	}
	else
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: layout (array or object of widget definitions)");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<TSharedPtr<FJsonObject>> FlatWidgetList;
	bool bHasNestedChildren = false;
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : *LayoutArray)
	{
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (WidgetDef.IsValid() && WidgetDef->HasField(TEXT("children")))
		{
			bHasNestedChildren = true;
			break;
		}
	}
	if (bHasNestedChildren)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Detected nested children arrays, flattening layout..."));
		for (const TSharedPtr<FJsonValue>& WidgetDefValue : *LayoutArray)
		{
			const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
			if (WidgetDef.IsValid())
			{
				FlattenWidgetLayout(WidgetDef, FString(), FlatWidgetList);
			}
		}
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Flattened to %d widgets"), FlatWidgetList.Num());
	}
	else
	{
		for (const TSharedPtr<FJsonValue>& WidgetDefValue : *LayoutArray)
		{
			const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
			if (WidgetDef.IsValid())
			{
				FlatWidgetList.Add(WidgetDef);
			}
		}
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Attempting to load widget at path: %s"), *WidgetPath);
	UObject* LoadedObj = UEditorAssetLibrary::LoadAsset(WidgetPath);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] LoadAsset result: %s"), LoadedObj ? TEXT("SUCCESS") : TEXT("FAILED"));
	if (!LoadedObj)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Widget not found, attempting auto-creation"));
		FString WidgetName;
		int32 LastSlashIndex;
		if (WidgetPath.FindLastChar('/', LastSlashIndex))
		{
			WidgetName = WidgetPath.RightChop(LastSlashIndex + 1);
		}
		else
		{
			WidgetName = WidgetPath;
		}
		int32 DotIndex;
		if (WidgetName.FindChar('.', DotIndex))
		{
			WidgetName = WidgetName.Left(DotIndex);
		}
		FString SavePath = WidgetPath;
		if (LastSlashIndex > 0)
		{
			SavePath = WidgetPath.Left(LastSlashIndex);
		}
		else
		{
			SavePath = TEXT("/Game");
		}
		UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
			UUserWidget::StaticClass(),
			CreatePackage(*SavePath),
			FName(*WidgetName),
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass()
		);
		if (NewBP)
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("[create_widget_from_layout] Auto-created widget: %s"), *WidgetName);
			FAssetRegistryModule::AssetCreated(NewBP);
			LoadedObj = NewBP;
		}
		else
		{
			UE_LOG(LogBpGeneratorUltimate, Error, TEXT("[create_widget_from_layout] Failed to auto-create widget"));
		}
	}
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObj);
	if (!WidgetBP)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load or create UWidgetBlueprint at path: %s"), *WidgetPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Create Widget Layout")));
	WidgetBP->Modify();
	if (!WidgetBP->WidgetTree)
	{
		WidgetBP->WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional);
	}
	else if (WidgetBP->WidgetTree->RootWidget)
	{
		WidgetBP->WidgetTree->RemoveWidget(WidgetBP->WidgetTree->RootWidget);
	}
	WidgetBP->WidgetTree->Modify();
	TMap<FString, UWidget*> CreatedWidgetsMap;
	for (const TSharedPtr<FJsonObject>& WidgetDef : FlatWidgetList)
	{
		if (!WidgetDef.IsValid()) continue;
		FString WidgetTypeStr;
		if (!WidgetDef->TryGetStringField(TEXT("widget_type"), WidgetTypeStr))
		{
			if (!WidgetDef->TryGetStringField(TEXT("type"), WidgetTypeStr))
			{
				WidgetDef->TryGetStringField(TEXT("class"), WidgetTypeStr);
			}
		}
		FString WidgetName;
		if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName))
		{
			WidgetDef->TryGetStringField(TEXT("name"), WidgetName);
		}
		FString ParentName;
		if (!WidgetDef->TryGetStringField(TEXT("parent_name"), ParentName))
		{
			WidgetDef->TryGetStringField(TEXT("parent"), ParentName);
		}
		if (WidgetName.IsEmpty() || WidgetTypeStr.IsEmpty())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("Widget definition missing 'name' or 'type'.");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetTypeStr);
		if (!WidgetClass) { FString PrefixedType = TEXT("U") + WidgetTypeStr; WidgetClass = FindFirstObjectSafe<UClass>(*PrefixedType); }
		if (!WidgetClass || !WidgetClass->IsChildOf<UWidget>())
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Invalid widget type '%s'"), *WidgetTypeStr);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		FName NewWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
		UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetName);
		if (!NewWidget)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Failed to construct widget '%s'."), *WidgetName);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(NewWidget))
		{
			TextBlock->SetAutoWrapText(true);
		}
		CreatedWidgetsMap.Add(WidgetName, NewWidget);
		if (ParentName.IsEmpty())
		{
			WidgetBP->WidgetTree->RootWidget = NewWidget;
		}
		else
		{
			UWidget** FoundParentWidgetPtr = CreatedWidgetsMap.Find(ParentName);
			if (!FoundParentWidgetPtr || !*FoundParentWidgetPtr)
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Could not find parent '%s'."), *ParentName);
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
			UWidget* ParentWidget = *FoundParentWidgetPtr;
			if (UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ParentWidget))
			{
				UCanvasPanelSlot* CanvasSlot = CanvasPanel->AddChildToCanvas(NewWidget);
				if (CanvasSlot)
				{
					CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
					CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
					CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
					CanvasSlot->SetSize(FVector2D(100.0f, 100.0f));
				}
			}
			else if (UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget))
			{
				ParentPanel->AddChild(NewWidget);
			}
			else if (UContentWidget* ParentContent = Cast<UContentWidget>(ParentWidget))
			{
				ParentContent->SetContent(NewWidget);
			}
			else
			{
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Parent '%s' is not a container."), *ParentName);
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
		}
	}
	for (const TSharedPtr<FJsonObject>& WidgetDef : FlatWidgetList)
	{
		if (!WidgetDef.IsValid()) continue;
		FString WidgetName;
		if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName))
		{
			WidgetDef->TryGetStringField(TEXT("name"), WidgetName);
		}
		UWidget* TargetWidget = *CreatedWidgetsMap.Find(WidgetName);
		if (!TargetWidget) continue;
		TArray<TPair<FString, FString>> ParsedProperties;
		const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr;
		const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
		if (WidgetDef->TryGetArrayField(TEXT("properties"), PropertiesArray))
		{
			for (const TSharedPtr<FJsonValue>& PropValue : *PropertiesArray)
			{
				const TSharedPtr<FJsonObject>& PropObject = PropValue->AsObject();
				if (PropObject.IsValid())
				{
					FString Key, Value;
					if (!PropObject->TryGetStringField(TEXT("key"), Key))
					{
						PropObject->TryGetStringField(TEXT("name"), Key);
					}
					if (!PropObject->TryGetStringField(TEXT("value"), Value))
					{
						const TSharedPtr<FJsonObject>* ValueObj = nullptr;
						if (PropObject->TryGetObjectField(TEXT("value"), ValueObj))
						{
							if (Key.Equals(TEXT("AnchorData"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("LayoutData"), ESearchCase::IgnoreCase))
							{
								const TSharedPtr<FJsonObject>* AnchorsObj = nullptr;
								if ((*ValueObj)->TryGetObjectField(TEXT("Anchors"), AnchorsObj))
								{
									FString MinX, MinY, MaxX, MaxY;
									const TSharedPtr<FJsonObject>* MinObj = nullptr;
									const TSharedPtr<FJsonObject>* MaxObj = nullptr;
									if ((*AnchorsObj)->TryGetObjectField(TEXT("Minimum"), MinObj))
									{
										(*MinObj)->TryGetStringField(TEXT("X"), MinX);
										(*MinObj)->TryGetStringField(TEXT("Y"), MinY);
									}
									if ((*AnchorsObj)->TryGetObjectField(TEXT("Maximum"), MaxObj))
									{
										(*MaxObj)->TryGetStringField(TEXT("X"), MaxX);
										(*MaxObj)->TryGetStringField(TEXT("Y"), MaxY);
									}
									if (!MinX.IsEmpty() && !MinY.IsEmpty() && !MaxX.IsEmpty() && !MaxY.IsEmpty())
									{
										FString AnchorStr = FString::Printf(TEXT("(%s,%s,%s,%s)"), *MinX, *MinY, *MaxX, *MaxY);
										ParsedProperties.Add(TPair<FString, FString>(TEXT("Slot.Anchors"), AnchorStr));
									}
								}
								const TSharedPtr<FJsonObject>* OffsetsObj = nullptr;
								if ((*ValueObj)->TryGetObjectField(TEXT("Offsets"), OffsetsObj))
								{
									FString Left, Top, Right, Bottom;
									(*OffsetsObj)->TryGetStringField(TEXT("Left"), Left);
									(*OffsetsObj)->TryGetStringField(TEXT("Top"), Top);
									(*OffsetsObj)->TryGetStringField(TEXT("Right"), Right);
									(*OffsetsObj)->TryGetStringField(TEXT("Bottom"), Bottom);
									if (!Left.IsEmpty() && !Top.IsEmpty())
									{
										FString PosStr = FString::Printf(TEXT("(%s,%s)"), *Left, *Top);
										ParsedProperties.Add(TPair<FString, FString>(TEXT("Slot.Position"), PosStr));
									}
									float LeftVal = FCString::Atof(*Left);
									float TopVal = FCString::Atof(*Top);
									float RightVal = FCString::Atof(*Right);
									float BottomVal = FCString::Atof(*Bottom);
									float Width = RightVal - LeftVal;
									float Height = BottomVal - TopVal;
									if (Width > 0 && Height > 0)
									{
										FString SizeStr = FString::Printf(TEXT("(%f,%f)"), Width, Height);
										ParsedProperties.Add(TPair<FString, FString>(TEXT("Slot.Size"), SizeStr));
									}
								}
								continue;
							}
							double RVal = 0.0, GVal = 0.0, BVal = 0.0, AVal = 1.0;
							bool bHasR = (*ValueObj)->TryGetNumberField(TEXT("R"), RVal);
							bool bHasG = (*ValueObj)->TryGetNumberField(TEXT("G"), GVal);
							bool bHasB = (*ValueObj)->TryGetNumberField(TEXT("B"), BVal);
							(*ValueObj)->TryGetNumberField(TEXT("A"), AVal);
							if (bHasR || bHasG || bHasB)
							{
								Value = FString::Printf(TEXT("(R=%.6f,G=%.6f,B=%.6f,A=%.6f)"), RVal, GVal, BVal, AVal);
							}
							else
							{
								TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Value);
								FJsonSerializer::Serialize((*ValueObj).ToSharedRef(), Writer);
							}
						}
					}
					if (!Key.IsEmpty())
					{
						ParsedProperties.Add(TPair<FString, FString>(Key, Value));
					}
				}
			}
		}
		else if (WidgetDef->TryGetObjectField(TEXT("properties"), PropertiesObject))
		{
			for (const auto& Pair : (*PropertiesObject)->Values)
			{
				ParsedProperties.Add(TPair<FString, FString>(Pair.Key, Pair.Value->AsString()));
			}
		}
		for (const auto& PropPair : ParsedProperties)
		{
			FString PropertyPath = PropPair.Key;
			FString PropValStr = PropPair.Value;
			void* ContainerPtr = TargetWidget;
			UStruct* ContainerStruct = TargetWidget->GetClass();
			if (PropertyPath.StartsWith(TEXT("Slot.")))
			{
				if (TargetWidget->Slot)
				{
					ContainerPtr = TargetWidget->Slot;
					ContainerStruct = TargetWidget->Slot->GetClass();
					PropertyPath.RightChopInline(5, EAllowShrinking::No);
					if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TargetWidget->Slot))
					{
						if (PropertyPath.Equals(TEXT("Anchors"), ESearchCase::IgnoreCase))
						{
							CanvasSlot->SetAnchors(ParseAnchorPreset(PropValStr));
							continue;
						}
						else if (PropertyPath.Equals(TEXT("Position"), ESearchCase::IgnoreCase))
						{
							CanvasSlot->SetPosition(ParseVector2D(PropValStr));
							continue;
						}
						else if (PropertyPath.Equals(TEXT("Size"), ESearchCase::IgnoreCase))
						{
							CanvasSlot->SetSize(ParseVector2D(PropValStr));
							continue;
						}
						else if (PropertyPath.Equals(TEXT("Alignment"), ESearchCase::IgnoreCase))
						{
							CanvasSlot->SetAlignment(ParseVector2D(PropValStr));
							continue;
						}
					}
				}
				else
				{
					continue;
				}
			}
			TArray<FString> PathParts;
			PropertyPath.ParseIntoArray(PathParts, TEXT("."));
			FProperty* FinalProperty = nullptr;
			for (int32 i = 0; i < PathParts.Num(); ++i)
			{
				FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
				if (!CurrentProp)
				{
					break;
				}
				if (i == PathParts.Num() - 1)
				{
					FinalProperty = CurrentProp;
				}
				else
				{
					if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp))
					{
						ContainerStruct = StructProp->Struct;
						ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr);
						if (!ContainerPtr) break;
					}
					else
					{
						break;
					}
				}
			}
			if (FinalProperty && ContainerPtr)
			{
				FString FinalPropValStr = PropValStr;
				if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'")))
				{
					FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr);
				}
				if (FStructProperty* StructProp = CastField<FStructProperty>(FinalProperty))
				{
					UStruct* Struct = StructProp->Struct;
					if (Struct)
					{
						FString StructName = Struct->GetName();
						if (StructName.Contains(TEXT("SlateColor")))
						{
							FLinearColor ParsedColor = ParseColor(PropValStr);
							FSlateColor SlateColor(ParsedColor);
							void* DestPtr = StructProp->ContainerPtrToValuePtr<void>(ContainerPtr);
							*reinterpret_cast<FSlateColor*>(DestPtr) = SlateColor;
							UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Set FSlateColor property '%s' = '%s' -> (R=%.2f,G=%.2f,B=%.2f,A=%.2f)"),
								*PropertyPath, *PropValStr, ParsedColor.R, ParsedColor.G, ParsedColor.B, ParsedColor.A);
							continue;
						}
						else if (StructName.Contains(TEXT("LinearColor")))
						{
							FLinearColor ParsedColor = ParseColor(PropValStr);
							void* DestPtr = StructProp->ContainerPtrToValuePtr<void>(ContainerPtr);
							*reinterpret_cast<FLinearColor*>(DestPtr) = ParsedColor;
							UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Set FLinearColor property '%s' = '%s' -> (R=%.2f,G=%.2f,B=%.2f,A=%.2f)"),
								*PropertyPath, *PropValStr, ParsedColor.R, ParsedColor.G, ParsedColor.B, ParsedColor.A);
							continue;
						}
					}
				}
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Setting property '%s' = '%s' on widget '%s'"),
					*PropertyPath, *FinalPropValStr, *TargetWidget->GetName());
				FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None);
			}
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
	Result.bSuccess = true;
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("widgets_created"), CreatedWidgetsMap.Num());
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully created widget layout with %d widgets."), CreatedWidgetsMap.Num()));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== create_widget_from_layout SUCCESS ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Widgets created: %d"), CreatedWidgetsMap.Num());
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Result JSON length: %d chars"), ResultString.Len());
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== create_widget_from_layout RETURNING ==="));
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_GetBehaviorTreeSummary(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BtPath;
	if (!Args->TryGetStringField(TEXT("bt_path"), BtPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: bt_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBehaviorTree* TargetBT = Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(BtPath));
	if (!TargetBT)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load asset as a Behavior Tree at path: %s"), *BtPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FBtGraphDescriber Describer;
	FString Summary = Describer.Describe(TargetBT);
	if (Summary.IsEmpty())
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("The summarizer returned an empty string. The Behavior Tree may be empty or corrupted.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	}
	else
	{
		Result.bSuccess = true;
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("summary"), Summary);
		ResultObject->SetStringField(TEXT("bt_path"), BtPath);
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteUnusedVariables(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	Result.bSuccess = false;
	Result.ErrorMessage = TEXT("The delete_unused_variables tool is not yet fully implemented. Please manually delete unused variables from the Blueprint Editor by right-clicking them in the MyBlueprint panel.");
	ResultObject->SetBoolField(TEXT("success"), false);
	ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
	ResultObject->SetStringField(TEXT("info"), TEXT("You can identify unused variables by looking at your graphs - variables not appearing in any node are likely unused."));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_DeleteFunction(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString BlueprintPath;
	if (!Args->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: blueprint_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString FunctionName;
	if (!Args->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: function_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BlueprintPath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TSet<FString> ProtectedFunctions = {
		TEXT("EventGraph"),
		TEXT("ConstructionScript"),
		TEXT("UserConstructionScript"),
		TEXT("UbergraphPage"),
		TEXT("InteractiveRevision"),
		TEXT("ProcessEvent"),
		TEXT("ReceiveBeginPlay"),
		TEXT("ReceiveTick"),
		TEXT("ReceiveActorBeginOverlap"),
		TEXT("ReceiveActorEndOverlap"),
		TEXT("ReceiveAnyDamage"),
		TEXT("ReceivePointDamage"),
		TEXT("ReceiveRadialDamage"),
		TEXT("ReceiveDestroyed"),
		TEXT("ReceiveActorOnClicked"),
		TEXT("ReceiveActorOnInputTouchBegin"),
		TEXT("ReceiveActorOnInputTouchEnd"),
		TEXT("ReceiveActorOnInputTouchEnter"),
		TEXT("ReceiveActorOnInputTouchLeave")
	};
	if (ProtectedFunctions.Contains(FunctionName))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Cannot delete protected function '%s'. This is a built-in Blueprint event function."), *FunctionName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Cannot delete protected function '%s'."), *FunctionName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UEdGraph* FunctionGraphToDelete = nullptr;
	for (UEdGraph* Graph : TargetBlueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName() == *FunctionName)
		{
			FunctionGraphToDelete = Graph;
			break;
		}
	}
	if (!FunctionGraphToDelete)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Function '%s' not found in Blueprint."), *FunctionName));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Function '%s' not found in Blueprint."), *FunctionName);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Function")));
	TargetBlueprint->Modify();
	FunctionGraphToDelete->Modify();
	TargetBlueprint->FunctionGraphs.Remove(FunctionGraphToDelete);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("function_name"), FunctionName);
	ResultObject->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted function '%s' from Blueprint."), *FunctionName));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_UndoLastGenerated(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FBpGeneratorUltimateModule& BpGeneratorModule = FModuleManager::LoadModuleChecked<FBpGeneratorUltimateModule>("BpGeneratorUltimate");
	UBlueprint* TargetBlueprint = BpGeneratorModule.GetTargetBlueprint();
	if (!TargetBlueprint)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("No Blueprint is currently selected. Please select a Blueprint first."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No Blueprint selected");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<FGuid> NodesToDelete = FBpGeneratorCompiler::PopLastCreatedNodes();
	if (NodesToDelete.Num() == 0)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("No recently generated code found to undo. The undo stack is empty."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("No code to undo");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	bool bDeletedAll = FBpGeneratorCompiler::DeleteNodesByGUIDs(TargetBlueprint, NodesToDelete);
	if (bDeletedAll)
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("blueprint_path"), TargetBlueprint->GetPathName());
		ResultObject->SetNumberField(TEXT("nodes_deleted"), NodesToDelete.Num());
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted %d nodes from the last generated code."), NodesToDelete.Num()));
		Result.bSuccess = true;
		FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Undid last generated code (%d nodes deleted)"), NodesToDelete.Num())));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Failed to delete some or all nodes. Some nodes may have already been deleted."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to delete nodes");
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ListPlugins(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> PluginsArray;
	IPluginManager& PluginManager = IPluginManager::Get();
	const TArray<TSharedRef<IPlugin>>& Plugins = PluginManager.GetEnabledPlugins();
	for (const TSharedRef<IPlugin>& Plugin : Plugins)
	{
		TSharedPtr<FJsonObject> PluginObj = MakeShareable(new FJsonObject);
		PluginObj->SetStringField(TEXT("name"), Plugin->GetName());
		PluginObj->SetStringField(TEXT("friendly_name"), Plugin->GetFriendlyName());
		PluginObj->SetStringField(TEXT("version"), Plugin->GetDescriptor().VersionName);
		PluginObj->SetStringField(TEXT("enabled"), Plugin->IsEnabled() ? TEXT("true") : TEXT("false"));
		FString PluginType = TEXT("External");
		FString BaseDir = Plugin->GetBaseDir();
		if (BaseDir.Contains(FPaths::EnginePluginsDir()))
		{
			PluginType = TEXT("Engine");
		}
		else if (BaseDir.Contains(FPaths::ProjectPluginsDir()))
		{
			PluginType = TEXT("Project");
		}
		PluginObj->SetStringField(TEXT("type"), PluginType);
		PluginObj->SetStringField(TEXT("directory"), Plugin->GetBaseDir());
		PluginsArray.Add(MakeShareable(new FJsonValueObject(PluginObj)));
	}
	ResultObject->SetArrayField(TEXT("plugins"), PluginsArray);
	ResultObject->SetNumberField(TEXT("count"), PluginsArray.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_FindPlugin(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!Args->HasField(TEXT("search_pattern")))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Missing required parameter: search_pattern"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing search_pattern parameter");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString SearchPattern = Args->GetStringField(TEXT("search_pattern"));
	TArray<TSharedPtr<FJsonValue>> FoundPluginsArray;
	IPluginManager& PluginManager = IPluginManager::Get();
	const TArray<TSharedRef<IPlugin>>& Plugins = PluginManager.GetEnabledPlugins();
	FString LowerSearchPattern = SearchPattern.ToLower().Replace(TEXT(" "), TEXT(""));
	for (const TSharedRef<IPlugin>& Plugin : Plugins)
	{
		FString PluginNameLower = Plugin->GetName().ToLower();
		FString FriendlyNameLower = Plugin->GetFriendlyName().ToLower();
		if (PluginNameLower.Contains(LowerSearchPattern) || FriendlyNameLower.Contains(LowerSearchPattern))
		{
			FString BaseDir = Plugin->GetBaseDir();
			if (FPaths::IsRelative(BaseDir))
			{
				FString EngineDirAbs = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
				FString CombinedPath = FPaths::Combine(EngineDirAbs, BaseDir);
				BaseDir = FPaths::ConvertRelativePathToFull(CombinedPath);
			}
			if (IFileManager::Get().DirectoryExists(*BaseDir))
			{
				TSharedPtr<FJsonObject> PluginObj = MakeShareable(new FJsonObject);
				PluginObj->SetStringField(TEXT("name"), Plugin->GetName());
				PluginObj->SetStringField(TEXT("friendly_name"), Plugin->GetFriendlyName());
				PluginObj->SetStringField(TEXT("version"), Plugin->GetDescriptor().VersionName);
				PluginObj->SetStringField(TEXT("enabled"), Plugin->IsEnabled() ? TEXT("true") : TEXT("false"));
				PluginObj->SetStringField(TEXT("directory"), BaseDir);
				FoundPluginsArray.Add(MakeShareable(new FJsonValueObject(PluginObj)));
			}
		}
	}
	if (FoundPluginsArray.Num() == 0)
	{
		FString EnginePluginsDir = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
		ResultObject->SetStringField(TEXT("debug_engine_plugins_dir"), EnginePluginsDir);
		auto ScanPluginDirectory = [&](const FString& SearchDir)
		{
			TArray<FString> AllItems;
			IFileManager::Get().FindFiles(AllItems, *SearchDir, TEXT("*"));
			FString DebugFoundFolders;
			FString AllItemsList;
			int32 FolderCount = 0;
			int32 UpluginCount = 0;
			for (const FString& Item : AllItems)
			{
				if (!AllItemsList.IsEmpty())
					AllItemsList += TEXT(",");
				AllItemsList += Item;
			}
			for (const FString& Item : AllItems)
			{
				if (Item == TEXT(".") || Item == TEXT(".."))
					continue;
				FString ItemPath = FPaths::Combine(SearchDir, Item);
				if (IFileManager::Get().DirectoryExists(*ItemPath))
				{
					FolderCount++;
					TArray<FString> UpluginFiles;
					IFileManager::Get().FindFiles(UpluginFiles, *ItemPath, TEXT("*.uplugin"));
					if (UpluginFiles.Num() > 0)
					{
						UpluginCount++;
						FString PluginNameFromFolder = Item;
						FString FolderNameLower = PluginNameFromFolder.ToLower();
						FString UpluginPath = FPaths::Combine(ItemPath, UpluginFiles[0]);
						FString UpluginContent;
						if (FFileHelper::LoadFileToString(UpluginContent, *UpluginPath))
						{
							FString FriendlyName = PluginNameFromFolder;
							int32 FriendlyNamePos = UpluginContent.Find(TEXT("\"FriendlyName\""));
							if (FriendlyNamePos != INDEX_NONE)
							{
								int32 ColonPos = UpluginContent.Find(TEXT(":"), FriendlyNamePos + 15);
								if (ColonPos != INDEX_NONE)
								{
									for (int32 i = ColonPos + 1; i < UpluginContent.Len() && i < ColonPos + 50; i++)
									{
										if (UpluginContent[i] == TEXT('"'))
										{
											for (int32 j = i + 1; j < UpluginContent.Len() && j < i + 200; j++)
											{
												if (UpluginContent[j] == TEXT('"'))
												{
													FriendlyName = UpluginContent.Mid(i + 1, j - i - 1);
													break;
												}
											}
											break;
										}
									}
								}
							}
							FString FriendlyNameLower = FriendlyName.ToLower().Replace(TEXT(" "), TEXT(""));
							if (!DebugFoundFolders.IsEmpty())
								DebugFoundFolders += TEXT("; ");
							DebugFoundFolders += FString::Printf(TEXT("%s->%s"), *FolderNameLower, *FriendlyNameLower);
							if (FolderNameLower.Contains(LowerSearchPattern) || FriendlyNameLower.Contains(LowerSearchPattern))
							{
								TSharedPtr<FJsonObject> PluginObj = MakeShareable(new FJsonObject);
								PluginObj->SetStringField(TEXT("name"), PluginNameFromFolder);
								PluginObj->SetStringField(TEXT("friendly_name"), FriendlyName);
								PluginObj->SetStringField(TEXT("version"), TEXT("Unknown"));
								PluginObj->SetStringField(TEXT("enabled"), TEXT("false"));
								PluginObj->SetStringField(TEXT("directory"), ItemPath);
								PluginObj->SetStringField(TEXT("source"), TEXT("Engine"));
								FoundPluginsArray.Add(MakeShareable(new FJsonValueObject(PluginObj)));
							}
						}
					}
				}
			}
			static int32 DebugIndex = 0;
			FString ScanResult = FString::Printf(TEXT("Dir:%s Items:%s Folders:%d WithPlugin:%d"),
				*SearchDir, *AllItemsList, FolderCount, UpluginCount);
			if (!DebugFoundFolders.IsEmpty())
			{
				ScanResult += TEXT(" Parsed:") + DebugFoundFolders;
			}
			ResultObject->SetStringField(FString::Printf(TEXT("debug_scan_%d"), DebugIndex++), ScanResult);
		};
		ScanPluginDirectory(EnginePluginsDir);
		FString MarketplaceDir = FPaths::Combine(EnginePluginsDir, TEXT("Marketplace"));
		if (IFileManager::Get().DirectoryExists(*MarketplaceDir))
		{
			ScanPluginDirectory(MarketplaceDir);
		}
		if (FoundPluginsArray.Num() == 0)
		{
			TArray<FString> PossibleEnginePaths;
			PossibleEnginePaths.Add(TEXT("C:\\Program Files\\Epic Games\\UE_5.5\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("C:\\Program Files\\Epic Games\\UE_5.4\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("C:\\Program Files\\Epic Games\\UE_5.3\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("C:\\Program Files\\Epic Games\\UE_5.6\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("D:\\Epic Games\\UE_5.5\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("D:\\Epic Games\\UE_5.4\\Engine\\Plugins\\Marketplace"));
			PossibleEnginePaths.Add(TEXT("D:\\UnrealEngine\\UE_5.5\\Engine\\Plugins\\Marketplace"));
			FString DebugPathsScanned;
			for (const FString& MarketPath : PossibleEnginePaths)
			{
				bool bDirExists = IFileManager::Get().DirectoryExists(*MarketPath);
				if (bDirExists)
				{
					DebugPathsScanned += FString::Printf(TEXT("[%s exists] "), *MarketPath);
					ScanPluginDirectory(MarketPath);
					if (FoundPluginsArray.Num() > 0)
						break;
				}
				else
				{
					DebugPathsScanned += FString::Printf(TEXT("[%s not_exist] "), *MarketPath);
				}
			}
			if (FoundPluginsArray.Num() == 0)
			{
				ResultObject->SetStringField(TEXT("debug_paths_scanned"), DebugPathsScanned);
			}
		}
	}
	if (FoundPluginsArray.Num() == 0)
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("No plugins found matching pattern: %s"), *SearchPattern));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("No plugins found: %s"), *SearchPattern);
	}
	else
	{
		ResultObject->SetArrayField(TEXT("plugins"), FoundPluginsArray);
		ResultObject->SetNumberField(TEXT("count"), FoundPluginsArray.Num());
		ResultObject->SetStringField(TEXT("search_pattern"), SearchPattern);
		ResultObject->SetBoolField(TEXT("success"), true);
		Result.bSuccess = true;
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ReadCppFile(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!Args->HasField(TEXT("file_path")))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Missing required parameter: file_path"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing file_path parameter");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString FilePath = Args->GetStringField(TEXT("file_path"));
	FString AbsolutePath = FilePath;
	if (FPaths::IsRelative(FilePath) && !FilePath.StartsWith(TEXT("../")))
	{
		AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
	}
	else if (FilePath.StartsWith(TEXT("../")))
	{
		AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
	}
	if (!FPaths::FileExists(AbsolutePath))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("File not found: %s"), *AbsolutePath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("File not found: %s"), *AbsolutePath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString Extension = FPaths::GetExtension(AbsolutePath).ToLower();
	if (Extension != TEXT("h") && Extension != TEXT("cpp") && Extension != TEXT("hpp") && Extension != TEXT("c") && Extension != TEXT("cc") && Extension != TEXT("inl"))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("File is not a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl): %s"), *Extension));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Not a C++ file: %s"), *Extension);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *AbsolutePath))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to read file: %s"), *AbsolutePath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to read: %s"), *AbsolutePath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("file_path"), FilePath);
	ResultObject->SetStringField(TEXT("absolute_path"), AbsolutePath);
	ResultObject->SetStringField(TEXT("content"), FileContent);
	ResultObject->SetNumberField(TEXT("content_length"), FileContent.Len());
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_WriteCppFile(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!Args->HasField(TEXT("file_path")) || !Args->HasField(TEXT("content")))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Missing required parameters: file_path and content"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing file_path or content parameter");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString FilePath = Args->GetStringField(TEXT("file_path"));
	FString FileContent = Args->GetStringField(TEXT("content"));
	FString AbsolutePath = FilePath;
	if (FPaths::IsRelative(FilePath))
	{
		AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
	}
	FString Extension = FPaths::GetExtension(AbsolutePath).ToLower();
	if (Extension != TEXT("h") && Extension != TEXT("cpp") && Extension != TEXT("hpp") && Extension != TEXT("c") && Extension != TEXT("cc") && Extension != TEXT("inl"))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("File is not a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl): %s"), *Extension));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Not a C++ file: %s"), *Extension);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString EnginePluginsDir = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
	if (!AbsolutePath.StartsWith(ProjectDir) && !AbsolutePath.StartsWith(EnginePluginsDir))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Cannot write files outside the project directory or engine plugins directory for security reasons."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Security violation: path outside project/plugins");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ProjectSourceDir = FPaths::Combine(ProjectDir, TEXT("Source"));
	if (AbsolutePath.StartsWith(ProjectSourceDir) && !IFileManager::Get().DirectoryExists(*ProjectSourceDir))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("This project does not have a Source folder. It appears to be a Blueprint-only project."));
		ResultObject->SetStringField(TEXT("instructions"), TEXT("To add C++ code to your project, go to Tools > New C++ Class and create any class (e.g., a generic UActorComponent subclass). This will generate the Source folder and necessary project files."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Project has no Source folder - create a C++ class first from Tools menu");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString ParentPath = FPaths::GetPath(AbsolutePath);
	if (!IFileManager::Get().DirectoryExists(*ParentPath))
	{
		IFileManager::Get().MakeDirectory(*ParentPath, true);
	}
	if (!FFileHelper::SaveStringToFile(FileContent, *AbsolutePath))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to write file: %s"), *AbsolutePath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to write: %s"), *AbsolutePath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("file_path"), AbsolutePath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully wrote %d characters to file."), FileContent.Len()));
	ResultObject->SetNumberField(TEXT("bytes_written"), FileContent.Len());
	Result.bSuccess = true;
	FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Wrote C++ file: %s"), *AbsolutePath)));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ListPluginFiles(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!Args->HasField(TEXT("plugin_name")))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Missing required parameter: plugin_name"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing plugin_name parameter");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString PluginName = Args->GetStringField(TEXT("plugin_name"));
	IPluginManager& PluginManager = IPluginManager::Get();
	TSharedPtr<IPlugin> FoundPlugin = nullptr;
	const TArray<TSharedRef<IPlugin>>& Plugins = PluginManager.GetEnabledPlugins();
	for (const TSharedRef<IPlugin>& Plugin : Plugins)
	{
		if (Plugin->GetName().ToLower() == PluginName.ToLower())
		{
			FoundPlugin = Plugin;
			break;
		}
		if (Plugin->GetFriendlyName().ToLower() == PluginName.ToLower())
		{
			FoundPlugin = Plugin;
			break;
		}
	}
	if (!FoundPlugin.IsValid())
	{
		FString SearchLower = PluginName.ToLower().Replace(TEXT(" "), TEXT(""));
		for (const TSharedRef<IPlugin>& Plugin : Plugins)
		{
			if (Plugin->GetName().ToLower().Contains(SearchLower) ||
				Plugin->GetFriendlyName().ToLower().Contains(SearchLower))
			{
				FoundPlugin = Plugin;
				break;
			}
		}
	}
	if (!FoundPlugin.IsValid())
	{
		FString FoundPluginDir;
		FString SearchLower = PluginName.ToLower().Replace(TEXT(" "), TEXT(""));
		auto ScanEnginePlugins = [&](const FString& SearchDir) -> bool {
			if (!IFileManager::Get().DirectoryExists(*SearchDir))
				return false;
			TArray<FString> AllItems;
			IFileManager::Get().FindFiles(AllItems, *SearchDir, TEXT("*"));
			for (const FString& Item : AllItems)
			{
				if (Item == TEXT(".") || Item == TEXT(".."))
					continue;
				FString ItemPath = FPaths::Combine(SearchDir, Item);
				if (IFileManager::Get().DirectoryExists(*ItemPath))
				{
					TArray<FString> UpluginFiles;
					IFileManager::Get().FindFiles(UpluginFiles, *ItemPath, TEXT("*.uplugin"));
					if (UpluginFiles.Num() > 0)
					{
						FString UpluginPath = FPaths::Combine(ItemPath, UpluginFiles[0]);
						FString UpluginContent;
						if (FFileHelper::LoadFileToString(UpluginContent, *UpluginPath))
						{
							FString FriendlyName;
							int32 FriendlyNamePos = UpluginContent.Find(TEXT("\"FriendlyName\""));
							if (FriendlyNamePos != INDEX_NONE)
							{
								int32 ColonPos = UpluginContent.Find(TEXT(":"), FriendlyNamePos + 15);
								if (ColonPos != INDEX_NONE)
								{
									for (int32 i = ColonPos + 1; i < UpluginContent.Len() && i < ColonPos + 50; i++)
									{
										if (UpluginContent[i] == TEXT('"'))
										{
											for (int32 j = i + 1; j < UpluginContent.Len() && j < i + 200; j++)
											{
												if (UpluginContent[j] == TEXT('"'))
												{
													FriendlyName = UpluginContent.Mid(i + 1, j - i - 1);
													break;
												}
											}
											break;
										}
									}
								}
							}
							FString FolderNameLower = Item.ToLower();
							if (FriendlyName.ToLower().Replace(TEXT(" "), TEXT("")).Contains(SearchLower) ||
								FolderNameLower.Contains(SearchLower))
							{
								FoundPluginDir = ItemPath;
								return true;
							}
						}
					}
				}
			}
			return false;
		};
		FString EnginePluginsDir = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
		if (ScanEnginePlugins(EnginePluginsDir))
		{
		}
		else
		{
			FString MarketplaceDir = FPaths::Combine(EnginePluginsDir, TEXT("Marketplace"));
			if (!ScanEnginePlugins(MarketplaceDir))
			{
				FString AvailablePlugins;
				for (const TSharedRef<IPlugin>& Plugin : Plugins)
				{
					if (!AvailablePlugins.IsEmpty())
						AvailablePlugins += TEXT(", ");
					AvailablePlugins += FString::Printf(TEXT("%s (%s)"), *Plugin->GetName(), *Plugin->GetFriendlyName());
				}
				ResultObject->SetBoolField(TEXT("success"), false);
				ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Plugin not found: %s. Available enabled plugins: %s"), *PluginName, *AvailablePlugins));
				Result.bSuccess = false;
				Result.ErrorMessage = FString::Printf(TEXT("Plugin not found: %s"), *PluginName);
				FString ResultString;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
				FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
				Result.ResultJson = ResultString;
				return Result;
			}
		}
		TArray<TSharedPtr<FJsonValue>> FilesArray;
		FString PluginBaseDir = FoundPluginDir;
		bool bDirExists = IFileManager::Get().DirectoryExists(*PluginBaseDir);
		TArray<FString> AllFilesInDir;
		IFileManager::Get().FindFiles(AllFilesInDir, *PluginBaseDir, TEXT("*"));
		TArray<FString> HeaderFiles;
		IFileManager::Get().FindFilesRecursive(HeaderFiles, *PluginBaseDir, TEXT("*.h"), true, false);
		for (const FString& File : HeaderFiles)
		{
			TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
			FileObj->SetStringField(TEXT("file_path"), File);
			FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
			FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
			FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
		}
		TArray<FString> CppFiles;
		IFileManager::Get().FindFilesRecursive(CppFiles, *PluginBaseDir, TEXT("*.cpp"), true, false);
		for (const FString& File : CppFiles)
		{
			TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
			FileObj->SetStringField(TEXT("file_path"), File);
			FileObj->SetStringField(TEXT("file_type"), TEXT("source"));
			FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
			FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
		}
		TArray<FString> HppFiles;
		IFileManager::Get().FindFilesRecursive(HppFiles, *PluginBaseDir, TEXT("*.hpp"), true, false);
		for (const FString& File : HppFiles)
		{
			TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
			FileObj->SetStringField(TEXT("file_path"), File);
			FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
			FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
			FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
		}
		ResultObject->SetArrayField(TEXT("files"), FilesArray);
		ResultObject->SetStringField(TEXT("plugin_name"), PluginName);
		ResultObject->SetStringField(TEXT("plugin_directory"), PluginBaseDir);
		ResultObject->SetNumberField(TEXT("file_count"), FilesArray.Num());
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("debug_dir_exists"), bDirExists ? TEXT("true") : TEXT("false"));
		FString ImmediateContents;
		for (const FString& Item : AllFilesInDir)
		{
			if (!ImmediateContents.IsEmpty())
				ImmediateContents += TEXT(", ");
			ImmediateContents += Item;
		}
		ResultObject->SetStringField(TEXT("debug_immediate_contents"), ImmediateContents);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		Result.bSuccess = true;
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> FilesArray;
	FString PluginBaseDir = FoundPlugin->GetBaseDir();
	if (FPaths::IsRelative(PluginBaseDir))
	{
		FString EngineDirAbs = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
		FString CombinedPath = FPaths::Combine(EngineDirAbs, PluginBaseDir);
		PluginBaseDir = FPaths::ConvertRelativePathToFull(CombinedPath);
	}
	if (!IFileManager::Get().DirectoryExists(*PluginBaseDir))
	{
		TArray<FString> PossibleEnginePaths;
		PossibleEnginePaths.Add(TEXT("C:/Program Files/Epic Games/UE_5.5/Engine/"));
		PossibleEnginePaths.Add(TEXT("C:/Program Files/Epic Games/UE_5.4/Engine/"));
		PossibleEnginePaths.Add(TEXT("C:/Program Files/Epic Games/UE_5.3/Engine/"));
		PossibleEnginePaths.Add(TEXT("D:/Epic Games/UE_5.5/Engine/"));
		PossibleEnginePaths.Add(TEXT("D:/Epic Games/UE_5.4/Engine/"));
		PossibleEnginePaths.Add(TEXT("D:/UnrealEngine/UE_5.5/Engine/"));
		FString PluginToFind = FoundPlugin->GetName();
		for (const FString& EnginePath : PossibleEnginePaths)
		{
			FString MarketplacePath = FPaths::Combine(EnginePath, TEXT("Plugins/Marketplace"));
			TArray<FString> MarketFolders;
			IFileManager::Get().FindFiles(MarketFolders, *MarketplacePath, TEXT("*"));
			for (const FString& Folder : MarketFolders)
			{
				if (Folder == TEXT(".") || Folder == TEXT(".."))
					continue;
				FString FolderPath = FPaths::Combine(MarketplacePath, Folder);
				if (IFileManager::Get().DirectoryExists(*FolderPath))
				{
					TArray<FString> UpluginFiles;
					IFileManager::Get().FindFiles(UpluginFiles, *FolderPath, TEXT("*.uplugin"));
					if (UpluginFiles.Num() > 0)
					{
						FString UpluginPath = FPaths::Combine(FolderPath, UpluginFiles[0]);
						FString UpluginContent;
						if (FFileHelper::LoadFileToString(UpluginContent, *UpluginPath))
						{
							FString FriendlyName;
							int32 FriendlyNamePos = UpluginContent.Find(TEXT("\"FriendlyName\""));
							if (FriendlyNamePos != INDEX_NONE)
							{
								int32 ColonPos = UpluginContent.Find(TEXT(":"), FriendlyNamePos + 15);
								if (ColonPos != INDEX_NONE)
								{
									for (int32 i = ColonPos + 1; i < UpluginContent.Len() && i < ColonPos + 50; i++)
									{
										if (UpluginContent[i] == TEXT('"'))
										{
											for (int32 j = i + 1; j < UpluginContent.Len() && j < i + 200; j++)
											{
												if (UpluginContent[j] == TEXT('"'))
												{
													FriendlyName = UpluginContent.Mid(i + 1, j - i - 1);
													break;
												}
											}
											break;
										}
									}
								}
							}
							FString FriendlyNameLower = FriendlyName.ToLower().Replace(TEXT(" "), TEXT(""));
							FString PluginToFindLower = PluginToFind.ToLower().Replace(TEXT(" "), TEXT(""));
							if (Folder.Contains(PluginToFind) || PluginToFind.Contains(Folder) ||
								FriendlyNameLower.Contains(PluginToFindLower) || PluginToFindLower.Contains(FriendlyNameLower))
							{
								PluginBaseDir = FolderPath;
								break;
							}
						}
					}
				}
			}
			if (IFileManager::Get().DirectoryExists(*PluginBaseDir))
				break;
		}
	}
	bool bDirExists = IFileManager::Get().DirectoryExists(*PluginBaseDir);
	TArray<FString> AllFilesInDir;
	if (bDirExists)
	{
		IFileManager::Get().FindFiles(AllFilesInDir, *PluginBaseDir, TEXT("*"));
	}
	TArray<FString> SubDirs;
	for (const FString& Item : AllFilesInDir)
	{
		FString ItemPath = FPaths::Combine(PluginBaseDir, Item);
		if (IFileManager::Get().DirectoryExists(*ItemPath))
		{
			SubDirs.Add(Item);
		}
	}
	TArray<FString> HeaderFiles;
	IFileManager::Get().FindFilesRecursive(HeaderFiles, *PluginBaseDir, TEXT("*.h"), true, false);
	for (const FString& File : HeaderFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	TArray<FString> CppFiles;
	IFileManager::Get().FindFilesRecursive(CppFiles, *PluginBaseDir, TEXT("*.cpp"), true, false);
	for (const FString& File : CppFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("source"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	TArray<FString> HppFiles;
	IFileManager::Get().FindFilesRecursive(HppFiles, *PluginBaseDir, TEXT("*.hpp"), true, false);
	for (const FString& File : HppFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	TArray<FString> InlFiles;
	IFileManager::Get().FindFilesRecursive(InlFiles, *PluginBaseDir, TEXT("*.inl"), true, false);
	for (const FString& File : InlFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("inline"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(PluginBaseDir.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	ResultObject->SetStringField(TEXT("plugin_name"), FoundPlugin->GetName());
	ResultObject->SetStringField(TEXT("plugin_directory"), PluginBaseDir);
	ResultObject->SetArrayField(TEXT("files"), FilesArray);
	ResultObject->SetNumberField(TEXT("total_files"), FilesArray.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	Result.bSuccess = true;
	ResultObject->SetStringField(TEXT("debug_dir_exists"), bDirExists ? TEXT("true") : TEXT("false"));
	FString ImmediateContents;
	for (const FString& Item : AllFilesInDir)
	{
		if (!ImmediateContents.IsEmpty())
			ImmediateContents += TEXT(", ");
		ImmediateContents += Item;
	}
	ResultObject->SetStringField(TEXT("debug_immediate_contents"), ImmediateContents);
	FString SubDirsList;
	for (const FString& Dir : SubDirs)
	{
		if (!SubDirsList.IsEmpty())
			SubDirsList += TEXT(", ");
		SubDirsList += Dir;
	}
	ResultObject->SetStringField(TEXT("debug_subdirectories"), SubDirsList);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_ScanDirectory(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	if (!Args->HasField(TEXT("directory_path")))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Missing required parameter: directory_path"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing directory_path parameter");
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FString DirectoryPath = Args->GetStringField(TEXT("directory_path"));
	if (!IFileManager::Get().DirectoryExists(*DirectoryPath))
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), FString::Printf(TEXT("Directory not found: %s"), *DirectoryPath));
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Directory not found: %s"), *DirectoryPath);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	TArray<TSharedPtr<FJsonValue>> FilesArray;
	TArray<FString> HeaderFiles;
	IFileManager::Get().FindFilesRecursive(HeaderFiles, *DirectoryPath, TEXT("*.h"), true, false);
	for (const FString& File : HeaderFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(DirectoryPath.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	TArray<FString> CppFiles;
	IFileManager::Get().FindFilesRecursive(CppFiles, *DirectoryPath, TEXT("*.cpp"), true, false);
	for (const FString& File : CppFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("source"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(DirectoryPath.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	TArray<FString> HppFiles;
	IFileManager::Get().FindFilesRecursive(HppFiles, *DirectoryPath, TEXT("*.hpp"), true, false);
	for (const FString& File : HppFiles)
	{
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("file_path"), File);
		FileObj->SetStringField(TEXT("file_type"), TEXT("header"));
		FileObj->SetStringField(TEXT("relative_path"), File.Mid(DirectoryPath.Len() + 1));
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	ResultObject->SetStringField(TEXT("directory"), DirectoryPath);
	ResultObject->SetArrayField(TEXT("files"), FilesArray);
	ResultObject->SetNumberField(TEXT("file_count"), FilesArray.Num());
	ResultObject->SetNumberField(TEXT("header_count"), HeaderFiles.Num());
	ResultObject->SetNumberField(TEXT("source_count"), CppFiles.Num() + HppFiles.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_SelectFolder(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	UE_LOG(LogBpGeneratorUltimate, Log, TEXT("SelectFolder tool: Starting..."));
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform != nullptr)
	{
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("SelectFolder tool: DesktopPlatform acquired"));
		FString SelectedPath;
		void* ParentWindowWindowHandle = nullptr;
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
		{
			ParentWindowWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("SelectFolder tool: Parent window handle: %p"), ParentWindowWindowHandle);
		}
		else
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("SelectFolder tool: No valid parent window found"));
		}
		FString DefaultPath = TEXT("C:/Program Files/Epic Games/UE_5.5/Engine/Plugins/Marketplace");
		if (!IFileManager::Get().DirectoryExists(*DefaultPath))
		{
			DefaultPath = FPaths::EnginePluginsDir();
		}
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("SelectFolder tool: Opening dialog with default path: %s"), *DefaultPath);
		bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowWindowHandle,
			TEXT("Select Plugin Source Folder"),
			DefaultPath,
			SelectedPath
		);
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("SelectFolder tool: Dialog closed, bFolderSelected=%d, Path=%s"), bFolderSelected, *SelectedPath);
		if (bFolderSelected && !SelectedPath.IsEmpty())
		{
			ResultObject->SetStringField(TEXT("selected_path"), SelectedPath);
			ResultObject->SetBoolField(TEXT("success"), true);
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("User selected folder: %s"), *SelectedPath));
			Result.bSuccess = true;
			Result.ErrorMessage = TEXT("Folder selected");
		}
		else
		{
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), TEXT("User cancelled folder selection"));
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("Folder selection cancelled");
		}
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Error, TEXT("SelectFolder tool: DesktopPlatform module not available"));
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("Folder dialog not available on this platform"));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Folder dialog not available");
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CheckProjectSource(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString ProjectSourceDir = FPaths::Combine(ProjectDir, TEXT("Source"));
	bool bSourceExists = IFileManager::Get().DirectoryExists(*ProjectSourceDir);
	ResultObject->SetStringField(TEXT("project_dir"), ProjectDir);
	ResultObject->SetStringField(TEXT("source_dir"), ProjectSourceDir);
	ResultObject->SetBoolField(TEXT("source_exists"), bSourceExists);
	if (bSourceExists)
	{
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("message"), TEXT("Project has a Source folder. You can write C++ files directly using relative paths like 'Source/MyClass.h'."));
		Result.bSuccess = true;
		Result.ErrorMessage = TEXT("Source folder exists");
	}
	else
	{
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("This project does not have a Source folder. It is a Blueprint-only project."));
		ResultObject->SetStringField(TEXT("instructions"), TEXT("To add C++ code to your project, go to Tools > New C++ Class and create any class. This will generate the Source folder and necessary project files."));
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Project has no Source folder - create a C++ class first from Tools menu");
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateDataTable(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString TableName, SavePath, RowStructPath;
	if (!Args->TryGetStringField(TEXT("table_name"), TableName) && !Args->TryGetStringField(TEXT("name"), TableName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name or table_name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	Args->TryGetStringField(TEXT("row_struct"), RowStructPath);
	if (SavePath.IsEmpty())
	{
		SavePath = GetContentBrowserCurrentFolder();
	}
	UScriptStruct* RowStruct = nullptr;
	if (!RowStructPath.IsEmpty())
	{
		RowStruct = LoadObject<UScriptStruct>(nullptr, *RowStructPath);
		if (!RowStruct)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Could not load row struct at path: %s"), *RowStructPath);
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			Result.ResultJson = ResultString;
			return Result;
		}
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(TableName, SavePath, UDataTable::StaticClass(), nullptr);
	UDataTable* DataTable = Cast<UDataTable>(NewAsset);
	if (!DataTable)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create DataTable asset. The asset may already exist or path is invalid.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (RowStruct)
	{
		DataTable->RowStruct = RowStruct;
	}
	int32 RowsAdded = 0;
	const TArray<TSharedPtr<FJsonValue>>* RowsArray = nullptr;
	if (Args->TryGetArrayField(TEXT("rows"), RowsArray) && RowStruct)
	{
		for (const TSharedPtr<FJsonValue>& RowValue : *RowsArray)
		{
			const TSharedPtr<FJsonObject>* RowObj = nullptr;
			if (!RowValue->TryGetObject(RowObj) || !RowObj)
			{
				continue;
			}
			FString RowName;
			if (!(*RowObj)->TryGetStringField(TEXT("row_name"), RowName))
			{
				(*RowObj)->TryGetStringField(TEXT("RowName"), RowName);
			}
			if (RowName.IsEmpty())
			{
				continue;
			}
			uint8* RowData = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
			RowStruct->InitializeStruct(RowData);
			bool bRowSuccess = true;
			for (auto& FieldPair : (*RowObj)->Values)
			{
				const FString& FieldName = FieldPair.Key;
				TSharedPtr<FJsonValue> FieldValue = FieldPair.Value;
				if (FieldName == TEXT("row_name") || FieldName == TEXT("RowName"))
				{
					continue;
				}
				FProperty* MatchedProperty = nullptr;
				for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
				{
					FProperty* Property = *PropIt;
					if (Property && !Property->IsNative())
					{
						FString PropName = Property->GetName();
						if (PropName.StartsWith(FieldName + TEXT("_")))
						{
							MatchedProperty = Property;
							break;
						}
					}
				}
				if (!MatchedProperty || !FieldValue.IsValid())
				{
					continue;
				}
				FString PropertyValueStr;
				bool bFoundValue = false;
				switch (FieldValue->Type)
				{
					case EJson::String:
						PropertyValueStr = FieldValue->AsString();
						bFoundValue = true;
						break;
					case EJson::Number:
						PropertyValueStr = FString::SanitizeFloat(FieldValue->AsNumber());
						bFoundValue = true;
						break;
					case EJson::Boolean:
						PropertyValueStr = FieldValue->AsBool() ? TEXT("True") : TEXT("False");
						bFoundValue = true;
						break;
					default:
						break;
				}
				if (bFoundValue && !PropertyValueStr.IsEmpty())
				{
					void* PropertyData = MatchedProperty->ContainerPtrToValuePtr<void>(RowData);
					if (!MatchedProperty->ImportText_Direct(*PropertyValueStr, PropertyData, nullptr, PPF_None))
					{
						bRowSuccess = false;
					}
				}
			}
			if (bRowSuccess)
			{
				TMap<FName, uint8*>& MutableRowMap = const_cast<TMap<FName, uint8*>&>(DataTable->GetRowMap());
				MutableRowMap.Add(FName(*RowName), RowData);
				RowsAdded++;
			}
			else
			{
				RowStruct->DestroyStruct(RowData);
				FMemory::Free(RowData);
			}
		}
	}
	DataTable->MarkPackageDirty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FString PackagePath = FPackageName::GetLongPackagePath(DataTable->GetPathName());
	if (!PackagePath.IsEmpty())
	{
		AssetRegistryModule.Get().ScanPathsSynchronous({ PackagePath }, true);
	}
	FString AssetPath = DataTable->GetPathName();
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObject->SetNumberField(TEXT("row_count"), RowsAdded);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Created DataTable: %s with %d row(s)"), *AssetPath, RowsAdded));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateInputAction(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ActionName, SavePath, ValueType;
	if (!Args->TryGetStringField(TEXT("name"), ActionName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	Args->TryGetStringField(TEXT("value_type"), ValueType);
	if (SavePath.IsEmpty())
	{
		SavePath = GetContentBrowserCurrentFolder();
	}
	EInputActionValueType ActionType = EInputActionValueType::Boolean;
	if (ValueType == TEXT("float"))
	{
		ActionType = EInputActionValueType::Axis1D;
	}
	else if (ValueType == TEXT("vector2d"))
	{
		ActionType = EInputActionValueType::Axis2D;
	}
	else if (ValueType == TEXT("vector3d"))
	{
		ActionType = EInputActionValueType::Axis3D;
	}
	FString PackagePath = FPackageName::FilenameToLongPackageName(FPaths::Combine(SavePath, ActionName));
	UPackage* Package = CreatePackage(*PackagePath);
	UInputAction* InputAction = NewObject<UInputAction>(Package, *ActionName, RF_Public | RF_Standalone | RF_Transactional);
	if (!InputAction)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create InputAction object");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	InputAction->ValueType = ActionType;
	InputAction->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(InputAction);
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *SavePath, *ActionName, *ActionName);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Created InputAction: %s with value type %s"), *AssetPath, *ValueType));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_CreateInputMappingContext(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ContextName, SavePath;
	if (!Args->TryGetStringField(TEXT("name"), ContextName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: name");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	Args->TryGetStringField(TEXT("save_path"), SavePath);
	if (SavePath.IsEmpty())
	{
		SavePath = GetContentBrowserCurrentFolder();
	}
	FString PackagePath = FPackageName::FilenameToLongPackageName(FPaths::Combine(SavePath, ContextName));
	UPackage* Package = CreatePackage(*PackagePath);
	UInputMappingContext* MappingContext = NewObject<UInputMappingContext>(Package, *ContextName, RF_Public | RF_Standalone | RF_Transactional);
	if (!MappingContext)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Failed to create InputMappingContext object");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	MappingContext->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(MappingContext);
	FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *SavePath, *ContextName, *ContextName);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), AssetPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Created InputMappingContext: %s"), *AssetPath));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddInputMapping(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString ContextPath, ActionPath, KeyName;
	if (!Args->TryGetStringField(TEXT("context_path"), ContextPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: context_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!Args->TryGetStringField(TEXT("action"), ActionPath) && !Args->TryGetStringField(TEXT("action_path"), ActionPath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: action or action_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	if (!Args->TryGetStringField(TEXT("key"), KeyName))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: key");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *ContextPath);
	if (!Context)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to load InputMappingContext: %s"), *ContextPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionPath);
	if (!Action)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to load InputAction: %s"), *ActionPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	FKey Key(*KeyName);
	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
	Context->MarkPackageDirty();
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Added mapping: %s -> %s in %s"), *KeyName, *ActionPath, *ContextPath));
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
FString SBpGeneratorUltimateWidget::GetContentBrowserCurrentFolder() const
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	FString CurrentPath;
	FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
	if (!CurrentPathItem.GetVirtualPathName().IsNone())
	{
		CurrentPath = CurrentPathItem.GetInternalPathString();
	}
	if (CurrentPath.IsEmpty())
	{
		TArray<FString> SelectedFolders;
		ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
		if (SelectedFolders.Num() > 0)
		{
			CurrentPath = SelectedFolders[0];
		}
	}
	if (CurrentPath.IsEmpty())
	{
		CurrentPath = TEXT("/Game");
	}
	return CurrentPath;
}
FToolExecutionResult SBpGeneratorUltimateWidget::ExecuteTool_AddDataTableRows(const TSharedPtr<FJsonObject>& Args)
{
	FToolExecutionResult Result;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString DataTablePath;
	if (!Args->TryGetStringField(TEXT("table_path"), DataTablePath))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing required parameter: table_path");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
	if (!DataTable)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = FString::Printf(TEXT("Failed to load DataTable: %s"), *DataTablePath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	const TArray<TSharedPtr<FJsonValue>>* RowsArray = nullptr;
	if (!Args->TryGetArrayField(TEXT("rows"), RowsArray) || !RowsArray)
	{
		Result.bSuccess = false;
		Result.ErrorMessage = TEXT("Missing or invalid 'rows' array");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), Result.ErrorMessage);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		Result.ResultJson = ResultString;
		return Result;
	}
	int32 RowsAdded = 0;
	TArray<FString> Errors;
	for (const TSharedPtr<FJsonValue>& RowValue : *RowsArray)
	{
		const TSharedPtr<FJsonObject>* RowObj = nullptr;
		if (!RowValue->TryGetObject(RowObj) || !RowObj)
		{
			Errors.Add(TEXT("Invalid row format - expected object"));
			continue;
		}
		FString RowName;
		if (!(*RowObj)->TryGetStringField(TEXT("row_name"), RowName))
		{
			(*RowObj)->TryGetStringField(TEXT("RowName"), RowName);
		}
		if (RowName.IsEmpty())
		{
			Errors.Add(TEXT("Row missing 'row_name' field"));
			continue;
		}
		if (DataTable->RowStruct)
		{
			UScriptStruct* RowStruct = DataTable->RowStruct;
			uint8* RowData = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
			RowStruct->InitializeStruct(RowData);
			bool bRowSuccess = true;
			for (const auto& Pair : (*RowObj)->Values)
			{
				if (Pair.Key == TEXT("row_name") || Pair.Key == TEXT("RowName")) continue;
				FProperty* Property = RowStruct->FindPropertyByName(FName(*Pair.Key));
				if (Property && !Property->IsNative() && Pair.Value.IsValid())
				{
					void* PropertyData = Property->ContainerPtrToValuePtr<void>(RowData);
					FString PropertyValueStr;
					if (Pair.Value->Type == EJson::String)
					{
						PropertyValueStr = Pair.Value->AsString();
					}
					else if (Pair.Value->Type == EJson::Number)
					{
						PropertyValueStr = FString::SanitizeFloat(Pair.Value->AsNumber());
					}
					else if (Pair.Value->Type == EJson::Boolean)
					{
						PropertyValueStr = Pair.Value->AsBool() ? TEXT("True") : TEXT("False");
					}
					else
					{
						PropertyValueStr = Pair.Value->AsString();
					}
					if (!Property->ImportText_Direct(*PropertyValueStr, PropertyData, nullptr, PPF_None))
					{
						Errors.Add(FString::Printf(TEXT("Failed to set property '%s' on row '%s'"), *Pair.Key, *RowName));
						bRowSuccess = false;
					}
				}
			}
			if (bRowSuccess)
			{
				TMap<FName, uint8*>& MutableRowMap = const_cast<TMap<FName, uint8*>&>(DataTable->GetRowMap());
				MutableRowMap.Add(FName(*RowName), RowData);
				RowsAdded++;
			}
			else
			{
				RowStruct->DestroyStruct(RowData);
				FMemory::Free(RowData);
			}
		}
		else
		{
			Errors.Add(TEXT("DataTable has no row struct assigned"));
			break;
		}
	}
	DataTable->MarkPackageDirty();
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("rows_added"), RowsAdded);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Added %d rows to DataTable"), RowsAdded));
	if (Errors.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> ErrorArray;
		for (const FString& Err : Errors)
		{
			ErrorArray.Add(MakeShared<FJsonValueString>(Err));
		}
		ResultObject->SetArrayField(TEXT("warnings"), ErrorArray);
	}
	Result.bSuccess = true;
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	Result.ResultJson = ResultString;
	return Result;
}
void SBpGeneratorUltimateWidget::ProcessArchitectResponse(const FString& AiMessage)
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ProcessArchitectResponse START ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Message length: %d chars"), AiMessage.Len());
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Current depth: %d / %d"), CurrentToolCallDepth, MaxToolCallDepth);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  bIsArchitectThinking: %s"), bIsArchitectThinking ? TEXT("true") : TEXT("false"));
	if (CurrentToolCallDepth >= MaxToolCallDepth)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("Tool call depth limit (%d) reached. Stopping chain."), MaxToolCallDepth);
		FNotificationInfo Info(FText::FromString(TEXT("Tool call chain limit reached. Stopping to prevent infinite loop.")));
		Info.ExpireDuration = 5.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		CurrentToolCallDepth = 0;
		bIsArchitectThinking = false;
		RefreshArchitectChatView();
		return;
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Calling ExtractToolCallFromResponse..."));
	FToolCallExtractionResult ExtractionResult = ExtractToolCallFromResponse(AiMessage);
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Extraction result: bHasToolCall=%s, ToolName=%s"),
		ExtractionResult.bHasToolCall ? TEXT("true") : TEXT("false"),
		*ExtractionResult.ToolName);
	if (ExtractionResult.bHasToolCall)
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  === DISPATCHING TOOL: %s ==="), *ExtractionResult.ToolName);
		if (ExtractionResult.Arguments.IsValid())
		{
			FString ArgsString;
			TSharedRef<TJsonWriter<>> ArgsWriter = TJsonWriterFactory<>::Create(&ArgsString);
			FJsonSerializer::Serialize(ExtractionResult.Arguments.ToSharedRef(), ArgsWriter);
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Arguments: %s"), *ArgsString.Left(500));
		}
		CurrentToolCallDepth++;
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Depth now: %d"), CurrentToolCallDepth);
		FToolExecutionResult ToolResult = DispatchToolCall(ExtractionResult.ToolName, ExtractionResult.Arguments);
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Tool result: bSuccess=%s, bIsPending=%s, Error=%s"),
			ToolResult.bSuccess ? TEXT("true") : TEXT("false"),
			ToolResult.bIsPending ? TEXT("true") : TEXT("false"),
			*ToolResult.ErrorMessage);
		if (ToolResult.bIsPending)
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Tool is pending async completion, waiting for callback..."));
		}
		else
		{
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Calling ContinueConversationWithToolResult..."));
			ContinueConversationWithToolResult(ExtractionResult.ToolName, ToolResult);
			UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  ContinueConversationWithToolResult returned"));
		}
	}
	else
	{
		UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  === NO TOOL CALL - FINAL ANSWER ==="));
		CurrentToolCallDepth = 0;
		bIsArchitectThinking = false;
		RefreshArchitectChatView();
		if (ArchitectTokenCounterText.IsValid())
		{
			int32 TokenCount = EstimateFullRequestTokens(ArchitectConversationHistory);
			ArchitectTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: ~%d"), TokenCount)));
		}
		if (bJustExecutedCodeGenerationTool)
		{
			UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Skipping code block compilation - code generation tool was just executed"));
			bJustExecutedCodeGenerationTool = false;
			return;
		}
		AsyncTask(ENamedThreads::GameThread, [this, AiMessage]()
		{
			FBpGeneratorUltimateModule& BpGeneratorModule = FModuleManager::LoadModuleChecked<FBpGeneratorUltimateModule>("BpGeneratorUltimate");
			UBlueprint* TargetBlueprint = BpGeneratorModule.GetTargetBlueprint();
			if (!TargetBlueprint)
			{
				return;
			}
			const FString CodeDelimiter = TEXT("```");
			int32 StartPos = AiMessage.Find(CodeDelimiter);
			if (StartPos != INDEX_NONE)
			{
				int32 EndPos = AiMessage.Find(CodeDelimiter, ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos + 3);
				if (EndPos != INDEX_NONE)
				{
					FString ExtractedCode = AiMessage.Mid(StartPos + 3, EndPos - (StartPos + 3));
					if (ExtractedCode.StartsWith(TEXT("cpp")))
					{
						ExtractedCode.RightChopInline(3, EAllowShrinking::No);
					}
					FBpGeneratorCompiler Compiler(TargetBlueprint);
					if (Compiler.Compile(ExtractedCode.TrimStartAndEnd()))
					{
						FNotificationInfo Info(LOCTEXT("CompileSuccessNotify", "Blueprint Generation Successful!"));
						Info.ExpireDuration = 5.0f;
						FSlateNotificationManager::Get().AddNotification(Info);
					}
					else
					{
						FNotificationInfo Info(LOCTEXT("CompileFailNotify", "Blueprint Generation Failed. See Output Log for details."));
						Info.ExpireDuration = 8.0f;
						FSlateNotificationManager::Get().AddNotification(Info);
					}
				}
			}
		});
	}
}
void SBpGeneratorUltimateWidget::ContinueConversationWithToolResult(const FString& ToolName, const FToolExecutionResult& Result)
{
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ContinueConversationWithToolResult START ==="));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Tool: %s, Success: %s"), *ToolName, Result.bSuccess ? TEXT("true") : TEXT("false"));
	if (ToolName == TEXT("generate_blueprint_logic") || ToolName == TEXT("insert_code"))
	{
		bJustExecutedCodeGenerationTool = true;
		UE_LOG(LogBpGeneratorUltimate, Log, TEXT("Set bJustExecutedCodeGenerationTool=true for tool: %s"), *ToolName);
	}
	if (ToolName == TEXT("classify_intent") && Result.bSuccess)
	{
		TSharedPtr<FJsonObject> JsonResult;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Result.ResultJson);
		if (FJsonSerializer::Deserialize(Reader, JsonResult) && JsonResult.IsValid())
		{
			FString Intent;
			if (JsonResult->TryGetStringField(TEXT("intent"), Intent))
			{
				UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  classify_intent returned: %s"), *Intent);
				if (Intent == TEXT("CODE") && !bPatternsInjectedForSession)
				{
					InjectPatternsAsSystemMessage();
					UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Injected patterns because intent is CODE"));
				}
			}
		}
	}
	FString ToolResultMessage = FString::Printf(
		TEXT("[TOOL_RESULT:%s:success]\n```json\n%s\n```"),
		*ToolName,
		*Result.ResultJson
	);
	if (!Result.bSuccess && !Result.ErrorMessage.IsEmpty())
	{
		ToolResultMessage = FString::Printf(
			TEXT("[TOOL_RESULT:%s:error]\nError: %s\n```json\n%s\n```"),
			*ToolName,
			*Result.ErrorMessage,
			*Result.ResultJson
		);
	}
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Tool result message length: %d chars"), ToolResultMessage.Len());
	TSharedPtr<FJsonObject> UserContent = MakeShareable(new FJsonObject);
	UserContent->SetStringField(TEXT("role"), TEXT("user"));
	TArray<TSharedPtr<FJsonValue>> UserParts;
	TSharedPtr<FJsonObject> UserPartText = MakeShareable(new FJsonObject);
	UserPartText->SetStringField(TEXT("text"), ToolResultMessage);
	UserParts.Add(MakeShareable(new FJsonValueObject(UserPartText)));
	UserContent->SetArrayField(TEXT("parts"), UserParts);
	ArchitectConversationHistory.Add(MakeShareable(new FJsonValueObject(UserContent)));
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Conversation history now has %d messages"), ArchitectConversationHistory.Num());
	SaveArchitectChatHistory(ActiveArchitectChatID);
	RefreshArchitectChatView();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("  Setting bIsArchitectThinking=true and calling SendArchitectChatRequest..."));
	bIsArchitectThinking = true;
	SendArchitectChatRequest();
	UE_LOG(LogBpGeneratorUltimate, Warning, TEXT("=== ContinueConversationWithToolResult END ==="));
}
FReply SBpGeneratorUltimateWidget::OnToggleSidebarClicked()
{
	SetSidebarVisible(!bSidebarVisible);
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::SetSidebarVisible(bool bVisible)
{
	bSidebarVisible = bVisible;
	if (ArchitectSplitter.IsValid())
	{
		ArchitectSplitter->Invalidate(EInvalidateWidgetReason::Layout);
	}
	if (AnalystSplitter.IsValid())
	{
		AnalystSplitter->Invalidate(EInvalidateWidgetReason::Layout);
	}
	if (ProjectSplitter.IsValid())
	{
		ProjectSplitter->Invalidate(EInvalidateWidgetReason::Layout);
	}
}
bool SBpGeneratorUltimateWidget::IsAnyViewThinking() const
{
	return bIsThinking || bIsProjectThinking || bIsArchitectThinking;
}
EVisibility SBpGeneratorUltimateWidget::GetLoadingIndicatorVisibility() const
{
	return IsAnyViewThinking() ? EVisibility::Visible : EVisibility::Collapsed;
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateBurgerMenuButton()
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(FMargin(10, 5))
		.ForegroundColor(FSlateColor(FLinearColor::White))
		.OnClicked(this, &SBpGeneratorUltimateWidget::OnToggleSidebarClicked)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("☰")))
		.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
		.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateLoadingIndicator()
{
	FString LoadingDots = TEXT(".");
	int32 DotsCount = (int32)(LoadingAnimationTime * 2) % 4;
	for (int32 i = 0; i < DotsCount; i++)
	{
		LoadingDots += TEXT(".");
	}
	FText LoadingText = FText::FromString(FString::Printf(TEXT("AI is thinking%s"), *LoadingDots));
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("⟳")))
		.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
		.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SAssignNew(LoadingIndicatorText, STextBlock)
			.Text(LoadingText)
			.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::Create3DModelGenWidget()
{
	Model3DTypeOptions.Empty();
	Model3DTypeOptions.Add(MakeShared<FString>(TEXT("Meshy v4.0 (3D)")));
	Model3DTypeOptions.Add(MakeShared<FString>(TEXT("Meshy v3.0 (3D + Texture)")));
	Model3DTypeOptions.Add(MakeShared<FString>(TEXT("Meshy v4.0 (Preview Only)")));
	Model3DTypeOptions.Add(MakeShared<FString>(TEXT("Others (OpenAI-compatible)")));
	Model3DTypeOptions.Add(MakeShared<FString>(TEXT("Custom (Specify Endpoint)")));
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(15)
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
				.Padding(15)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ModelGenTitle", "3D Model Generation"))
								.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
								.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ModelGenDesc", "Generate 3D models using various providers. All providers use preview mode with optional auto-refinement to full quality."))
								.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
								.AutoWrapText(true)
						]
				]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(15)
		[
			SNew(SScrollBox) + SScrollBox::Slot()
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("MeshyApiKeyLabel", "Meshy API Key:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SAssignNew(MeshyApiKeyInput, SEditableTextBox)
									.HintText(LOCTEXT("MeshyApiKeyHint", "Enter your Meshy API key (get one at meshy.ai)"))
									.Text_Lambda([this]() { return FText::FromString(MeshyApiKey); })
									.OnTextChanged_Lambda([this](const FText& NewText)
									{
										MeshyApiKey = NewText.ToString();
										GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), *MeshyApiKey, GEditorPerProjectIni);
										GConfig->Flush(false, GEditorPerProjectIni);
									})
									.IsPassword_Lambda([this]() { return !bMeshyApiKeyVisible; })
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SAssignNew(MeshyApiKeyVisibilityButton, SButton)
									.ButtonStyle(FAppStyle::Get(), "NoBorder")
									.ContentPadding(0)
									.OnClicked(this, &SBpGeneratorUltimateWidget::OnToggleMeshyApiKeyVisibility)
									.ToolTipText_Lambda([this]() { return bMeshyApiKeyVisible ? LOCTEXT("HideApiKey", "Hide API Key") : LOCTEXT("ShowApiKey", "Show API Key"); })
									[
										SNew(STextBlock)
											.Text_Lambda([this]() { return FText::FromString(bMeshyApiKeyVisible ? TEXT("👁") : TEXT("")); })
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
											.ColorAndOpacity(FSlateColor(FLinearColor::Gray))
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(15, 10, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("MeshGenModeLabel", "Generation Mode:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
							[
								SNew(SCheckBox)
									.IsChecked_Lambda([this]() { return CurrentMeshGenMode == EMeshGenMode::TextTo3D ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged(this, &SBpGeneratorUltimateWidget::OnTextTo3DModeClicked)
									.Style(FAppStyle::Get(), "RadioButton")
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("TextTo3DMode", "Text to 3D"))
											.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									]
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SCheckBox)
									.IsChecked_Lambda([this]() { return CurrentMeshGenMode == EMeshGenMode::ImageTo3D ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
									.OnCheckStateChanged(this, &SBpGeneratorUltimateWidget::OnImageTo3DModeClicked)
									.Style(FAppStyle::Get(), "RadioButton")
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("ImageTo3DMode", "Image to 3D"))
											.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(15, 10, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ModelPromptLabel", "Prompt:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SAssignNew(ModelPromptInput, SMultiLineEditableTextBox)
							.HintText_Lambda([this]()
							{
								return CurrentMeshGenMode == EMeshGenMode::ImageTo3D ?
									LOCTEXT("ModelPromptHintImage", "Optional additional description to guide conversion (e.g., 'add more details to the handle')...") :
									LOCTEXT("ModelPromptHint", "Describe the 3D model you want to generate (e.g., 'a detailed fantasy sword with ornate engravings and a leather wrapped hilt')...");
							})
							.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 5, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("SourceImageLabel", "Source Image:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SAssignNew(SourceImagePreview, SImage)
									.Visibility_Lambda([this]()
									{
										return bHasSourceImage ? EVisibility::Visible : EVisibility::Collapsed;
									})
									.DesiredSizeOverride(FVector2D(100, 100))
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 5, 0)
									[
										SAssignNew(BrowseSourceImageButton, SButton)
											.Text(LOCTEXT("BrowseImageBtn", "Browse..."))
											.OnClicked(this, &SBpGeneratorUltimateWidget::OnBrowseSourceImageClicked)
											.ButtonColorAndOpacity(FSlateColor(CurrentTheme.SecondaryColor.CopyWithNewOpacity(0.6f)))
											.ForegroundColor(FSlateColor(FLinearColor::White))
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
									[
										SAssignNew(RemoveSourceImageButton, SButton)
											.Text(LOCTEXT("RemoveImageBtn", "Remove"))
											.OnClicked(this, &SBpGeneratorUltimateWidget::OnRemoveSourceImageClicked)
											.ButtonColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f).CopyWithNewOpacity(0.6f)))
											.ForegroundColor(FSlateColor(FLinearColor::White))
											.Visibility_Lambda([this]() { return bHasSourceImage ? EVisibility::Visible : EVisibility::Collapsed; })
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
									[
										SAssignNew(SourceImagePathText, STextBlock)
											.Text_Lambda([this]()
											{
												if (bHasSourceImage && !SourceImageAttachment.Name.IsEmpty())
												{
													return FText::FromString(FPaths::GetBaseFilename(SourceImageAttachment.Name));
												}
												return FText::GetEmpty();
											})
											.AutoWrapText(true)
											.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ModelNameLabel", "Model Name:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SAssignNew(ModelNameInput, SEditableTextBox)
							.HintText(LOCTEXT("ModelNameHint", "SM_MyGeneratedModel (optional)"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ModelSavePathLabel", "Save Path:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SAssignNew(ModelSavePathInput, SEditableTextBox)
							.HintText(LOCTEXT("ModelSavePathHint", "/Game/GeneratedModels"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SAssignNew(ModelAutoRefineCheckBox, SCheckBox)
									.IsChecked(ECheckBoxState::Checked)
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("AutoRefineLabel", "Auto-Refine to Full Quality"))
											.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 5, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ModelTypeLabel", "3D Model Type:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SAssignNew(Model3DTypeComboBox, STextComboBox)
									.OptionsSource(&Model3DTypeOptions)
									.InitiallySelectedItem(Model3DTypeOptions[0])
									.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
									{
										if (NewSelection.IsValid())
										{
											OnModel3DTypeChanged(NewSelection, SelectInfo);
										}
									})
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 5)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SAssignNew(ModelPreviewQualityCheckBox, SCheckBox)
									.IsChecked_Lambda([this]()
									{
										return FApiKeyManager::Get().GetActiveSelectedQuality() == EMeshQuality::Preview ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
									})
									.OnCheckStateChanged(this, &SBpGeneratorUltimateWidget::OnModelQualityChanged)
									.Content()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("ModelQualityLabel", "Preview Mode (Auto-Refine if checked)"))
											.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 5, 10, 5)
					[
						SAssignNew(MeshGenCustomFieldsWrapper, SVerticalBox)
							.Visibility_Lambda([this]()
							{
								E3DModelType CurrentType = FApiKeyManager::Get().GetActiveSelected3DModel();
								return (CurrentType == E3DModelType::custom || CurrentType == E3DModelType::others) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CustomEndpointLabel", "Custom Endpoint:"))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
							[
								SAssignNew(MeshGenCustomEndpointInput, SEditableTextBox)
									.HintText_Lambda([this]()
									{
										return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenEndpoint());
									})
									.Text_Lambda([this]() { return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenEndpoint()); })
									.OnTextChanged_Lambda([this](const FText& NewText)
									{
										FApiKeySlot Slot = FApiKeyManager::Get().GetSlot(FApiKeyManager::Get().GetActiveSlotIndex());
										Slot.MeshGenEndpoint = NewText.ToString();
										FApiKeyManager::Get().SetSlot(FApiKeyManager::Get().GetActiveSlotIndex(), Slot);
									})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(10, 5, 10, 5)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CustomModelNameLabel", "Model Name:"))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
							[
								SAssignNew(MeshGenCustomModelNameInput, SEditableTextBox)
									.HintText_Lambda([this]()
									{
										return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenModel());
									})
									.Text_Lambda([this]() { return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenModel()); })
									.OnTextChanged_Lambda([this](const FText& NewText)
									{
										FApiKeySlot Slot = FApiKeyManager::Get().GetSlot(FApiKeyManager::Get().GetActiveSlotIndex());
										Slot.MeshGenModel = NewText.ToString();
										FApiKeyManager::Get().SetSlot(FApiKeyManager::Get().GetActiveSlotIndex(), Slot);
									})
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(10, 5, 10, 5)
							[
								SNew(STextBlock)
									.Text(LOCTEXT("CustomModelFormatLabel", "Output Format:"))
									.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
							[
								SAssignNew(MeshGenCustomFormatInput, SEditableTextBox)
									.HintText_Lambda([this]()
									{
										return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenFormat());
									})
									.Text_Lambda([this]() { return FText::FromString(FApiKeyManager::Get().GetActiveMeshGenFormat()); })
									.OnTextChanged_Lambda([this](const FText& NewText)
									{
										FApiKeySlot Slot = FApiKeyManager::Get().GetSlot(FApiKeyManager::Get().GetActiveSlotIndex());
										Slot.MeshGenFormat = NewText.ToString();
										FApiKeyManager::Get().SetSlot(FApiKeyManager::Get().GetActiveSlotIndex(), Slot);
									})
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(15)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0)
							[
								SAssignNew(GenerateModelButton, SButton)
									.Text(LOCTEXT("GenerateModelBtn", "Generate 3D Model"))
									.HAlign(HAlign_Center)
									.OnClicked(this, &SBpGeneratorUltimateWidget::OnGenerateModelClicked)
									.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.6f)))
									.ForegroundColor(FSlateColor(FLinearColor::White))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
							[
								SAssignNew(CancelModelGenButton, SButton)
									.Text(LOCTEXT("CancelModelBtn", "Cancel"))
									.HAlign(HAlign_Center)
									.OnClicked(this, &SBpGeneratorUltimateWidget::OnCancelModelGenClicked)
									.ButtonColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f).CopyWithNewOpacity(0.6f)))
									.ForegroundColor(FSlateColor(FLinearColor::White))
									.Visibility_Lambda([this]() { return bIsModelGenerating ? EVisibility::Visible : EVisibility::Collapsed; })
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
							.Padding(10)
							[
								SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(STextBlock)
											.Text(LOCTEXT("ModelStatusLabel", "Status:"))
											.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
											.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
									[
										SAssignNew(ModelGenStatusText, STextBlock)
											.Text(FText::FromString("Ready to generate. Enter a prompt above and click Generate."))
											.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
											.AutoWrapText(true)
									]
							]
					]
			]
		];
}
TSharedRef<SWidget> SBpGeneratorUltimateWidget::CreateTextureGenWidget()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(15)
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
				.Padding(15)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(LOCTEXT("TextureGenTitle", "Texture Generation"))
								.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
								.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryColor))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("TextureGenDesc", "Generate textures using Google Imagen API. Supports multiple aspect ratios and automatic material creation."))
								.ColorAndOpacity(FSlateColor(CurrentTheme.SecondaryTextColor))
								.AutoWrapText(true)
						]
				]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(15)
		[
			SNew(SScrollBox) + SScrollBox::Slot()
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("PromptLabel", "Prompt:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SAssignNew(TexturePromptInput, SEditableTextBox)
							.HintText(LOCTEXT("PromptHint", "Describe the texture you want to generate (e.g., 'a futuristic sci-fi metal surface with glowing blue lines')..."))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10, 10, 10, 5)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("AspectRatioLabel", "Aspect Ratio:"))
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
					[
						SAssignNew(AspectRatioComboBox, STextComboBox)
							.OptionsSource(&AspectRatioOptions)
							.InitiallySelectedItem(AspectRatioOptions[0])
							.OnSelectionChanged(this, &SBpGeneratorUltimateWidget::OnAspectRatioChanged)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0)
							[
								SAssignNew(GenerateTextureButton, SButton)
									.Text(LOCTEXT("GenerateBtn", "Generate Texture"))
									.HAlign(HAlign_Center)
									.OnClicked(this, &SBpGeneratorUltimateWidget::OnGenerateTextureClicked)
									.ButtonColorAndOpacity(FSlateColor(CurrentTheme.TertiaryColor.CopyWithNewOpacity(0.6f)))
									.ForegroundColor(FSlateColor(FLinearColor::White))
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(5, 0)
							[
								SAssignNew(CancelTextureGenButton, SButton)
									.Text(LOCTEXT("CancelBtn", "Cancel"))
									.HAlign(HAlign_Center)
									.OnClicked(this, &SBpGeneratorUltimateWidget::OnCancelTextureGenClicked)
									.ButtonColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f).CopyWithNewOpacity(0.6f)))
									.ForegroundColor(FSlateColor(FLinearColor::White))
									.Visibility_Lambda([this]() { return bIsTextureGenerating ? EVisibility::Visible : EVisibility::Collapsed; })
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SNew(SBox)
							.HeightOverride(100)
							.MinDesiredWidth(100)
							.MaxDesiredHeight(300)
							[
								SNew(SBorder)
									.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
									.BorderBackgroundColor(CurrentTheme.BorderBackgroundColor)
									[
										SNew(SOverlay)
											+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
											[
												SNew(STextBlock)
													.Text(LOCTEXT("PreviewPlaceholder", "Texture Preview"))
													.ColorAndOpacity(FSlateColor(CurrentTheme.HintTextColor))
													.Visibility_Lambda([this]() {
														return bHasGeneratedTexture ? EVisibility::Collapsed : EVisibility::Visible;
													})
											]
											+ SOverlay::Slot()
											[
												SAssignNew(TexturePreviewImage, SImage)
													.Visibility_Lambda([this]() {
														return bHasGeneratedTexture ? EVisibility::Visible : EVisibility::Collapsed;
													})
											]
									]
							]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(10)
					[
						SAssignNew(TextureGenStatusText, STextBlock)
							.Text(FText::GetEmpty())
							.ColorAndOpacity(FSlateColor(CurrentTheme.PrimaryTextColor))
							.AutoWrapText(true)
					]
			]
		];
}
FReply SBpGeneratorUltimateWidget::OnGenerateTextureClicked()
{
	if (!TexturePromptInput.IsValid()) return FReply::Handled();
	FString Prompt = TexturePromptInput->GetText().ToString();
	if (Prompt.IsEmpty())
	{
		UpdateTextureGenStatus(LOCTEXT("ErrorEmptyPrompt", "Please enter a prompt"), FLinearColor(1.0f, 0.3f, 0.3f));
		return FReply::Handled();
	}
	FString ApiKey = FApiKeyManager::Get().GetActiveApiKey();
	if (ApiKey.IsEmpty())
	{
		UpdateTextureGenStatus(LOCTEXT("ErrorNoApiKey", "No API key configured. Please add a Google Imagen API key in settings."), FLinearColor(1.0f, 0.3f, 0.3f));
		return FReply::Handled();
	}
	FString AspectRatio = AspectRatioComboBox.IsValid() && AspectRatioComboBox->GetSelectedItem().IsValid() ? *AspectRatioComboBox->GetSelectedItem() : TEXT("1:1");
	FTextureGenRequest Request;
	Request.Prompt = Prompt;
	Request.AspectRatio = AspectRatio;
	Request.NegativePrompt = TEXT("");
	Request.Seed = -1;
	bIsTextureGenerating = true;
	UpdateTextureGenStatus(LOCTEXT("StatusGenerating", "Generating texture... This may take 30-60 seconds."), FLinearColor(1.0f, 0.8f, 0.0f));
	if (GenerateTextureButton.IsValid()) GenerateTextureButton->SetEnabled(false);
	if (CancelTextureGenButton.IsValid()) CancelTextureGenButton->SetVisibility(EVisibility::Visible);
	FTextureGenManager::Get().GenerateTexture(Request, ApiKey,
		[this](const FTextureGenResult& Result)
		{
			bIsTextureGenerating = false;
			if (GenerateTextureButton.IsValid()) GenerateTextureButton->SetEnabled(true);
			if (CancelTextureGenButton.IsValid()) CancelTextureGenButton->SetVisibility(EVisibility::Collapsed);
			if (Result.bSuccess)
			{
				LastTextureGenResult = Result;
				bHasGeneratedTexture = true;
				FText SuccessMsg = FText::Format(LOCTEXT("SuccessFormat", "Texture generated successfully!\n\nAsset: {0}\nMaterial: {1}"), FText::FromString(Result.AssetPath), FText::FromString(Result.MaterialPath));
				UpdateTextureGenStatus(SuccessMsg, FLinearColor(0.3f, 1.0f, 0.3f));
			}
			else
			{
				FText ErrorMsg = FText::Format(LOCTEXT("ErrorFormat", "Failed to generate texture: {0}"), FText::FromString(Result.ErrorMessage));
				UpdateTextureGenStatus(ErrorMsg, FLinearColor(1.0f, 0.3f, 0.3f));
			}
		});
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCancelTextureGenClicked()
{
	if (bIsTextureGenerating)
	{
		FTextureGenManager::Get().CancelGeneration();
		bIsTextureGenerating = false;
		UpdateTextureGenStatus(LOCTEXT("StatusCancelled", "Texture generation cancelled."), FLinearColor(1.0f, 0.6f, 0.0f));
		if (GenerateTextureButton.IsValid()) GenerateTextureButton->SetEnabled(true);
		if (CancelTextureGenButton.IsValid()) CancelTextureGenButton->SetVisibility(EVisibility::Collapsed);
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnGenerateModelClicked()
{
	if (!ModelPromptInput.IsValid()) return FReply::Handled();
	FString Prompt = ModelPromptInput->GetText().ToString();
	FString ApiKey = MeshyApiKey;
	if (ApiKey.IsEmpty())
	{
		UpdateModelGenStatus(LOCTEXT("ErrorNoApiKey", "Please enter your Meshy API key above. Get one at meshy.ai"), FLinearColor(1.0f, 0.3f, 0.3f));
		return FReply::Handled();
	}
	if (CurrentMeshGenMode == EMeshGenMode::ImageTo3D)
	{
		if (!bHasSourceImage)
		{
			UpdateModelGenStatus(LOCTEXT("ErrorNoImage", "Please select a source image first"), FLinearColor(1.0f, 0.3f, 0.3f));
			return FReply::Handled();
		}
		FString ModelName = ModelNameInput.IsValid() ? ModelNameInput->GetText().ToString() : TEXT("");
		FString SavePath;
		if (ModelSavePathInput.IsValid())
		{
			SavePath = ModelSavePathInput->GetText().ToString();
			if (SavePath.IsEmpty())
			{
				SavePath = TEXT("/Game/GeneratedModels");
			}
		}
		else
		{
			SavePath = TEXT("/Game/GeneratedModels");
		}
		bool bAutoRefine = true;
		if (ModelAutoRefineCheckBox.IsValid())
		{
			bAutoRefine = ModelAutoRefineCheckBox->IsChecked();
		}
		E3DModelType SelectedModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
		EMeshQuality SelectedQuality = FApiKeyManager::Get().GetActiveSelectedQuality();
		FString CustomEndpoint;
		FString CustomModel;
		FString CustomFormat;
		if (SelectedModelType == E3DModelType::custom || SelectedModelType == E3DModelType::others)
		{
			CustomEndpoint = FApiKeyManager::Get().GetActiveMeshGenEndpoint();
			CustomModel = FApiKeyManager::Get().GetActiveMeshGenModel();
			CustomFormat = FApiKeyManager::Get().GetActiveMeshGenFormat();
		}
		FMeshCreationRequest Request;
		Request.Prompt = Prompt;
		Request.ImageBase64Data = SourceImageAttachment.Base64Data;
		Request.CustomAssetName = ModelName;
		Request.SavePath = SavePath;
		Request.bAutoRefine = bAutoRefine;
		Request.Seed = -1;
		Request.ModelType = SelectedModelType;
		Request.Quality = SelectedQuality;
		Request.CustomEndpoint = CustomEndpoint;
		Request.CustomModelName = CustomModel;
		Request.ModelFormat = CustomFormat.IsEmpty() ? TEXT("glb") : CustomFormat;
		bIsModelGenerating = true;
		bIsPreviewMode = true;
		UpdateModelGenStatus(LOCTEXT("StatusGeneratingImage", "Generating 3D model from image... This may take 2-5 minutes."), FLinearColor(1.0f, 0.8f, 0.0f));
		UpdateModelGenButtons();
		FMeshAssetManager::Get().CreateMeshFromImage(Request, ApiKey,
			[this](const FMeshCreationResult& Result)
			{
				bIsModelGenerating = false;
				bIsPreviewMode = true;
				UpdateModelGenButtons();
				if (Result.bSuccess)
				{
					CurrentModelTaskId = Result.TaskId;
					bIsPreviewMode = Result.bIsPreview;
					FText SuccessMsg = FText::Format(LOCTEXT("PreviewSuccessImageFormat", "3D model generated from image!\n\nTask ID: {0}"), FText::FromString(Result.TaskId));
					UpdateModelGenStatus(SuccessMsg, FLinearColor(0.3f, 1.0f, 0.3f));
					FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ 3D Model Generated: %s"), *Result.AssetPath)));
					Info.ExpireDuration = 5.0f;
					Info.bUseSuccessFailIcons = true;
					FSlateNotificationManager::Get().AddNotification(Info);
				}
				else
				{
					UpdateModelGenStatus(FText::FromString(Result.ErrorMessage), FLinearColor(1.0f, 0.3f, 0.3f));
					FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Model Generation Failed: %s"), *Result.ErrorMessage)));
					Info.ExpireDuration = 10.0f;
					Info.bUseSuccessFailIcons = true;
					FSlateNotificationManager::Get().AddNotification(Info);
				}
			});
		return FReply::Handled();
	}
	if (Prompt.IsEmpty())
	{
		UpdateModelGenStatus(LOCTEXT("ErrorEmptyPrompt", "Please enter a prompt"), FLinearColor(1.0f, 0.3f, 0.3f));
		return FReply::Handled();
	}
	FString ModelName = ModelNameInput.IsValid() ? ModelNameInput->GetText().ToString() : TEXT("");
	FString SavePath;
	if (ModelSavePathInput.IsValid())
	{
		SavePath = ModelSavePathInput->GetText().ToString();
		if (SavePath.IsEmpty())
		{
			SavePath = TEXT("/Game/GeneratedModels");
		}
	}
	else
	{
		SavePath = TEXT("/Game/GeneratedModels");
	}
	bool bAutoRefine = true;
	if (ModelAutoRefineCheckBox.IsValid())
	{
		bAutoRefine = ModelAutoRefineCheckBox->IsChecked();
	}
	E3DModelType SelectedModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
	EMeshQuality SelectedQuality = FApiKeyManager::Get().GetActiveSelectedQuality();
	FString CustomEndpoint;
	FString CustomModel;
	FString CustomFormat;
	if (SelectedModelType == E3DModelType::custom || SelectedModelType == E3DModelType::others)
	{
		CustomEndpoint = FApiKeyManager::Get().GetActiveMeshGenEndpoint();
		CustomModel = FApiKeyManager::Get().GetActiveMeshGenModel();
		CustomFormat = FApiKeyManager::Get().GetActiveMeshGenFormat();
	}
	GConfig->SetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), *ApiKey, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
	if (ModelName.IsEmpty())
	{
		ModelName = FString::Printf(TEXT("SM_Generated_%s"), *FGuid::NewGuid().ToString().Left(8));
	}
	FMeshCreationRequest Request;
	Request.Prompt = Prompt;
	Request.CustomAssetName = ModelName;
	Request.SavePath = SavePath;
	Request.bAutoRefine = bAutoRefine;
	Request.Seed = -1;
	Request.ModelType = SelectedModelType;
	Request.Quality = SelectedQuality;
	Request.CustomEndpoint = CustomEndpoint;
	Request.CustomModelName = CustomModel;
	Request.ModelFormat = CustomFormat.IsEmpty() ? TEXT("glb") : CustomFormat;
	bIsModelGenerating = true;
	bIsPreviewMode = true;
	UpdateModelGenStatus(LOCTEXT("StatusGenerating", "Generating 3D model preview... This may take 2-5 minutes."), FLinearColor(1.0f, 0.8f, 0.0f));
	if (GenerateModelButton.IsValid()) GenerateModelButton->SetEnabled(false);
	if (CancelModelGenButton.IsValid()) CancelModelGenButton->SetVisibility(EVisibility::Visible);
	FMeshAssetManager::Get().CreateMeshFromText(Request, ApiKey,
		[this](const FMeshCreationResult& Result)
		{
			bIsModelGenerating = false;
			if (GenerateModelButton.IsValid()) GenerateModelButton->SetEnabled(true);
			if (CancelModelGenButton.IsValid()) CancelModelGenButton->SetVisibility(EVisibility::Collapsed);
			if (Result.bSuccess)
			{
				CurrentModelTaskId = Result.TaskId;
				bIsPreviewMode = Result.bIsPreview;
				FText SuccessMsg;
				if (Result.bIsPreview)
				{
					SuccessMsg = FText::Format(LOCTEXT("PreviewSuccessFormat", "Preview model generated!\n\nTask ID: {0}\n\nRefining to full quality..."), FText::FromString(Result.TaskId));
				}
				else
				{
					SuccessMsg = FText::Format(LOCTEXT("ModelSuccessFormat", "3D Model generated successfully!\n\nAsset: {0}"), FText::FromString(Result.AssetPath));
				}
				UpdateModelGenStatus(SuccessMsg, FLinearColor(0.3f, 1.0f, 0.3f));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✓ 3D Model Generated: %s"), *Result.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				UpdateModelGenStatus(FText::FromString(Result.ErrorMessage), FLinearColor(1.0f, 0.3f, 0.3f));
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("✗ Model Generation Failed: %s"), *Result.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		});
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnCancelModelGenClicked()
{
	if (bIsModelGenerating)
	{
		FMeshAssetManager::Get().CancelCreation();
		bIsModelGenerating = false;
		UpdateModelGenStatus(LOCTEXT("StatusCancelled", "3D model generation cancelled."), FLinearColor(1.0f, 0.6f, 0.0f));
		if (GenerateModelButton.IsValid()) GenerateModelButton->SetEnabled(true);
		if (CancelModelGenButton.IsValid()) CancelModelGenButton->SetVisibility(EVisibility::Collapsed);
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::UpdateModelGenButtons()
{
	if (GenerateModelButton.IsValid()) GenerateModelButton->SetEnabled(false);
	if (CancelModelGenButton.IsValid()) CancelModelGenButton->SetVisibility(EVisibility::Visible);
}
int32 SBpGeneratorUltimateWidget::EstimateConversationTokens(const TArray<TSharedPtr<FJsonValue>>& ConversationHistory)
{
	int32 TotalChars = 0;
	for (const TSharedPtr<FJsonValue>& Message : ConversationHistory)
	{
		const TSharedPtr<FJsonObject>& MsgObj = Message->AsObject();
		FString Role;
		if (!MsgObj->TryGetStringField(TEXT("role"), Role)) continue;
		if (Role == TEXT("context"))
		{
			FString Content;
			if (MsgObj->TryGetStringField(TEXT("content"), Content))
			{
				TotalChars += Content.Len();
			}
			continue;
		}
		const TArray<TSharedPtr<FJsonValue>>* PartsArray;
		if (MsgObj->TryGetArrayField(TEXT("parts"), PartsArray))
		{
			for (const TSharedPtr<FJsonValue>& Part : *PartsArray)
			{
				FString Text;
				if (Part->AsObject()->TryGetStringField(TEXT("text"), Text))
				{
					TotalChars += Text.Len();
				}
			}
		}
	}
	return TotalChars / 4;
}
void SBpGeneratorUltimateWidget::UpdateConversationTokens(const FString& ChatID, int32 PromptTokens, int32 CompletionTokens)
{
	if (ChatID.IsEmpty()) return;
	bool bFound = false;
	for (auto& Conv : ConversationList)
	{
		if (Conv->ID == ChatID)
		{
			Conv->TotalPromptTokens += PromptTokens;
			Conv->TotalCompletionTokens += CompletionTokens;
			Conv->TotalTokens = Conv->TotalPromptTokens + Conv->TotalCompletionTokens;
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		for (auto& Conv : ProjectConversationList)
		{
			if (Conv->ID == ChatID)
			{
				Conv->TotalPromptTokens += PromptTokens;
				Conv->TotalCompletionTokens += CompletionTokens;
				Conv->TotalTokens = Conv->TotalPromptTokens + Conv->TotalCompletionTokens;
				bFound = true;
				break;
			}
		}
	}
	if (!bFound)
	{
		for (auto& Conv : ArchitectConversationList)
		{
			if (Conv->ID == ChatID)
			{
				Conv->TotalPromptTokens += PromptTokens;
				Conv->TotalCompletionTokens += CompletionTokens;
				Conv->TotalTokens = Conv->TotalPromptTokens + Conv->TotalCompletionTokens;
				bFound = true;
				break;
			}
		}
	}
	if (ChatListView.IsValid()) ChatListView->RequestListRefresh();
	if (ProjectChatListView.IsValid()) ProjectChatListView->RequestListRefresh();
	if (ArchitectChatListView.IsValid()) ArchitectChatListView->RequestListRefresh();
	if (ConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ChatID; }) != nullptr)
	{
		SaveManifest();
	}
	else if (ProjectConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ChatID; }) != nullptr)
	{
		SaveProjectManifest();
	}
	else if (ArchitectConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ChatID; }) != nullptr)
	{
		SaveArchitectManifest();
	}
	if (ArchitectTokenCounterText.IsValid() && ActiveArchitectChatID == ChatID)
	{
		TSharedPtr<FConversationInfo>* ActiveConv = ArchitectConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ChatID; });
		if (ActiveConv != nullptr)
		{
			ArchitectTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: %d"), (*ActiveConv)->TotalTokens)));
		}
	}
	if (AnalystTokenCounterText.IsValid() && ActiveChatID == ChatID)
	{
		TSharedPtr<FConversationInfo>* ActiveConv = ConversationList.FindByPredicate([&](const TSharedPtr<FConversationInfo>& Info) { return Info->ID == ChatID; });
		if (ActiveConv != nullptr)
		{
			AnalystTokenCounterText->SetText(FText::FromString(FString::Printf(TEXT("Tokens: %d"), (*ActiveConv)->TotalTokens)));
		}
	}
}
void SBpGeneratorUltimateWidget::UpdateModelGenStatus(const FText& Status, const FLinearColor& Color)
{
	if (ModelGenStatusText.IsValid())
	{
		ModelGenStatusText->SetText(Status);
		ModelGenStatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
void SBpGeneratorUltimateWidget::OnTextureGenResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
}
void SBpGeneratorUltimateWidget::OnAspectRatioChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
}
void SBpGeneratorUltimateWidget::UpdateTextureGenStatus(const FText& Status, const FLinearColor& Color)
{
	if (TextureGenStatusText.IsValid())
	{
		TextureGenStatusText->SetText(Status);
		TextureGenStatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
FReply SBpGeneratorUltimateWidget::OnToggleMeshyApiKeyVisibility()
{
	bMeshyApiKeyVisible = !bMeshyApiKeyVisible;
	if (MeshyApiKeyInput.IsValid())
	{
		MeshyApiKeyInput->SetText(FText::FromString(MeshyApiKey));
	}
	if (MeshyApiKeyVisibilityButton.IsValid())
	{
		MeshyApiKeyVisibilityButton->SetToolTipText(bMeshyApiKeyVisible ? LOCTEXT("HideApiKey", "Hide API Key") : LOCTEXT("ShowApiKey", "Show API Key"));
	}
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::OnModel3DTypeChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid()) return;
	E3DModelType NewType;
	if (*NewSelection == TEXT("Meshy v4.0 (3D)"))
	{
		NewType = E3DModelType::Meshy_v4_0_3d;
	}
	else if (*NewSelection == TEXT("Meshy v3.0 (3D + Texture)"))
	{
		NewType = E3DModelType::meshy_v4_0_3d_tex;
	}
	else if (*NewSelection == TEXT("Meshy v4.0 (Preview Only)"))
	{
		NewType = E3DModelType::meshy_v4_0_3d;
	}
	else if (*NewSelection == TEXT("Others (OpenAI-compatible)"))
	{
		NewType = E3DModelType::others;
	}
	else if (*NewSelection == TEXT("Custom (Specify Endpoint)"))
	{
		NewType = E3DModelType::custom;
	}
	else
	{
		return;
	}
	FApiKeyManager::Get().SetActiveSelected3DModel(NewType);
}
void SBpGeneratorUltimateWidget::OnModelQualityChanged(ECheckBoxState NewState)
{
	EMeshQuality NewQuality = (NewState == ECheckBoxState::Checked) ? EMeshQuality::Preview : EMeshQuality::Full;
	FApiKeyManager::Get().SetActiveSelectedQuality(NewQuality);
}
void SBpGeneratorUltimateWidget::OnTextTo3DModeClicked(ECheckBoxState NewState)
{
	CurrentMeshGenMode = EMeshGenMode::TextTo3D;
}
void SBpGeneratorUltimateWidget::OnImageTo3DModeClicked(ECheckBoxState NewState)
{
	CurrentMeshGenMode = EMeshGenMode::ImageTo3D;
}
FReply SBpGeneratorUltimateWidget::OnBrowseSourceImageClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform) return FReply::Handled();
	TArray<FString> OutFiles;
	const FString FileTypes = TEXT("Image Files|*.png;*.jpg;*.jpeg;*.bmp");
	bool bOpened = DesktopPlatform->OpenFileDialog(
		FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
		TEXT("Select Source Image"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OutFiles
	);
	if (bOpened && OutFiles.Num() > 0)
	{
		FString ImagePath = OutFiles[0];
		FString FileName = FPaths::GetCleanFilename(ImagePath);
		FString Extension = FPaths::GetExtension(ImagePath).ToLower();
		TArray<uint8> ImageData;
		if (FFileHelper::LoadFileToArray(ImageData, *ImagePath))
		{
			FString MimeType = TEXT("image/png");
			if (Extension == TEXT("jpg") || Extension == TEXT("jpeg"))
			{
				MimeType = TEXT("image/jpeg");
			}
			else if (Extension == TEXT("bmp"))
			{
				MimeType = TEXT("image/bmp");
			}
			FString Base64Data = FBase64::Encode(ImageData);
			SourceImageAttachment.Name = FileName;
			SourceImageAttachment.MimeType = MimeType;
			SourceImageAttachment.Base64Data = Base64Data;
			bHasSourceImage = true;
			RefreshSourceImagePreview();
		}
	}
	return FReply::Handled();
}
FReply SBpGeneratorUltimateWidget::OnRemoveSourceImageClicked()
{
	SourceImageAttachment = FAttachedImage();
	bHasSourceImage = false;
	RefreshSourceImagePreview();
	return FReply::Handled();
}
void SBpGeneratorUltimateWidget::RefreshSourceImagePreview()
{
	if (!SourceImagePreview.IsValid()) return;
	if (bHasSourceImage && !SourceImageAttachment.Base64Data.IsEmpty())
	{
		TArray<uint8> ImageData;
		FBase64::Decode(SourceImageAttachment.Base64Data, ImageData);
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		EImageFormat DetectedFormat = ImageWrapperModule.DetectImageFormat(ImageData.GetData(), ImageData.Num());
		if (DetectedFormat != EImageFormat::Invalid)
		{
			TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(DetectedFormat);
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
			{
				TArray<uint8> RawData;
				if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
				{
					UTexture2D* Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
					if (Texture)
					{
						Texture->Filter = TF_Default;
						Texture->AddToRoot();
						FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
						void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
						Mip.BulkData.Unlock();
						Texture->UpdateResource();
						SourceImageBrush = MakeShared<FSlateDynamicImageBrush>(Texture, FVector2D(ImageWrapper->GetWidth(), ImageWrapper->GetHeight()), NAME_None);
						SourceImagePreview->SetImage(SourceImageBrush.Get());
					}
				}
			}
		}
	}
	else
	{
		SourceImageBrush.Reset();
		SourceImagePreview->SetImage(nullptr);
	}
}
#undef LOCTEXT_NAMESPACE
