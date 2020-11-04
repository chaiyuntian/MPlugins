/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailerTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogThumbnailer, Log, All)

enum EAssetType
{
	None,
	StaticMesh,
	SkeletalMesh,
	Actor
};

USTRUCT()
struct FThumbnailerCameraSettings
{
	GENERATED_BODY()
public:
	UPROPERTY()
		FVector Location;
	UPROPERTY()
		FRotator Rotation;
public:
	FThumbnailerCameraSettings()
		: Location(FVector(-150.0f, 50.0f, 100.f)), Rotation(FRotator(-25.f, -25.f, 0.f))
	{}
	FThumbnailerCameraSettings(FVector location, FRotator rotation)
		: Location(location), Rotation(rotation)
	{}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsUpdated, class UThumbnailerSettings*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnStaticMeshPropertyChanged, class UStaticMeshComponent*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSkeletalMeshPropertyChanged, class USkeletalMeshComponent*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChildActorCompPropertyChanged, class UThumbnailChildActorComponent*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssetTypeChanged, EAssetType);

DECLARE_MULTICAST_DELEGATE_OneParam(FSetStaticMesh, UStaticMesh*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSetSkeletalMesh, USkeletalMesh*);
DECLARE_MULTICAST_DELEGATE_OneParam(FSetChildActorClass, TSubclassOf<AActor>);

DECLARE_MULTICAST_DELEGATE(FOnCollpaseCategories);
DECLARE_MULTICAST_DELEGATE(FOnResetCameraButtonClicked);
DECLARE_MULTICAST_DELEGATE(FOnFocusCameraButtonClicked);
DECLARE_MULTICAST_DELEGATE(FOnResetMeshSelectionsButtonClicked);
DECLARE_MULTICAST_DELEGATE(FOnCaptureThumbnailButtonClicked);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveCameraPositionButtonClicked, const FThumbnailerCameraSettings&);