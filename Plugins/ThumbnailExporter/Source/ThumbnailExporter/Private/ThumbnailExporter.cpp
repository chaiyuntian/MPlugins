// Copyright 2018 Lian Zhang, All Rights Reserved.

#include "ThumbnailExporter.h"
#include "ThumbnailExporterStyle.h"
#include "ThumbnailExporterCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Runtime/Slate/Public/Widgets/Input/SButton.h"
#include "Input/Reply.h"
#include "Runtime/SlateCore/Public/Widgets/SBoxPanel.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "ContentBrowserModule.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "ObjectTools.h"
#include "Runtime/Engine/Classes/Engine/StreamableManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Runtime/Slate/Public/Widgets/Input/SComboButton.h"
#include "EditorStyleSet.h"
#include "Runtime/Slate/Public/Framework/Commands/UIAction.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Runtime/SlateCore/Public/Types/SlateStructs.h"



static const FName ThumbnailExporterTabName("ThumbnailExporter");

#define LOCTEXT_NAMESPACE "FThumbnailExporterModule"

void FThumbnailExporterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FThumbnailExporterStyle::Initialize();
	FThumbnailExporterStyle::ReloadTextures();

	FThumbnailExporterCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FThumbnailExporterCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FThumbnailExporterModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FThumbnailExporterModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ThumbnailExporterTabName, FOnSpawnTab::CreateRaw(this, &FThumbnailExporterModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FThumbnailExporterTabTitle", "ThumbnailExporter"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	ImageRes = Res_256;
	OutputPath = FText::FromString(FPaths::ProjectDir());
}

void FThumbnailExporterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FThumbnailExporterStyle::Shutdown();

	FThumbnailExporterCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ThumbnailExporterTabName);
}

TSharedRef<SDockTab> FThumbnailExporterModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FThumbnailExporterModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("ThumbnailExporter.cpp"))
		);

	//TSharedRef<SVerticalBox> vertBox = SNew(SVerticalBox);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[			
			SNew(SVerticalBox)

	        //image resolution 
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Res", "Export Images Resolution:  "))
				]

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					 SNew(SComboButton)				 
					 .OnGetMenuContent_Raw(this, &FThumbnailExporterModule::GetResContent)		 
					 .ButtonContent()
					[		
						SNew(STextBlock) .Text_Raw(this, &FThumbnailExporterModule::GetResText)
				    ]		
			    ]			
			]
	        //export method 
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Method", "Export Images Method:      "))
				]

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					 SNew(SComboButton)				 
					 .OnGetMenuContent_Raw(this, &FThumbnailExporterModule::GetMethodContent)		 
					 .ButtonContent()
					[		
						SNew(STextBlock) .Text_Raw(this, &FThumbnailExporterModule::GetMethodText)
				    ]		
			    ]			
			]
		   //path
			+ SVerticalBox::Slot()
				.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
			    .VAlign(VAlign_Center)
			 	.AutoWidth()
			   [
					SNew(SButton)
					.Text(LOCTEXT("Name", "Select Output Folder"))
					.OnClicked_Raw(this, &FThumbnailExporterModule::OpenPathPick)
			   ]

			   + SHorizontalBox::Slot()
			   .Padding(25,0,0,0)
			   .VAlign(VAlign_Center)
			   [
					SNew(STextBlock)
					.Justification(ETextJustify::Right)
					.Text_Raw(this, &FThumbnailExporterModule::GetOutputPath)
				    
			   ]
			]
		  //export button
			+ SVerticalBox::Slot()
				.AutoHeight()

				[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()				
				[
					SNew(SButton)
					.Text(LOCTEXT("tipName", "Export Images"))
				.OnClicked_Raw(this, &FThumbnailExporterModule::ExportImages)
				]	
			]

			//output mesaage
			+ SVerticalBox::Slot()
				.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Raw(this, &FThumbnailExporterModule::GetOutputMessageText)
				]
			]

		];
}

void FThumbnailExporterModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(ThumbnailExporterTabName);
}

void FThumbnailExporterModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FThumbnailExporterCommands::Get().OpenPluginWindow);
}

void FThumbnailExporterModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FThumbnailExporterCommands::Get().OpenPluginWindow);
}

FReply FThumbnailExporterModule::ExportImages()
{
	if (GEditor)
	{
		USelection * _selection = GEditor->GetSelectedActors();
		TArray<FAssetData> _selectedAssets;
		GEditor->GetContentBrowserSelections(_selectedAssets);
		FStreamableManager * _stream = new FStreamableManager;
		if (ExportMethod == ImageExportMethod::ExportBySelected)
		{
			if (_selectedAssets.Num() > 0)
			{
				for (auto _asset : _selectedAssets)
				{
					FString _l, _r;
					_asset.GetFullName().Split("/", &_l, &_r);
					FStringAssetReference ref(_l + "'/" + _r + "'");
					SaveThumbnail(_stream->LoadSynchronous(ref));
				}
				OutputMessageText = FText::FromString(FString::Printf(TEXT("Export %d Images Successfully"), _selectedAssets.Num()));
			}
			else
			{
				OutputMessageText = FText::FromString(TEXT("Export Failed"));
			}
		}
		else
		{
			if (_selectedAssets.Num() > 0)
			{
				FString _assetPath = _selectedAssets[0].PackagePath.ToString();
				TArray<UObject*> _outAssets;
				if (EngineUtils::FindOrLoadAssetsByPath(_assetPath, _outAssets, EngineUtils::ATL_Regular))
				{
					for (auto _asset : _outAssets)
					{
						SaveThumbnail(_stream->LoadSynchronous(_asset));
					}
				}
				OutputMessageText = FText::FromString(FString::Printf(TEXT("Export %d Images Successfully"), _outAssets.Num()));
			}
			else
			{
				OutputMessageText = FText::FromString(TEXT("Export Failed"));
			}
		}
	}
	return FReply::Handled();
}

