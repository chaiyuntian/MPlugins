/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "ThumbnailerSettings.h"
#include "ThumbnailerPluginPrivatePCH.h"
#include "ThumbnailerHelper.h"

UThumbnailerSettings::UThumbnailerSettings()
	: ThumbnailResolution(512.f, 512.f)
	, ThumbnailSaveDir("Thumbnails")
	, FilePrefix("Thumb_")
	, bUseCustomDepthMask(false)
	, bShowNotifications(true)
	, bHighlightThumbnails(false)
	, bSuppressOverwriteDialog(false)
	, bDrawGrid(false)
	, bDrawAxes(false)
	, FieldOfView(85.0f)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FThumbnailerCore::OnSaveCameraPositionButtonClicked.AddUObject(this, &UThumbnailerSettings::SaveCameraSettings);
		FThumbnailerCore::OnSettingsUpdated.Broadcast(this);
	}
}

void UThumbnailerSettings::SaveCameraSettings(const FThumbnailerCameraSettings& settings)
{
	FThumbnailerHelper::NotifyUser("Camera Position Saved", SNotificationItem::CS_Success);

	CameraSettings = settings;

	SaveConfig();

	FThumbnailerCore::OnSettingsUpdated.Broadcast(this);
}

#if WITH_EDITOR
void UThumbnailerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (ThumbnailSaveDir.Len() == 0)
	{
		ThumbnailSaveDir = "Thumbnails";
	}

	if (FilePrefix.Len() == 0)
	{
		FilePrefix = "Thumb_";
	}

	GetHighResScreenshotConfig().SetMaskEnabled(bUseCustomDepthMask);
	SaveConfig();

	FThumbnailerCore::OnSettingsUpdated.Broadcast(this);
}
#endif