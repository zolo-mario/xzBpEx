// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Containers/UnrealString.h"
#include "AssetReferenceTypes.h"
class FTimerManager;
struct FMeshCreationRequest
{
	FString Prompt;
	FString NegativePrompt;
	FString CustomAssetName;
	FString SavePath;
	int32 Seed = -1;
	bool bAutoRefine = true;
	FString ImagePath;
	FString ImageBase64Data;
	FString PreviewTaskId;
	FString ModelFormat;
	E3DModelType ModelType = E3DModelType::Meshy_v4_0_3d;
	EMeshQuality Quality = EMeshQuality::Preview;
	FString CustomEndpoint;
	FString CustomModelName;
};
struct FMeshCreationResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	FString DownloadPath;
	FString AssetPath;
	FString TaskId;
	FString Status;
	FString ThumbnailUrl;
	int32 Progress = 0;
	bool bIsPreview = true;
};
class FMeshAssetManager
{
public:
	static FMeshAssetManager& Get();
	void CreateMeshFromText(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete);
	void CreateMeshFromImage(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete);
	void RefineMeshPreview(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete);
	void CancelCreation();
private:
	FMeshAssetManager() = default;
	~FMeshAssetManager() = default;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequest;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActivePollRequest;
	TSharedPtr<FTSTicker::FDelegateHandle> PollTicker;
	int32 PollCount = 0;
	FString LastLoggedStatus;
	void OnCreationComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest, const FString& ApiKey);
	void PollTaskStatus(const FString& TaskId, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest);
	void ScheduleNextPoll(const FString& TaskId, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest);
	void ClearPollTimer();
	void OnPollComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest, const FString& ApiKey);
	void DownloadModel(const FString& ModelUrl, const FString& Filename, TFunction<void(const FString&)> OnDownloadComplete);
	void OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString Filename, TFunction<void(const FString&)> Callback);
	void ImportMeshAsAsset(const FString& FilePath, const FString& AssetName, const FString& PackagePath, FMeshCreationResult& Result);
};
