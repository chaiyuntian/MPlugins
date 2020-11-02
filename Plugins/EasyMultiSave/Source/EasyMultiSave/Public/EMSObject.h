//Easy Multi Save - Copyright (C) 2020 by Michael Hegemann.  

#pragma once

#include "CoreMinimal.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EMSData.h"
#include "EMSPersistentSaveGame.h"
#include "EMSProfileSaveGame.h"
#include "EMSInfoSaveGame.h"
#include "EMSPluginSettings.h"
#include "EMSAsyncLoadGame.h"
#include "EMSAsyncSaveGame.h"
#include "GameFramework/SaveGame.h"
#include "EMSObject.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogEasyMultiSave, Log, All);

const int PlayerIndex = 0;

UCLASS(config=Game, configdonotcheckdefaults)
class EASYMULTISAVE_API UEMSObject : public UObject
{
	GENERATED_BODY()

/** Variables */

public:

	UPROPERTY(Transient)
	UWorld* World;

	UPROPERTY(config)
	FString CurrentSaveGameName;

	UPROPERTY(config)
	FString CurrentSaveUserName;

	UPROPERTY(Transient)
	UEMSInfoSaveGame* CachedSlotInfoSave;

	UPROPERTY(Transient)
	UEMSPersistentSaveGame* CachedPersistentSave;

	UPROPERTY(Transient)
	UEMSProfileSaveGame* CachedProfileSave;

	UPROPERTY(Transient)
	TArray<AActor*> ActorList;

	TArray<FActorSaveData> SavedActors;
	TArray<FLevelScriptSaveData> SavedScripts;
	FGameObjectSaveData SavedGameMode;
	FGameObjectSaveData SavedGameState;
	FGameObjectSaveData SavedGameInstance;

	FControllerSaveData SavedController;
	FPawnSaveData SavedPawn;
	FGameObjectSaveData SavedPlayerState;

/** Blueprint Library function accessors */
	
public:

	bool SaveLocalProfile();
	UEMSProfileSaveGame* GetLocalProfileSaveGame();

	void SetCurrentSaveGameName(const FString & SaveGameName);
	void SetCurrentSaveUserName(const FString& UserName);

	TArray<FString> GetSortedSaveSlots();

	bool SavePersistentObject();
	UEMSPersistentSaveGame* GetPersistentSave();

	UTexture2D* ImportSaveThumbnail(const FString& SaveGameName);
	void ExportSaveThumbnail(UTextureRenderTarget2D* TextureRenderTarget, const FString& SaveGameName);

	void DeleteAllSaveDataForSlot(const FString& SaveGameName);

/** Other public Functions  */

public:

	virtual class UWorld* GetWorld() const override;

	static UEMSObject* Get(UObject* WorldContextObject);

	FString GetCurrentSaveGameName();

	void SaveSlotInfoObject();
	UEMSInfoSaveGame* GetSlotInfoObject(FString SaveGameName = FString());

	void PrepareLoadAndSaveActors(uint32 Flags, bool bFullReload = false);
	void LogFinishLoadingLevel();

	void SavePlayerActors();
	void LoadPlayerActors(UEMSAsyncLoadGame* LoadTask);

	void SaveLevelActors();
	void LoadLevelActors(UEMSAsyncLoadGame* LoadTask);

	void StartLoadLevelActors(UEMSAsyncLoadGame* LoadTask);
	void LoadAllLevelActors(UEMSAsyncLoadGame* LoadTask);

	bool SpawnOrUpdateLevelActor(const FActorSaveData& ActorArray);
	EUpdateActorResult UpdateLevelActor(const FActorSaveData& ActorArray);
	void SpawnLevelActor(const FActorSaveData& ActorArray);
	void ProcessLevelActor(AActor* Actor, const FActorSaveData& ActorArray);

	bool TryLoadPlayerFile();
	bool TryLoadLevelFile();

	APlayerController* GetPlayerController();
	APawn* GetPlayerPawn();

	FTimerManager& GetTimerManager();

	bool IsAsyncSaveOrLoadTaskActive(const ESaveGameMode& Mode = ESaveGameMode::MODE_All);

/** Internal Functions  */

protected:

	UFUNCTION()
	void OuterActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	bool VerifyOrCreateDirectory(const FString& NewDir);

