/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "SEditorViewport.h"
#include "UObject/GCObject.h"
#include "CoreMinimal.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UPostProcessComponent;
class FThumbnailerPreviewScene;
class FThumbnailerViewportClient;
class AThumbnailActor;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		SThumbnailerViewport
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
class SThumbnailerViewport : public SEditorViewport, public FGCObject
{
public:
	SThumbnailerViewport();
	~SThumbnailerViewport();
public:
	void SetThumbnailActor(AThumbnailActor* actor);
public:
	TSharedPtr<FThumbnailerViewportClient> ThumbnailerViewportClient;

	AThumbnailActor* ThumbnailActor;
public:
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
private:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
	//		FGCObject
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
	void AddReferencedObjects(FReferenceCollector& Collector) override;
private:
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
	//		SEditorViewport
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
};