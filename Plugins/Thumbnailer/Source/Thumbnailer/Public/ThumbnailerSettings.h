/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailerTypes.h"
#include "ThumbnailerSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Thumbnailer Plugin"))
class THUMBNAILER_API UThumbnailerSettings : public UObject
{
	GENERATED_BODY()
public:
	UThumbnailerSettings();
public:
	// Thumbnail resolution
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		FVector2D ThumbnailResolution;
	// Directory to save thumbnails to
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		FString ThumbnailSaveDir;
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		FString FilePrefix;
	// Override default thumbnail naming
	UPROPERTY(EditAnywhere, Transient, Category = "Thumbnailer Settings")
		FString OverrideThumbnailName;
	// Transparent background (NOTE: Enable stencil in project settings)
	UPROPERTY(EditAnywhere, Transient, Category = "Thumbnailer Settings")
		uint8 bUseCustomDepthMask : 1;
	// Show thumbnail notifications?
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		uint8 bShowNotifications : 1;
	// Move to created thumbnails in the content browser?
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		uint8 bHighlightThumbnails : 1;
	// Disable overwrite warning when creating thumbnails
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		uint8 bSuppressOverwriteDialog : 1;
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		uint8 bDrawGrid : 1;
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		uint8 bDrawAxes : 1;
	UPROPERTY(config, EditAnywhere, Category = "Thumbnailer Settings")
		float FieldOfView;
	UPROPERTY(config)
		FThumbnailerCameraSettings CameraSettings;

public:
	void SaveCameraSettings(const FThumbnailerCameraSettings& settings);
#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
