// Copyright 2026, BlueprintsLab, All rights reserved
#pragma once
#include "CoreMinimal.h"
#include "AssetReferenceTypes.h"
class BPGENERATORULTIMATE_API FApiKeyManager
{
public:
	static FApiKeyManager& Get();
	void SetSlot(int32 SlotIndex, const FApiKeySlot& Slot);
	FApiKeySlot GetSlot(int32 SlotIndex) const;
	void ClearSlot(int32 SlotIndex);
	TArray<FApiKeySlot> GetAllSlots() const;
	int32 GetActiveSlotIndex() const;
	void SetActiveSlot(int32 SlotIndex);
	FApiKeySlot GetActiveSlot() const;
	FString GetActiveApiKey() const;
	FString GetActiveProvider() const;
	FString GetActiveBaseURL() const;
	FString GetActiveModelName() const;
	FString GetActiveGeminiModel() const;
	FString GetActiveOpenAIModel() const;
	FString GetActiveClaudeModel() const;
	FString GetActiveTextureGenEndpoint() const;
	FString GetActiveTextureGenModel() const;
	bool GetActiveTextureGenMode() const;
	FString GetActiveMeshGenEndpoint() const;
	FString GetActiveMeshGenModel() const;
	FString GetActiveMeshGenFormat() const;
	bool GetActiveMeshGenAutoRefine() const;
	E3DModelType GetActiveSelected3DModel() const;
	EMeshQuality GetActiveSelectedQuality() const;
	void SetActiveSelected3DModel(E3DModelType ModelType);
	void SetActiveSelectedQuality(EMeshQuality Quality);
private:
	FApiKeyManager();
	~FApiKeyManager();
	FApiKeySlot Slots[MAX_API_KEY_SLOTS];
	int32 ActiveSlotIndex;
	mutable FCriticalSection SlotsLock;
	void LoadFromConfig();
	void SaveToConfig();
	static FString GetConfigFilePath();
	FString DecryptKey(const FString& EncryptedKey) const;
	FString EncryptKey(const FString& Key) const;
};
