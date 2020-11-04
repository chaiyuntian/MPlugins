/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "ThumbnailerTypes.h"
#include "Slate/ThumbnailerViewport.h"

class FAdvancedPreviewScene;
class UThumbnailChildActorComponent;
class UThumbnailerSettings;

class FThumbnailerViewportClient : public FEditorViewportClient
{
public:
	FThumbnailerViewportClient(const TSharedRef<SThumbnailerViewport>& thumbnailViewport, const TSharedRef<FAdvancedPreviewScene>& previewScene);
	virtual ~FThumbnailerViewportClient();
public:
	void ResetCamera();
	void FocusCamera();
	void ToggleCameraLock();
	void UpdateSettings(UThumbnailerSettings* settings);
	void UpdateCameraSettings(FThumbnailerCameraSettings settings);
	virtual void Tick(float DeltaSeconds) override;
public:
	TWeakPtr<SThumbnailerViewport> ThumbnailerViewport;
	FAdvancedPreviewScene* AdvancedPreviewScene;
private:
	FThumbnailerCameraSettings CachedCameraSettings;
};