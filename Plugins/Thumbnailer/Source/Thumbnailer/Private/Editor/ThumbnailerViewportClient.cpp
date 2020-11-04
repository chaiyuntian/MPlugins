/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Editor/ThumbnailerViewportClient.h"
#include "Editor/ThumbnailerPreviewScene.h"
#include "ThumbnailerModule.h"
#include "ThumbnailerSettings.h"
#include "ThumbnailerHelper.h"
#include "ThumbnailerPluginPrivatePCH.h"

FThumbnailerViewportClient::FThumbnailerViewportClient(const TSharedRef<SThumbnailerViewport>& thumbnailViewport, const TSharedRef<FAdvancedPreviewScene>& previewScene)
	: FEditorViewportClient(nullptr, &previewScene.Get(), StaticCastSharedRef<SEditorViewport>(thumbnailViewport))
	, ThumbnailerViewport(thumbnailViewport)
	, AdvancedPreviewScene(static_cast<FAdvancedPreviewScene*>(PreviewScene))
{
	SetRealtime(true);
	SetViewportType(LVT_Perspective);
	SetViewMode(VMI_Lit);

	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = true;
	DrawHelper.GridColorAxis = FColor(160, 160, 160);
	DrawHelper.GridColorMajor = FColor(144, 144, 144);
	DrawHelper.GridColorMinor = FColor(128, 128, 128);

	bDrawAxes = false;

	EngineShowFlags.SetSeparateTranslucency(true);
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.SetCompositeEditorPrimitives(true);
	EngineShowFlags.SetSelectionOutline(GetDefault<ULevelEditorViewportSettings>()->bUseSelectionOutline);
	OverrideNearClipPlane(1.0f);
	bUsingOrbitCamera = true;

	bDrawVertices = false;

	EngineShowFlags.EnableAdvancedFeatures();
	
	UpdateSettings(FThumbnailerHelper::GetThumbnailer()->GetSettings());
	UpdateCameraSettings(FThumbnailerHelper::GetThumbnailer()->GetSettings()->CameraSettings);
}

FThumbnailerViewportClient::~FThumbnailerViewportClient()
{
}

void FThumbnailerViewportClient::ResetCamera()
{
	SetViewLocation(CachedCameraSettings.Location);
	SetViewRotation(CachedCameraSettings.Rotation);
}

void FThumbnailerViewportClient::FocusCamera()
{
	SetViewLocationForOrbiting(FThumbnailerCore::GetMeshViewPoint());
}

void FThumbnailerViewportClient::ToggleCameraLock()
{
	SetCameraLock();

	if (bCameraLock)
		ResetCamera();
}

void FThumbnailerViewportClient::UpdateSettings(UThumbnailerSettings* settings)
{
	DrawHelper.bDrawGrid = settings->bDrawGrid;
	EngineShowFlags.SetGrid(settings->bDrawGrid);
	bDrawAxes = settings->bDrawAxes; 
	ViewFOV = settings->FieldOfView;

	CachedCameraSettings = settings->CameraSettings;
}

void FThumbnailerViewportClient::UpdateCameraSettings(FThumbnailerCameraSettings settings)
{
	CachedCameraSettings = settings;

	SetViewLocation(settings.Location);
	SetViewRotation(settings.Rotation);
}

void FThumbnailerViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}
