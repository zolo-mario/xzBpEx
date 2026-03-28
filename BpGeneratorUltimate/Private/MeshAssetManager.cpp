// Copyright 2026, BlueprintsLab, All rights reserved
#include "MeshAssetManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Containers/Ticker.h"
#include "Misc/FileHelper.h"
#include "EditorAssetLibrary.h"
#include "ApiKeyManager.h"
FMeshAssetManager& FMeshAssetManager::Get()
{
	static FMeshAssetManager Instance;
	return Instance;
}
void FMeshAssetManager::CreateMeshFromText(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete)
{
	FString Endpoint;
	FString Mode;
	if (!Request.CustomEndpoint.IsEmpty())
	{
		Endpoint = Request.CustomEndpoint;
	}
	else
	{
		switch (Request.ModelType)
		{
			case E3DModelType::Meshy_v4_0_3d:
				Endpoint = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");
				break;
			case E3DModelType::meshy_v4_0_3d_tex:
				Endpoint = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");
				break;
			case E3DModelType::meshy_v4_0_3d:
				Endpoint = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");
				break;
			case E3DModelType::others:
				Endpoint = FApiKeyManager::Get().GetActiveMeshGenEndpoint();
				break;
			case E3DModelType::custom:
				Endpoint = Request.CustomEndpoint;
				break;
			default:
				Endpoint = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");
				break;
		}
	}
	TSharedPtr<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	JsonPayload->SetStringField(TEXT("mode"), TEXT("preview"));
	JsonPayload->SetStringField(TEXT("prompt"), Request.Prompt);
	if (!Request.NegativePrompt.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("negative_prompt"), Request.NegativePrompt);
	}
	if (Request.Seed > 0)
	{
		JsonPayload->SetNumberField(TEXT("seed"), Request.Seed);
	}
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Creating mesh: %s (ModelType: %d, Quality: %d)"), *Request.Prompt, (int32)Request.ModelType, (int32)Request.Quality);
	HttpRequest->OnProcessRequestComplete().BindLambda([this, OnComplete, Request, ApiKey](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		OnCreationComplete(HttpRequest, HttpResponse, bSucceeded, OnComplete, Request, ApiKey);
	});
	HttpRequest->ProcessRequest();
	ActiveRequest = HttpRequest;
}
void FMeshAssetManager::CreateMeshFromImage(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete)
{
	FString Endpoint = TEXT("https://api.meshy.ai/v1/image-to-3d");
	TSharedPtr<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	FString ImageData;
	if (!Request.ImageBase64Data.IsEmpty())
	{
		FString MimeType = TEXT("image/png");
		FString Base64Data = Request.ImageBase64Data;
		ImageData = FString::Printf(TEXT("data:%s;base64,%s"), *MimeType, *Base64Data);
		JsonPayload->SetStringField(TEXT("image_url"), ImageData);
	}
	else if (!Request.ImagePath.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("image_url"), Request.ImagePath);
	}
	if (!Request.Prompt.IsEmpty())
	{
		JsonPayload->SetStringField(TEXT("prompt"), Request.Prompt);
	}
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Creating mesh from image (Image-to-3D)"));
	UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Endpoint: %s"), *Endpoint);
	UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Has base64 data: %s, Prompt: %s"), !Request.ImageBase64Data.IsEmpty() ? TEXT("Yes") : TEXT("No"), *Request.Prompt);
	HttpRequest->OnProcessRequestComplete().BindLambda([this, OnComplete, Request, ApiKey](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		OnCreationComplete(HttpRequest, HttpResponse, bSucceeded, OnComplete, Request, ApiKey);
	});
	HttpRequest->ProcessRequest();
	ActiveRequest = HttpRequest;
}
void FMeshAssetManager::RefineMeshPreview(const FMeshCreationRequest& Request, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> OnComplete)
{
	FString Endpoint = TEXT("https://api.meshy.ai/openapi/v2/text-to-3d");
	TSharedPtr<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetURL(Endpoint);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	TSharedPtr<FJsonObject> JsonPayload = MakeShareable(new FJsonObject);
	JsonPayload->SetStringField(TEXT("mode"), TEXT("refine"));
	JsonPayload->SetStringField(TEXT("preview_task_id"), Request.PreviewTaskId);
	JsonPayload->SetBoolField(TEXT("enable_pbr"), true);
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonPayload.ToSharedRef(), Writer);
	HttpRequest->SetContentAsString(RequestBody);
	UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Starting refine with preview_task_id: %s"), *Request.PreviewTaskId);
	HttpRequest->OnProcessRequestComplete().BindLambda([this, OnComplete, Request, ApiKey](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		OnCreationComplete(HttpRequest, HttpResponse, bSucceeded, OnComplete, Request, ApiKey);
	});
	HttpRequest->ProcessRequest();
	ActiveRequest = HttpRequest;
}
void FMeshAssetManager::CancelCreation()
{
	if (ActiveRequest.IsValid())
	{
		ActiveRequest->CancelRequest();
		ActiveRequest.Reset();
	}
	if (ActivePollRequest.IsValid())
	{
		ActivePollRequest->CancelRequest();
		ActivePollRequest.Reset();
	}
	ClearPollTimer();
}
void FMeshAssetManager::OnCreationComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest, const FString& ApiKey)
{
	FMeshCreationResult Result;
	Result.bSuccess = false;
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] Request failed: bWasSuccessful=%d, Response.IsValid=%d"), bWasSuccessful, Response.IsValid() ? 1 : 0);
		Result.ErrorMessage = TEXT("Request failed");
		Callback(Result);
		return;
	}
	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200 && ResponseCode != 202)
	{
		FString ResponseString = Response->GetContentAsString();
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] HTTP Error: %d - %s"), ResponseCode, *ResponseString.Left(200));
		Result.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString.Left(200));
		ClearPollTimer();
		Callback(Result);
		return;
	}
	FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*ResponseString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FString TaskId = JsonObject->GetStringField(TEXT("result"));
		UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Task created: %s"), *TaskId);
		if (!TaskId.IsEmpty())
		{
			Result.TaskId = TaskId;
			PollCount = 0;
			LastLoggedStatus = TEXT("");
			UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Task created: %s. Waiting for completion..."), *TaskId);
			PollTaskStatus(TaskId, ApiKey, Callback, OriginalRequest);
			return;
		}
		else
		{
			Result.ErrorMessage = TEXT("Failed to get task ID from response");
			UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] Error: Failed to get task ID from JSON response"));
		}
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to parse JSON response: %s"), *ResponseString.Left(200));
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] Error parsing JSON: %s"), *Result.ErrorMessage);
	}
	Callback(Result);
}
void FMeshAssetManager::PollTaskStatus(const FString& TaskId, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest)
{
	FString PollEndpoint;
	if (!OriginalRequest.ImageBase64Data.IsEmpty() || !OriginalRequest.ImagePath.IsEmpty())
	{
		PollEndpoint = FString::Printf(TEXT("https://api.meshy.ai/v1/image-to-3d/%s"), *TaskId);
	}
	else
	{
		PollEndpoint = FString::Printf(TEXT("https://api.meshy.ai/openapi/v2/text-to-3d/%s"), *TaskId);
	}
	TSharedPtr<IHttpRequest> PollRequest = FHttpModule::Get().CreateRequest();
	PollRequest->SetVerb(TEXT("GET"));
	PollRequest->SetURL(PollEndpoint);
	PollRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	PollRequest->OnProcessRequestComplete().BindLambda([this, Callback, OriginalRequest, ApiKey](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		OnPollComplete(HttpRequest, HttpResponse, bSucceeded, Callback, OriginalRequest, ApiKey);
	});
	PollRequest->ProcessRequest();
	ActivePollRequest = PollRequest;
}
void FMeshAssetManager::ScheduleNextPoll(const FString& TaskId, const FString& ApiKey, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest)
{
	TSharedPtr<FTSTicker::FDelegateHandle> TickerHandle = MakeShareable(new FTSTicker::FDelegateHandle());
	*TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, TaskId, ApiKey, Callback, OriginalRequest, TickerHandle](float DeltaTime)
		{
			if (TickerHandle.IsValid())
			{
				FTSTicker::GetCoreTicker().RemoveTicker(*TickerHandle);
			}
			PollTaskStatus(TaskId, ApiKey, Callback, OriginalRequest);
			return false;
		}), 5.0f);
	PollTicker = TickerHandle;
}
void FMeshAssetManager::ClearPollTimer()
{
	if (PollTicker.IsValid())
	{
		FTSTicker::FDelegateHandle Handle = *PollTicker;
		FTSTicker::GetCoreTicker().RemoveTicker(Handle);
		PollTicker.Reset();
	}
}
void FMeshAssetManager::OnPollComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FMeshCreationResult&)> Callback, const FMeshCreationRequest& OriginalRequest, const FString& ApiKey)
{
	FMeshCreationResult Result;
	Result.bSuccess = false;
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] Poll request failed: bWasSuccessful=%d, Response.IsValid=%d"), bWasSuccessful, Response.IsValid() ? 1 : 0);
		Result.ErrorMessage = TEXT("Poll request failed");
		ClearPollTimer();
		Callback(Result);
		return;
	}
	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		FString ResponseString = Response->GetContentAsString();
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] HTTP Error: %d - %s"), ResponseCode, *ResponseString.Left(200));
		Result.ErrorMessage = FString::Printf(TEXT("HTTP %d: %s"), ResponseCode, *ResponseString.Left(200));
		ClearPollTimer();
		Callback(Result);
		return;
	}
	FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*ResponseString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FString Status = JsonObject->GetStringField(TEXT("status"));
		Result.Status = Status;
		Result.TaskId = JsonObject->GetStringField(TEXT("id"));
		if (JsonObject->HasField(TEXT("thumbnail_url")))
		{
			Result.ThumbnailUrl = JsonObject->GetStringField(TEXT("thumbnail_url"));
		}
		if (JsonObject->HasField(TEXT("progress")))
		{
			Result.Progress = JsonObject->GetNumberField(TEXT("progress"));
		}
		PollCount++;
		if (Status != LastLoggedStatus || PollCount % 5 == 0 || Status == TEXT("SUCCEEDED"))
		{
			UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Task Status: %s, Progress: %d"), *Status, Result.Progress);
			LastLoggedStatus = Status;
		}
		if (Status == TEXT("SUCCEEDED")){
			Result.bSuccess = true;
			Result.Progress = 100;
			if (JsonObject->HasField(TEXT("model_urls")))
			{
				const TSharedPtr<FJsonObject>* ModelUrlsObj;
				if (JsonObject->TryGetObjectField(TEXT("model_urls"), ModelUrlsObj) && ModelUrlsObj->IsValid())
				{
					FString GlbUrl;
					if (ModelUrlsObj->Get()->TryGetStringField(TEXT("glb"), GlbUrl) && !GlbUrl.IsEmpty())
					{
						FString TaskType = JsonObject->GetStringField(TEXT("type"));
						FString Mode = JsonObject->GetStringField(TEXT("mode"));
						if (TaskType.Contains(TEXT("preview")) || Mode == TEXT("preview"))
						{
							Result.bIsPreview = true;
							if (OriginalRequest.bAutoRefine)
							{
								UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Preview completed, starting auto-refine..."));
								ClearPollTimer();
								FMeshCreationRequest RefineRequest = OriginalRequest;
								RefineRequest.PreviewTaskId = Result.TaskId;
								RefineMeshPreview(RefineRequest, ApiKey, Callback);
								return;
							}
						}
						else
						{
							Result.bIsPreview = false;
						}
						UE_LOG(LogTemp, Log, TEXT("[MeshAssetManager] Model URL obtained: %s"), *GlbUrl.Left(100));
						FString Filename = FString::Printf(TEXT("GeneratedContent/%s_%s.glb"), *OriginalRequest.CustomAssetName, *FGuid::NewGuid().ToString());
						DownloadModel(GlbUrl, Filename, [this, Callback, OriginalRequest, Result](const FString& DownloadPath) mutable
						{
							ImportMeshAsAsset(DownloadPath, OriginalRequest.CustomAssetName, OriginalRequest.SavePath, Result);
							ClearPollTimer();
							Callback(Result);
						});
						return;
					}
				}
				UE_LOG(LogTemp, Warning, TEXT("[MeshAssetManager] Task succeeded but no model URL found"));
			}
		}
		else if (Status == TEXT("FAILED") || Status == TEXT("CANCELED")){
			Result.ErrorMessage = FString::Printf(TEXT("Task %s"), *Status);
			if (JsonObject->HasField(TEXT("task_error")))
			{
				TSharedPtr<FJsonObject> ErrorObj = JsonObject->GetObjectField(TEXT("task_error"));
				Result.ErrorMessage = ErrorObj->GetStringField(TEXT("message"));
			}
			ClearPollTimer();
			Callback(Result);
			return;
		}
		else if (Status == TEXT("IN_PROGRESS") || Status == TEXT("PENDING")){
			ScheduleNextPoll(Result.TaskId, ApiKey, Callback, OriginalRequest);
			return;
			}
	}
	else
	{
		Result.ErrorMessage = FString::Printf(TEXT("Failed to parse JSON response: %s"), *ResponseString.Left(200));
		UE_LOG(LogTemp, Error, TEXT("[MeshAssetManager] Error: %s"), *Result.ErrorMessage);
		ClearPollTimer();
		Callback(Result);
	}
}
void FMeshAssetManager::DownloadModel(const FString& ModelUrl, const FString& Filename, TFunction<void(const FString&)> OnDownloadComplete)
{
	TSharedPtr<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetTimeout(300.0f);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->SetURL(ModelUrl);
	HttpRequest->OnProcessRequestComplete().BindLambda([OnDownloadComplete, Filename](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		FString DownloadPath;
		if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
		{
			TArray<uint8> ImageData = Response->GetContent();
			if (ImageData.Num() > 0)
			{
				FString FullPath = FPaths::ProjectSavedDir() / Filename;
				FFileHelper::SaveArrayToFile(ImageData, *FullPath);
				DownloadPath = FullPath;
			}
			else
			{
				DownloadPath = TEXT("");
			}
		}
		OnDownloadComplete(DownloadPath);
	});
	HttpRequest->ProcessRequest();
}
void FMeshAssetManager::OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString Filename, TFunction<void(const FString&)> Callback)
{
	FString DownloadPath;
	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		TArray<uint8> FileData = Response->GetContent();
		if (FileData.Num() > 0)
		{
			FString FullPath = FPaths::ProjectSavedDir() / Filename;
			if (FFileHelper::SaveArrayToFile(FileData, *FullPath))
			{
				DownloadPath = FullPath;
			}
		}
	}
	Callback(DownloadPath);
}
void FMeshAssetManager::ImportMeshAsAsset(const FString& FilePath, const FString& AssetName, const FString& PackagePath, FMeshCreationResult& Result)
{
	FString FullPackagePath = PackagePath;
	if (FullPackagePath.IsEmpty())
	{
		FullPackagePath = TEXT("/Game/GeneratedModels");
		UE_LOG(LogTemp, Warning, TEXT("[MeshAssetManager] Save path was empty, using default: %s"), *FullPackagePath);
	}
	FString AssetPath = FString::Printf(TEXT("%s/%s"), *FullPackagePath, *AssetName);
	if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		Result.ErrorMessage = FString::Printf(TEXT("Asset already exists: %s"), *AssetPath);
		return;
	}
	FString Directory = FPaths::GetPath(FullPackagePath);
	if (!UEditorAssetLibrary::DoesDirectoryExist(Directory))
	{
		UEditorAssetLibrary::MakeDirectory(Directory);
	}
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<FString> FilesToImport;
	FilesToImport.Add(FilePath);
	TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(FilesToImport, FullPackagePath, nullptr, false);
	if (ImportedAssets.Num() > 0)
	{
		for (UObject* ImportedAsset : ImportedAssets)
		{
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(ImportedAsset))
			{
				StaticMesh->MarkPackageDirty();
				Result.AssetPath = AssetPath;
				Result.bSuccess = true;
				break;
			}
		}
	}
	if (!Result.bSuccess && Result.ErrorMessage.IsEmpty())
	{
		Result.ErrorMessage = TEXT("Failed to import mesh - no StaticMesh found in imported assets");
	}
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().ScanPathsSynchronous({ PackagePath }, true);
}
