/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Slate/ThumbnailerViewport.h"
#include "Editor/ThumbnailerViewportClient.h"
#include "Actors/ThumbnailActor.h"
#include "Editor/ThumbnailerPreviewScene.h"
#include "ThumbnailerPluginPrivatePCH.h"
#include "ThumbnailerHelper.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		SThumbnailerViewport
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

SThumbnailerViewport::SThumbnailerViewport()
	: PreviewScene(MakeShareable(new FThumbnailerPreviewScene(FPreviewScene::ConstructionValues())))
{}

SThumbnailerViewport::~SThumbnailerViewport()
{
	if (ThumbnailerViewportClient.IsValid())
	{
		ThumbnailerViewportClient->Viewport = NULL;
	}
	
	if (ThumbnailActor != nullptr)
	{
		if (!ThumbnailActor->IsPendingKill() && !ThumbnailActor->IsPendingKillPending())
			ThumbnailActor->Destroy();
	}
}

void SThumbnailerViewport::SetThumbnailActor(AThumbnailActor* actor)
{
	ThumbnailActor = actor;

	static_cast<FThumbnailerPreviewScene*>(PreviewScene.Get())->SetChildActorComp(ThumbnailActor->GetChildActorComp());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		FGCObject
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

void SThumbnailerViewport::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ThumbnailActor);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		SEditorViewport
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

TSharedRef<FEditorViewportClient> SThumbnailerViewport::MakeEditorViewportClient()
{
	ThumbnailerViewportClient = MakeShareable(new FThumbnailerViewportClient(SharedThis(this), PreviewScene.ToSharedRef()));
	return ThumbnailerViewportClient.ToSharedRef();
}