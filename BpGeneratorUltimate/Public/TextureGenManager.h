// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Dom/JsonObject.h"
#include "Delegates/DelegateCombinations.h"
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTextureGenComplete, const FString&, const FString&);
struct FTextureGenRequest
{
	FString Prompt;
	FString AspectRatio;
	FString NegativePrompt;
	FString CustomAssetName;
	FString SavePath;
	int32 Seed = -1;
};
struct FTextureGenResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	FString ImagePath;
	FString AssetPath;
	FString MaterialPath;
	int32 Width = 0;
	int32 Height = 0;
};
class FTextureGenManager
{
public:
	static FTextureGenManager& Get();
	void GenerateTexture(const FTextureGenRequest& Request, const FString& ApiKey, TFunction<void(const FTextureGenResult&)> OnComplete);
	void CancelGeneration();
private:
	FTextureGenManager() = default;
	~FTextureGenManager() = default;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequest;
	void OnGenerationComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, TFunction<void(const FTextureGenResult&)> Callback, bool bIsImagenFormat, bool bIsDALL_EFormat, bool bIsOpenAIFormat, const FTextureGenRequest& OriginalRequest);
	void SaveTextureToFile(const TArray<uint8>& ImageData, const FString& Filename);
	void ImportTextureAsAsset(const FString& FilePath, const FString& AssetName, const FString& PackagePath, FTextureGenResult& Result);
	void CreateMaterialFromTexture(const FString& TexturePath, const FString& MaterialName, const FString& PackagePath, FTextureGenResult& Result);
};