FText FThumbnailExporterModule::GetOutputMessageText() const
{
	return OutputMessageText;
}

void FThumbnailExporterModule::SaveThumbnail(UObject * obj)
{
	int32 _imageRes = 256;
	switch (ImageRes)
	{
	case Res_64:
		_imageRes = 64;
		break;
	case Res_128:
		_imageRes = 128;
		break;
	case Res_256:
		_imageRes = 256;
		break;
	case Res_512:
		_imageRes = 512;
		break;
	case Res_1024:
		_imageRes = 1024;
		break;
	default:
		break;
	}
	FObjectThumbnail _objectThumnail;
	ThumbnailTools::RenderThumbnail(obj, _imageRes, _imageRes, ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, NULL, &_objectThumnail);
	TArray<uint8> _myData = _objectThumnail.GetUncompressedImageData();

	TArray<FColor> _imageRawColor;
	for (int i = 0; i < _myData.Num(); i += 4)
	{
		_imageRawColor.Add(FColor(_myData[i + 2], _myData[i + 1], _myData[i], _myData[i + 3]));
	}
	FString _fileName = OutputPath.ToString() +"/"+ obj->GetName() + FString(".jpg");
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::JPEG);
	ImageWrapper->SetRaw(_imageRawColor.GetData(), _imageRawColor.GetAllocatedSize(), _imageRes, _imageRes, ERGBFormat::BGRA, 8);
	const TArray64<uint8>& _ImageData = ImageWrapper->GetCompressed(100); 	
 	FFileHelper::SaveArrayToFile(_ImageData, *_fileName);	
}

FText FThumbnailExporterModule::GetResText() const
{
	switch (ImageRes)
	{
	case Res_64:
		return  FText::FromString("64*64");
		break;
	case Res_128:
		return FText::FromString("128*128");
		break;
	case Res_256:
		return FText::FromString("256*256");
		break;
	case Res_512:
		return FText::FromString("512*512");
		break;
	case Res_1024:
		return FText::FromString("1024*1024");
		break;
	default:
		return FText::FromString("NULL");
		break;
	}
}

void FThumbnailExporterModule::SelectRes(ImageExportResolution res)
{
	ImageRes = res;
}

TSharedRef<SWidget> FThumbnailExporterModule::GetResContent()
{
	FMenuBuilder menu(true, NULL);

	menu.AddMenuEntry(
		FText::FromString(TEXT("64*64")),
		FText::FromString(TEXT("64*64")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectRes,ImageExportResolution::Res_64))
	);

	menu.AddMenuEntry(
		FText::FromString(TEXT("128*128")),
		FText::FromString(TEXT("128*128")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectRes, ImageExportResolution::Res_128))
	);

	menu.AddMenuEntry(
		FText::FromString(TEXT("256*256")),
		FText::FromString(TEXT("256*256")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectRes, ImageExportResolution::Res_256))
	);
	menu.AddMenuEntry(
		FText::FromString(TEXT("512*512")),
		FText::FromString(TEXT("512*512")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectRes, ImageExportResolution::Res_512))
	);
	menu.AddMenuEntry(
		FText::FromString(TEXT("1024*1024")),
		FText::FromString(TEXT("1024*1024")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectRes, ImageExportResolution::Res_1024))
	);
	return menu.MakeWidget();
}

FText FThumbnailExporterModule::GetOutputPath() const
{
	return OutputPath;
}

FReply FThumbnailExporterModule::OpenPathPick()
{
	IDesktopPlatform* _desktopPlatform = FDesktopPlatformModule::Get();
	if (_desktopPlatform)
	{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
		FString _outFolderName;
		if (_desktopPlatform->OpenDirectoryDialog(ParentWindowWindowHandle, FString("select path"), FPaths::ProjectDir(), _outFolderName))
		{
			OutputPath = FText::FromString(_outFolderName);
		}
		
	}
	return FReply::Handled();
}

FText FThumbnailExporterModule::GetMethodText() const
{
	switch (ExportMethod)
	{
	case ExportBySelected:
		return FText::FromString("ExportSelected");
		break;
	case ExportByPath:
		return FText::FromString("ExportByPath");
		break;
	default:
		return FText::FromString("NULL");
		break;
	}
}

void FThumbnailExporterModule::SelectMethod(ImageExportMethod method)
{
	ExportMethod = method;
}

TSharedRef<class SWidget> FThumbnailExporterModule::GetMethodContent()
{
	FMenuBuilder menu(true, NULL);

	menu.AddMenuEntry(
		FText::FromString(TEXT("ExportSelected")),
		FText::FromString(TEXT("Export Images By The Selected Assets")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectMethod, ImageExportMethod::ExportBySelected))
	);

	menu.AddMenuEntry(
		FText::FromString(TEXT("ExportByPath")),
		FText::FromString(TEXT("Export Images By The Selected Asset's Path")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FThumbnailExporterModule::SelectMethod, ImageExportMethod::ExportByPath))
	);
	return menu.MakeWidget();
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FThumbnailExporterModule, ThumbnailExporter)