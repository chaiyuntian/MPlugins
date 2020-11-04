/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Slate/ThumbnailerStyle.h"
#include "IDetailCustomization.h"
#include "ThumbnailerTypes.h"

class UThumbnailerSettings;
class SThumbnailerViewport;
class FUICommandList;
class SDockTab;
class FSpawnTabArgs;
class FThumbnailerViewportClient;

class FThumbnailerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
public:
	TSharedPtr<FThumbnailerViewportClient> GetViewportClient() const;
	TSharedPtr<SThumbnailerViewport> GetViewport() const;
	UThumbnailerSettings* GetSettings() const { return SettingsObject; }
public:
	void OnScreenshotRequestProcessed();
private:
	void InitializeSlateStyle();
	void CreateToolbarButton();
	void RegisterContentBrowserAssetExtender();
	void UnregisterContentBrowserAssetExtender();
private:
	TSharedRef<SDockTab> CreatePluginTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> CreatePluginSettingsTab(const FSpawnTabArgs& SpawnTabArgs);
private:
	UPROPERTY()
		UThumbnailerSettings* SettingsObject;
private:
	TSharedPtr<FUICommandList> ThumbnailerCommands;
	TSharedPtr<SThumbnailerViewport> ThumbnailViewport;
	TSharedPtr<SDockTab> CurrentSettingsTab;
	TSharedPtr<SDockTab> CurrentThumbnailerTab;
	TSharedPtr<SWidget> AdvancedPreviewDetailsTab;
public:
	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle ScreenshotRequestProcessed;
private:
	void HandleCaptureThumbnailButtonClicked();
};