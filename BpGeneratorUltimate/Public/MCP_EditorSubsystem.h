// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "EditorUtilitySubsystem.h"
#include "EditorSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HttpModule.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "MCP_EditorSubsystem.generated.h"
enum class EJobStatus : uint8 { InProgress, Completed, Failed };
struct FExplanationJob
{
    FGuid JobId;
    EJobStatus Status;
    FString ResultText;
    FString ErrorText;
    double StartTime;
    FExplanationJob() : Status(EJobStatus::InProgress), StartTime(FPlatformTime::Seconds()) {}
};
class FSocket;
class FRunnableThread;
class FMCPServerWorker : public FRunnable
{
public:
    FMCPServerWorker(FSocket* InListenSocket, class UMCP_EditorSubsystem* InOwner);
    virtual ~FMCPServerWorker();
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
private:
    FSocket* ListenSocket;
    FThreadSafeBool bStopping;
    UMCP_EditorSubsystem* OwnerSubsystem;
};
struct FSpawnRequest
{
    FString ActorClass;
    FVector Location;
    FRotator Rotation;
    FVector Scale;
    FString ActorLabel;
};
UCLASS()
class BPGENERATORULTIMATE_API UMCP_EditorSubsystem : public UEditorUtilitySubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void HandleAIRequest(const FString& BpPath, const FString& UserPrompt);
    void HandleGenerateBpLogic(const FString& BpPath, const FString& Code, FString& OutError);
    void HandleAddComponent(const FString& BpPath, const FString& ComponentClass, const FString& ComponentName, FString& OutJsonString, FString& OutError);
    void HandleExportToFile(const FString& FileName, const FString& FileContent, const FString& FileFormat, FString& OutError);
    FString HandleEditComponentProperty(const FString& BpPath, const FString& ComponentName, const FString& PropertyName, const FString& PropertyValue);
    void HandleAddVariable(const FString& BpPath, const FString& VarName, const FString& VarType, const FString& DefaultValue, const FString& Category, FString& OutError);
    void HandleCreateBlueprint(const FString& BpName, const FString& ParentClass, const FString& SavePath, FString& OutAssetPath, FString& OutError);
    void HandleGetSelectedNodes(FString& OutNodesText, FString& OutError);
    void HandleEditDataAssetDefaults(const FString& AssetPath, const TArray<TSharedPtr<FJsonValue>>& Edits, FString& OutError);
    void HandleGetAssetSummary(const FString& AssetPath, FString& OutSummary, FString& OutError);
    void HandleGetBehaviorTreeSummary(const FString& BtPath, FString& OutSummary, FString& OutError);
    void HandleAddWidgetsToLayout(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Layout, FString& OutError);
    void HandleEditWidgetProperties(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Edits, FString& OutError);
    void HandleScanAndIndexProject(FString& OutError);
    void HandleSpawnActorInLevel(const FString& ActorClass, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FString& ActorLabel, FString& OutError);
    void HandleSpawnMultipleActorsInLevel(const TArray<FSpawnRequest>& SpawnRequests, FString& OutError);
    void HandleAdvancedSpawn(
        int32 Count, const FString& ActorClass, const FString& BoundingActorLabel,
        const FVector& LocMin, const FVector& LocMax,
        const FVector& ScaleMin, const FVector& ScaleMax,
        const FRotator& RotMin, const FRotator& RotMax,
        FString& OutError
    );
    void HandleGetFocusedContentBrowserPath(FString& OutPath, FString& OutError);
    void HandleCreateProjectFolder(const FString& FolderPath, FString& OutError);
    void HandleGetAllSceneActors(FString& OutJsonString, FString& OutError);
    void HandleSetSelectedActors(const TArray<FString>& ActorLabels, FString& OutError);
    void HandleCreateMaterial(const FString& MaterialPath, const FLinearColor& Color, FString& OutError);
    void HandleSetActorMaterial(const FString& ActorLabel, const FString& MaterialPath, FString& OutError);
    void HandleSetMultipleActorMaterials(const TArray<FString>& ActorLabels, const FString& MaterialPath, FString& OutError);
    void HandleAddWidgetToUserWidget(const FString& WidgetPath, const FString& WidgetType, const FString& WidgetName, const FString& ParentName, FString& OutNewName, FString& OutError);
    void HandleCreateWidgetFromLayout(const FString& WidgetPath, const TArray<TSharedPtr<FJsonValue>>& Layout, FString& OutError);
    void HandleEditWidgetProperty(const FString& WidgetPath, const FString& WidgetName, const FString& PropertyName, const FString& Value, FString& OutError);
    void HandleListAssetsInFolder(const FString& FolderPath, FString& OutJsonString, FString& OutError);
    void HandleGetSelectedContentBrowserAssets(FString& OutJsonString, FString& OutError);
    void HandleGetProjectRootPath(FString& OutPath, FString& OutError);
    void HandleGetDetailedBlueprintSummary(const FString& BpPath, FString& OutSummary, FString& OutError);
    void HandleGetMaterialGraphSummary(const FString& MaterialPath, FString& OutSummary, FString& OutError);
    void HandleAddFoliageInstances(const FString& FoliageTypePath, const TArray<FTransform>& Instances, FString& OutError);
    void HandleCreateStruct(const FString& StructName, const FString& SavePath, const TArray<TSharedPtr<FJsonValue>>& Variables, FString& OutAssetPath, FString& OutError);
    void HandleCreateEnum(const FString& EnumName, const FString& SavePath, const TArray<FString>& Enumerators, FString& OutAssetPath, FString& OutError);
    void HandleInsertCode(const FString& BpPath, const FString& AiGeneratedCode, FString& OutError);
    void HandleGetDataAssetDetails(const FString& AssetPath, FString& OutDetailsJson, FString& OutError);
    void HandleGetBatchWidgetProperties(const TArray<FString>& WidgetClasses, FString& OutJsonString, FString& OutError);
    void HandleDeleteVariable(const FString& BpPath, const FString& VarName, FString& OutError);
    void HandleCategorizeVariables(const FString& BpPath, const TSharedPtr<FJsonObject>* Categories, FString& OutJsonString, FString& OutError);
    void HandleDeleteComponent(const FString& BpPath, const FString& ComponentName, FString& OutError);
    void HandleDeleteNodes(FString& OutJsonString, FString& OutError);
    void HandleDeleteWidget(const FString& WidgetPath, const FString& WidgetName, FString& OutError);
    void HandleDeleteUnusedVariables(const FString& BpPath, FString& OutJsonString, FString& OutError);
    void HandleDeleteFunction(const FString& BpPath, const FString& FunctionName, FString& OutError);
    void HandleUndoLastGenerated(FString& OutJsonString, FString& OutError);
    void HandleFindAssetByName(const FString& NamePattern, const FString& AssetType, FString& OutJsonString, FString& OutError);
    void HandleMoveAsset(const FString& AssetPath, const FString& DestinationFolder, FString& OutJsonString, FString& OutError);
    void HandleMoveAssets(const TArray<FString>& AssetPaths, const FString& DestinationFolder, FString& OutJsonString, FString& OutError);
    void HandleGenerateTexture(const FString& Prompt, const FString& SavePath, const FString& AspectRatio, const FString& AssetName, FString& OutJsonString, FString& OutError);
    void HandleGeneratePBRMaterial(const FString& Prompt, const FString& SavePath, const FString& MaterialName, FString& OutJsonString, FString& OutError);
    void HandleCreateMaterialFromTextures(const FString& SavePath, const FString& MaterialName, const TArray<TSharedPtr<FJsonValue>>& TexturePaths, FString& OutJsonString, FString& OutError);
    void HandleCreateModel3D(const FString& Prompt, const FString& ModelName, const FString& SavePath, bool bAutoRefine, int32 Seed, FString& OutJsonString, FString& OutError);
    void HandleRefineModel3D(const FString& TaskId, const FString& ModelName, const FString& SavePath, FString& OutJsonString, FString& OutError);
    void HandleImageToModel3D(const FString& ImagePath, const FString& Prompt, const FString& ModelName, const FString& SavePath, FString& OutJsonString, FString& OutError);
    void HandleCreateInputAction(const FString& ActionName, const FString& SavePath, const FString& ValueType, FString& OutJsonString, FString& OutError);
    void HandleCreateInputMappingContext(const FString& ContextName, const FString& SavePath, FString& OutJsonString, FString& OutError);
    void HandleAddInputMapping(const FString& ContextPath, const FString& ActionPath, const FString& Key, const TArray<TSharedPtr<FJsonValue>>& Modifiers, const TArray<TSharedPtr<FJsonValue>>& Triggers, FString& OutJsonString, FString& OutError);
    void HandleCreateDataTable(const FString& TableName, const FString& SavePath, const FString& RowStructPath, const TArray<TSharedPtr<FJsonValue>>& Rows, FString& OutJsonString, FString& OutError);
    void HandleAddDataTableRows(const FString& TablePath, const TArray<TSharedPtr<FJsonValue>>& Rows, FString& OutJsonString, FString& OutError);
    void HandleListPlugins(FString& OutJsonString, FString& OutError);
    void HandleFindPlugin(const FString& SearchPattern, FString& OutJsonString, FString& OutError);
    void HandleListPluginFiles(const FString& PluginName, const FString& RelativePath, FString& OutJsonString, FString& OutError);
    void HandleReadCppFile(const FString& FilePath, FString& OutJsonString, FString& OutError);
    void HandleWriteCppFile(const FString& FilePath, const FString& Content, FString& OutJsonString, FString& OutError);
    void HandleScanDirectory(const FString& DirectoryPath, const TArray<FString>& Extensions, FString& OutJsonString, FString& OutError);
    void HandleSelectFolder(const FString& FolderPath, FString& OutJsonString, FString& OutError);
    void HandleCheckProjectSource(FString& OutJsonString, FString& OutError);
    void HandleClassifyIntent(const FString& UserMessage, FString& OutJsonString, FString& OutError);
private:
    void StartServer();
    void StopServer();
    UEdGraphNode* GetActiveSelectedGraphNode(UBlueprint* TargetBlueprint);
    FString ChopStringByTripleQuotes(const FString& InputString);
    TMap<FHttpRequestPtr, UBlueprint*> ActiveAIRequests;
    FSocket* ListenerSocket = nullptr;
    FRunnableThread* ServerThread = nullptr;
    FMCPServerWorker* ServerWorker = nullptr;
    int32 ActualServerPort = 0;
    TMap<FGuid, FExplanationJob> ActiveExplanationJobs;
};
