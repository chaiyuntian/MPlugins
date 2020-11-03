// Mans Isaksson 2020

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"

DECLARE_LOG_CATEGORY_EXTERN(LogThumbnailGeneratorEd, Log, All);

class FThumbnailGeneratorEditorCommands : public TCommands<FThumbnailGeneratorEditorCommands>
{
public:
	FThumbnailGeneratorEditorCommands();

	virtual void RegisterCommands() override;

public:

	TSharedPtr<FUICommandInfo> GenerateThumbnail;

	TSharedPtr<FUICommandInfo> SaveThumbnail;

	TSharedPtr<FUICommandInfo> ExportThumbnail;
};

class FThumbnailGeneratorEditorModule : public IModuleInterface
{
private:
	FDelegateHandle SaveDelegateHandle;
	FDelegateHandle LevelEditorTabManagerChangedHandle;
	FDelegateHandle ContentBrowserCommandExtenderDelegateHandle;
	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle AssetEditorExtenderDelegateHandle;
	FDelegateHandle LevelEditorExtenderDelegateHandle;
	FDelegateHandle PreSavePackageDelegateHandle;

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterSettings();
	void UnregisterSettings();
	bool HandleSettingsSaved();

	void RegisterDetailsCustomization();
	void UnregisterDetailsCustomization();

	void RegisterThumbnailGeneratorEvents();
	void UnregisterThumbnailGeneratorEvents();

	void RegisterTabSpawner();
	void UnregisterTabSpawner();

	void RegisterLevelEditorCommandExtensions();
	void UnregisterLevelEditorCommandExtensions();

	void RegisterContentBrowserCommandExtensions();
	void UnregisterContentBrowserCommandExtensions();

	void OpenThumbnailGenerator(const TSubclassOf<AActor> ActorClass);

	void CreateAssetContextMenu(FMenuBuilder& MenuBuilder);

	static void PreSavePackage(UPackage* Package);

public:
	static UTexture2D* SaveThumbnail(class UTexture2D* Thumbnail, const FString &OutputDirectory, const FString &OutputName, bool bWithOverwriteUI);

};

