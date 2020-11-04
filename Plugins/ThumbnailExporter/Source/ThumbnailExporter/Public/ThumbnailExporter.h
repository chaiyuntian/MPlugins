// Copyright 2018 Lian Zhang,  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


enum ImageExportResolution
{
		Res_64,
		Res_128,
		Res_256,
		Res_512,
		Res_1024
};
enum ImageExportMethod
{
		ExportBySelected,
		ExportByPath
};


class FToolBarBuilder;
class FMenuBuilder;

class FThumbnailExporterModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:

	TSharedPtr<class FUICommandList> PluginCommands;

	FText OutputMessageText;
	FText GetOutputMessageText() const;

	FReply ExportImages();	//	ExportImages button click
	void SaveThumbnail(UObject * obj);

	FText OutputPath;
	FReply OpenPathPick();
	FText GetOutputPath() const;

	ImageExportResolution ImageRes;
	FText GetResText() const;
	void SelectRes(ImageExportResolution res);
	TSharedRef<class SWidget> GetResContent();

	ImageExportMethod ExportMethod;
	FText GetMethodText() const;
	void SelectMethod(ImageExportMethod method);
	TSharedRef<class SWidget> GetMethodContent();

};