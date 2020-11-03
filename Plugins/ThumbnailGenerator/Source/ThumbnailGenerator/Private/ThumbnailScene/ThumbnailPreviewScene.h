// Mans Isaksson 2020

#pragma once
#include "ThumbnailScene/ThumbnailSceneInterface.h"
#include "Engine/Public/PreviewScene.h"

class FThumbnailPreviewScene : public FPreviewScene, public FThumbnailSceneInterface
{
private:
	class AActor*                     SkySphereActor       = nullptr;
	class UDirectionalLightComponent* DirectionalFillLight = nullptr;

	FLinearColor LastEnvironmentColor = FLinearColor::White;

public:

	FThumbnailPreviewScene();

	/* returns true if the sky light has changed */
	static bool UpdateLightSources(const FThumbnailSettings& ThumbnailSettings, class UDirectionalLightComponent* DirectionalLight, 
		class UDirectionalLightComponent* DirectionalFillLight, USkyLightComponent* SkyLight, bool bForceUpdate);

	static bool UpdateSkySphere(const FThumbnailSettings& ThumbnailSettings, UWorld* World, AActor** SkySphereActorPtr, bool bForceUpdate);

	/** Begin FThumbnailSceneInterface */
	virtual void UpdateScene(const FThumbnailSettings& ThumbnailSettings, bool bForceUpdate = false) override;

	virtual class UWorld* GetThumbnailWorld() const override { return GetWorld(); };
	/** End FThumbnailSceneInterface*/

	/** Begin FPreviewScene */
	virtual FLinearColor GetBackgroundColor() const override {return FColor(0, 0, 0, 0); }
	/** End FPreviewScene */

	/** Begin FGCObject */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	/** End FGCObject */
};