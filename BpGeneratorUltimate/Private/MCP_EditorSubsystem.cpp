// Copyright 2026, BlueprintsLab, All rights reserved
#include "MCP_EditorSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Async/Async.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "BpGeneratorUltimate.h"
#include "BpGeneratorCompiler.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "JsonUtilities.h"
#include "UObject/SavePackage.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "MaterialNodeDescriber.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ApiKeyManager.h"
#include "AssetReferenceTypes.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Application/SlateApplication.h"
#include "K2Node_Event.h"
#include "BlueprintEditor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_MacroInstance.h"
#include "Misc/FileHelper.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CallFunction.h"
#include "HAL/FileManager.h"
#include "EditorLevelLibrary.h"
#include "EngineUtils.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "WidgetBlueprint.h"
#include "Components/PanelSlot.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "BpGraphDescriber.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "InstancedFoliageActor.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "InstancedFoliage.h"
#include "GraphEditor.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "Editor/UMGEditor/Public/WidgetBlueprintEditor.h"
#include "MaterialGraphDescriber.h"
#include <MaterialGraph/MaterialGraph.h>
#include "UObject/Package.h"
#include "IMaterialEditor.h"
#include "MaterialGraph/MaterialGraphNode.h"
#if WITH_EDITOR
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Engine/Texture.h"
#endif
#include <Editor/MaterialEditor/Private/MaterialEditor.h>
#include "BtGraphDescriber.h"
#include "BehaviorTreeEditor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/ContentWidget.h"
#include "UObject/TextProperty.h"
#include "Components/PanelWidget.h"
#include "ApiKeyManager.h"
#include "TextureGenManager.h"
#include "MeshAssetManager.h"
#include "Engine/DataTable.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "AssetRegistry/AssetData.h"
class FBpSummarizer
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
void UMCP_EditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Warning, TEXT("MCP Editor Subsystem Initializing..."));
    StartServer();
}
void UMCP_EditorSubsystem::Deinitialize()
{
    StopServer();
    UE_LOG(LogTemp, Warning, TEXT("MCP Editor Subsystem Deinitialized."));
    Super::Deinitialize();
}
void UMCP_EditorSubsystem::StartServer()
{
    const FIPv4Address Address = FIPv4Address::Any;
    const int32 BasePort = 9877;
    const int32 MaxPortAttempts = 10;
    for (int32 PortOffset = 0; PortOffset < MaxPortAttempts; PortOffset++)
    {
        int32 Port = BasePort + PortOffset;
        FIPv4Endpoint Endpoint(Address, Port);
        ListenerSocket = FTcpSocketBuilder(TEXT("MCPListenerSocket")).AsReusable().BoundToEndpoint(Endpoint).Listening(8);
        if (ListenerSocket)
        {
            ActualServerPort = Port;
            UE_LOG(LogTemp, Log, TEXT("MCP Server: Listening on port %d"), Port);
            ServerWorker = new FMCPServerWorker(ListenerSocket, this);
            ServerThread = FRunnableThread::Create(ServerWorker, TEXT("MCPServerWorkerThread"));
            return;
        }
        UE_LOG(LogTemp, Warning, TEXT("MCP Server: Port %d is busy, trying next..."), Port);
    }
    UE_LOG(LogTemp, Error, TEXT("MCP Server: Failed to bind to any port from %d to %d. MCP features will be disabled."), BasePort, BasePort + MaxPortAttempts - 1);
}
void UMCP_EditorSubsystem::StopServer()
{
    if (ServerThread)
    {
        ServerWorker->Stop();
        ServerThread->WaitForCompletion();
        delete ServerThread;
        ServerThread = nullptr;
        delete ServerWorker;
        ServerWorker = nullptr;
    }
    if (ListenerSocket)
    {
        ListenerSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
        ListenerSocket = nullptr;
    }
    UE_LOG(LogTemp, Log, TEXT("MCP Server: Stopped."));
}
void UMCP_EditorSubsystem::HandleAIRequest(const FString& BpPath, const FString& CodeFromAI)
{
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBP)
	{
		UE_LOG(LogTemp, Error, TEXT("MCP: HandleAIRequest failed. Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	FBpGeneratorCompiler Compiler(TargetBP);
	Compiler.Compile(CodeFromAI);
}
void UMCP_EditorSubsystem::HandleGenerateBpLogic(const FString& BpPath, const FString& Code, FString& OutError)
{
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBP)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	FBpGeneratorCompiler Compiler(TargetBP);
	if (!Compiler.Compile(Code))
	{
		if (OutError.IsEmpty())
		{
			OutError = TEXT("The Blueprint compiler encountered a critical error and had to stop. Check the Output Log for FATAL messages related to unhandled exceptions.");
		}
	}
}
void UMCP_EditorSubsystem::HandleInsertCode(const FString& BpPath, const FString& AiGeneratedCode, FString& OutError)
{
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBP)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(TargetBP, true);
	if (FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditor))
	{
		const TSet<UObject*> SelectedNodes = BlueprintEditor->GetSelectedNodes();
		if (SelectedNodes.Num() == 1)
		{
			if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedNodes.begin()))
			{
				FBpGeneratorCompiler Compiler(TargetBP);
				FString DummyFunctionCode = FString::Printf(TEXT("void InsertedLogic() { %s }"), *AiGeneratedCode);
				if (!Compiler.InsertCode(Node, DummyFunctionCode))
				{
					OutError = TEXT("Compiler failed to insert code at the selected node.");
				}
				return;
			}
		}
	}
	OutError = TEXT("Insertion failed. You must have a single Blueprint node selected in the active editor.");
}
void UMCP_EditorSubsystem::HandleGetProjectRootPath(FString& OutPath, FString& OutError)
{
	OutPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	if (OutPath.IsEmpty())
	{
		OutError = TEXT("Could not determine the project root path via FPaths::GetProjectFilePath().");
	}
	OutPath.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
}
void UMCP_EditorSubsystem::HandleGetDetailedBlueprintSummary(const FString& BpPath, FString& OutSummary, FString& OutError)
{
	UBlueprint* TargetBP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBP)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	FBpSummarizer Summarizer;
	OutSummary = Summarizer.Summarize(TargetBP);
	if (OutSummary.IsEmpty())
	{
		OutError = TEXT("The summarizer returned an empty string. The Blueprint may be empty or corrupted.");
	}
}
void UMCP_EditorSubsystem::HandleGetMaterialGraphSummary(const FString& MaterialPath, FString& OutSummary, FString& OutError)
{
	UObject* LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *MaterialPath);
	if (!LoadedAsset)
	{
		OutError = FString::Printf(TEXT("Failed to load any asset at path: %s."), *MaterialPath);
		return;
	}
	if (LoadedAsset->IsA(UMaterialInstance::StaticClass()))
	{
		OutError = TEXT("The selected asset is a Material Instance, which does not have a node graph.");
		return;
	}
	UMaterial* TargetMaterial = Cast<UMaterial>(LoadedAsset);
	if (!TargetMaterial)
	{
		OutError = FString::Printf(TEXT("The asset at path '%s' is not a base Material."), *MaterialPath);
		return;
	}
	FMaterialGraphDescriber Describer;
	OutSummary = Describer.Describe(TargetMaterial);
	if (OutSummary.IsEmpty())
	{
		OutError = TEXT("The summarizer returned an empty string or an error occurred.");
	}
}
void UMCP_EditorSubsystem::HandleScanAndIndexProject(FString& OutError)
{
	TArray<FAssetData> GameAssets;
	{
		TPromise<TArray<FAssetData>> AssetPromise;
		TFuture<TArray<FAssetData>> AssetFuture = AssetPromise.GetFuture();
		AsyncTask(ENamedThreads::GameThread, [&AssetPromise]() mutable
		{
			TArray<FAssetData> AssetDataList;
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), AssetDataList, true);
			TArray<FAssetData> BtAssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), BtAssetData, true);
			AssetDataList.Append(BtAssetData);
			TArray<FAssetData> EnumAssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UEnum::StaticClass()->GetClassPathName(), EnumAssetData, true);
			AssetDataList.Append(EnumAssetData);
			TArray<FAssetData> StructAssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UScriptStruct::StaticClass()->GetClassPathName(), StructAssetData, true);
			AssetDataList.Append(StructAssetData);
			TArray<FAssetData> DataAssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UDataAsset::StaticClass()->GetClassPathName(), DataAssetData, true);
			AssetDataList.Append(DataAssetData);
			TArray<FAssetData> DataTableAssetData;
			AssetRegistryModule.Get().GetAssetsByClass(UDataTable::StaticClass()->GetClassPathName(), DataTableAssetData, true);
			AssetDataList.Append(DataTableAssetData);
			TArray<FAssetData> LocalGameAssets;
			for (const FAssetData& Data : AssetDataList)
			{
				if (Data.PackagePath.ToString().StartsWith(TEXT("/Game")))
				{
					LocalGameAssets.Add(Data);
				}
			}
			AssetPromise.SetValue(LocalGameAssets);
		});
		if (AssetFuture.WaitFor(FTimespan::FromSeconds(30.0)))
		{
			GameAssets = AssetFuture.Get();
		}
		else
		{
			OutError = TEXT("Timeout while gathering assets on game thread");
			return;
		}
	}
	TSharedPtr<FJsonObject> IndexObject = MakeShareable(new FJsonObject());
	FBpSummarizer BpSummarizer;
	FBtGraphDescriber BtDescriber;
	int32 WidgetCount = 0, ActorCount = 0, AnimBPCount = 0, BTCount = 0;
	int32 EnumCount = 0, StructCount = 0, InterfaceCount = 0;
	int32 DataAssetCount = 0, DataTableCount = 0;
	int32 ProcessedCount = 0;
	const int32 TotalAssets = GameAssets.Num();
	const int32 BatchSize = 25;
	UE_LOG(LogTemp, Warning, TEXT("MCP: Starting project scan. Found %d total assets in /Game."), TotalAssets);
	for (int32 BatchStart = 0; BatchStart < TotalAssets; BatchStart += BatchSize)
	{
		int32 BatchEnd = FMath::Min(BatchStart + BatchSize, TotalAssets);
		for (int32 i = BatchStart; i < BatchEnd; i++)
		{
			const FAssetData& Data = GameAssets[i];
			ProcessedCount++;
			FString ClassName = Data.AssetClassPath.ToString();
			if (ClassName.Contains(TEXT("AnimSequence")) ||
				ClassName.Contains(TEXT("AnimMontage")) ||
				ClassName.Contains(TEXT("AnimComposite")) ||
				ClassName.Contains(TEXT("SoundWave")) ||
				ClassName.Contains(TEXT("Texture2D")) ||
				ClassName.Contains(TEXT("StaticMesh")) ||
				ClassName.Contains(TEXT("SkeletalMesh")))
			{
				continue;
			}
			UObject* Asset = Data.GetAsset();
			if (!Asset) continue;
			FString Summary;
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
			{
				UClass* ParentClass = Blueprint->ParentClass;
				if (ParentClass)
				{
					if (ParentClass->IsChildOf(UUserWidget::StaticClass())) WidgetCount++;
					else if (ParentClass->IsChildOf(AActor::StaticClass())) ActorCount++;
					else if (ParentClass->IsChildOf(UAnimInstance::StaticClass())) AnimBPCount++;
					else if (ParentClass->IsChildOf(UInterface::StaticClass())) InterfaceCount++;
				}
				UE_LOG(LogTemp, Log, TEXT("MCP: Summarizing Blueprint: %s"), *Asset->GetPathName());
				Summary = BpSummarizer.Summarize(Blueprint);
			}
			else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(Asset))
			{
				BTCount++;
				UE_LOG(LogTemp, Log, TEXT("MCP: Summarizing Behavior Tree: %s"), *Asset->GetPathName());
				Summary = BtDescriber.Describe(BehaviorTree);
			}
			else if (UEnum* Enum = Cast<UEnum>(Asset))
			{
				EnumCount++;
				Summary = TEXT("--- ENUM ---\n**Values:**\n");
				for (int32 j = 0; j < Enum->NumEnums(); j++)
				{
					Summary += FString::Printf(TEXT("- %s\n"), *Enum->GetDisplayNameTextByIndex(j).ToString());
				}
			}
			else if (UScriptStruct* Struct = Cast<UScriptStruct>(Asset))
			{
				StructCount++;
				Summary = FString::Printf(TEXT("--- STRUCT ---\n**Name:** %s\n**Members:**\n"), *Struct->GetName());
				for (TFieldIterator<FProperty> It(Struct); It; ++It)
				{
					Summary += FString::Printf(TEXT("- %s (%s)\n"), *It->GetName(), *It->GetClass()->GetName());
				}
			}
			else if (UDataAsset* DataAsset = Cast<UDataAsset>(Asset))
			{
				DataAssetCount++;
				Summary = FString::Printf(TEXT("--- DATA ASSET ---\n**Path:** %s\n"), *DataAsset->GetPathName());
			}
			else if (UDataTable* DataTable = Cast<UDataTable>(Asset))
			{
				DataTableCount++;
				Summary = FString::Printf(TEXT("--- DATA TABLE ---\n**Path:** %s\n**Row Count:** %d\n"),
					*DataTable->GetPathName(), DataTable->GetRowMap().Num());
			}
			if (!Summary.IsEmpty())
			{
				IndexObject->SetStringField(Asset->GetPathName(), Summary);
			}
		}
		FPlatformProcess::Sleep(0.005f);
	}
	IndexObject->SetNumberField(TEXT("_total_assets"), ProcessedCount);
	IndexObject->SetNumberField(TEXT("_widget_count"), WidgetCount);
	IndexObject->SetNumberField(TEXT("_actor_count"), ActorCount);
	IndexObject->SetNumberField(TEXT("_animbp_count"), AnimBPCount);
	IndexObject->SetNumberField(TEXT("_bt_count"), BTCount);
	IndexObject->SetNumberField(TEXT("_enum_count"), EnumCount);
	IndexObject->SetNumberField(TEXT("_struct_count"), StructCount);
	IndexObject->SetNumberField(TEXT("_interface_count"), InterfaceCount);
	IndexObject->SetNumberField(TEXT("_dataasset_count"), DataAssetCount);
	IndexObject->SetNumberField(TEXT("_datatable_count"), DataTableCount);
	IndexObject->SetStringField(TEXT("_scan_timestamp"), FDateTime::Now().ToIso8601());
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(IndexObject.ToSharedRef(), Writer);
	const FString SavePath = FPaths::ProjectSavedDir() / TEXT("AI") / TEXT("ProjectIndex.json");
	if (FFileHelper::SaveStringToFile(OutputString, *SavePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("MCP: Project scan complete. Processed %d assets. Index saved to %s"), ProcessedCount, *SavePath);
	}
	else
	{
		OutError = FString::Printf(TEXT("Failed to save Project Index file to %s"), *SavePath);
		UE_LOG(LogTemp, Error, TEXT("MCP: %s"), *OutError);
	}
}
void UMCP_EditorSubsystem::HandleAddComponent(const FString& BpPath, const FString& ComponentClass, const FString& ComponentName, FString& OutJsonString, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	UClass* FoundComponentClass = FindFirstObjectSafe<UClass>(*ComponentClass);
	if (!FoundComponentClass)
	{
		FoundComponentClass = FindFirstObjectSafe<UClass>(*(ComponentClass + TEXT("Component")));
	}
	if (!FoundComponentClass || !FoundComponentClass->IsChildOf(UActorComponent::StaticClass()))
	{
		OutError = FString::Printf(TEXT("Component class '%s' could not be found or is not a valid Actor Component."), *ComponentClass);
		return;
	}
	UActorComponent* NewComponentInstance = NewObject<UActorComponent>(GetTransientPackage(), FoundComponentClass, FName(*ComponentName));
	if (!NewComponentInstance)
	{
		OutError = FString::Printf(TEXT("Failed to create new component instance for '%s'."), *ComponentName);
		return;
	}
	TArray<UActorComponent*> ComponentsToAdd;
	ComponentsToAdd.Add(NewComponentInstance);
	FKismetEditorUtilities::FAddComponentsToBlueprintParams Params;
	FKismetEditorUtilities::AddComponentsToBlueprint(TargetBlueprint, ComponentsToAdd, Params);
	UE_LOG(LogTemp, Log, TEXT("Successfully added component '%s' to Blueprint '%s'"), *ComponentName, *BpPath);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("component_name"), ComponentName);
	ResultObject->SetStringField(TEXT("component_class"), ComponentClass);
	ResultObject->SetStringField(TEXT("blueprint_path"), BpPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Added '%s' to '%s'"), *ComponentName, *BpPath));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleExportToFile(const FString& FileName, const FString& FileContent, const FString& FileFormat, FString& OutError)
{
	if (FileName.IsEmpty() || FileContent.IsEmpty())
	{
		OutError = TEXT("File name and content cannot be empty.");
		return;
	}
	FString SafeFileName = FPaths::MakeValidFileName(FileName);
	FString FinalFileName = FString::Printf(TEXT("%s.%s"), *SafeFileName, *FileFormat);
	const FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("Exported Explanations");
	if (!IFileManager::Get().DirectoryExists(*SaveDirectory))
	{
		if (!IFileManager::Get().MakeDirectory(*SaveDirectory, true))
		{
			OutError = FString::Printf(TEXT("Failed to create directory: %s"), *SaveDirectory);
			return;
		}
	}
	const FString FullPath = SaveDirectory / FinalFileName;
	if (FFileHelper::SaveStringToFile(FileContent, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Log, TEXT("MCP: Successfully exported content to file: %s"), *FullPath);
	}
	else
	{
		OutError = FString::Printf(TEXT("Failed to save file to path: %s"), *FullPath);
	}
}
void UMCP_EditorSubsystem::HandleGetDataAssetDetails(const FString& AssetPath, FString& OutDetailsJson, FString& OutError)
{
	UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Asset)
	{
		OutError = FString::Printf(TEXT("Could not load asset at path: %s"), *AssetPath);
		return;
	}
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
	if (UUserDefinedEnum* Enum = Cast<UUserDefinedEnum>(Asset))
	{
		RootObject->SetStringField("type", "Enum");
		TArray<TSharedPtr<FJsonValue>> EnumValues;
		for (int32 i = 0; i < Enum->NumEnums() - 1; ++i)
		{
			FString DisplayName = Enum->GetDisplayNameTextByIndex(i).ToString();
			EnumValues.Add(MakeShareable(new FJsonValueString(DisplayName)));
		}
		RootObject->SetArrayField("values", EnumValues);
	}
	else if (UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Asset))
	{
		RootObject->SetStringField("type", "Struct");
		TArray<TSharedPtr<FJsonValue>> StructMembers;
		const TArray<FStructVariableDescription>& VarDescriptions = FStructureEditorUtils::GetVarDesc(Struct);
		for (const FStructVariableDescription& VarDesc : VarDescriptions)
		{
			TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject());
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
		RootObject->SetArrayField("members", StructMembers);
	}
	else if (Asset->IsA<UBlueprint>())
	{
		UClass* BpClass = Cast<UBlueprint>(Asset)->GeneratedClass;
		if (BpClass)
		{
			RootObject->SetStringField("type", "Blueprint");
			TArray<TSharedPtr<FJsonValue>> BpMembers;
			for (TFieldIterator<FProperty> PropIt(BpClass); PropIt; ++PropIt)
			{
				FProperty* Property = *PropIt;
				if (Property && Property->HasAnyPropertyFlags(CPF_Edit) && Property->GetOwnerClass() == BpClass)
				{
					TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject());
					MemberObject->SetStringField("name", Property->GetName());
					MemberObject->SetStringField("type", Property->GetClass()->GetName());
					BpMembers.Add(MakeShareable(new FJsonValueObject(MemberObject)));
				}
			}
			RootObject->SetArrayField("members", BpMembers);
		}
	}
	else
	{
		RootObject->SetStringField("type", "DataObject");
		TArray<TSharedPtr<FJsonValue>> Members;
		for (TFieldIterator<FProperty> PropIt(Asset->GetClass()); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (Property && Property->HasAnyPropertyFlags(CPF_Edit))
			{
				TSharedPtr<FJsonObject> MemberObject = MakeShareable(new FJsonObject());
				MemberObject->SetStringField("name", Property->GetName());
				MemberObject->SetStringField("type", Property->GetClass()->GetName());
				Members.Add(MakeShareable(new FJsonValueObject(MemberObject)));
			}
		}
		RootObject->SetArrayField("members", Members);
	}
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	OutDetailsJson = OutputString;
}
void UMCP_EditorSubsystem::HandleGetSelectedNodes(FString& OutNodesText, FString& OutError)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		OutError = "Could not get the Asset Editor Subsystem.";
		return;
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
		OutError = "No active asset editor found.";
		return;
	}
	const FName EditorName = ActiveEditor->GetEditorName();
	TSet<UObject*> SelectedNodes;
	bool bFoundEditor = false;
	if (EditorName == FName(TEXT("BlueprintEditor")))
	{
		FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
		SelectedNodes = BlueprintEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		OutNodesText = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("WidgetBlueprintEditor")))
	{
		FWidgetBlueprintEditor* WidgetEditor = static_cast<FWidgetBlueprintEditor*>(ActiveEditor);
		SelectedNodes = WidgetEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		OutNodesText = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("AnimationBlueprintEditor")))
	{
		FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor);
		SelectedNodes = BlueprintEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBpGraphDescriber Describer;
		OutNodesText = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("MaterialEditor")))
	{
		FMaterialEditor* MaterialEditor = static_cast<FMaterialEditor*>(ActiveEditor);
		SelectedNodes = MaterialEditor->GetSelectedNodes();
		bFoundEditor = true;
		FMaterialNodeDescriber Describer;
		OutNodesText = Describer.Describe(SelectedNodes);
	}
	else if (EditorName == FName(TEXT("Behavior Tree")))
	{
		FBehaviorTreeEditor* BehaviorTreeEditor = static_cast<FBehaviorTreeEditor*>(ActiveEditor);
		SelectedNodes = BehaviorTreeEditor->GetSelectedNodes();
		bFoundEditor = true;
		FBtGraphDescriber Describer;
		OutNodesText = Describer.DescribeSelection(SelectedNodes);
	}
	if (!bFoundEditor)
	{
		OutError = FString::Printf(TEXT("The active editor ('%s') is not a supported editor type for node selection."), *EditorName.ToString());
		return;
	}
	if (SelectedNodes.Num() == 0)
	{
		OutNodesText = "No nodes are currently selected in the active graph editor.";
		return;
	}
}
void UMCP_EditorSubsystem::HandleEditDataAssetDefaults(const FString& AssetPath, const TArray<TSharedPtr<FJsonValue>>& Edits, FString& OutError)
{
	UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Asset)
	{
		OutError = FString::Printf(TEXT("Could not load a valid asset at path: %s"), *AssetPath);
		return;
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
		OutError = FString::Printf(TEXT("Could not resolve a valid object to modify for asset: %s"), *AssetPath);
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Edit Asset Defaults")));
	TargetObject->Modify();
	for (const TSharedPtr<FJsonValue>& EditValue : Edits)
	{
		const TSharedPtr<FJsonObject>& EditObject = EditValue->AsObject();
		if (!EditObject.IsValid()) continue;
		FString PropertyName, PropValStr;
		if (!EditObject->TryGetStringField(TEXT("property_name"), PropertyName) ||
			!EditObject->TryGetStringField(TEXT("value"), PropValStr))
		{
			OutError = "Invalid edit operation format. Each edit must have 'property_name' and 'value'.";
			return;
		}
		FProperty* Property = TargetObject->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (!Property)
		{
			OutError = FString::Printf(TEXT("Property '%s' not found on asset '%s'."), *PropertyName, *AssetPath);
			continue;
		}
		void* PropertyData = Property->ContainerPtrToValuePtr<void>(TargetObject);
		if (Property->ImportText_Direct(*PropValStr, PropertyData, nullptr, PPF_None) == nullptr)
		{
			OutError += FString::Printf(TEXT("\nFailed to import value '%s' for property '%s'."), *PropValStr, *PropertyName);
		}
	}
	Asset->MarkPackageDirty();
}
void UMCP_EditorSubsystem::HandleGetAssetSummary(const FString& AssetPath, FString& OutSummary, FString& OutError)
{
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!LoadedAsset)
	{
		OutError = FString::Printf(TEXT("Failed to load any asset at path: %s."), *AssetPath);
		return;
	}
	if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset))
	{
		FBpSummarizer Summarizer;
		OutSummary = Summarizer.Summarize(Blueprint);
	}
	else if (UMaterial* Material = Cast<UMaterial>(LoadedAsset))
	{
		FMaterialGraphDescriber Describer;
		OutSummary = Describer.Describe(Material);
	}
	else if (UBehaviorTree* BehaviorTree = Cast<UBehaviorTree>(LoadedAsset))
	{
		FBtGraphDescriber Describer;
		OutSummary = Describer.Describe(BehaviorTree);
	}
	else
	{
		OutError = FString::Printf(TEXT("The selected asset ('%s') is not a supported type for summary (Blueprint, Material, or Behavior Tree)."), *LoadedAsset->GetClass()->GetName());
		return;
	}
	if (OutSummary.IsEmpty())
	{
		OutError = TEXT("The summarizer returned an empty string. The asset may be empty or corrupted.");
	}
}
void UMCP_EditorSubsystem::HandleGetBehaviorTreeSummary(const FString& BtPath, FString& OutSummary, FString& OutError)
{
	UBehaviorTree* TargetBT = LoadObject<UBehaviorTree>(nullptr, *BtPath);
	if (!TargetBT)
	{
		OutError = FString::Printf(TEXT("Could not load asset as a Behavior Tree at path: %s"), *BtPath);
		return;
	}
	FBtGraphDescriber Describer;
	OutSummary = Describer.Describe(TargetBT);
	if (OutSummary.IsEmpty())
	{
		OutError = TEXT("The summarizer returned an empty string. The Behavior Tree may be empty or corrupted.");
	}
}
void UMCP_EditorSubsystem::HandleGetFocusedContentBrowserPath(FString& OutPath, FString& OutError)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	FContentBrowserItemPath CurrentPathItem = ContentBrowserSingleton.GetCurrentPath();
	if (!CurrentPathItem.GetVirtualPathName().IsNone())
	{
		OutPath = CurrentPathItem.GetInternalPathString();
	}
	if (OutPath.IsEmpty())
	{
		TArray<FString> SelectedFolders;
		ContentBrowserSingleton.GetSelectedPathViewFolders(SelectedFolders);
		if (SelectedFolders.Num() > 0)
		{
			OutPath = SelectedFolders[0];
		}
		else
		{
			OutError = "Could not determine the current Content Browser path. Please click on a folder in the Content Browser.";
			OutPath = TEXT("/Game/");
		}
	}
}
void UMCP_EditorSubsystem::HandleGetAllSceneActors(FString& OutJsonString, FString& OutError)
{
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	TArray<AActor*> AllActors;
	if (EditorActorSubsystem)
	{
		AllActors = EditorActorSubsystem->GetAllLevelActors();
	}
	TArray<TSharedPtr<FJsonValue>> ActorJsonArray;
	for (AActor* Actor : AllActors)
	{
		if (Actor)
		{
			TSharedPtr<FJsonObject> ActorObject = MakeShareable(new FJsonObject());
			ActorObject->SetStringField(TEXT("label"), Actor->GetActorLabel());
			ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
			FVector Location = Actor->GetActorLocation();
			TArray<TSharedPtr<FJsonValue>> LocationArray;
			LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.X)));
			LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Y)));
			LocationArray.Add(MakeShareable(new FJsonValueNumber(Location.Z)));
			ActorObject->SetArrayField(TEXT("location"), LocationArray);
			ActorJsonArray.Add(MakeShareable(new FJsonValueObject(ActorObject)));
		}
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ActorJsonArray, Writer);
}
void UMCP_EditorSubsystem::HandleCreateMaterial(const FString& MaterialPath, const FLinearColor& Color, FString& OutError)
{
	if (!MaterialPath.StartsWith(TEXT("/Game/")))
	{
		OutError = "Material path must start with /Game/.";
		return;
	}
	FString PackagePath = FPackageName::GetLongPackagePath(MaterialPath);
	FString MaterialName = FPackageName::GetShortName(MaterialPath);
	if (UEditorAssetLibrary::DoesAssetExist(MaterialPath + TEXT(".") + MaterialName))
	{
		OutError = FString::Printf(TEXT("Material '%s' already exists."), *MaterialPath);
		return;
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(MaterialName, PackagePath, UMaterial::StaticClass(), MaterialFactory);
	UMaterial* Material = Cast<UMaterial>(NewAsset);
	if (!Material)
	{
		OutError = "Failed to create new material object via AssetTools.";
		return;
	}
	Material->PreEditChange(nullptr);
	UMaterialExpressionConstant3Vector* ColorExpression = NewObject<UMaterialExpressionConstant3Vector>(Material);
	Material->GetExpressionCollection().AddExpression(ColorExpression);
	ColorExpression->Constant = Color;
	FExpressionInput* BaseColorInput = Material->GetExpressionInputForProperty(EMaterialProperty::MP_BaseColor);
	if (BaseColorInput)
	{
		BaseColorInput->Expression = ColorExpression;
	}
	Material->PostEditChange();
	Material->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Material);
	FString PackageFileName = FPackageName::LongPackageNameToFilename(Material->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Material->GetPackage(), Material, *PackageFileName, SaveArgs))
	{
		OutError = FString::Printf(TEXT("Failed to save new material package to '%s'. Check logs for details."), *PackageFileName);
		return;
	}
}
void UMCP_EditorSubsystem::HandleSetMultipleActorMaterials(const TArray<FString>& ActorLabels, const FString& MaterialPath, FString& OutError)
{
	if (ActorLabels.Num() == 0)
	{
		OutError = "No actor labels were provided for the bulk material operation.";
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("MCP: Beginning bulk material set for %d actors."), ActorLabels.Num());
	int32 FailCount = 0;
	for (const FString& Label : ActorLabels)
	{
		FString SingleError;
		HandleSetActorMaterial(Label, MaterialPath, SingleError);
		if (!SingleError.IsEmpty())
		{
			FailCount++;
			UE_LOG(LogTemp, Warning, TEXT("MCP: A material set in the batch failed for actor '%s'. Reason: %s"), *Label, *SingleError);
		}
	}
	if (FailCount > 0)
	{
		OutError = FString::Printf(TEXT("Finished bulk operation, but failed to set material for %d out of %d actors. See Output Log for details."), FailCount, ActorLabels.Num());
	}
}
bool StringToPinType(const FString& TypeStr, FEdGraphPinType& OutPinType)
{
	if (TypeStr.Equals(TEXT("bool"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	else if (TypeStr.Equals(TEXT("byte"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
	else if (TypeStr.Equals(TEXT("int"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	else if (TypeStr.Equals(TEXT("int32"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	else if (TypeStr.Equals(TEXT("int64"), ESearchCase::IgnoreCase)) OutPinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
	else if (TypeStr.Equals(TEXT("float"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
	else if (TypeStr.Equals(TEXT("double"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Double; }
	else if (TypeStr.Equals(TEXT("real"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Real; OutPinType.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
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
	else if (TypeStr.Equals(TEXT("color"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
	else if (TypeStr.Equals(TEXT("fcolor"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
	else if (TypeStr.Equals(TEXT("linearcolor"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
	else if (TypeStr.Equals(TEXT("quaternion"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get(); }
	else if (TypeStr.Equals(TEXT("fquat"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get(); }
	else if (TypeStr.Equals(TEXT("quat"), ESearchCase::IgnoreCase)) { OutPinType.PinCategory = UEdGraphSchema_K2::PC_Struct; OutPinType.PinSubCategoryObject = TBaseStructure<FQuat>::Get(); }
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
void UMCP_EditorSubsystem::HandleCreateStruct(const FString& StructName, const FString& SavePath, const TArray<TSharedPtr<FJsonValue>>& Variables, FString& OutAssetPath, FString& OutError)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(StructName, SavePath, UUserDefinedStruct::StaticClass(), nullptr);
	if (UUserDefinedStruct* NewStruct = Cast<UUserDefinedStruct>(NewAsset))
	{
		NewStruct->EditorData = NewObject<UUserDefinedStructEditorData>(NewStruct);
		FStructureEditorUtils::ModifyStructData(NewStruct);
		for (const TSharedPtr<FJsonValue>& VarValue : Variables)
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
				if (StringToPinType(VarType, PinType))
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
}
void UMCP_EditorSubsystem::HandleCreateEnum(const FString& EnumName, const FString& SavePath, const TArray<FString>& Enumerators, FString& OutAssetPath, FString& OutError)
{
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
}
void UMCP_EditorSubsystem::HandleAddWidgetToUserWidget(const FString& WidgetPath, const FString& WidgetType, const FString& WidgetName, const FString& ParentName, FString& OutNewName, FString& OutError)
{
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
	if (!WidgetBP)
	{
		OutError = FString::Printf(TEXT("Could not load User Widget Blueprint at path: %s"), *WidgetPath);
		return;
	}
	if (!WidgetBP->WidgetTree)
	{
		WidgetBP->WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional);
	}
	UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetType);
	if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		WidgetClass = FindFirstObjectSafe<UClass>(*("U" + WidgetType));
	}
	if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass()))
	{
		OutError = FString::Printf(TEXT("Could not find a valid UWidget class named '%s'."), *WidgetType);
		return;
	}
	FName NewWidgetFName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
	UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetFName);
	if (!NewWidget)
	{
		OutError = "Failed to construct new widget in the WidgetTree.";
		return;
	}
	OutNewName = NewWidgetFName.ToString();
	if (WidgetBP->WidgetTree->RootWidget == nullptr)
	{
		if (!ParentName.IsEmpty())
		{
			WidgetBP->WidgetTree->RemoveWidget(NewWidget);
			OutError = FString::Printf(TEXT("The WidgetTree is empty, so a parent named '%s' cannot exist. The first widget added becomes the root."), *ParentName);
			return;
		}
		WidgetBP->WidgetTree->RootWidget = NewWidget;
		UE_LOG(LogTemp, Log, TEXT("MCP: Set '%s' as the new root widget for '%s'."), *NewWidgetFName.ToString(), *WidgetPath);
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
				return;
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
			return;
		}
		ParentPanel->AddChild(NewWidget);
		UE_LOG(LogTemp, Log, TEXT("MCP: Added '%s' as a child to '%s'."), *NewWidgetFName.ToString(), *ParentWidget->GetName());
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
}
void UMCP_EditorSubsystem::HandleCreateWidgetFromLayout(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Layout, FString& OutError)
{
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
	if (!WidgetBP) { OutError = FString::Printf(TEXT("Could not load UWidgetBlueprint at path: %s"), *WidgetPath); return; }
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Create Widget Layout")));
	WidgetBP->Modify();
	if (!WidgetBP->WidgetTree) { WidgetBP->WidgetTree = NewObject<UWidgetTree>(WidgetBP, TEXT("WidgetTree"), RF_Transactional); }
	else if (WidgetBP->WidgetTree->RootWidget) { WidgetBP->WidgetTree->RemoveWidget(WidgetBP->WidgetTree->RootWidget); }
	WidgetBP->WidgetTree->Modify();
	TMap<FString, UWidget*> CreatedWidgetsMap;
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : Layout) {
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetTypeStr; if (!WidgetDef->TryGetStringField(TEXT("widget_type"), WidgetTypeStr)) { if (!WidgetDef->TryGetStringField(TEXT("type"), WidgetTypeStr)) { WidgetDef->TryGetStringField(TEXT("class"), WidgetTypeStr); } }
		FString WidgetName; if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName)) { WidgetDef->TryGetStringField(TEXT("name"), WidgetName); }
		FString ParentName; if (!WidgetDef->TryGetStringField(TEXT("parent_name"), ParentName)) { WidgetDef->TryGetStringField(TEXT("parent"), ParentName); }
		if (WidgetName.IsEmpty() || WidgetTypeStr.IsEmpty()) { OutError = TEXT("Widget definition missing 'name' or 'type'."); return; }
		UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetTypeStr); if (!WidgetClass) { WidgetClass = FindFirstObjectSafe<UClass>(*("U" + WidgetTypeStr)); }
		if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass())) { OutError = FString::Printf(TEXT("Invalid widget type '%s'"), *WidgetTypeStr); return; }
		FName NewWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
		UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetName);
		if (!NewWidget) { OutError = FString::Printf(TEXT("Failed to construct widget '%s'."), *WidgetName); return; }
		CreatedWidgetsMap.Add(WidgetName, NewWidget);
		if (ParentName.IsEmpty()) { WidgetBP->WidgetTree->RootWidget = NewWidget; }
		else {
			UWidget** FoundParentWidgetPtr = CreatedWidgetsMap.Find(ParentName);
			if (!FoundParentWidgetPtr || !*FoundParentWidgetPtr) { OutError = FString::Printf(TEXT("Could not find parent '%s'."), *ParentName); return; }
			UWidget* ParentWidget = *FoundParentWidgetPtr;
			if (UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget)) { ParentPanel->AddChild(NewWidget); }
			else if (UContentWidget* ParentContent = Cast<UContentWidget>(ParentWidget)) { ParentContent->SetContent(NewWidget); }
			else { OutError = FString::Printf(TEXT("Parent '%s' is not a container."), *ParentName); return; }
		}
	}
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : Layout) {
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetName; if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName)) { WidgetDef->TryGetStringField(TEXT("name"), WidgetName); }
		UWidget* TargetWidget = *CreatedWidgetsMap.Find(WidgetName);
		TArray<TPair<FString, FString>> ParsedProperties;
		const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr; const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
		if (WidgetDef->TryGetArrayField(TEXT("properties"), PropertiesArray)) {
			for (const TSharedPtr<FJsonValue>& PropValue : *PropertiesArray) {
				const TSharedPtr<FJsonObject>& PropObject = PropValue->AsObject();
				if (PropObject.IsValid()) { FString Key, Value; if (!PropObject->TryGetStringField(TEXT("key"), Key)) { PropObject->TryGetStringField(TEXT("name"), Key); } PropObject->TryGetStringField(TEXT("value"), Value); if (!Key.IsEmpty()) { ParsedProperties.Add(TPair<FString, FString>(Key, Value)); } }
			}
		}
		else if (WidgetDef->TryGetObjectField(TEXT("properties"), PropertiesObject)) {
			for (const auto& Pair : (*PropertiesObject)->Values) { ParsedProperties.Add(TPair<FString, FString>(Pair.Key, Pair.Value->AsString())); }
		}
		for (const auto& PropPair : ParsedProperties) {
			FString PropertyPath = PropPair.Key;
			FString PropValStr = PropPair.Value;
			void* ContainerPtr = TargetWidget;
			UStruct* ContainerStruct = TargetWidget->GetClass();
			if (PropertyPath.StartsWith(TEXT("Slot."))) { if (TargetWidget->Slot) { ContainerPtr = TargetWidget->Slot; ContainerStruct = TargetWidget->Slot->GetClass(); PropertyPath.RightChopInline(5); } else continue; }
			TArray<FString> PathParts; PropertyPath.ParseIntoArray(PathParts, TEXT("."));
			FProperty* FinalProperty = nullptr;
			for (int32 i = 0; i < PathParts.Num(); ++i) {
				FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
				if (!CurrentProp) { OutError = FString::Printf(TEXT("Property '%s' not found on '%s'"), *PathParts[i], *ContainerStruct->GetName()); return; }
				if (i == PathParts.Num() - 1) { FinalProperty = CurrentProp; }
				else {
					if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp)) { ContainerStruct = StructProp->Struct; ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr); if (!ContainerPtr) { OutError = FString::Printf(TEXT("Container pointer became null while traversing path at '%s'."), *PathParts[i]); return; } }
					else { OutError = FString::Printf(TEXT("Property '%s' is not a struct and cannot be traversed."), *PathParts[i]); return; }
				}
			}
			if (FinalProperty && ContainerPtr) {
				FString FinalPropValStr = PropValStr;
				if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'"))) { FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr); }
				if (FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None) == nullptr) { OutError = FString::Printf(TEXT("Failed to import value for '%s'."), *PropertyPath); return; }
			}
			else { OutError = FString::Printf(TEXT("Failed to resolve final property for path '%s'."), *PropertyPath); return; }
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
}
void UMCP_EditorSubsystem::HandleAddWidgetsToLayout(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Layout, FString& OutError)
{
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree) { OutError = TEXT("Could not load a valid widget blueprint to add to."); return; }
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Add Widgets to Layout")));
	WidgetBP->Modify(); WidgetBP->WidgetTree->Modify();
	TArray<UWidget*> AllCurrentWidgets; WidgetBP->WidgetTree->GetAllWidgets(AllCurrentWidgets);
	TMap<FString, UWidget*> AllWidgetsMap;
	for (UWidget* Widget : AllCurrentWidgets) { if (Widget) AllWidgetsMap.Add(Widget->GetFName().ToString(), Widget); }
	TMap<FString, UWidget*> NewWidgetsMap;
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : Layout) {
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetTypeStr; if (!WidgetDef->TryGetStringField(TEXT("widget_type"), WidgetTypeStr)) { if (!WidgetDef->TryGetStringField(TEXT("type"), WidgetTypeStr)) { WidgetDef->TryGetStringField(TEXT("class"), WidgetTypeStr); } }
		FString WidgetName; if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName)) { WidgetDef->TryGetStringField(TEXT("name"), WidgetName); }
		FString ParentName; if (!WidgetDef->TryGetStringField(TEXT("parent_name"), ParentName)) { WidgetDef->TryGetStringField(TEXT("parent"), ParentName); }
		if (WidgetName.IsEmpty() || WidgetTypeStr.IsEmpty()) { OutError = TEXT("Widget definition missing 'name' or 'type'."); return; }
		if (ParentName.IsEmpty()) { OutError = FString::Printf(TEXT("Widget '%s' is missing a 'parent_name'."), *WidgetName); return; }
		UClass* WidgetClass = FindFirstObjectSafe<UClass>(*WidgetTypeStr); if (!WidgetClass) { WidgetClass = FindFirstObjectSafe<UClass>(*("U" + WidgetTypeStr)); }
		if (!WidgetClass || !WidgetClass->IsChildOf(UWidget::StaticClass())) { OutError = FString::Printf(TEXT("Invalid widget type '%s'."), *WidgetTypeStr); return; }
		FName NewWidgetName = FBlueprintEditorUtils::FindUniqueKismetName(WidgetBP, WidgetName);
		UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, NewWidgetName);
		if (!NewWidget) { OutError = FString::Printf(TEXT("Failed to construct widget '%s'."), *WidgetName); return; }
		AllWidgetsMap.Add(NewWidgetName.ToString(), NewWidget); NewWidgetsMap.Add(WidgetName, NewWidget);
		UWidget** FoundParentWidgetPtr = AllWidgetsMap.Find(ParentName);
		if (!FoundParentWidgetPtr || !*FoundParentWidgetPtr) { OutError = FString::Printf(TEXT("Could not find parent '%s'."), *ParentName); return; }
		UWidget* ParentWidget = *FoundParentWidgetPtr;
		if (UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget)) { ParentPanel->AddChild(NewWidget); }
		else if (UContentWidget* ParentContent = Cast<UContentWidget>(ParentWidget)) { ParentContent->SetContent(NewWidget); }
		else { OutError = FString::Printf(TEXT("Parent '%s' is not a container."), *ParentName); return; }
	}
	for (const TSharedPtr<FJsonValue>& WidgetDefValue : Layout) {
		const TSharedPtr<FJsonObject>& WidgetDef = WidgetDefValue->AsObject();
		if (!WidgetDef.IsValid()) continue;
		FString WidgetName; if (!WidgetDef->TryGetStringField(TEXT("widget_name"), WidgetName)) { WidgetDef->TryGetStringField(TEXT("name"), WidgetName); }
		UWidget* TargetWidget = *NewWidgetsMap.Find(WidgetName);
		TArray<TPair<FString, FString>> ParsedProperties;
		const TArray<TSharedPtr<FJsonValue>>* PropertiesArray = nullptr; const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
		if (WidgetDef->TryGetArrayField(TEXT("properties"), PropertiesArray)) {
			for (const TSharedPtr<FJsonValue>& PropValue : *PropertiesArray) {
				const TSharedPtr<FJsonObject>& PropObject = PropValue->AsObject();
				if (PropObject.IsValid()) { FString Key, Value; if (!PropObject->TryGetStringField(TEXT("key"), Key)) { PropObject->TryGetStringField(TEXT("name"), Key); } PropObject->TryGetStringField(TEXT("value"), Value); if (!Key.IsEmpty()) { ParsedProperties.Add(TPair<FString, FString>(Key, Value)); } }
			}
		}
		else if (WidgetDef->TryGetObjectField(TEXT("properties"), PropertiesObject)) {
			for (const auto& Pair : (*PropertiesObject)->Values) { ParsedProperties.Add(TPair<FString, FString>(Pair.Key, Pair.Value->AsString())); }
		}
		for (const auto& PropPair : ParsedProperties) {
			FString PropertyPath = PropPair.Key;
			FString PropValStr = PropPair.Value;
			void* ContainerPtr = TargetWidget;
			UStruct* ContainerStruct = TargetWidget->GetClass();
			if (PropertyPath.StartsWith(TEXT("Slot."))) { if (TargetWidget->Slot) { ContainerPtr = TargetWidget->Slot; ContainerStruct = TargetWidget->Slot->GetClass(); PropertyPath.RightChopInline(5); } else continue; }
			TArray<FString> PathParts; PropertyPath.ParseIntoArray(PathParts, TEXT("."));
			FProperty* FinalProperty = nullptr;
			for (int32 i = 0; i < PathParts.Num(); ++i) {
				FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
				if (!CurrentProp) { OutError = FString::Printf(TEXT("Property '%s' not found on '%s'"), *PathParts[i], *ContainerStruct->GetName()); return; }
				if (i == PathParts.Num() - 1) { FinalProperty = CurrentProp; }
				else {
					if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp)) { ContainerStruct = StructProp->Struct; ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr); if (!ContainerPtr) { OutError = FString::Printf(TEXT("Container pointer became null while traversing path at '%s'."), *PathParts[i]); return; } }
					else { OutError = FString::Printf(TEXT("Property '%s' is not a struct."), *PathParts[i]); return; }
				}
			}
			if (FinalProperty && ContainerPtr) {
				FString FinalPropValStr = PropValStr;
				if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'"))) { FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr); }
				if (FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None) == nullptr) { OutError = FString::Printf(TEXT("Failed to import value for '%s'."), *PropertyPath); return; }
			}
			else { OutError = FString::Printf(TEXT("Failed to resolve final property for path '%s'."), *PropertyPath); return; }
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
}
void UMCP_EditorSubsystem::HandleEditWidgetProperties(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Edits, FString& OutError)
{
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree) { OutError = TEXT("Could not load a valid widget blueprint to edit."); return; }
	const FScopedTransaction Transaction(FText::FromString(TEXT("MCP: Edit Widget Properties")));
	WidgetBP->Modify();
	for (const TSharedPtr<FJsonValue>& EditValue : Edits)
	{
		const TSharedPtr<FJsonObject>& EditObject = EditValue->AsObject();
		if (!EditObject.IsValid()) continue;
		FString WidgetName, PropertyPath, PropValStr;
		if (!EditObject->TryGetStringField(TEXT("widget_name"), WidgetName) ||
			!EditObject->TryGetStringField(TEXT("property_name"), PropertyPath) ||
			!EditObject->TryGetStringField(TEXT("value"), PropValStr))
		{
			OutError = "Invalid edit operation format."; return;
		}
		UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
		if (!TargetWidget) { OutError = FString::Printf(TEXT("Could not find widget '%s' to edit."), *WidgetName); return; }
		TargetWidget->Modify();
		void* ContainerPtr = TargetWidget;
		UStruct* ContainerStruct = TargetWidget->GetClass();
		if (PropertyPath.StartsWith(TEXT("Slot."))) { if (TargetWidget->Slot) { TargetWidget->Slot->Modify(); ContainerPtr = TargetWidget->Slot; ContainerStruct = TargetWidget->Slot->GetClass(); PropertyPath.RightChopInline(5); } else continue; }
		TArray<FString> PathParts; PropertyPath.ParseIntoArray(PathParts, TEXT("."));
		FProperty* FinalProperty = nullptr;
		for (int32 i = 0; i < PathParts.Num(); ++i) {
			FProperty* CurrentProp = ContainerStruct->FindPropertyByName(FName(*PathParts[i]));
			if (!CurrentProp) { OutError = FString::Printf(TEXT("Property '%s' not found on '%s'"), *PathParts[i], *ContainerStruct->GetName()); return; }
			if (i == PathParts.Num() - 1) { FinalProperty = CurrentProp; }
			else {
				if (FStructProperty* StructProp = CastField<FStructProperty>(CurrentProp)) { ContainerStruct = StructProp->Struct; ContainerPtr = CurrentProp->ContainerPtrToValuePtr<void>(ContainerPtr); if (!ContainerPtr) { OutError = FString::Printf(TEXT("Container pointer became null at '%s'."), *PathParts[i]); return; } }
				else { OutError = FString::Printf(TEXT("Property '%s' is not a struct."), *PathParts[i]); return; }
			}
		}
		if (FinalProperty && ContainerPtr) {
			FString FinalPropValStr = PropValStr;
			if (FinalProperty->IsA<FTextProperty>() && !PropValStr.StartsWith(TEXT("\"")) && !PropValStr.StartsWith(TEXT("'"))) { FinalPropValStr = FString::Printf(TEXT("\"%s\""), *PropValStr); }
			if (FinalProperty->ImportText_Direct(*FinalPropValStr, FinalProperty->ContainerPtrToValuePtr<void>(ContainerPtr), nullptr, PPF_None) == nullptr) { OutError = FString::Printf(TEXT("Failed to import value for '%s'."), *PropertyPath); return; }
		}
		else { OutError = FString::Printf(TEXT("Failed to resolve final property for path '%s'."), *PropertyPath); return; }
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBP);
	WidgetBP->GetPackage()->MarkPackageDirty();
}
void RecursivelyGetProperties(UStruct* InStruct, const FString& Prefix, TArray<TSharedPtr<FJsonValue>>& OutPropertiesArray, int32 Depth)
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
				TSharedPtr<FJsonObject> StructPropObject = MakeShareable(new FJsonObject());
				StructPropObject->SetStringField("name", PropName);
				StructPropObject->SetStringField("type", StructProperty->Struct->GetName());
				OutPropertiesArray.Add(MakeShareable(new FJsonValueObject(StructPropObject)));
				RecursivelyGetProperties(StructProperty->Struct, PropName + TEXT("."), OutPropertiesArray, Depth + 1);
			}
			else
			{
				TSharedPtr<FJsonObject> PropObject = MakeShareable(new FJsonObject());
				PropObject->SetStringField("name", PropName);
				PropObject->SetStringField("type", Property->GetCPPType());
				OutPropertiesArray.Add(MakeShareable(new FJsonValueObject(PropObject)));
			}
		}
	}
}
void UMCP_EditorSubsystem::HandleGetBatchWidgetProperties(const TArray<FString>& WidgetClasses, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
	for (const FString& ClassName : WidgetClasses)
	{
		UClass* FoundClass = FindFirstObjectSafe<UClass>(*ClassName);
		if (!FoundClass) FoundClass = FindFirstObjectSafe<UClass>(*("U" + ClassName));
		if (!FoundClass) continue;
		TArray<TSharedPtr<FJsonValue>> PropertiesArray;
		RecursivelyGetProperties(FoundClass, TEXT(""), PropertiesArray, 0);
		ResultObject->SetArrayField(ClassName, PropertiesArray);
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleSetActorMaterial(const FString& ActorLabel, const FString& MaterialPath, FString& OutError)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutError = "Failed to get a valid editor world.";
		return;
	}
	AActor* TargetActor = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (*It && (*It)->GetActorLabel() == ActorLabel)
		{
			TargetActor = *It;
			break;
		}
	}
	if (!TargetActor)
	{
		OutError = FString::Printf(TEXT("Could not find an actor with the label '%s' in the level."), *ActorLabel);
		return;
	}
	UMaterialInterface* MaterialToApply = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
	if (!MaterialToApply)
	{
		OutError = FString::Printf(TEXT("Failed to load material at path '%s'."), *MaterialPath);
		return;
	}
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	if (PrimitiveComponents.Num() == 0)
	{
		OutError = FString::Printf(TEXT("No primitive components (like Static Mesh or Skeletal Mesh) found on actor '%s' to apply a material to."), *ActorLabel);
		return;
	}
	for (UPrimitiveComponent* Comp : PrimitiveComponents)
	{
		for (int32 i = 0; i < Comp->GetNumMaterials(); ++i)
		{
			Comp->SetMaterial(i, MaterialToApply);
		}
	}
}
void UMCP_EditorSubsystem::HandleSetSelectedActors(const TArray<FString>& ActorLabels, FString& OutError)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutError = TEXT("Failed to get a valid editor world.");
		return;
	}
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
			UE_LOG(LogTemp, Warning, TEXT("MCP: Could not find an actor with label '%s' to select."), *Label);
		}
	}
	if (ActorsToSelect.Num() > 0)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (EditorActorSubsystem)
		{
			EditorActorSubsystem->SetSelectedLevelActors(ActorsToSelect);
		}
	}
	else if (ActorLabels.Num() > 0)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		if (EditorActorSubsystem)
		{
			EditorActorSubsystem->SetSelectedLevelActors({});
		}
	}
}
void UMCP_EditorSubsystem::HandleCreateProjectFolder(const FString& FolderPath, FString& OutError)
{
	if (FolderPath.IsEmpty() || !FolderPath.StartsWith(TEXT("/Game/")))
	{
		OutError = "Invalid folder path. Path must start with /Game/.";
		return;
	}
	if (UEditorAssetLibrary::DoesDirectoryExist(FolderPath))
	{
		OutError = FString::Printf(TEXT("Folder '%s' already exists."), *FolderPath);
		return;
	}
	if (!UEditorAssetLibrary::MakeDirectory(FolderPath))
	{
		OutError = FString::Printf(TEXT("Failed to create folder at '%s'."), *FolderPath);
	}
	else
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().ScanPathsSynchronous({ FolderPath }, true);
	}
}
void UMCP_EditorSubsystem::HandleEditWidgetProperty(const FString& WidgetPath, const FString& WidgetName, const FString& PropertyName, const FString& Value, FString& OutError)
{
	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		OutError = FString::Printf(TEXT("Could not load a valid User Widget Blueprint at path: %s"), *WidgetPath);
		return;
	}
	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!TargetWidget)
	{
		OutError = FString::Printf(TEXT("Could not find a widget named '%s' in the hierarchy."), *WidgetName);
		return;
	}
	UObject* PropertyTargetObject = nullptr;
	FString ActualPropertyName = PropertyName;
	if (PropertyName.StartsWith(TEXT("Slot.")))
	{
		UPanelSlot* Slot = TargetWidget->Slot;
		if (!Slot)
		{
			OutError = FString::Printf(TEXT("Widget '%s' does not have a valid 'Slot' property to edit."), *WidgetName);
			return;
		}
		PropertyTargetObject = Slot;
		ActualPropertyName = PropertyName.RightChop(5);
	}
	else
	{
		PropertyTargetObject = TargetWidget;
	}
	FProperty* Property = PropertyTargetObject->GetClass()->FindPropertyByName(FName(*ActualPropertyName));
	if (Property)
	{
		UStruct* OwnerStruct = Property->GetOwnerStruct();
		if (OwnerStruct)
		{
			if (UClass* OwnerClass = Cast<UClass>(OwnerStruct))
			{
				if (!PropertyTargetObject->IsA(OwnerClass))
				{
					OutError += FString::Printf(TEXT("\n[Property Error] Widget '%s': Property '%s' owner mismatch."), *WidgetName, *ActualPropertyName);
					return;
				}
			}
		}
		void* DestPtr = Property->ContainerPtrToValuePtr<void>(PropertyTargetObject);
		if (DestPtr == nullptr)
		{
			OutError += FString::Printf(TEXT("\n[Property Error] Widget '%s': Property '%s' has an invalid destination pointer."), *WidgetName, *ActualPropertyName);
			return;
		}
		PropertyTargetObject->Modify();
		if (Property->ImportText_Direct(*Value, DestPtr, nullptr, PPF_None) == nullptr)
		{
			OutError += FString::Printf(TEXT("\n[Property Error] Widget '%s': Failed to import value '%s' into property '%s'."), *WidgetName, *Value, *ActualPropertyName);
		}
	}
	else
	{
		OutError += FString::Printf(TEXT("\n[Property Error] Widget '%s': Property '%s' not found on target '%s'."), *WidgetName, *ActualPropertyName, *PropertyTargetObject->GetClass()->GetName());
	}
}
void UMCP_EditorSubsystem::HandleSpawnActorInLevel(const FString& ActorClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FString& ActorLabel, FString& OutError)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutError = TEXT("Failed to get a valid editor world to spawn actor in.");
		return;
	}
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if (!EditorActorSubsystem)
	{
		OutError = TEXT("Could not get the Editor Actor Subsystem.");
		return;
	}
	AActor* SpawnedActor = nullptr;
	const TSet<FString> BasicShapes = { "Cube", "Sphere", "Cylinder", "Cone" };
	if (BasicShapes.Contains(ActorClass))
	{
		FString MeshPath = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), *ActorClass, *ActorClass);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
		if (!Mesh)
		{
			OutError = FString::Printf(TEXT("Failed to load basic shape mesh: %s"), *MeshPath);
			return;
		}
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(EditorActorSubsystem->SpawnActorFromClass(AStaticMeshActor::StaticClass(), Location, Rotation));
		if (MeshActor)
		{
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			SpawnedActor = MeshActor;
		}
	}
	else if (ActorClass.StartsWith(TEXT("/Game/")))
	{
		UObject* LoadedObject = LoadObject<UObject>(nullptr, *ActorClass);
		if (!LoadedObject)
		{
			OutError = FString::Printf(TEXT("Failed to load any asset at path: %s"), *ActorClass);
			return;
		}
		if (UBlueprint* LoadedBP = Cast<UBlueprint>(LoadedObject))
		{
			if (UClass* GeneratedClass = LoadedBP->GeneratedClass)
			{
				if (GeneratedClass->IsChildOf(AActor::StaticClass()))
				{
					SpawnedActor = EditorActorSubsystem->SpawnActorFromClass(GeneratedClass, Location, Rotation);
				}
				else
				{
					OutError = FString::Printf(TEXT("Blueprint at path '%s' is not a spawnable Actor class."), *ActorClass);
					return;
				}
			}
			else
			{
				OutError = FString::Printf(TEXT("Blueprint at path '%s' has no valid GeneratedClass."), *ActorClass);
				return;
			}
		}
		else if (UStaticMesh* LoadedMesh = Cast<UStaticMesh>(LoadedObject))
		{
			AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(EditorActorSubsystem->SpawnActorFromClass(AStaticMeshActor::StaticClass(), Location, Rotation));
			if (MeshActor && MeshActor->GetStaticMeshComponent())
			{
				MeshActor->GetStaticMeshComponent()->SetStaticMesh(LoadedMesh);
				SpawnedActor = MeshActor;
			}
			else
			{
				OutError = FString::Printf(TEXT("Failed to spawn a StaticMeshActor for mesh '%s'."), *ActorClass);
				return;
			}
		}
		else
		{
			OutError = FString::Printf(TEXT("Asset at path '%s' is not a spawnable Blueprint or a Static Mesh."), *ActorClass);
			return;
		}
	}
	else
	{
		UClass* ClassToSpawn = FindFirstObjectSafe<UClass>(*ActorClass);
		if (!ClassToSpawn)
		{
			ClassToSpawn = FindFirstObjectSafe<UClass>(*("A" + ActorClass));
		}
		if (ClassToSpawn && ClassToSpawn->IsChildOf(AActor::StaticClass()))
		{
			SpawnedActor = EditorActorSubsystem->SpawnActorFromClass(ClassToSpawn, Location, Rotation);
		}
		else
		{
			OutError = FString::Printf(TEXT("Could not find a spawnable C++ class or Blueprint named '%s'."), *ActorClass);
			return;
		}
	}
	if (SpawnedActor)
	{
		SpawnedActor->SetActorScale3D(Scale);
		if (!ActorLabel.IsEmpty())
		{
			SpawnedActor->SetActorLabel(ActorLabel);
		}
		EditorActorSubsystem->SetSelectedLevelActors({ SpawnedActor });
		UE_LOG(LogTemp, Log, TEXT("MCP: Successfully spawned actor '%s'."), *SpawnedActor->GetActorLabel());
	}
	else
	{
		OutError = FString::Printf(TEXT("Failed to spawn actor of class '%s'. The class may not be spawnable or an unknown error occurred."), *ActorClass);
	}
}
void UMCP_EditorSubsystem::HandleListAssetsInFolder(const FString& FolderPath, FString& OutJsonString, FString& OutError)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetData;
	AssetRegistryModule.Get().GetAssetsByPath(FName(*FolderPath), AssetData, true);
	TArray<TSharedPtr<FJsonValue>> AssetJsonArray;
	for (const FAssetData& Data : AssetData)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShareable(new FJsonObject());
		AssetObject->SetStringField(TEXT("name"), Data.AssetName.ToString());
		AssetObject->SetStringField(TEXT("path"), Data.GetObjectPathString());
		AssetObject->SetStringField(TEXT("class"), Data.AssetClassPath.GetAssetName().ToString());
		AssetJsonArray.Add(MakeShareable(new FJsonValueObject(AssetObject)));
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(AssetJsonArray, Writer);
}
void UMCP_EditorSubsystem::HandleGetSelectedContentBrowserAssets(FString& OutJsonString, FString& OutError)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	TArray<FAssetData> SelectedAssets;
	ContentBrowserSingleton.GetSelectedAssets(SelectedAssets);
	if (SelectedAssets.Num() == 0)
	{
		OutError = "No assets are currently selected in the Content Browser.";
		return;
	}
	TArray<TSharedPtr<FJsonValue>> AssetJsonArray;
	for (const FAssetData& Data : SelectedAssets)
	{
		TSharedPtr<FJsonObject> AssetObject = MakeShareable(new FJsonObject());
		AssetObject->SetStringField(TEXT("name"), Data.AssetName.ToString());
		AssetObject->SetStringField(TEXT("path"), Data.GetObjectPathString());
		AssetObject->SetStringField(TEXT("class"), Data.AssetClassPath.GetAssetName().ToString());
		AssetJsonArray.Add(MakeShareable(new FJsonValueObject(AssetObject)));
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(AssetJsonArray, Writer);
}
void UMCP_EditorSubsystem::HandleSpawnMultipleActorsInLevel(const TArray<FSpawnRequest>& SpawnRequests, FString& OutError)
{
	if (SpawnRequests.Num() == 0)
	{
		OutError = TEXT("No spawn requests provided in the batch.");
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("MCP: Beginning bulk spawn of %d actors."), SpawnRequests.Num());
	for (const FSpawnRequest& Request : SpawnRequests)
	{
		FString SingleSpawnError;
		HandleSpawnActorInLevel(Request.ActorClass, Request.Location, Request.Rotation, Request.Scale, Request.ActorLabel, SingleSpawnError);
		if (!SingleSpawnError.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("MCP: A spawn in the batch failed. ActorClass: '%s', Reason: %s"), *Request.ActorClass, *SingleSpawnError);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("MCP: Bulk spawn complete."));
}
void UMCP_EditorSubsystem::HandleAdvancedSpawn(
	int32 Count, const FString& ActorClass, const FString& BoundingActorLabel,
	const FVector& LocMin, const FVector& LocMax,
	const FVector& ScaleMin, const FVector& ScaleMax,
	const FRotator& RotMin, const FRotator& RotMax,
	FString& OutError
)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		OutError = TEXT("Failed to get a valid editor world.");
		return;
	}
	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
	if (!EditorActorSubsystem)
	{
		OutError = TEXT("Could not get the Editor Actor Subsystem.");
		return;
	}
	FBox BoundingBox(ForceInit);
	const FString SelectedActorMagicString = TEXT("_SELECTED_");
	if (!BoundingActorLabel.IsEmpty())
	{
		AActor* BoundingActor = nullptr;
		if (BoundingActorLabel.Equals(SelectedActorMagicString))
		{
			TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
			if (SelectedActors.Num() != 1)
			{
				OutError = FString::Printf(TEXT("To spawn within a selected volume, you must select exactly one actor in the level. You have %d selected."), SelectedActors.Num());
				return;
			}
			BoundingActor = SelectedActors[0];
		}
		else
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (*It && (*It)->GetActorLabel() == BoundingActorLabel)
				{
					BoundingActor = *It;
					break;
				}
			}
			if (!BoundingActor)
			{
				OutError = FString::Printf(TEXT("Could not find any actor in the level with the label '%s'."), *BoundingActorLabel);
				return;
			}
		}
		FVector Origin, Extent;
		BoundingActor->GetActorBounds(false, Origin, Extent);
		BoundingBox = FBox(Origin - Extent, Origin + Extent);
	}
	else
	{
		BoundingBox = FBox(LocMin, LocMax);
	}
	TArray<FSpawnRequest> SpawnRequests;
	for (int32 i = 0; i < Count; ++i)
	{
		FSpawnRequest Req;
		Req.ActorClass = ActorClass;
		Req.Location = FMath::RandPointInBox(BoundingBox);
		Req.Scale = FVector(
			FMath::RandRange(ScaleMin.X, ScaleMax.X),
			FMath::RandRange(ScaleMin.Y, ScaleMax.Y),
			FMath::RandRange(ScaleMin.Z, ScaleMax.Z)
		);
		Req.Rotation = FRotator(
			FMath::RandRange(RotMin.Pitch, RotMax.Pitch),
			FMath::RandRange(RotMin.Yaw, RotMax.Yaw),
			FMath::RandRange(RotMin.Roll, RotMax.Roll)
		);
		Req.ActorLabel = FString::Printf(TEXT("%s_%d"), *ActorClass, i + 1);
		SpawnRequests.Add(Req);
	}
	HandleSpawnMultipleActorsInLevel(SpawnRequests, OutError);
}
FString UMCP_EditorSubsystem::HandleEditComponentProperty(const FString& BpPath, const FString& ComponentName, const FString& PropertyName, const FString& PropertyValue)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Could not load Blueprint at path: %s\"}"), *BpPath);
	}
	USCS_Node* TargetNode = TargetBlueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
	if (!TargetNode)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Component '%s' not found in Blueprint '%s'\"}"), *ComponentName, *BpPath);
	}
	UActorComponent* ComponentTemplate = TargetNode->ComponentTemplate;
	if (!ComponentTemplate)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Component template for '%s' is invalid.\"}"), *ComponentName);
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
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Property '%s' not found on component '%s'\", \"suggestions\": \"%s\"}"), *PropertyName, *ComponentName, *SuggestionStr);
	}
	bool bSuccess = false;
	void* PropertyData = Property->ContainerPtrToValuePtr<void>(ComponentTemplate);
	if (PropertyData)
	{
		if (Property->ImportText_Direct(*PropertyValue, PropertyData, nullptr, PPF_None))
		{
			bSuccess = true;
		}
	}
	if (!bSuccess)
	{
		if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				FVector VectorValue;
				if (VectorValue.InitFromString(PropertyValue))
				{
					*static_cast<FVector*>(PropertyData) = VectorValue;
					bSuccess = true;
				}
			}
			else if (StructProp->Struct == TBaseStructure<FRotator>::Get())
			{
				FRotator RotatorValue;
				if (RotatorValue.InitFromString(PropertyValue))
				{
					*static_cast<FRotator*>(PropertyData) = RotatorValue;
					bSuccess = true;
				}
			}
		}
	}
	if (!bSuccess)
	{
		return FString::Printf(TEXT("{\"success\": false, \"error\": \"Failed to set property '%s' to value '%s'. The value format may be incorrect for the property type.\"}"), *PropertyName, *PropertyValue);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	return FString::Printf(TEXT("{\"success\": true, \"message\": \"Set property '%s' on component '%s' to '%s'\"}"), *PropertyName, *ComponentName, *PropertyValue);
}
void UMCP_EditorSubsystem::HandleAddVariable(const FString& BpPath, const FString& VarName, const FString& VarType, const FString& DefaultValue, const FString& Category, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	FGuid ExistingVarGuid = FBlueprintEditorUtils::FindMemberVariableGuidByName(TargetBlueprint, FName(*VarName));
	if (ExistingVarGuid.IsValid())
	{
		OutError = TEXT("");
		return;
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
			else if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Struct;
				OutPinType.PinSubCategoryObject = ScriptStruct;
			}
			else if (UUserDefinedEnum* UserEnum = Cast<UUserDefinedEnum>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = UserEnum;
			}
			else if (UEnum* Enum = Cast<UEnum>(FoundType))
			{
				OutPinType.PinCategory = K2Schema->PC_Byte;
				OutPinType.PinSubCategoryObject = Enum;
			}
			else
			{
				return false;
			}
		}
		return true;
	};
	if (!ParseTypeString(VarType, PinType))
	{
		OutError = FString::Printf(TEXT("Unknown or unsupported variable type: %s"), *VarType);
		return;
	}
	FBlueprintEditorUtils::AddMemberVariable(TargetBlueprint, FName(*VarName), PinType, DefaultValue);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
}
void UMCP_EditorSubsystem::HandleDeleteVariable(const FString& BpPath, const FString& VarName, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	FGuid VarGuid = FBlueprintEditorUtils::FindMemberVariableGuidByName(TargetBlueprint, FName(*VarName));
	if (!VarGuid.IsValid())
	{
		OutError = FString::Printf(TEXT("Variable '%s' not found in Blueprint."), *VarName);
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Variable")));
	TargetBlueprint->Modify();
	FBlueprintEditorUtils::RemoveMemberVariable(TargetBlueprint, FName(*VarName));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
}
void UMCP_EditorSubsystem::HandleCategorizeVariables(const FString& BpPath, const TSharedPtr<FJsonObject>* Categories, FString& OutJsonString, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
	if (Categories && Categories->IsValid())
	{
		const FScopedTransaction Transaction(FText::FromString(TEXT("Categorize Variables")));
		TargetBlueprint->Modify();
		int32 UpdatedCount = 0;
		int32 SkippedCount = 0;
		for (FBPVariableDescription& VarDesc : TargetBlueprint->NewVariables)
		{
			FString VarName = VarDesc.VarName.ToString();
			if ((*Categories)->HasField(VarName))
			{
				if (VarDesc.Category.ToString() == TEXT("Default") || VarDesc.Category.IsEmpty())
				{
					FString NewCategory = (*Categories)->GetStringField(VarName);
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
		ResultObject->SetStringField(TEXT("blueprint_path"), BpPath);
		if (SkippedCount > 0)
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully categorized %d variables. Skipped %d variables that were already in custom categories."), UpdatedCount, SkippedCount));
		}
		else
		{
			ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully categorized %d variables."), UpdatedCount));
		}
	}
	else
	{
		TArray<TSharedPtr<FJsonValue>> VariablesArray;
		for (const FBPVariableDescription& VarDesc : TargetBlueprint->NewVariables)
		{
			if (VarDesc.Category.ToString() == TEXT("Default") || VarDesc.Category.IsEmpty())
			{
				TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject());
				VarObj->SetStringField(TEXT("name"), VarDesc.VarName.ToString());
				VarObj->SetStringField(TEXT("type"), VarDesc.VarType.PinCategory.ToString());
				VarObj->SetStringField(TEXT("category"), VarDesc.Category.ToString());
				VariablesArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
			}
		}
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetArrayField(TEXT("variables"), VariablesArray);
		ResultObject->SetStringField(TEXT("blueprint_path"), BpPath);
		ResultObject->SetStringField(TEXT("blueprint_name"), TargetBlueprint->GetName());
		ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Found %d uncategorized variables. Provide a 'categories' object to organize them. Already-categorized variables will be preserved."), VariablesArray.Num()));
	}
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleDeleteComponent(const FString& BpPath, const FString& ComponentName, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	USCS_Node* ComponentNode = TargetBlueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
	if (!ComponentNode)
	{
		OutError = FString::Printf(TEXT("Component '%s' not found in Blueprint."), *ComponentName);
		return;
	}
	if (ComponentNode->IsNative())
	{
		OutError = FString::Printf(TEXT("Cannot delete native/root component '%s'."), *ComponentName);
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Component")));
	TargetBlueprint->Modify();
	TargetBlueprint->SimpleConstructionScript->RemoveNode(ComponentNode);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
}
void UMCP_EditorSubsystem::HandleDeleteNodes(FString& OutJsonString, FString& OutError)
{
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		OutError = TEXT("Could not get the Asset Editor Subsystem.");
		return;
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
		OutError = TEXT("No active Blueprint editor found.");
		return;
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
	int32 DeletedCount = 0;
	if (FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(ActiveEditor))
	{
		const TSet<UObject*>& SelectedNodes = BlueprintEditor->GetSelectedNodes();
		if (SelectedNodes.Num() == 0)
		{
			OutError = TEXT("No nodes selected in the active Blueprint editor.");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), OutError);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			OutJsonString = ResultString;
			return;
		}
		const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Nodes")));
		for (UObject* NodeObj : SelectedNodes)
		{
			if (UEdGraphNode* Node = Cast<UEdGraphNode>(NodeObj))
			{
				if (Node->GetClass()->GetName() == TEXT("EdGraphCommentNode"))
				{
					continue;
				}
				UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(Node);
				if (Blueprint)
				{
					Blueprint->Modify();
					Node->Modify();
					Node->DestroyNode();
					DeletedCount++;
				}
			}
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(BlueprintEditor->GetBlueprintObj());
		if (DeletedCount == 0)
		{
			OutError = TEXT("No deletable nodes were found in the selection (comment nodes are skipped).");
			ResultObject->SetBoolField(TEXT("success"), false);
			ResultObject->SetStringField(TEXT("error"), OutError);
			FString ResultString;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
			FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
			OutJsonString = ResultString;
			return;
		}
	}
	else
	{
		OutError = TEXT("Active editor is not a Blueprint editor.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("deleted_count"), DeletedCount);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted %d node(s)."), DeletedCount));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleDeleteWidget(const FString& WidgetPath, const FString& WidgetName, FString& OutError)
{
	UObject* LoadedObj = UEditorAssetLibrary::LoadAsset(WidgetPath);
	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObj);
	if (!WidgetBP)
	{
		OutError = FString::Printf(TEXT("Could not load Widget Blueprint at path: %s"), *WidgetPath);
		return;
	}
	if (!WidgetBP->WidgetTree)
	{
		OutError = TEXT("Widget Blueprint does not have a valid WidgetTree.");
		return;
	}
	UWidget* TargetWidget = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!TargetWidget)
	{
		OutError = FString::Printf(TEXT("Widget '%s' not found in the Widget Blueprint."), *WidgetName);
		return;
	}
	if (WidgetBP->WidgetTree->RootWidget == TargetWidget)
	{
		OutError = TEXT("Cannot delete the root widget. Please add a new root widget first or reparent existing widgets.");
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Widget")));
	WidgetBP->Modify();
	WidgetBP->WidgetTree->RemoveWidget(TargetWidget);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
}
void UMCP_EditorSubsystem::HandleDeleteUnusedVariables(const FString& BpPath, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
	ResultObject->SetBoolField(TEXT("success"), false);
	ResultObject->SetStringField(TEXT("error"), TEXT("The delete_unused_variables tool is not yet fully implemented. Please manually delete unused variables from the Blueprint Editor by right-clicking them in the MyBlueprint panel."));
	ResultObject->SetStringField(TEXT("info"), TEXT("You can identify unused variables by looking at your graphs - variables not appearing in any node are likely unused."));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleDeleteFunction(const FString& BpPath, const FString& FunctionName, FString& OutError)
{
	UBlueprint* TargetBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BpPath));
	if (!TargetBlueprint)
	{
		OutError = FString::Printf(TEXT("Could not load Blueprint at path: %s"), *BpPath);
		return;
	}
	UEdGraph* FunctionGraph = nullptr;
	for (UEdGraph* Graph : TargetBlueprint->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FunctionName))
		{
			FunctionGraph = Graph;
			break;
		}
	}
	if (!FunctionGraph)
	{
		OutError = FString::Printf(TEXT("Function '%s' not found in Blueprint."), *FunctionName);
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Function")));
	TargetBlueprint->Modify();
	FBlueprintEditorUtils::RemoveGraph(TargetBlueprint, FunctionGraph);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
}
void UMCP_EditorSubsystem::HandleUndoLastGenerated(FString& OutJsonString, FString& OutError)
{
	FBpGeneratorUltimateModule& BpGeneratorModule = FModuleManager::LoadModuleChecked<FBpGeneratorUltimateModule>("BpGeneratorUltimate");
	UBlueprint* TargetBlueprint = BpGeneratorModule.GetTargetBlueprint();
	if (!TargetBlueprint)
	{
		TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("No Blueprint is currently selected. Please select a Blueprint first."));
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		OutError = TEXT("No Blueprint selected");
		return;
	}
	TArray<FGuid> NodesToDelete = FBpGeneratorCompiler::PopLastCreatedNodes();
	if (NodesToDelete.Num() == 0)
	{
		TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), TEXT("No recently generated code found to undo. The undo stack is empty."));
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		OutError = TEXT("No code to undo");
		return;
	}
	const FScopedTransaction Transaction(FText::FromString(TEXT("Undo Last Generated Code")));
	bool bDeletedAll = FBpGeneratorCompiler::DeleteNodesByGUIDs(TargetBlueprint, NodesToDelete);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(TargetBlueprint);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject());
	ResultObject->SetBoolField(TEXT("success"), bDeletedAll);
	ResultObject->SetStringField(TEXT("blueprint_path"), TargetBlueprint->GetPathName());
	ResultObject->SetNumberField(TEXT("nodes_deleted"), NodesToDelete.Num());
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully undid last code generation. Deleted %d node(s)."), NodesToDelete.Num()));
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleCreateBlueprint(const FString& BpName, const FString& ParentClass, const FString& SavePath, FString& OutAssetPath, FString& OutError)
{
	UClass* FoundParentClass = FindFirstObjectSafe<UClass>(*ParentClass);
	if (!FoundParentClass)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AssetData;
		AssetRegistryModule.Get().GetAllAssets(AssetData);
		for (const FAssetData& Data : AssetData)
		{
			if (Data.AssetName.ToString() == ParentClass)
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
	if (!FoundParentClass)
	{
		OutError = FString::Printf(TEXT("Parent class '%s' could not be found."), *ParentClass);
		return;
	}
	FString PackagePath = SavePath;
	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}
	PackagePath += BpName;
	if (FPackageName::DoesPackageExist(PackagePath))
	{
		OutError = FString::Printf(TEXT("Asset already exists at path '%s'."), *PackagePath);
		return;
	}
	UClass* BlueprintClass = UBlueprint::StaticClass();
	UClass* BlueprintGeneratedClass = UBlueprintGeneratedClass::StaticClass();
	EBlueprintType BlueprintType = BPTYPE_Normal;
	if (FoundParentClass->IsChildOf(UUserWidget::StaticClass()))
	{
		BlueprintClass = UWidgetBlueprint::StaticClass();
		BlueprintGeneratedClass = UWidgetBlueprintGeneratedClass::StaticClass();
		UE_LOG(LogTemp, Log, TEXT("MCP: Detected UserWidget parent. Creating a WidgetBlueprint asset."));
	}
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(FoundParentClass, CreatePackage(*PackagePath), FName(*BpName), BlueprintType, BlueprintClass, BlueprintGeneratedClass);
	if (NewBlueprint)
	{
		FAssetRegistryModule::AssetCreated(NewBlueprint);
		NewBlueprint->MarkPackageDirty();
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		if (UPackage::SavePackage(NewBlueprint->GetPackage(), NewBlueprint, *PackageFileName, SaveArgs))
		{
			OutAssetPath = NewBlueprint->GetPathName();
		}
		else
		{
			OutError = TEXT("Failed to save new Blueprint package to disk.");
		}
	}
	else
	{
		OutError = TEXT("FKismetEditorUtilities::CreateBlueprint failed to return a valid Blueprint object.");
	}
}
UEdGraphNode* UMCP_EditorSubsystem::GetActiveSelectedGraphNode(UBlueprint* TargetBlueprint)
{
	if (!TargetBlueprint) return nullptr;
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem) return nullptr;
	IAssetEditorInstance* AssetEditor = AssetEditorSubsystem->FindEditorForAsset(TargetBlueprint, true);
	if (FBlueprintEditor* BlueprintEditor = static_cast<FBlueprintEditor*>(AssetEditor))
	{
		const TSet<UObject*> SelectedNodes = BlueprintEditor->GetSelectedNodes();
		if (SelectedNodes.Num() == 1)
		{
			if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedNodes.begin()))
			{
				UE_LOG(LogTemp, Log, TEXT("MCP Subsystem: Found active selected node: %s"), *Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
				return Node;
			}
		}
	}
	return nullptr;
}
FString UMCP_EditorSubsystem::ChopStringByTripleQuotes(const FString& InputString)
{
    const TCHAR* Delimiter = TEXT("```");
    int32 StartPos = InputString.Find(Delimiter);
    if (StartPos == INDEX_NONE) return InputString.TrimStartAndEnd();
    int32 ActualStartPos = StartPos + 3;
    if (InputString.Mid(ActualStartPos, 3).Equals(TEXT("cpp"), ESearchCase::IgnoreCase)) ActualStartPos += 3;
    int32 EndPos = InputString.Find(Delimiter, ESearchCase::CaseSensitive, ESearchDir::FromStart, ActualStartPos);
    if (EndPos == INDEX_NONE) return InputString.TrimStartAndEnd();
    return InputString.Mid(ActualStartPos, EndPos - ActualStartPos).TrimStartAndEnd();
}
void UMCP_EditorSubsystem::HandleFindAssetByName(const FString& NamePattern, const FString& AssetType, FString& OutJsonString, FString& OutError)
{
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
		else if (AssetType.Equals(TEXT("DataTable"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		}
	}
	AssetRegistry.GetAssets(Filter, FoundAssets);
	TArray<FAssetData> MatchingAssets;
	for (const FAssetData& Asset : FoundAssets)
	{
		if (Asset.AssetName.ToString().Contains(NamePattern, ESearchCase::IgnoreCase))
		{
			MatchingAssets.Add(Asset);
			if (MatchingAssets.Num() >= 20) break;
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
	ResultObject->SetBoolField(TEXT("success"), true);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleMoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutJsonString, FString& OutError)
{
	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
		return;
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
		TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("old_path"), AssetPath);
		ResultObject->SetStringField(TEXT("new_path"), NewAssetPath);
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	}
	else
	{
		OutError = FString::Printf(TEXT("Failed to move asset '%s' to '%s'"), *AssetPath, *DestinationFolder);
	}
}
void UMCP_EditorSubsystem::HandleMoveAssets(const TArray<FString>& AssetPaths, const FString& DestinationFolder, FString& OutJsonString, FString& OutError)
{
	if (!UEditorAssetLibrary::DoesDirectoryExist(DestinationFolder))
	{
		UEditorAssetLibrary::MakeDirectory(DestinationFolder);
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().ScanPathsSynchronous({ DestinationFolder }, true);
	}
	int32 MovedCount = 0;
	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	for (const FString& AssetPath : AssetPaths)
	{
		TSharedPtr<FJsonObject> MoveResult = MakeShareable(new FJsonObject);
		MoveResult->SetStringField(TEXT("original_path"), AssetPath);
		if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			MoveResult->SetBoolField(TEXT("success"), false);
			MoveResult->SetStringField(TEXT("error"), TEXT("Asset not found"));
		}
		else
		{
			FString AssetName = FPackageName::GetLongPackageAssetName(AssetPath);
			FString NewAssetPath = DestinationFolder / AssetName;
			bool bMoved = UEditorAssetLibrary::RenameAsset(AssetPath, NewAssetPath);
			MoveResult->SetBoolField(TEXT("success"), bMoved);
			if (bMoved)
			{
				MoveResult->SetStringField(TEXT("new_path"), NewAssetPath);
				MovedCount++;
			}
			else
			{
				MoveResult->SetStringField(TEXT("error"), TEXT("Failed to move"));
			}
		}
		ResultsArray.Add(MakeShareable(new FJsonValueObject(MoveResult)));
	}
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("moved_count"), MovedCount);
	ResultObject->SetNumberField(TEXT("total_count"), AssetPaths.Num());
	ResultObject->SetArrayField(TEXT("results"), ResultsArray);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleGenerateTexture(const FString& Prompt, const FString& SavePath, const FString& AspectRatio, const FString& AssetName, FString& OutJsonString, FString& OutError)
{
	FString ApiKey = FApiKeyManager::Get().GetActiveApiKey();
	if (ApiKey.IsEmpty())
	{
		OutError = TEXT("No API key configured. Please configure an API key in the plugin settings.");
		return;
	}
	FString TargetSavePath = SavePath.IsEmpty() ? TEXT("/Game/GeneratedTextures") : SavePath;
	FString TargetAspectRatio = AspectRatio.IsEmpty() ? TEXT("1:1") : AspectRatio;
	FTextureGenRequest Request;
	Request.Prompt = Prompt;
	Request.AspectRatio = TargetAspectRatio;
	Request.SavePath = TargetSavePath;
	Request.CustomAssetName = AssetName;
	FTextureGenManager::Get().GenerateTexture(Request, ApiKey, [Prompt, TargetSavePath](const FTextureGenResult& Result)
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([Result, Prompt, TargetSavePath]()
		{
			if (Result.bSuccess)
			{
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Texture Generated: %s"), *Result.AssetPath)));
				Info.ExpireDuration = 5.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("Texture Generation Failed: %s"), *Result.ErrorMessage)));
				Info.ExpireDuration = 10.0f;
				Info.bUseSuccessFailIcons = true;
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		}, TStatId(), nullptr, ENamedThreads::GameThread);
	});
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("Texture generation started. Check Content Browser for the result (may take 5-30 seconds)."));
	ResultObject->SetStringField(TEXT("save_path"), TargetSavePath);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleGeneratePBRMaterial(const FString& Prompt, const FString& SavePath, const FString& MaterialName, FString& OutJsonString, FString& OutError)
{
	OutError = TEXT("PBR Material generation via MCP is not recommended due to long generation times (2-5 minutes). Use the Blueprint Architect widget UI instead.");
}
void UMCP_EditorSubsystem::HandleCreateMaterialFromTextures(const FString& SavePath, const FString& MaterialName, const TArray<TSharedPtr<FJsonValue>>& TexturePaths, FString& OutJsonString, FString& OutError)
{
	if (TexturePaths.Num() == 0)
	{
		OutError = TEXT("No texture paths provided");
		return;
	}
	FString TargetSavePath = SavePath;
	if (TargetSavePath.IsEmpty())
	{
		FString DummyError;
		HandleGetFocusedContentBrowserPath(TargetSavePath, DummyError);
	}
	if (TargetSavePath.IsEmpty()) TargetSavePath = TEXT("/Game/Materials");
	FString FullMaterialPath = FString::Printf(TEXT("%s/%s"), *TargetSavePath, *MaterialName);
	if (UEditorAssetLibrary::DoesAssetExist(FullMaterialPath))
	{
		OutError = FString::Printf(TEXT("Material already exists at: %s"), *FullMaterialPath);
		return;
	}
	auto DetectTextureType = [](const FString& Name) -> int32 {
		FString LowerName = Name.ToLower();
		if (LowerName.Contains(TEXT("basecolor")) || LowerName.Contains(TEXT("base_color")) ||
			LowerName.Contains(TEXT("albedo")) || LowerName.Contains(TEXT("diffuse")) ||
			LowerName.Contains(TEXT("color")) || LowerName.EndsWith(TEXT("_bc")) ||
			LowerName.Contains(TEXT("_bc.")) || LowerName.EndsWith(TEXT("_d")) ||
			LowerName.Contains(TEXT("_d.")))
		{
			return 0;
		}
		if (LowerName.Contains(TEXT("normal")) || LowerName.Contains(TEXT("_nrm")) ||
			LowerName.EndsWith(TEXT("_n")) || LowerName.Contains(TEXT("_n.")))
		{
			return 1;
		}
		if (LowerName.Contains(TEXT("roughness")) || LowerName.Contains(TEXT("rough")) ||
			LowerName.EndsWith(TEXT("_r")) || LowerName.Contains(TEXT("_r.")) ||
			LowerName.Contains(TEXT("_rough.")))
		{
			return 2;
		}
		if (LowerName.Contains(TEXT("metallic")) || LowerName.Contains(TEXT("metal")) ||
			(LowerName.EndsWith(TEXT("_m")) && !LowerName.Contains(TEXT("norm"))) ||
			LowerName.Contains(TEXT("_m.")))
		{
			return 3;
		}
		if (LowerName.Contains(TEXT("ao")) || LowerName.Contains(TEXT("ambient")) ||
			LowerName.Contains(TEXT("occlusion")) || LowerName.Contains(TEXT("_ao")) ||
			LowerName.Contains(TEXT("_ao.")))
		{
			return 4;
		}
		if (LowerName.Contains(TEXT("emissive")) || LowerName.Contains(TEXT("emit")) ||
			LowerName.Contains(TEXT("emission")) || LowerName.EndsWith(TEXT("_e")) ||
			LowerName.Contains(TEXT("_e.")))
		{
			return 5;
		}
		if (LowerName.Contains(TEXT("opacity")) || LowerName.Contains(TEXT("alpha")))
		{
			return 6;
		}
		return -1;
	};
	TMap<int32, FString> TextureMap;
	TArray<FString> DetectedTextures;
	for (const TSharedPtr<FJsonValue>& TexValue : TexturePaths)
	{
		FString TexPath;
		if (TexValue->Type == EJson::String)
		{
			TexPath = TexValue->AsString();
		}
		else if (TexValue->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject>* TexObj = nullptr;
			if (TexValue->TryGetObject(TexObj) && TexObj)
			{
				(*TexObj)->TryGetStringField(TEXT("path"), TexPath);
			}
		}
		if (TexPath.IsEmpty()) continue;
		FString TexName = FPaths::GetBaseFilename(TexPath);
		int32 TexType = DetectTextureType(TexName);
		if (TexValue->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject>* TexObj = nullptr;
			if (TexValue->TryGetObject(TexObj) && TexObj)
			{
				FString ManualType;
				if ((*TexObj)->TryGetStringField(TEXT("type"), ManualType))
				{
					if (ManualType == TEXT("BaseColor")) TexType = 0;
					else if (ManualType == TEXT("Normal")) TexType = 1;
					else if (ManualType == TEXT("Roughness")) TexType = 2;
					else if (ManualType == TEXT("Metallic")) TexType = 3;
					else if (ManualType == TEXT("AO") || ManualType == TEXT("AmbientOcclusion")) TexType = 4;
					else if (ManualType == TEXT("Emissive")) TexType = 5;
					else if (ManualType == TEXT("Opacity")) TexType = 6;
				}
			}
		}
		if (TexType >= 0 && !TextureMap.Contains(TexType))
		{
			TextureMap.Add(TexType, TexPath);
			const TCHAR* TypeNames[] = {TEXT("BaseColor"), TEXT("Normal"), TEXT("Roughness"), TEXT("Metallic"), TEXT("AO"), TEXT("Emissive"), TEXT("Opacity")};
			DetectedTextures.Add(FString::Printf(TEXT("%s -> %s"), *TexName, TypeNames[TexType]));
		}
	}
	if (TextureMap.Num() == 0)
	{
		OutError = TEXT("Could not detect any valid texture types. Use naming patterns like 'BaseColor', 'Normal', 'Roughness', 'Metallic', 'AO' or provide 'type' field.");
		return;
	}
	UPackage* Package = CreatePackage(*FullMaterialPath);
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(), Package, FName(*MaterialName),
		RF_Public | RF_Standalone, nullptr, GWarn));
	if (!NewMaterial)
	{
		OutError = TEXT("Failed to create material");
		return;
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
		case 0:
			NewMaterial->GetEditorOnlyData()->BaseColor.Expression = Expression;
			ConnectedTextures.Add(TEXT("BaseColor"));
			break;
		case 1:
			NewMaterial->GetEditorOnlyData()->Normal.Expression = Expression;
			ConnectedTextures.Add(TEXT("Normal"));
			break;
		case 2:
			NewMaterial->GetEditorOnlyData()->Roughness.Expression = Expression;
			ConnectedTextures.Add(TEXT("Roughness"));
			break;
		case 3:
			NewMaterial->GetEditorOnlyData()->Metallic.Expression = Expression;
			ConnectedTextures.Add(TEXT("Metallic"));
			break;
		case 4:
			NewMaterial->GetEditorOnlyData()->AmbientOcclusion.Expression = Expression;
			ConnectedTextures.Add(TEXT("AmbientOcclusion"));
			break;
		case 5:
			NewMaterial->GetEditorOnlyData()->EmissiveColor.Expression = Expression;
			ConnectedTextures.Add(TEXT("Emissive"));
			break;
		case 6:
			NewMaterial->GetEditorOnlyData()->Opacity.Expression = Expression;
			NewMaterial->BlendMode = BLEND_Translucent;
			ConnectedTextures.Add(TEXT("Opacity"));
			break;
		}
	}
	NewMaterial->MarkPackageDirty();
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FString PackagePath = FPackageName::GetLongPackagePath(NewMaterial->GetPathName());
	if (!PackagePath.IsEmpty())
	{
		AssetRegistryModule.Get().ScanPathsSynchronous({ PackagePath }, true);
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("material_path"), NewMaterial->GetPathName());
	ResultObject->SetNumberField(TEXT("textures_connected"), ConnectedTextures.Num());
	ResultObject->SetArrayField(TEXT("connected"), *new TArray<TSharedPtr<FJsonValue>>());
	TArray<TSharedPtr<FJsonValue>>& ConnectedArray = const_cast<TArray<TSharedPtr<FJsonValue>>&>(ResultObject->GetArrayField(TEXT("connected")));
	for (const FString& Tex : ConnectedTextures)
	{
		ConnectedArray.Add(MakeShareable(new FJsonValueString(Tex)));
	}
	ResultObject->SetArrayField(TEXT("detected"), *new TArray<TSharedPtr<FJsonValue>>());
	TArray<TSharedPtr<FJsonValue>>& DetectedArray = const_cast<TArray<TSharedPtr<FJsonValue>>&>(ResultObject->GetArrayField(TEXT("detected")));
	for (const FString& Tex : DetectedTextures)
	{
		DetectedArray.Add(MakeShareable(new FJsonValueString(Tex)));
	}
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleCreateModel3D(const FString& Prompt, const FString& ModelName, const FString& SavePath, bool bAutoRefine, int32 Seed, FString& OutJsonString, FString& OutError)
{
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	E3DModelType ModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
	FString ApiKey;
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), ApiKey, GEditorPerProjectIni);
	if (ApiKey.IsEmpty())
	{
		ApiKey = ActiveSlot.ApiKey;
	}
	if (ApiKey.IsEmpty() && ModelType != E3DModelType::custom && ModelType != E3DModelType::others)
	{
		OutError = TEXT("No API key configured. Please configure an API key in the plugin settings.");
		return;
	}
	FString TargetSavePath = SavePath.IsEmpty() ? TEXT("/Game/GeneratedModels") : SavePath;
	FMeshCreationRequest Request;
	Request.Prompt = Prompt;
	Request.CustomAssetName = ModelName;
	Request.SavePath = TargetSavePath;
	Request.bAutoRefine = bAutoRefine;
	Request.Seed = Seed;
	Request.ModelType = ModelType;
	if (ModelType == E3DModelType::custom || ModelType == E3DModelType::others)
	{
		Request.CustomEndpoint = ActiveSlot.MeshGenEndpoint;
		Request.CustomModelName = ActiveSlot.MeshGenModel;
		Request.ModelFormat = ActiveSlot.MeshGenFormat.IsEmpty() ? TEXT("glb") : ActiveSlot.MeshGenFormat;
	}
	std::atomic<bool> bStarted{false};
	FString TaskId;
	FMeshAssetManager::Get().CreateMeshFromText(Request, ApiKey, [&](const FMeshCreationResult& Result)
	{
		bStarted = true;
		if (Result.bSuccess)
		{
			TaskId = Result.TaskId;
		}
	});
	FPlatformProcess::Sleep(0.5f);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("task_id"), TaskId);
	ResultObject->SetStringField(TEXT("message"), TEXT("3D model generation started. Use refine_model_3d to check status."));
	ResultObject->SetStringField(TEXT("prompt"), Prompt);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleRefineModel3D(const FString& TaskId, const FString& ModelName, const FString& SavePath, FString& OutJsonString, FString& OutError)
{
	if (TaskId.IsEmpty())
	{
		OutError = TEXT("Missing task_id");
		return;
	}
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKey;
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), ApiKey, GEditorPerProjectIni);
	if (ApiKey.IsEmpty())
	{
		ApiKey = ActiveSlot.ApiKey;
	}
	if (ApiKey.IsEmpty())
	{
		OutError = TEXT("No API key configured. Please configure it in the plugin settings.");
		return;
	}
	FString TargetSavePath = SavePath.IsEmpty() ? TEXT("/Game/GeneratedModels") : SavePath;
	FMeshCreationRequest Request;
	Request.PreviewTaskId = TaskId;
	Request.CustomAssetName = ModelName;
	Request.SavePath = TargetSavePath;
	FMeshAssetManager::Get().RefineMeshPreview(Request, ApiKey, [&](const FMeshCreationResult& Result)
	{
	});
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("task_id"), TaskId);
	ResultObject->SetStringField(TEXT("message"), TEXT("3D model refinement started. Check Content Browser for the generated mesh."));
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleImageToModel3D(const FString& ImagePath, const FString& Prompt, const FString& ModelName, const FString& SavePath, FString& OutJsonString, FString& OutError)
 {
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	E3DModelType ModelType = FApiKeyManager::Get().GetActiveSelected3DModel();
	FString ApiKey;
	GConfig->GetString(TEXT("BpGeneratorUltimate"), TEXT("MeshyApiKey"), ApiKey, GEditorPerProjectIni);
	if (ApiKey.IsEmpty())
	{
		ApiKey = ActiveSlot.ApiKey;
	}
	if (ApiKey.IsEmpty() && ModelType != E3DModelType::custom && ModelType != E3DModelType::others)
	{
		OutError = TEXT("No API key configured. Please configure an API key in the plugin settings.");
		return;
	}
	if (!FPaths::FileExists(ImagePath))
	{
		OutError = FString::Printf(TEXT("Image file not found: %s"), *ImagePath);
		return;
	}
	FString TargetSavePath = SavePath.IsEmpty() ? TEXT("/Game/GeneratedModels") : SavePath;
	FMeshCreationRequest Request;
	Request.ImagePath = ImagePath;
	Request.Prompt = Prompt;
	Request.CustomAssetName = ModelName;
	Request.SavePath = TargetSavePath;
	Request.ModelType = ModelType;
	if (ModelType == E3DModelType::custom || ModelType == E3DModelType::others)
	{
		Request.CustomEndpoint = ActiveSlot.MeshGenEndpoint;
		Request.CustomModelName = ActiveSlot.MeshGenModel;
		Request.ModelFormat = ActiveSlot.MeshGenFormat.IsEmpty() ? TEXT("glb") : ActiveSlot.MeshGenFormat;
	}
	FMeshAssetManager::Get().CreateMeshFromImage(Request, ApiKey, [&](const FMeshCreationResult& Result)
	{
	});
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("pending"), true);
	ResultObject->SetStringField(TEXT("message"), TEXT("Image-to-3D generation started. Check Content Browser for the generated mesh."));
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleCreateInputAction(const FString& ActionName, const FString& SavePath, const FString& ValueType, FString& OutJsonString, FString& OutError)
{
	FString TargetSavePath = SavePath;
	if (TargetSavePath.IsEmpty())
	{
		FString DummyError;
		HandleGetFocusedContentBrowserPath(TargetSavePath, DummyError);
	}
	if (TargetSavePath.IsEmpty()) TargetSavePath = TEXT("/Game/Input");
	if (!UEditorAssetLibrary::DoesDirectoryExist(TargetSavePath))
	{
		UEditorAssetLibrary::MakeDirectory(TargetSavePath);
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UInputAction* NewAction = Cast<UInputAction>(AssetToolsModule.Get().CreateAsset(ActionName, TargetSavePath, UInputAction::StaticClass(), nullptr));
	if (!NewAction)
	{
		OutError = TEXT("Failed to create Input Action asset");
		return;
	}
	EInputActionValueType ActionType = EInputActionValueType::Boolean;
	if (ValueType == TEXT("float")) ActionType = EInputActionValueType::Axis1D;
	else if (ValueType == TEXT("vector2d")) ActionType = EInputActionValueType::Axis2D;
	else if (ValueType == TEXT("vector3d")) ActionType = EInputActionValueType::Axis3D;
	NewAction->ValueType = ActionType;
	NewAction->MarkPackageDirty();
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), NewAction->GetPathName());
	ResultObject->SetStringField(TEXT("value_type"), ValueType.IsEmpty() ? TEXT("bool") : ValueType);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleCreateInputMappingContext(const FString& ContextName, const FString& SavePath, FString& OutJsonString, FString& OutError)
{
	FString TargetSavePath = SavePath;
	if (TargetSavePath.IsEmpty())
	{
		FString DummyError;
		HandleGetFocusedContentBrowserPath(TargetSavePath, DummyError);
	}
	if (TargetSavePath.IsEmpty()) TargetSavePath = TEXT("/Game/Input");
	if (!UEditorAssetLibrary::DoesDirectoryExist(TargetSavePath))
	{
		UEditorAssetLibrary::MakeDirectory(TargetSavePath);
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UInputMappingContext* NewContext = Cast<UInputMappingContext>(AssetToolsModule.Get().CreateAsset(ContextName, TargetSavePath, UInputMappingContext::StaticClass(), nullptr));
	if (!NewContext)
	{
		OutError = TEXT("Failed to create Input Mapping Context asset");
		return;
	}
	NewContext->MarkPackageDirty();
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), NewContext->GetPathName());
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleAddInputMapping(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TArray<TSharedPtr<FJsonValue>>& Modifiers, const TArray<TSharedPtr<FJsonValue>>& Triggers, FString& OutJsonString, FString& OutError)
{
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, *ContextPath);
	if (!Context)
	{
		OutError = FString::Printf(TEXT("Could not load Input Mapping Context: %s"), *ContextPath);
		return;
	}
	UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionPath);
	if (!Action)
	{
		OutError = FString::Printf(TEXT("Could not load Input Action: %s"), *ActionPath);
		return;
	}
	FKey MappingKey(*Key);
	if (!MappingKey.IsValid())
	{
		OutError = FString::Printf(TEXT("Invalid key: %s"), *Key);
		return;
	}
	FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, MappingKey);
	Context->MarkPackageDirty();
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Added mapping: %s -> %s"), *ActionPath, *Key));
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleCreateDataTable(const FString& TableName, const FString& SavePath, const FString& RowStructPath, const TArray<TSharedPtr<FJsonValue>>& Rows, FString& OutJsonString, FString& OutError)
{
	FString TargetSavePath = SavePath;
	if (TargetSavePath.IsEmpty())
	{
		FString DummyError;
		HandleGetFocusedContentBrowserPath(TargetSavePath, DummyError);
	}
	if (TargetSavePath.IsEmpty()) TargetSavePath = TEXT("/Game/DataTables");
	UScriptStruct* RowStruct = nullptr;
	if (!RowStructPath.IsEmpty())
	{
		RowStruct = LoadObject<UScriptStruct>(nullptr, *RowStructPath);
		if (!RowStruct)
		{
			OutError = FString::Printf(TEXT("Could not load row struct: %s"), *RowStructPath);
			return;
		}
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(TableName, TargetSavePath, UDataTable::StaticClass(), nullptr);
	UDataTable* DataTable = Cast<UDataTable>(NewAsset);
	if (!DataTable)
	{
		OutError = TEXT("Failed to create DataTable asset");
		return;
	}
	if (RowStruct)
	{
		DataTable->RowStruct = RowStruct;
	}
	int32 RowsAdded = 0;
	if (Rows.Num() > 0 && RowStruct)
	{
		for (const TSharedPtr<FJsonValue>& RowValue : Rows)
		{
			const TSharedPtr<FJsonObject>* RowObj = nullptr;
			if (!RowValue->TryGetObject(RowObj) || !RowObj) continue;
			FString RowName;
			if (!(*RowObj)->TryGetStringField(TEXT("row_name"), RowName))
			{
				(*RowObj)->TryGetStringField(TEXT("RowName"), RowName);
			}
			if (RowName.IsEmpty()) continue;
			uint8* RowData = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
			RowStruct->InitializeStruct(RowData);
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
					MatchedProperty->ImportText_Direct(*PropertyValueStr, PropertyData, nullptr, PPF_None);
				}
			}
			TMap<FName, uint8*>& MutableRowMap = const_cast<TMap<FName, uint8*>&>(DataTable->GetRowMap());
			MutableRowMap.Add(FName(*RowName), RowData);
			RowsAdded++;
		}
	}
	DataTable->MarkPackageDirty();
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("asset_path"), DataTable->GetPathName());
	ResultObject->SetNumberField(TEXT("rows_added"), RowsAdded);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleAddDataTableRows(const FString& TablePath, const TArray<TSharedPtr<FJsonValue>>& Rows, FString& OutJsonString, FString& OutError)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *TablePath);
	if (!DataTable)
	{
		OutError = FString::Printf(TEXT("Could not load DataTable: %s"), *TablePath);
		return;
	}
	UScriptStruct* RowStruct = DataTable->RowStruct;
	if (!RowStruct)
	{
		OutError = TEXT("DataTable has no row struct defined");
		return;
	}
	int32 RowsAdded = 0;
	for (const TSharedPtr<FJsonValue>& RowValue : Rows)
	{
		const TSharedPtr<FJsonObject>* RowObj = nullptr;
		if (!RowValue->TryGetObject(RowObj) || !RowObj) continue;
		FString RowName;
		if (!(*RowObj)->TryGetStringField(TEXT("row_name"), RowName)) continue;
		uint8* RowData = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
		RowStruct->InitializeStruct(RowData);
		for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (Property && !Property->IsNative())
			{
				FString PropertyValueStr;
				if ((*RowObj)->TryGetStringField(Property->GetName(), PropertyValueStr))
				{
					void* PropertyData = Property->ContainerPtrToValuePtr<void>(RowData);
					Property->ImportText_Direct(*PropertyValueStr, PropertyData, nullptr, PPF_None);
				}
			}
		}
		TMap<FName, uint8*>& MutableRowMap = const_cast<TMap<FName, uint8*>&>(DataTable->GetRowMap());
		MutableRowMap.Add(FName(*RowName), RowData);
		RowsAdded++;
	}
	DataTable->MarkPackageDirty();
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetNumberField(TEXT("rows_added"), RowsAdded);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleScanDirectory(const FString& DirectoryPath, const TArray<FString>& Extensions, FString& OutJsonString, FString& OutError)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*DirectoryPath))
	{
		OutError = FString::Printf(TEXT("Directory not found: %s"), *DirectoryPath);
		return;
	}
	TArray<FString> FoundFiles;
	TArray<FString> TargetExtensions = Extensions.Num() > 0 ? Extensions : TArray<FString>{TEXT(".cpp"), TEXT(".h"), TEXT(".cs")};
	class FFileVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TArray<FString>& Files;
		const TArray<FString>& Exts;
		FFileVisitor(TArray<FString>& InFiles, const TArray<FString>& InExts) : Files(InFiles), Exts(InExts) {}
		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			if (!bIsDirectory)
			{
				FString Ext = FPaths::GetExtension(FilenameOrDirectory);
				if (Exts.Contains(FString::Printf(TEXT(".%s"), *Ext.ToLower())) || Exts.Num() == 0)
				{
					Files.Add(FilenameOrDirectory);
				}
			}
			return true;
		}
	};
	FFileVisitor Visitor(FoundFiles, TargetExtensions);
	PlatformFile.IterateDirectoryRecursively(*DirectoryPath, Visitor);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> FilesArray;
	for (const FString& File : FoundFiles)
	{
		FilesArray.Add(MakeShareable(new FJsonValueString(File)));
	}
	ResultObject->SetArrayField(TEXT("files"), FilesArray);
	ResultObject->SetNumberField(TEXT("count"), FoundFiles.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleSelectFolder(const FString& FolderPath, FString& OutJsonString, FString& OutError)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> FolderPaths;
	FolderPaths.Add(FolderPath);
	ContentBrowserModule.Get().SyncBrowserToAssets(TArray<FAssetData>(), true);
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("folder_path"), FolderPath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Content Browser synced to: %s"), *FolderPath));
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleCheckProjectSource(FString& OutJsonString, FString& OutError)
{
	FString ProjectDir = FPaths::ProjectDir();
	FString SourceDir = FPaths::Combine(*ProjectDir, TEXT("Source"));
	bool bHasSource = FPaths::DirectoryExists(*SourceDir) && IFileManager::Get().DirectoryExists(*SourceDir);
	TArray<FString> UProjectFiles;
	IFileManager::Get().FindFiles(UProjectFiles, *FPaths::Combine(*ProjectDir, TEXT("*.uproject")), true, false);
	bool bHasProjectFile = UProjectFiles.Num() > 0;
	bool bHasSourceControl = false;
	FString ConfigPath = FPaths::Combine(*ProjectDir, TEXT("Config"));
	if (FPaths::DirectoryExists(*ConfigPath))
	{
		bHasSourceControl = FPaths::DirectoryExists(*FPaths::Combine(*ProjectDir, TEXT(".git"))) ||
						   FPaths::DirectoryExists(*FPaths::Combine(*ProjectDir, TEXT(".svn"))) ||
						   FPaths::DirectoryExists(*FPaths::Combine(*ProjectDir, TEXT(".p4")));
	}
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetBoolField(TEXT("has_source_code"), bHasSource);
	ResultObject->SetBoolField(TEXT("has_project_file"), bHasProjectFile);
	ResultObject->SetBoolField(TEXT("has_source_control"), bHasSourceControl);
	ResultObject->SetStringField(TEXT("project_directory"), ProjectDir);
	ResultObject->SetStringField(TEXT("source_directory"), SourceDir);
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
}
void UMCP_EditorSubsystem::HandleClassifyIntent(const FString& UserMessage, FString& OutJsonString, FString& OutError)
{
	if (UserMessage.IsEmpty())
	{
		OutError = TEXT("User message is empty");
		return;
	}
	FApiKeySlot ActiveSlot = FApiKeyManager::Get().GetActiveSlot();
	FString ApiKeyToUse = ActiveSlot.ApiKey;
	FString ProviderStr = ActiveSlot.Provider;
	FString BaseURL = ActiveSlot.CustomBaseURL;
	FString ModelName = ActiveSlot.CustomModelName;
	if (ApiKeyToUse.IsEmpty() && ProviderStr != TEXT("Custom"))
	{
		TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
		ResultObject->SetBoolField(TEXT("success"), true);
		ResultObject->SetStringField(TEXT("intent"), TEXT("CHAT"));
		ResultObject->SetBoolField(TEXT("needs_patterns"), false);
		ResultObject->SetStringField(TEXT("message"), TEXT("No API key configured, defaulting to CHAT intent"));
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutJsonString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		return;
	}
	FString IntentPrompt = FString::Printf(
		TEXT("Classify the user's intent. Reply with ONLY ONE of these words: CODE, ASSET, MODEL, TEXTURE, CHAT\n\n")
		TEXT("CODE = User wants to generate Blueprint code, logic, functions, variables, or modify existing Blueprint graphs\n")
		TEXT("ASSET = User wants to CREATE new assets like Input Actions, Input Mapping Contexts, Actors, Widget Blueprints, Data Tables, or other UE asset types\n")
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
		JsonPayload->SetNumberField(TEXT("max_tokens"), 10.0);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else if (ProviderStr == TEXT("DeepSeek"))
	{
		FString DeepSeekModel = TEXT("deepseek-chat");
		Request->SetURL(TEXT("https://api.deepseek.com/v1/chat/completions"));
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKeyToUse));
		JsonPayload->SetStringField(TEXT("model"), DeepSeekModel);
		TArray<TSharedPtr<FJsonValue>> Messages;
		TSharedPtr<FJsonObject> Msg = MakeShareable(new FJsonObject);
		Msg->SetStringField(TEXT("role"), TEXT("user"));
		Msg->SetStringField(TEXT("content"), IntentPrompt);
		Messages.Add(MakeShareable(new FJsonValueObject(Msg)));
		JsonPayload->SetArrayField(TEXT("messages"), Messages);
	}
	else
	{
		OutError = FString::Printf(TEXT("Unsupported provider: %s"), *ProviderStr);
		return;
	}
	TSharedRef<TJsonWriter<>> BodyWriter = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), BodyWriter);
	Request->SetContentAsString(RequestBody);
	double StartTime = FPlatformTime::Seconds();
	TSharedPtr<FString> ResponseTextPtr = MakeShareable(new FString);
	TSharedPtr<bool> bSuccessPtr = MakeShareable(new bool(false));
	TSharedPtr<bool> bPromiseSetPtr = MakeShareable(new bool(false));
	TSharedPtr<TPromise<bool>> PromisePtr = MakeShareable(new TPromise<bool>());
	TFuture<bool> Future = PromisePtr->GetFuture();
	Request->OnProcessRequestComplete().BindLambda([ResponseTextPtr, bSuccessPtr, bPromiseSetPtr, PromisePtr](FHttpRequestPtr Req, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			*ResponseTextPtr = Response->GetContentAsString();
			*bSuccessPtr = true;
		}
		if (!*bPromiseSetPtr)
		{
			*bPromiseSetPtr = true;
			PromisePtr->SetValue(true);
		}
	});
	if (!Request->ProcessRequest())
	{
		if (!*bPromiseSetPtr)
		{
			*bPromiseSetPtr = true;
			PromisePtr->SetValue(false);
		}
		OutError = TEXT("Failed to start HTTP request for intent classification");
		return;
	}
	if (!Future.WaitFor(FTimespan::FromSeconds(30.0)))
	{
		Request->CancelRequest();
		if (!*bPromiseSetPtr)
		{
			*bPromiseSetPtr = true;
			PromisePtr->SetValue(false);
		}
		OutError = TEXT("Intent classification request timed out");
		TSharedPtr<FJsonObject> FallbackResult = MakeShareable(new FJsonObject);
		FallbackResult->SetBoolField(TEXT("success"), true);
		FallbackResult->SetStringField(TEXT("intent"), TEXT("CHAT"));
		FallbackResult->SetBoolField(TEXT("needs_patterns"), false);
		FallbackResult->SetStringField(TEXT("message"), TEXT("Request timed out, defaulting to CHAT intent"));
		TSharedRef<TJsonWriter<>> FallbackWriter = TJsonWriterFactory<>::Create(&OutJsonString);
		FJsonSerializer::Serialize(FallbackResult.ToSharedRef(), FallbackWriter);
		return;
	}
	if (!(*bSuccessPtr) || ResponseTextPtr->IsEmpty())
	{
		OutError = TEXT("Intent classification request failed");
		return;
	}
	FString IntentResult;
	if (ProviderStr == TEXT("Gemini"))
	{
		TSharedPtr<FJsonObject> ResponseJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*ResponseTextPtr);
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
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*ResponseTextPtr);
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
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*ResponseTextPtr);
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
		OutError = TEXT("Could not parse intent classification response");
		return;
	}
	IntentResult = IntentResult.TrimStartAndEnd().ToUpper();
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
	double Elapsed = FPlatformTime::Seconds() - StartTime;
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("intent"), FinalIntent);
	ResultObject->SetBoolField(TEXT("needs_patterns"), bNeedsPatterns);
	ResultObject->SetStringField(TEXT("raw_response"), IntentResult);
	ResultObject->SetNumberField(TEXT("elapsed_seconds"), Elapsed);
	if (bNeedsPatterns)
	{
		ResultObject->SetStringField(TEXT("recommendation"), TEXT("User intent is CODE generation. Call get_code_generation_patterns() before using generate_blueprint_logic() or insert_blueprint_code_at_selection()."));
	}
	else if (FinalIntent == TEXT("ASSET"))
	{
		ResultObject->SetStringField(TEXT("recommendation"), TEXT("User intent is ASSET creation. Use asset creation tools directly: create_blueprint(), create_enum(), create_struct(), create_data_table(), create_material(), create_input_action(), create_input_mapping_context(), add_input_mapping(). No code patterns needed."));
	}
	else
	{
		ResultObject->SetStringField(TEXT("recommendation"), FString::Printf(TEXT("User intent is %s. No code patterns needed."), *FinalIntent));
	}
	TSharedRef<TJsonWriter<>> ResultWriter = TJsonWriterFactory<>::Create(&OutJsonString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), ResultWriter);
}
void UMCP_EditorSubsystem::HandleListPlugins(FString& OutJsonString, FString& OutError)
{
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
		PluginObj->SetStringField(TEXT("directory"), BaseDir);
		PluginsArray.Add(MakeShareable(new FJsonValueObject(PluginObj)));
	}
	ResultObject->SetArrayField(TEXT("plugins"), PluginsArray);
	ResultObject->SetNumberField(TEXT("count"), PluginsArray.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleFindPlugin(const FString& SearchPattern, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
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
	ResultObject->SetArrayField(TEXT("plugins"), FoundPluginsArray);
	ResultObject->SetNumberField(TEXT("count"), FoundPluginsArray.Num());
	ResultObject->SetBoolField(TEXT("success"), true);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleListPluginFiles(const FString& PluginName, const FString& RelativePath, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
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
		OutError = FString::Printf(TEXT("Plugin '%s' not found"), *PluginName);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString PluginBaseDir = FoundPlugin->GetBaseDir();
	if (FPaths::IsRelative(PluginBaseDir))
	{
		FString EngineDirAbs = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
		FString CombinedPath = FPaths::Combine(EngineDirAbs, PluginBaseDir);
		PluginBaseDir = FPaths::ConvertRelativePathToFull(CombinedPath);
	}
	FString SearchPath = RelativePath.IsEmpty() ? PluginBaseDir : FPaths::Combine(PluginBaseDir, RelativePath);
	if (!FPaths::DirectoryExists(*SearchPath))
	{
		OutError = FString::Printf(TEXT("Directory not found: %s"), *SearchPath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	TArray<TSharedPtr<FJsonValue>> FilesArray;
	TArray<FString> AllItems;
	IFileManager::Get().FindFiles(AllItems, *SearchPath, TEXT("*"));
	for (const FString& Item : AllItems)
	{
		if (Item == TEXT(".") || Item == TEXT(".."))
			continue;
		FString ItemPath = FPaths::Combine(SearchPath, Item);
		bool bIsDirectory = IFileManager::Get().DirectoryExists(*ItemPath);
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("name"), Item);
		FileObj->SetStringField(TEXT("path"), ItemPath);
		FileObj->SetBoolField(TEXT("is_directory"), bIsDirectory);
		if (!bIsDirectory)
		{
			FileObj->SetStringField(TEXT("extension"), FPaths::GetExtension(Item).ToLower());
			int64 FileSize = IFileManager::Get().FileSize(*ItemPath);
			FileObj->SetNumberField(TEXT("size_bytes"), FileSize);
		}
		FilesArray.Add(MakeShareable(new FJsonValueObject(FileObj)));
	}
	ResultObject->SetArrayField(TEXT("files"), FilesArray);
	ResultObject->SetNumberField(TEXT("count"), FilesArray.Num());
	ResultObject->SetStringField(TEXT("directory"), SearchPath);
	ResultObject->SetStringField(TEXT("plugin_name"), PluginName);
	ResultObject->SetBoolField(TEXT("success"), true);
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleReadCppFile(const FString& FilePath, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
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
		OutError = FString::Printf(TEXT("File not found: %s"), *AbsolutePath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString Extension = FPaths::GetExtension(AbsolutePath).ToLower();
	if (Extension != TEXT("h") && Extension != TEXT("cpp") && Extension != TEXT("hpp") && Extension != TEXT("c") && Extension != TEXT("cc") && Extension != TEXT("inl"))
	{
		OutError = FString::Printf(TEXT("File is not a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl): %s"), *Extension);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *AbsolutePath))
	{
		OutError = FString::Printf(TEXT("Failed to read file: %s"), *AbsolutePath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("file_path"), FilePath);
	ResultObject->SetStringField(TEXT("absolute_path"), AbsolutePath);
	ResultObject->SetStringField(TEXT("content"), FileContent);
	ResultObject->SetNumberField(TEXT("content_length"), FileContent.Len());
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
void UMCP_EditorSubsystem::HandleWriteCppFile(const FString& FilePath, const FString& Content, FString& OutJsonString, FString& OutError)
{
	TSharedPtr<FJsonObject> ResultObject = MakeShareable(new FJsonObject);
	FString AbsolutePath = FilePath;
	if (FPaths::IsRelative(FilePath))
	{
		AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
	}
	FString Extension = FPaths::GetExtension(AbsolutePath).ToLower();
	if (Extension != TEXT("h") && Extension != TEXT("cpp") && Extension != TEXT("hpp") && Extension != TEXT("c") && Extension != TEXT("cc") && Extension != TEXT("inl"))
	{
		OutError = FString::Printf(TEXT("File is not a C++ source file (.h, .cpp, .hpp, .c, .cc, .inl): %s"), *Extension);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString EnginePluginsDir = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
	if (!AbsolutePath.StartsWith(ProjectDir) && !AbsolutePath.StartsWith(EnginePluginsDir))
	{
		OutError = TEXT("Cannot write files outside project directory or engine plugins directory for security reasons.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString ProjectSourceDir = FPaths::Combine(ProjectDir, TEXT("Source"));
	if (AbsolutePath.StartsWith(ProjectSourceDir) && !IFileManager::Get().DirectoryExists(*ProjectSourceDir))
	{
		OutError = TEXT("This project does not have a Source folder. It appears to be a Blueprint-only project.");
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		ResultObject->SetStringField(TEXT("instructions"), TEXT("To add C++ code to your project, go to Tools > New C++ Class and create any class (e.g., a generic UActorComponent subclass). This will generate Source folder and necessary project files."));
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	FString ParentPath = FPaths::GetPath(AbsolutePath);
	if (!IFileManager::Get().DirectoryExists(*ParentPath))
	{
		IFileManager::Get().MakeDirectory(*ParentPath, true);
	}
	if (!FFileHelper::SaveStringToFile(Content, *AbsolutePath))
	{
		OutError = FString::Printf(TEXT("Failed to write file: %s"), *AbsolutePath);
		ResultObject->SetBoolField(TEXT("success"), false);
		ResultObject->SetStringField(TEXT("error"), OutError);
		FString ResultString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
		FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
		OutJsonString = ResultString;
		return;
	}
	ResultObject->SetBoolField(TEXT("success"), true);
	ResultObject->SetStringField(TEXT("file_path"), AbsolutePath);
	ResultObject->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully wrote %d characters to file."), Content.Len()));
	ResultObject->SetNumberField(TEXT("bytes_written"), Content.Len());
	FString ResultString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
	FJsonSerializer::Serialize(ResultObject.ToSharedRef(), Writer);
	OutJsonString = ResultString;
}
FMCPServerWorker::FMCPServerWorker(FSocket* InListenSocket, UMCP_EditorSubsystem* InOwner) : ListenSocket(InListenSocket), bStopping(false), OwnerSubsystem(InOwner) {}
FMCPServerWorker::~FMCPServerWorker() {}
bool FMCPServerWorker::Init() { return true; }
void FMCPServerWorker::Stop() { bStopping = true; }
uint32 FMCPServerWorker::Run()
{
	while (!bStopping)
	{
		bool bHasPendingConnection;
		if (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
		{
			FSocket* ClientSocket = ListenSocket->Accept(TEXT("MCPClientConnection"));
			if (ClientSocket)
			{
				FString FullRequestString; TArray<uint8> ReceiveBuffer; ReceiveBuffer.SetNumUninitialized(8192); while (ClientSocket->GetConnectionState() == ESocketConnectionState::SCS_Connected && !bStopping) { int32 BytesRead = 0; if (ClientSocket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead)) { if (BytesRead > 0) { FullRequestString.Append(UTF8_TO_TCHAR(ReceiveBuffer.GetData()), BytesRead); TSharedPtr<FJsonObject> JsonObject; TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FullRequestString); if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) { break; } } } else { break; } }
				FString ResponseString = TEXT("{\"success\":false, \"error\":\"Invalid or empty request\"}");
				if (!FullRequestString.IsEmpty())
				{
					TSharedPtr<FJsonObject> JsonObject; TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FullRequestString);
					if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
					{
						FString CommandType;
						if (JsonObject->TryGetStringField(TEXT("type"), CommandType))
						{
							if (CommandType == TEXT("get_selected_blueprint_path"))
							{
								FString AssetPath = "";
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&AssetPath]()
									{
										FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
										TArray<FAssetData> SelectedAssets;
										ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);
										if (SelectedAssets.Num() > 0)
										{
											AssetPath = SelectedAssets[0].GetObjectPathString();
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								ResponseString = FString::Printf(TEXT("{\"success\":true, \"path\":\"%s\"}"), *AssetPath);
							}
							else if (CommandType == TEXT("generate_blueprint_logic"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString Code = JsonObject->GetStringField(TEXT("code"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, Code, &OutError]() {
									if (OwnerSubsystem)
									{
										OwnerSubsystem->HandleGenerateBpLogic(BpPath, Code, OutError);
									}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true, \"message\":\"Code compiled successfully.\"}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_selected_nodes"))
							{
								FString OutNodesText, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutNodesText, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetSelectedNodes(OutNodesText, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString EscapedNodesText = OutNodesText.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"nodes_text\":\"%s\"}"), *EscapedNodesText);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("scan_project"))
							{
								AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this]()
									{
										if (OwnerSubsystem)
										{
											FString DummyError;
											OwnerSubsystem->HandleScanAndIndexProject(DummyError);
										}
									});
								ResponseString = TEXT("{\"success\":true, \"pending\":true, \"message\":\"Project scan started in background. Check the index file in Saved/AI/ProjectIndex.json when complete.\"}");
							}
							else if (CommandType == TEXT("get_project_root_path"))
							{
								FString OutPath, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetProjectRootPath(OutPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"path\":\"%s\"}"), *OutPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_asset_summary"))
							{
								FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
								FString OutSummary, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetPath, &OutSummary, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetAssetSummary(AssetPath, OutSummary, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString EscapedSummary = OutSummary.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"summary\":\"%s\"}"), *EscapedSummary);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_data_asset_details"))
							{
								FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
								FString OutDetailsJson, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetPath, &OutDetailsJson, &OutError]() {
									if (OwnerSubsystem)
									{
										OwnerSubsystem->HandleGetDataAssetDetails(AssetPath, OutDetailsJson, OutError);
									}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString EscapedDetails = OutDetailsJson.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"details\":\"%s\"}"), *EscapedDetails);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("export_to_file"))
							{
								FString FileName = JsonObject->GetStringField(TEXT("file_name"));
								FString FileContent = JsonObject->GetStringField(TEXT("content"));
								FString FileFormat = JsonObject->GetStringField(TEXT("format"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FileName, FileContent, FileFormat, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleExportToFile(FileName, FileContent, FileFormat, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString FullPath = (FPaths::ProjectSavedDir() / TEXT("Exported Explanations") / FString::Printf(TEXT("%s.%s"), *FileName, *FileFormat));
									FPaths::MakeStandardFilename(FullPath);
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully exported to %s\"}"), *FullPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("edit_data_asset_defaults"))
							{
								FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
								const TArray<TSharedPtr<FJsonValue>>* EditsArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("edits"), EditsArray);
								FString OutError;
								if (EditsArray)
								{
									FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetPath, Edits = *EditsArray, &OutError]()
										{
											if (OwnerSubsystem)
											{
												OwnerSubsystem->HandleEditDataAssetDefaults(AssetPath, Edits, OutError);
											}
										}, TStatId(), nullptr, ENamedThreads::GameThread);
									Task->Wait();
								}
								else
								{
									OutError = "Invalid 'edits' payload. It must be an array of objects.";
								}
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully edited default properties in '%s'.\"}"), *AssetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("create_widget_from_layout"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("user_widget_path"));
								const TArray<TSharedPtr<FJsonValue>>* LayoutArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("layout_definition"), LayoutArray);
								FString OutError;
								if (LayoutArray)
								{
									FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, Layout = *LayoutArray, &OutError]()
										{
											if (OwnerSubsystem)
											{
												OwnerSubsystem->HandleCreateWidgetFromLayout(WidgetPath, Layout, OutError);
											}
										}, TStatId(), nullptr, ENamedThreads::GameThread);
									Task->Wait();
								}
								else
								{
									OutError = "Invalid 'layout_definition' provided. It must be an array.";
								}
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully created widget layout in '%s'.\"}"), *WidgetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("add_widgets_to_layout"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("user_widget_path"));
								const TArray<TSharedPtr<FJsonValue>>* LayoutArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("layout_definition"), LayoutArray);
								FString OutError;
								if (LayoutArray)
								{
									FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, Layout = *LayoutArray, &OutError]()
										{
											if (OwnerSubsystem)
											{
												OwnerSubsystem->HandleAddWidgetsToLayout(WidgetPath, Layout, OutError);
											}
										}, TStatId(), nullptr, ENamedThreads::GameThread);
									Task->Wait();
								}
								else
								{
									OutError = "Invalid 'layout_definition' provided. It must be an array.";
								}
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully added new widgets to '%s'.\"}"), *WidgetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
								}
							else if (CommandType == TEXT("edit_widget_properties"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("user_widget_path"));
								const TArray<TSharedPtr<FJsonValue>>* EditsArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("edits"), EditsArray);
								FString OutError;
								if (EditsArray)
								{
									FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, Edits = *EditsArray, &OutError]()
										{
											if (OwnerSubsystem)
											{
												OwnerSubsystem->HandleEditWidgetProperties(WidgetPath, Edits, OutError);
											}
										}, TStatId(), nullptr, ENamedThreads::GameThread);
									Task->Wait();
								}
								else
								{
									OutError = "Invalid 'edits' payload provided. It must be an array.";
								}
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully edited properties in '%s'.\"}"), *WidgetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
								}
							else if (CommandType == TEXT("get_widget_properties"))
							{
								TArray<FString> WidgetClasses;
								const TArray<TSharedPtr<FJsonValue>>* ClassesArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("widget_classes"), ClassesArray))
								{
									for (const auto& Val : *ClassesArray)
									{
										WidgetClasses.Add(Val->AsString());
									}
								}
								FString OutJson, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetClasses, &OutJson, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetBatchWidgetProperties(WidgetClasses, OutJson, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"properties\": %s}"), *OutJson);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("insert_code"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString Code = JsonObject->GetStringField(TEXT("code"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, Code, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleInsertCode(BpPath, Code, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Code inserted successfully.\"}"));
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("spawn_actor"))
							{
								FString ActorClass = JsonObject->GetStringField(TEXT("actor_class"));
								FString ActorLabel = JsonObject->GetStringField(TEXT("actor_label"));
								const TArray<TSharedPtr<FJsonValue>>* LocArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("location"), LocArray);
								FVector Location(
									LocArray && LocArray->Num() > 0 ? (*LocArray)[0]->AsNumber() : 0.0,
									LocArray && LocArray->Num() > 1 ? (*LocArray)[1]->AsNumber() : 0.0,
									LocArray && LocArray->Num() > 2 ? (*LocArray)[2]->AsNumber() : 0.0
								);
								const TArray<TSharedPtr<FJsonValue>>* RotArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("rotation"), RotArray);
								FRotator Rotation(
									RotArray && RotArray->Num() > 0 ? (*RotArray)[0]->AsNumber() : 0.0,
									RotArray && RotArray->Num() > 1 ? (*RotArray)[1]->AsNumber() : 0.0,
									RotArray && RotArray->Num() > 2 ? (*RotArray)[2]->AsNumber() : 0.0
								);
								const TArray<TSharedPtr<FJsonValue>>* ScaleArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("scale"), ScaleArray);
								FVector Scale(
									ScaleArray && ScaleArray->Num() > 0 ? (*ScaleArray)[0]->AsNumber() : 1.0,
									ScaleArray && ScaleArray->Num() > 1 ? (*ScaleArray)[1]->AsNumber() : 1.0,
									ScaleArray && ScaleArray->Num() > 2 ? (*ScaleArray)[2]->AsNumber() : 1.0
								);
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ActorClass, Location, Rotation, Scale, ActorLabel, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSpawnActorInLevel(ActorClass, Location, Rotation, Scale, ActorLabel, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("spawn_multiple_actors"))
							{
								TArray<FSpawnRequest> SpawnRequests;
								const TArray<TSharedPtr<FJsonValue>>* RequestsArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("requests"), RequestsArray))
								{
									for (const TSharedPtr<FJsonValue>& RequestValue : *RequestsArray)
									{
										const TSharedPtr<FJsonObject>& RequestObject = RequestValue->AsObject();
										if (RequestObject.IsValid())
										{
											FSpawnRequest NewRequest;
											NewRequest.ActorClass = RequestObject->GetStringField(TEXT("actor_class"));
											NewRequest.ActorLabel = RequestObject->GetStringField(TEXT("actor_label"));
											const TArray<TSharedPtr<FJsonValue>>* LocArray = nullptr;
											RequestObject->TryGetArrayField(TEXT("location"), LocArray);
											NewRequest.Location = FVector(
												LocArray && LocArray->Num() > 0 ? (*LocArray)[0]->AsNumber() : 0.0,
												LocArray && LocArray->Num() > 1 ? (*LocArray)[1]->AsNumber() : 0.0,
												LocArray && LocArray->Num() > 2 ? (*LocArray)[2]->AsNumber() : 0.0
											);
											const TArray<TSharedPtr<FJsonValue>>* RotArray = nullptr;
											RequestObject->TryGetArrayField(TEXT("rotation"), RotArray);
											NewRequest.Rotation = FRotator(
												RotArray && RotArray->Num() > 0 ? (*RotArray)[0]->AsNumber() : 0.0,
												RotArray && RotArray->Num() > 1 ? (*RotArray)[1]->AsNumber() : 0.0,
												RotArray && RotArray->Num() > 2 ? (*RotArray)[2]->AsNumber() : 0.0
											);
											const TArray<TSharedPtr<FJsonValue>>* ScaleArray = nullptr;
											RequestObject->TryGetArrayField(TEXT("scale"), ScaleArray);
											NewRequest.Scale = FVector(
												ScaleArray && ScaleArray->Num() > 0 ? (*ScaleArray)[0]->AsNumber() : 1.0,
												ScaleArray && ScaleArray->Num() > 1 ? (*ScaleArray)[1]->AsNumber() : 1.0,
												ScaleArray && ScaleArray->Num() > 2 ? (*ScaleArray)[2]->AsNumber() : 1.0
											);
											SpawnRequests.Add(NewRequest);
										}
									}
								}
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, SpawnRequests, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSpawnMultipleActorsInLevel(SpawnRequests, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Processed %d spawn requests.\"}"), SpawnRequests.Num());
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("create_struct"))
							{
								FString StructName = JsonObject->GetStringField(TEXT("struct_name"));
								FString SavePath = JsonObject->GetStringField(TEXT("save_path"));
								const TArray<TSharedPtr<FJsonValue>>* Variables = nullptr;
								JsonObject->TryGetArrayField(TEXT("variables"), Variables);
								FString OutAssetPath, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, StructName, SavePath, Variables, &OutAssetPath, &OutError]()
									{
										if (OwnerSubsystem && Variables)
										{
											OwnerSubsystem->HandleCreateStruct(StructName, SavePath, *Variables, OutAssetPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"asset_path\":\"%s\"}"), *OutAssetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
}
							else if (CommandType == TEXT("create_enum"))
							{
								FString EnumName = JsonObject->GetStringField(TEXT("enum_name"));
								FString SavePath = JsonObject->GetStringField(TEXT("save_path"));
								TArray<FString> Enumerators;
								const TArray<TSharedPtr<FJsonValue>>* EnumeratorsJson = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("enumerators"), EnumeratorsJson))
								{
									for (const TSharedPtr<FJsonValue>& Val : *EnumeratorsJson)
									{
										Enumerators.Add(Val->AsString());
									}
								}
								FString OutAssetPath, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, EnumName, SavePath, Enumerators, &OutAssetPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateEnum(EnumName, SavePath, Enumerators, OutAssetPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"asset_path\":\"%s\"}"), *OutAssetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("spawn_advanced"))
							{
								int32 Count = JsonObject->GetIntegerField(TEXT("count"));
								FString ActorClass = JsonObject->GetStringField(TEXT("actor_class"));
								FString BoundingActorLabel = JsonObject->GetStringField(TEXT("bounding_actor_label"));
								auto GetVectorFromJson = [](const TSharedPtr<FJsonObject>& Obj, const FString& Key, const FVector& Default) -> FVector
									{
										const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
										if (Obj->TryGetArrayField(Key, Arr) && Arr->Num() == 3)
										{
											return FVector((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
										}
										return Default;
									};
								auto GetRotatorFromJson = [](const TSharedPtr<FJsonObject>& Obj, const FString& Key, const FRotator& Default) -> FRotator
									{
										const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
										if (Obj->TryGetArrayField(Key, Arr) && Arr->Num() == 3)
										{
											return FRotator((*Arr)[0]->AsNumber(), (*Arr)[1]->AsNumber(), (*Arr)[2]->AsNumber());
										}
										return Default;
									};
								FVector LocMin = GetVectorFromJson(JsonObject, TEXT("random_location_min"), FVector(-1000, -1000, 0));
								FVector LocMax = GetVectorFromJson(JsonObject, TEXT("random_location_max"), FVector(1000, 1000, 0));
								FVector ScaleMin = GetVectorFromJson(JsonObject, TEXT("random_scale_min"), FVector(1, 1, 1));
								FVector ScaleMax = GetVectorFromJson(JsonObject, TEXT("random_scale_max"), FVector(1, 1, 1));
								FRotator RotMin = GetRotatorFromJson(JsonObject, TEXT("random_rotation_min"), FRotator::ZeroRotator);
								FRotator RotMax = GetRotatorFromJson(JsonObject, TEXT("random_rotation_max"), FRotator::ZeroRotator);
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, Count, ActorClass, BoundingActorLabel, LocMin, LocMax, ScaleMin, ScaleMax, RotMin, RotMax, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAdvancedSpawn(Count, ActorClass, BoundingActorLabel, LocMin, LocMax, ScaleMin, ScaleMax, RotMin, RotMax, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Processed advanced spawn request for %d actors.\"}"), Count);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_focused_folder_path"))
							{
								FString OutPath, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetFocusedContentBrowserPath(OutPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"path\":\"%s\"}"), *OutPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("create_project_folder"))
							{
								FString FolderPath = JsonObject->GetStringField(TEXT("folder_path"));
								if (!FolderPath.StartsWith(TEXT("/Game/")))
								{
									FolderPath = TEXT("/Game/") + FolderPath;
								}
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FolderPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateProjectFolder(FolderPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully created folder at %s\"}"), *FolderPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_all_scene_actors"))
							{
								FString OutJson, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJson, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetAllSceneActors(OutJson, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"actors\": %s}"), *OutJson);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("set_selected_actors"))
							{
								TArray<FString> ActorLabels;
								const TArray<TSharedPtr<FJsonValue>>* LabelsArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("actor_labels"), LabelsArray))
								{
									for (const TSharedPtr<FJsonValue>& Val : *LabelsArray)
									{
										ActorLabels.Add(Val->AsString());
									}
								}
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ActorLabels, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSetSelectedActors(ActorLabels, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Updated actor selection. %d actors selected.\"}"), ActorLabels.Num());
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("create_material"))
							{
								FString MaterialPath = JsonObject->GetStringField(TEXT("material_path"));
								const TArray<TSharedPtr<FJsonValue>>* ColorArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("color"), ColorArray);
								FLinearColor Color(
									ColorArray && ColorArray->Num() > 0 ? (*ColorArray)[0]->AsNumber() : 1.0,
									ColorArray && ColorArray->Num() > 1 ? (*ColorArray)[1]->AsNumber() : 1.0,
									ColorArray && ColorArray->Num() > 2 ? (*ColorArray)[2]->AsNumber() : 1.0,
									ColorArray && ColorArray->Num() > 3 ? (*ColorArray)[3]->AsNumber() : 1.0
								);
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, MaterialPath, Color, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateMaterial(MaterialPath, Color, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully created material '%s'\"}"), *MaterialPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("set_actor_material"))
							{
								FString ActorLabel = JsonObject->GetStringField(TEXT("actor_label"));
								FString MaterialPath = JsonObject->GetStringField(TEXT("material_path"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ActorLabel, MaterialPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSetActorMaterial(ActorLabel, MaterialPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully applied material '%s' to actor '%s'\"}"), *MaterialPath, *ActorLabel);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("set_multiple_actor_materials"))
							{
								FString MaterialPath = JsonObject->GetStringField(TEXT("material_path"));
								TArray<FString> ActorLabels;
								const TArray<TSharedPtr<FJsonValue>>* LabelsArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("actor_labels"), LabelsArray))
								{
									for (const TSharedPtr<FJsonValue>& Val : *LabelsArray)
									{
										ActorLabels.Add(Val->AsString());
									}
								}
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ActorLabels, MaterialPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSetMultipleActorMaterials(ActorLabels, MaterialPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully processed bulk material request for %d actors.\"}"), ActorLabels.Num());
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("add_widget_to_user_widget"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("user_widget_path"));
								FString WidgetType = JsonObject->GetStringField(TEXT("widget_type"));
								FString WidgetName = JsonObject->GetStringField(TEXT("widget_name"));
								FString ParentName = JsonObject->GetStringField(TEXT("parent_widget_name"));
								FString OutNewName, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, WidgetType, WidgetName, ParentName, &OutNewName, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAddWidgetToUserWidget(WidgetPath, WidgetType, WidgetName, ParentName, OutNewName, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"widget_name\":\"%s\", \"message\":\"Successfully added widget '%s' to '%s'.\"}"), *OutNewName, *OutNewName, *WidgetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("edit_widget_property"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("user_widget_path"));
								FString WidgetName = JsonObject->GetStringField(TEXT("widget_name"));
								FString PropName = JsonObject->GetStringField(TEXT("property_name"));
								FString PropValue = JsonObject->GetStringField(TEXT("value"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, WidgetName, PropName, PropValue, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleEditWidgetProperty(WidgetPath, WidgetName, PropName, PropValue, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"message\":\"Successfully set property '%s' on widget '%s'.\"}"), *PropName, *WidgetName);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("list_assets_in_folder"))
							{
								FString FolderPath = JsonObject->GetStringField(TEXT("folder_path"));
								FString OutJson, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FolderPath, &OutJson, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleListAssetsInFolder(FolderPath, OutJson, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"assets\": %s}"), *OutJson);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("get_selected_content_browser_assets"))
							{
								FString OutJson, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJson, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetSelectedContentBrowserAssets(OutJson, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"assets\": %s}"), *OutJson);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("create_blueprint"))
							{
								FString BpName = JsonObject->GetStringField(TEXT("blueprint_name"));
								FString ParentClass = JsonObject->GetStringField(TEXT("parent_class"));
								FString SavePath = JsonObject->GetStringField(TEXT("save_path"));
								FString OutAssetPath, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpName, ParentClass, SavePath, &OutAssetPath, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateBlueprint(BpName, ParentClass, SavePath, OutAssetPath, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"asset_path\":\"%s\"}"), *OutAssetPath);
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("add_component"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString CompClass = JsonObject->GetStringField(TEXT("component_class"));
								FString CompName = JsonObject->GetStringField(TEXT("component_name"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, CompClass, CompName, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAddComponent(BpPath, CompClass, CompName, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = OutJsonString;
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("edit_component_property"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString CompName = JsonObject->GetStringField(TEXT("component_name"));
								FString PropName = JsonObject->GetStringField(TEXT("property_name"));
								FString PropValue = JsonObject->GetStringField(TEXT("property_value"));
								FString JsonResult;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, CompName, PropName, PropValue, &JsonResult]()
									{
										if (OwnerSubsystem)
										{
											JsonResult = OwnerSubsystem->HandleEditComponentProperty(BpPath, CompName, PropName, PropValue);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								ResponseString = JsonResult;
							}
							else if (CommandType == TEXT("add_variable"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString VarName = JsonObject->GetStringField(TEXT("variable_name"));
								FString VarType = JsonObject->GetStringField(TEXT("variable_type"));
								FString DefaultValue = JsonObject->GetStringField(TEXT("default_value"));
								FString Category = JsonObject->GetStringField(TEXT("category"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, VarName, VarType, DefaultValue, Category, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAddVariable(BpPath, VarName, VarType, DefaultValue, Category, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("delete_variable"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString VarName = JsonObject->GetStringField(TEXT("var_name"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, VarName, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteVariable(BpPath, VarName, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("categorize_variables"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								const TSharedPtr<FJsonObject>* CategoriesObj = nullptr;
								bool bHasCategories = JsonObject->TryGetObjectField(TEXT("categories"), CategoriesObj);
								FString OutJsonString;
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, CategoriesObj, bHasCategories, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCategorizeVariables(BpPath, CategoriesObj, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = OutJsonString;
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("delete_component"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString ComponentName = JsonObject->GetStringField(TEXT("component_name"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, ComponentName, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteComponent(BpPath, ComponentName, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("delete_nodes"))
							{
								FString OutJsonString;
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteNodes(OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = OutJsonString;
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("delete_widget"))
							{
								FString WidgetPath = JsonObject->GetStringField(TEXT("widget_path"));
								FString WidgetName = JsonObject->GetStringField(TEXT("widget_name"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetPath, WidgetName, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteWidget(WidgetPath, WidgetName, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("delete_unused_variables"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString OutJsonString;
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteUnusedVariables(BpPath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								ResponseString = OutJsonString;
							}
							else if (CommandType == TEXT("delete_function"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString FunctionName = JsonObject->GetStringField(TEXT("function_name"));
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, FunctionName, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleDeleteFunction(BpPath, FunctionName, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									ResponseString = TEXT("{\"success\":true}");
								}
								else
								{
									ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError);
								}
							}
							else if (CommandType == TEXT("undo_last_generated"))
							{
								FString OutJsonString;
								FString OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleUndoLastGenerated(OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								ResponseString = OutJsonString;
							}
							else if (CommandType == TEXT("find_asset_by_name"))
							{
								FString NamePattern = JsonObject->GetStringField(TEXT("name_pattern"));
								FString AssetType;
								JsonObject->TryGetStringField(TEXT("asset_type"), AssetType);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, NamePattern, AssetType, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleFindAssetByName(NamePattern, AssetType, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("move_asset"))
							{
								FString AssetPath = JsonObject->GetStringField(TEXT("asset_path"));
								FString DestinationFolder = JsonObject->GetStringField(TEXT("destination_folder"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetPath, DestinationFolder, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleMoveAsset(AssetPath, DestinationFolder, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("move_assets"))
							{
								FString DestinationFolder = JsonObject->GetStringField(TEXT("destination_folder"));
								TArray<FString> AssetPaths;
								const TArray<TSharedPtr<FJsonValue>>* PathsArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("asset_paths"), PathsArray))
								{
									for (const auto& PathVal : *PathsArray)
									{
										AssetPaths.Add(PathVal->AsString());
									}
								}
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, AssetPaths, DestinationFolder, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleMoveAssets(AssetPaths, DestinationFolder, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("generate_texture"))
							{
								FString Prompt = JsonObject->GetStringField(TEXT("prompt"));
								FString SavePath, AspectRatio, AssetName;
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								JsonObject->TryGetStringField(TEXT("aspect_ratio"), AspectRatio);
								JsonObject->TryGetStringField(TEXT("asset_name"), AssetName);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, Prompt, SavePath, AspectRatio, AssetName, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGenerateTexture(Prompt, SavePath, AspectRatio, AssetName, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("generate_pbr_material"))
							{
								FString Prompt = JsonObject->GetStringField(TEXT("prompt"));
								FString SavePath, MaterialName;
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								JsonObject->TryGetStringField(TEXT("material_name"), MaterialName);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, Prompt, SavePath, MaterialName, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGeneratePBRMaterial(Prompt, SavePath, MaterialName, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("create_material_from_textures"))
							{
								FString SavePath = JsonObject->GetStringField(TEXT("save_path"));
								FString MaterialName = JsonObject->GetStringField(TEXT("material_name"));
								TArray<TSharedPtr<FJsonValue>> TexturePaths;
								const TArray<TSharedPtr<FJsonValue>>* TexArray = nullptr;
								if (JsonObject->TryGetArrayField(TEXT("texture_paths"), TexArray))
								{
									TexturePaths = *TexArray;
								}
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, SavePath, MaterialName, TexturePaths, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateMaterialFromTextures(SavePath, MaterialName, TexturePaths, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("create_model_3d"))
							{
								FString Prompt = JsonObject->GetStringField(TEXT("prompt"));
								FString ModelName, SavePath;
								bool bAutoRefine = true;
								int32 Seed = -1;
								JsonObject->TryGetStringField(TEXT("model_name"), ModelName);
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								JsonObject->TryGetBoolField(TEXT("auto_refine"), bAutoRefine);
								JsonObject->TryGetNumberField(TEXT("seed"), Seed);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, Prompt, ModelName, SavePath, bAutoRefine, Seed, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateModel3D(Prompt, ModelName, SavePath, bAutoRefine, Seed, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("refine_model_3d"))
							{
								FString TaskId = JsonObject->GetStringField(TEXT("task_id"));
								FString ModelName, SavePath;
								JsonObject->TryGetStringField(TEXT("model_name"), ModelName);
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, TaskId, ModelName, SavePath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleRefineModel3D(TaskId, ModelName, SavePath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("image_to_model_3d"))
							{
								FString ImagePath = JsonObject->GetStringField(TEXT("image_path"));
								FString Prompt, ModelName, SavePath;
								JsonObject->TryGetStringField(TEXT("prompt"), Prompt);
								JsonObject->TryGetStringField(TEXT("model_name"), ModelName);
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ImagePath, Prompt, ModelName, SavePath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleImageToModel3D(ImagePath, Prompt, ModelName, SavePath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("create_input_action"))
							{
								FString ActionName = JsonObject->GetStringField(TEXT("name"));
								FString SavePath, ValueType;
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								JsonObject->TryGetStringField(TEXT("value_type"), ValueType);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ActionName, SavePath, ValueType, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateInputAction(ActionName, SavePath, ValueType, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("create_input_mapping_context"))
							{
								FString ContextName = JsonObject->GetStringField(TEXT("name"));
								FString SavePath;
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ContextName, SavePath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateInputMappingContext(ContextName, SavePath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("add_input_mapping"))
							{
								FString ContextPath = JsonObject->GetStringField(TEXT("context_path"));
								FString ActionPath = JsonObject->GetStringField(TEXT("action_path"));
								FString Key = JsonObject->GetStringField(TEXT("key"));
								TArray<TSharedPtr<FJsonValue>> Modifiers, Triggers;
								const TArray<TSharedPtr<FJsonValue>>* ModArray = nullptr;
								const TArray<TSharedPtr<FJsonValue>>* TrigArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("modifiers"), ModArray);
								JsonObject->TryGetArrayField(TEXT("triggers"), TrigArray);
								if (ModArray) Modifiers = *ModArray;
								if (TrigArray) Triggers = *TrigArray;
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, ContextPath, ActionPath, Key, Modifiers, Triggers, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAddInputMapping(ContextPath, ActionPath, Key, Modifiers, Triggers, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("create_data_table"))
							{
								FString TableName = JsonObject->GetStringField(TEXT("name"));
								FString SavePath, RowStructPath;
								TArray<TSharedPtr<FJsonValue>> Rows;
								JsonObject->TryGetStringField(TEXT("save_path"), SavePath);
								JsonObject->TryGetStringField(TEXT("row_struct"), RowStructPath);
								const TArray<TSharedPtr<FJsonValue>>* RowsArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("rows"), RowsArray);
								if (RowsArray) Rows = *RowsArray;
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, TableName, SavePath, RowStructPath, Rows, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCreateDataTable(TableName, SavePath, RowStructPath, Rows, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("add_data_table_rows"))
							{
								FString TablePath = JsonObject->GetStringField(TEXT("table_path"));
								TArray<TSharedPtr<FJsonValue>> Rows;
								const TArray<TSharedPtr<FJsonValue>>* RowsArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("rows"), RowsArray);
								if (RowsArray) Rows = *RowsArray;
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, TablePath, Rows, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleAddDataTableRows(TablePath, Rows, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("get_detailed_blueprint_summary"))
							{
								FString BpPath = JsonObject->GetStringField(TEXT("blueprint_path"));
								FString OutSummary, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BpPath, &OutSummary, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetDetailedBlueprintSummary(BpPath, OutSummary, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString Escaped = OutSummary.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"summary\":\"%s\"}"), *Escaped);
								}
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("get_material_graph_summary"))
							{
								FString MaterialPath = JsonObject->GetStringField(TEXT("material_path"));
								FString OutSummary, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, MaterialPath, &OutSummary, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetMaterialGraphSummary(MaterialPath, OutSummary, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString Escaped = OutSummary.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"summary\":\"%s\"}"), *Escaped);
								}
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("get_behavior_tree_summary"))
							{
								FString BtPath = JsonObject->GetStringField(TEXT("behavior_tree_path"));
								FString OutSummary, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, BtPath, &OutSummary, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetBehaviorTreeSummary(BtPath, OutSummary, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty())
								{
									FString Escaped = OutSummary.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\"")).Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")).Replace(TEXT("\t"), TEXT("\\t"));
									ResponseString = FString::Printf(TEXT("{\"success\":true, \"summary\":\"%s\"}"), *Escaped);
								}
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("get_batch_widget_properties"))
							{
								TArray<FString> WidgetClasses;
								const TArray<TSharedPtr<FJsonValue>>* ClassesArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("widget_classes"), ClassesArray);
								if (ClassesArray)
								{
									for (const auto& ClassVal : *ClassesArray)
									{
										WidgetClasses.Add(ClassVal->AsString());
									}
								}
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WidgetClasses, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleGetBatchWidgetProperties(WidgetClasses, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("scan_directory"))
							{
								FString DirectoryPath = JsonObject->GetStringField(TEXT("directory_path"));
								TArray<FString> Extensions;
								const TArray<TSharedPtr<FJsonValue>>* ExtArray = nullptr;
								JsonObject->TryGetArrayField(TEXT("extensions"), ExtArray);
								if (ExtArray)
								{
									for (const auto& ExtVal : *ExtArray)
									{
										Extensions.Add(ExtVal->AsString());
									}
								}
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, DirectoryPath, Extensions, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleScanDirectory(DirectoryPath, Extensions, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("select_folder"))
							{
								FString FolderPath = JsonObject->GetStringField(TEXT("folder_path"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FolderPath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleSelectFolder(FolderPath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("check_project_source"))
							{
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleCheckProjectSource(OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("classify_intent"))
							{
								FString UserMessage = JsonObject->GetStringField(TEXT("user_message"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, UserMessage, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleClassifyIntent(UserMessage, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("list_plugins"))
							{
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleListPlugins(OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("find_plugin"))
							{
								FString SearchPattern = JsonObject->GetStringField(TEXT("search_pattern"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, SearchPattern, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleFindPlugin(SearchPattern, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("list_plugin_files"))
							{
								FString PluginName = JsonObject->GetStringField(TEXT("plugin_name"));
								FString RelativePath;
								JsonObject->TryGetStringField(TEXT("relative_path"), RelativePath);
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, PluginName, RelativePath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleListPluginFiles(PluginName, RelativePath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("read_cpp_file"))
							{
								FString FilePath = JsonObject->GetStringField(TEXT("file_path"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FilePath, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleReadCppFile(FilePath, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else if (CommandType == TEXT("write_cpp_file"))
							{
								FString FilePath = JsonObject->GetStringField(TEXT("file_path"));
								FString Content = JsonObject->GetStringField(TEXT("content"));
								FString OutJsonString, OutError;
								FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([this, FilePath, Content, &OutJsonString, &OutError]()
									{
										if (OwnerSubsystem)
										{
											OwnerSubsystem->HandleWriteCppFile(FilePath, Content, OutJsonString, OutError);
										}
									}, TStatId(), nullptr, ENamedThreads::GameThread);
								Task->Wait();
								if (OutError.IsEmpty()) { ResponseString = OutJsonString; }
								else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"%s\"}"), *OutError); }
							}
							else { ResponseString = FString::Printf(TEXT("{\"success\":false, \"error\":\"Unknown command type: %s\"}"), *CommandType); }
						}
					}
				}
				FTCHARToUTF8 Utf8Converter(*ResponseString, ResponseString.Len());
				const char* Utf8String = Utf8Converter.Get();
				int32 Utf8Size = Utf8Converter.Length();
				int32 Sent = 0;
				ClientSocket->Send((uint8*)Utf8String, Utf8Size, Sent);
				ClientSocket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
			}
		}
		FPlatformProcess::Sleep(0.01f);
	}
	return 0;
}
