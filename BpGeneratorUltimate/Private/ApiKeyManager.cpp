// Copyright 2026, BlueprintsLab, All rights reserved
#include "ApiKeyManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFilemanager.h"
FApiKeyManager& FApiKeyManager::Get()
{
	static FApiKeyManager Instance;
	return Instance;
}
FApiKeyManager::FApiKeyManager()
{
	for (int32 i = 0; i < MAX_API_KEY_SLOTS; ++i)
	{
		Slots[i].SlotIndex = i;
	}
	ActiveSlotIndex = 0;
	LoadFromConfig();
}
FApiKeyManager::~FApiKeyManager()
{
	SaveToConfig();
}
FString FApiKeyManager::GetConfigFilePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BpGeneratorUltimate"), TEXT("ApiKeySlots.json"));
}
void FApiKeyManager::LoadFromConfig()
{
	FString ConfigPath = GetConfigFilePath();
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ConfigPath))
	{
		return;
	}
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
	{
		return;
	}
	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		return;
	}
	FScopeLock Lock(&SlotsLock);
	ActiveSlotIndex = RootObj->GetIntegerField(TEXT("ActiveSlot"));
	if (ActiveSlotIndex < 0 || ActiveSlotIndex >= MAX_API_KEY_SLOTS)
	{
		ActiveSlotIndex = 0;
	}
	const TArray<TSharedPtr<FJsonValue>>& SlotsArray = RootObj->GetArrayField(TEXT("Slots"));
	for (int32 i = 0; i < FMath::Min(SlotsArray.Num(), MAX_API_KEY_SLOTS); ++i)
	{
		const TSharedPtr<FJsonObject>& SlotObj = SlotsArray[i]->AsObject();
		if (SlotObj.IsValid())
		{
			Slots[i].Name = SlotObj->GetStringField(TEXT("Name"));
			Slots[i].Provider = SlotObj->GetStringField(TEXT("Provider"));
			FString EncryptedKey = SlotObj->GetStringField(TEXT("ApiKey"));
			Slots[i].ApiKey = DecryptKey(EncryptedKey);
			Slots[i].CustomBaseURL = SlotObj->GetStringField(TEXT("CustomBaseURL"));
			Slots[i].CustomModelName = SlotObj->GetStringField(TEXT("CustomModelName"));
			Slots[i].GeminiModel = SlotObj->GetStringField(TEXT("GeminiModel"));
			if (Slots[i].GeminiModel.IsEmpty())
			{
				Slots[i].GeminiModel = TEXT("gemini-2.5-flash");
			}
			Slots[i].OpenAIModel = SlotObj->GetStringField(TEXT("OpenAIModel"));
			if (Slots[i].OpenAIModel.IsEmpty())
			{
				Slots[i].OpenAIModel = TEXT("gpt-5-mini-2025-08-07");
			}
			Slots[i].ClaudeModel = SlotObj->GetStringField(TEXT("ClaudeModel"));
			if (Slots[i].ClaudeModel.IsEmpty())
			{
				Slots[i].ClaudeModel = TEXT("claude-sonnet-4-6");
			}
			Slots[i].TextureGenEndpoint = SlotObj->GetStringField(TEXT("TextureGenEndpoint"));
			Slots[i].TextureGenModel = SlotObj->GetStringField(TEXT("TextureGenModel"));
			bool bTextureModeVal = true;
			SlotObj->TryGetBoolField(TEXT("TextureGenMode"), bTextureModeVal);
			Slots[i].bTextureMode = bTextureModeVal;
			Slots[i].MeshGenEndpoint = SlotObj->GetStringField(TEXT("MeshGenEndpoint"));
			Slots[i].MeshGenModel = SlotObj->GetStringField(TEXT("MeshGenModel"));
			Slots[i].MeshGenFormat = SlotObj->GetStringField(TEXT("MeshGenFormat"));
			bool bMeshGenAutoRefineVal = true;
			SlotObj->TryGetBoolField(TEXT("MeshGenAutoRefine"), bMeshGenAutoRefineVal);
			Slots[i].bMeshGenAutoRefine = bMeshGenAutoRefineVal;
			int32 Selected3DModelVal = 0;
			SlotObj->TryGetNumberField(TEXT("Selected3DModel"), Selected3DModelVal);
			Slots[i].Selected3DModel = static_cast<E3DModelType>(FMath::Clamp(Selected3DModelVal, 0, 4));
			int32 SelectedQualityVal = 0;
			SlotObj->TryGetNumberField(TEXT("SelectedQuality"), SelectedQualityVal);
			Slots[i].SelectedQuality = static_cast<EMeshQuality>(FMath::Clamp(SelectedQualityVal, 0, 1));
			Slots[i].SlotIndex = i;
		}
	}
}
void FApiKeyManager::SaveToConfig()
{
	FString ConfigPath = GetConfigFilePath();
	FString Directory = FPaths::GetPath(ConfigPath);
	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*Directory))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*Directory);
	}
	TSharedPtr<FJsonObject> RootObj = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> SlotsArray;
	FScopeLock Lock(&SlotsLock);
	for (int32 i = 0; i < MAX_API_KEY_SLOTS; ++i)
	{
		TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
		SlotObj->SetStringField(TEXT("Name"), Slots[i].Name);
		SlotObj->SetStringField(TEXT("Provider"), Slots[i].Provider);
		FString EncryptedKey = EncryptKey(Slots[i].ApiKey);
		SlotObj->SetStringField(TEXT("ApiKey"), EncryptedKey);
		SlotObj->SetStringField(TEXT("CustomBaseURL"), Slots[i].CustomBaseURL);
		SlotObj->SetStringField(TEXT("CustomModelName"), Slots[i].CustomModelName);
		SlotObj->SetStringField(TEXT("GeminiModel"), Slots[i].GeminiModel);
		SlotObj->SetStringField(TEXT("OpenAIModel"), Slots[i].OpenAIModel);
		SlotObj->SetStringField(TEXT("ClaudeModel"), Slots[i].ClaudeModel);
		SlotObj->SetStringField(TEXT("TextureGenEndpoint"), Slots[i].TextureGenEndpoint);
		SlotObj->SetStringField(TEXT("TextureGenModel"), Slots[i].TextureGenModel);
		SlotObj->SetBoolField(TEXT("TextureGenMode"), Slots[i].bTextureMode);
		SlotObj->SetStringField(TEXT("MeshGenEndpoint"), Slots[i].MeshGenEndpoint);
		SlotObj->SetStringField(TEXT("MeshGenModel"), Slots[i].MeshGenModel);
		SlotObj->SetStringField(TEXT("MeshGenFormat"), Slots[i].MeshGenFormat);
		SlotObj->SetBoolField(TEXT("MeshGenAutoRefine"), Slots[i].bMeshGenAutoRefine);
		SlotObj->SetNumberField(TEXT("Selected3DModel"), static_cast<int32>(Slots[i].Selected3DModel));
		SlotObj->SetNumberField(TEXT("SelectedQuality"), static_cast<int32>(Slots[i].SelectedQuality));
		SlotsArray.Add(MakeShared<FJsonValueObject>(SlotObj));
	}
	RootObj->SetArrayField(TEXT("Slots"), SlotsArray);
	RootObj->SetNumberField(TEXT("ActiveSlot"), ActiveSlotIndex);
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer);
	FFileHelper::SaveStringToFile(JsonString, *ConfigPath);
}
void FApiKeyManager::SetSlot(int32 SlotIndex, const FApiKeySlot& Slot)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return;
	FScopeLock Lock(&SlotsLock);
	Slots[SlotIndex] = Slot;
	Slots[SlotIndex].SlotIndex = SlotIndex;
	SaveToConfig();
}
FApiKeySlot FApiKeyManager::GetSlot(int32 SlotIndex) const
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return FApiKeySlot();
	FScopeLock Lock(&SlotsLock);
	return Slots[SlotIndex];
}
void FApiKeyManager::ClearSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return;
	FScopeLock Lock(&SlotsLock);
	Slots[SlotIndex].Clear();
	Slots[SlotIndex].SlotIndex = SlotIndex;
	SaveToConfig();
}
TArray<FApiKeySlot> FApiKeyManager::GetAllSlots() const
{
	FScopeLock Lock(&SlotsLock);
	TArray<FApiKeySlot> Result;
	for (int32 i = 0; i < MAX_API_KEY_SLOTS; ++i)
	{
		Result.Add(Slots[i]);
	}
	return Result;
}
int32 FApiKeyManager::GetActiveSlotIndex() const
{
	FScopeLock Lock(&SlotsLock);
	return ActiveSlotIndex;
}
void FApiKeyManager::SetActiveSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= MAX_API_KEY_SLOTS) return;
	FScopeLock Lock(&SlotsLock);
	ActiveSlotIndex = SlotIndex;
	SaveToConfig();
}
FApiKeySlot FApiKeyManager::GetActiveSlot() const
{
	FScopeLock Lock(&SlotsLock);
	return Slots[ActiveSlotIndex];
}
FString FApiKeyManager::GetActiveApiKey() const
{
	return GetActiveSlot().ApiKey;
}
FString FApiKeyManager::GetActiveProvider() const
{
	return GetActiveSlot().Provider;
}
FString FApiKeyManager::GetActiveBaseURL() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (Slot.IsCustomProvider())
	{
		return Slot.CustomBaseURL;
	}
	static const TMap<FString, FString> ProviderURLs = {
		{TEXT("OpenAI"), TEXT("https://api.openai.com/v1/chat/completions")},
		{TEXT("Claude"), TEXT("https://api.anthropic.com/v1/messages")},
		{TEXT("Gemini"), TEXT("https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent")},
		{TEXT("DeepSeek"), TEXT("https://api.deepseek.com/v1/chat/completions")}
	};
	if (const FString* URL = ProviderURLs.Find(Slot.Provider))
	{
		return *URL;
	}
	return TEXT("");
}
FString FApiKeyManager::GetActiveModelName() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (Slot.IsCustomProvider() && !Slot.CustomModelName.IsEmpty())
	{
		return Slot.CustomModelName;
	}
	if (Slot.Provider.Equals(TEXT("OpenAI"), ESearchCase::IgnoreCase) && !Slot.OpenAIModel.IsEmpty())
	{
		return Slot.OpenAIModel;
	}
	if (Slot.Provider.Equals(TEXT("Claude"), ESearchCase::IgnoreCase) && !Slot.ClaudeModel.IsEmpty())
	{
		return Slot.ClaudeModel;
	}
	if (Slot.Provider.Equals(TEXT("Gemini"), ESearchCase::IgnoreCase) && !Slot.GeminiModel.IsEmpty())
	{
		return Slot.GeminiModel;
	}
	static const TMap<FString, FString> ProviderModels = {
		{TEXT("OpenAI"), TEXT("gpt-5-mini-2025-08-07")},
		{TEXT("Claude"), TEXT("claude-sonnet-4-6")},
		{TEXT("Gemini"), TEXT("gemini-2.5-flash")},
		{TEXT("DeepSeek"), TEXT("deepseek-chat")}
	};
	if (const FString* Model = ProviderModels.Find(Slot.Provider))
	{
		return *Model;
	}
	return TEXT("gpt-5-mini-2025-08-07");
}
FString FApiKeyManager::GetActiveGeminiModel() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (!Slot.GeminiModel.IsEmpty())
	{
		return Slot.GeminiModel;
	}
	return TEXT("gemini-2.5-flash");
}
FString FApiKeyManager::GetActiveOpenAIModel() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (!Slot.OpenAIModel.IsEmpty())
	{
		return Slot.OpenAIModel;
	}
	return TEXT("gpt-5-mini-2025-08-07");
}
FString FApiKeyManager::GetActiveClaudeModel() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (!Slot.ClaudeModel.IsEmpty())
	{
		return Slot.ClaudeModel;
	}
	return TEXT("claude-sonnet-4-6");
}
FString FApiKeyManager::GetActiveTextureGenEndpoint() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (!Slot.TextureGenEndpoint.IsEmpty())
	{
		return Slot.TextureGenEndpoint;
	}
	return TEXT("https://generativelanguage.googleapis.com/v1beta/models/imagen-4.0-generate-001:predict");
}
FString FApiKeyManager::GetActiveTextureGenModel() const
{
	FApiKeySlot Slot = GetActiveSlot();
	if (!Slot.TextureGenModel.IsEmpty())
	{
		return Slot.TextureGenModel;
	}
	return TEXT("imagen-4.0-generate-001");
}
bool FApiKeyManager::GetActiveTextureGenMode() const
{
	FApiKeySlot Slot = GetActiveSlot();
	return Slot.bTextureMode;
}
FString FApiKeyManager::GetActiveMeshGenEndpoint() const
{
	FApiKeySlot Slot = GetActiveSlot();
	return Slot.MeshGenEndpoint.IsEmpty() ? TEXT("https://api.meshy.ai/openapi/v2/text-to-3d") : Slot.MeshGenEndpoint;
}
FString FApiKeyManager::GetActiveMeshGenModel() const
{
	FApiKeySlot Slot = GetActiveSlot();
	return Slot.MeshGenModel.IsEmpty() ? TEXT("meshy-v4-0-3d") : Slot.MeshGenModel;
}
FString FApiKeyManager::GetActiveMeshGenFormat() const
{
	FApiKeySlot Slot = GetActiveSlot();
	return Slot.MeshGenFormat.IsEmpty() ? TEXT("glb") : Slot.MeshGenFormat;
}
bool FApiKeyManager::GetActiveMeshGenAutoRefine() const
{
	return GetActiveSlot().bMeshGenAutoRefine;
}
E3DModelType FApiKeyManager::GetActiveSelected3DModel() const
{
	return GetActiveSlot().Selected3DModel;
}
EMeshQuality FApiKeyManager::GetActiveSelectedQuality() const
{
	return GetActiveSlot().SelectedQuality;
}
void FApiKeyManager::SetActiveSelected3DModel(E3DModelType ModelType)
{
	FScopeLock Lock(&SlotsLock);
	Slots[ActiveSlotIndex].Selected3DModel = ModelType;
	SaveToConfig();
}
void FApiKeyManager::SetActiveSelectedQuality(EMeshQuality Quality)
{
	FScopeLock Lock(&SlotsLock);
	Slots[ActiveSlotIndex].SelectedQuality = Quality;
	SaveToConfig();
}
FString FApiKeyManager::DecryptKey(const FString& EncryptedKey) const
{
	if (EncryptedKey.IsEmpty())
	{
		return FString();
	}
	TArray<uint8> DecodedBytes;
	FBase64::Decode(EncryptedKey, DecodedBytes);
	if (DecodedBytes.Num() < 2)
	{
		return FString();
	}
	FString Decrypted;
	for (int32 i = 0; i < DecodedBytes.Num(); i++)
	{
		Decrypted.AppendChar(DecodedBytes[i] ^ 0x55);
	}
	return Decrypted;
}
FString FApiKeyManager::EncryptKey(const FString& Key) const
{
	if (Key.IsEmpty())
	{
		return FString();
	}
	TArray<uint8> EncodedBytes;
	for (int32 i = 0; i < Key.Len(); i++)
	{
		EncodedBytes.Add(Key[i] ^ 0x55);
	}
	return FBase64::Encode(EncodedBytes);
}
