// Mans Isaksson 2020

#pragma once

class FThumbnailSceneInterface
{
public:
	virtual void UpdateScene(const struct FThumbnailSettings& ThumbnailSettings, bool bForceUpdate = false) = 0;

	virtual class UWorld* GetThumbnailWorld() const = 0;
};