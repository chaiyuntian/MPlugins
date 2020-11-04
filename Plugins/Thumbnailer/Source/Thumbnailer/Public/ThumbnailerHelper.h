/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailerTypes.h"

class FThumbnailerModule;
class AThumbnailActor;
class UTextureFactory;

class THUMBNAILER_API FThumbnailerHelper
{
public:
	static void NotifyThumbnailCreated(FString name, UTexture2D* obj);
	static void HighlightThumbnail(UObject* obj);
	static void ProcessThumbnail(FString file);
	static void NotifyUser(FString message, SNotificationItem::ECompletionState type, float duration = 5.f);
public:
	static FThumbnailerModule* GetThumbnailer();
	static TSharedPtr<FUICommandList> CreateThumbnailerCommands();
	static AThumbnailActor* SpawnThumbnailActor(UWorld* world);
	static UTextureFactory* CreateTextureFactory();
	static UThumbnailerSettings* CreateSettings();
};

class FThumbnailerCore
{
public:
	static void SetAssetType(EAssetType type);
	static EAssetType GetCurrentAssetType() { return CurrentAssetType; }
	static FString GetCurrentAssetName();
	static FVector GetMeshViewPoint();

	static void SetStaticMesh(UStaticMesh* mesh);
	static void SetSkeletalMesh(USkeletalMesh* mesh);
	static void SetChildActorClass(TSubclassOf<AActor> actorClass);
public:
	static FOnStaticMeshPropertyChanged OnStaticMeshPropertyChanged;
	static FOnSkeletalMeshPropertyChanged OnSkeletalMeshPropertyChanged;
	static FOnChildActorCompPropertyChanged OnChildActorCompPropertyChanged;

	static FOnAssetTypeChanged OnAssetTypeChanged;
	static FOnSettingsUpdated OnSettingsUpdated;

	static FSetStaticMesh OnSetStaticMesh;
	static FSetSkeletalMesh OnSetSkeletalMesh;
	static FSetChildActorClass OnSetChildActorClass;

	static FOnCollpaseCategories OnCollpaseCategories;
	static FOnResetCameraButtonClicked OnResetCameraButtonClicked;
	static FOnFocusCameraButtonClicked OnFocusCameraButtonClicked;
	static FOnResetMeshSelectionsButtonClicked OnResetMeshSelectionsButtonClicked;
	static FOnCaptureThumbnailButtonClicked OnCaptureThumbnailButtonClicked;
	static FOnSaveCameraPositionButtonClicked OnSaveCameraPositionButtonClicked;
private:
	static EAssetType CurrentAssetType;
};

FORCEINLINE static FString GetCurrentAssetName()
{
	return FThumbnailerCore::GetCurrentAssetName();
}