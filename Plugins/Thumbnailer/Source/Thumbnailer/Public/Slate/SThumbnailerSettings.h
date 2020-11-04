/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"

class FThumbnailerModule;
class SThumbnailerViewport;
class UThumbnailerSettings;

class SThumbnailerSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SThumbnailerSettings) 
		: _thumbnailSettings(nullptr)
		, _advancedPreviewDetailsTab(nullptr)
	{}
	SLATE_ARGUMENT(TSharedPtr<SThumbnailerViewport>, thumbnailViewport)
	SLATE_ARGUMENT(TWeakObjectPtr<UThumbnailerSettings>, thumbnailSettings)
	SLATE_ARGUMENT(TSharedPtr<SWidget>, advancedPreviewDetailsTab)
	SLATE_END_ARGS()
public:
	void Construct(const FArguments& InArgs);
protected:
	TSharedPtr<SThumbnailerViewport> ThumbnailViewport;
	TWeakObjectPtr<UThumbnailerSettings> SettingsObject;

	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SWidget> AdvancedPreviewDetailsTab;
};