	bool SaveBinaryArchive(FBufferArchive& BinaryData, const FString& FullSavePath);
	bool LoadBinaryArchive(EDataLoadType LoadType, const FString& FullSavePath, UObject* Object = nullptr);
	bool UnpackBinaryArchive(FMemoryReader FromBinary, EDataLoadType LoadType, UObject* Object = nullptr);

	USaveGame* CreateSaveObject(TSubclassOf<USaveGame> SaveGameClass);
	bool SaveObject(const FString& FullSavePath, USaveGame* SaveGameObject);
	USaveGame* LoadObject(const FString& FullSavePath, TSubclassOf<USaveGame> SaveGameClass);

	void SaveActorToBinary(AActor* Actor, FGameObjectSaveData& OutData);
	void LoadActorFromBinary(AActor* Actor, const FGameObjectSaveData& InData);

	void SerializeToBinary(UObject* Object, TArray<uint8>& OutData);
	void SerializeFromBinary(UObject* Object, const TArray<uint8>& InData);

	void SerializeActorStructProperties(AActor* Actor);
	void SerializeStructProperties(UObject* Object);
	void SerializeScriptStruct(UStruct* ScriptStruct);
	void SerializeArrayStruct(FArrayProperty* ArrayProp);

	void SaveActorComponents(AActor* Actor, TArray<FComponentSaveData>& OutComponents);
	void LoadActorComponents(AActor* Actor, const TArray<FComponentSaveData>& InComponents);

	bool HasSaveInterface(AActor* Actor);
	bool IsValidActor(AActor* Actor);
	bool IsValidForSaving(AActor* Actor);
	bool IsValidForLoading(AActor* Actor);

	bool CheckForExistingActor(const FActorSaveData& ActorArray);

	EActorType GetActorType(AActor* Actor);
    bool IsMovable(USceneComponent* SceneComp);

	TArray<uint8> BytesFromString(const FString& String);
	FString StringFromBytes(const TArray<uint8>& Bytes);

	FName GetLevelName();

	template <class TSaveGame>
	TSaveGame* GetDesiredSaveObject(const FString& FullSavePath, const TSubclassOf<TSaveGame>& Class, TSaveGame*& SaveGameObject);

/** File Access and Path Names  */

public:

	FORCEINLINE FString BaseSaveDir()
	{
		if (!CurrentSaveUserName.IsEmpty())
		{
			return FPaths::ProjectSavedDir() + TEXT("UserSaveGames/") + CurrentSaveUserName + "/";
		}

		return FPaths::ProjectSavedDir() + TEXT("SaveGames/");
	}

	FORCEINLINE FString SaveFolder(const FString& SaveGameName)
	{
		return BaseSaveDir() + SaveGameName + TEXT("/");
	}

	FORCEINLINE FString ProfileSaveFile()
	{
		return  BaseSaveDir() + LocalProfileName + SaveType;
	}

	FORCEINLINE FString FullSaveDir(const FString& DataType, FString SaveGameName = FString())
	{
		if (SaveGameName.IsEmpty())
		{
			SaveGameName = GetCurrentSaveGameName();
		}

		return SaveFolder(SaveGameName) + SaveGameName + DataType;
	}

	FORCEINLINE FString PersistentSaveFile()
	{
		return FullSaveDir(PersistentSuffix + SaveType);
	}

	FORCEINLINE FString CustomPersistentSaveFile(const FString& SaveGameName)
	{
		return SaveFolder(SaveGameName) + SaveGameName + PersistentSuffix + SaveType;
	}

	FORCEINLINE FString SlotInfoSaveFile(FString SaveGameName = FString())
	{
		if (SaveGameName.IsEmpty())
		{
			SaveGameName = GetCurrentSaveGameName();
		}

		return FullSaveDir(SlotSuffix + SaveType, SaveGameName);
	}

	FORCEINLINE FString ActorSaveFile()
	{
		return FullSaveDir(ActorSuffix + SaveType);
	}

	FORCEINLINE FString PlayerSaveFile()
	{
		return FullSaveDir(PlayerSuffix + SaveType);
	}

	FORCEINLINE FString ThumbnailSaveFile(const FString& SaveGameName)
	{
		return FullSaveDir(TEXT(".png"), SaveGameName);
	}

	FORCEINLINE FName LevelScriptSaveName(AActor* Actor)
	{
		return FName(*Actor->GetLevel()->GetOuter()->GetName());
	}
};
