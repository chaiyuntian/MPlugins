/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Slate/SThumbnailerSettings.h"
#include "Slate/ThumbnailerViewport.h"
#include "ThumbnailerSettings.h"
#include "ThumbnailerPluginPrivatePCH.h"

void SThumbnailerSettings::Construct(const FArguments& InArgs)
{
	ThumbnailViewport = InArgs._thumbnailViewport;
	SettingsObject = InArgs._thumbnailSettings;
	AdvancedPreviewDetailsTab = InArgs._advancedPreviewDetailsTab;

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs m_DetailArgs(false, false, false, FDetailsViewArgs::HideNameArea, false);
	m_DetailArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	m_DetailArgs.bShowOptions = false;

	DetailsView = PropertyModule.CreateDetailView(m_DetailArgs);
	DetailsView->SetObject(SettingsObject.Get(), true);
	 
    ChildSlot
    [
       SNew(SBorder)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SScrollBox).ScrollBarAlwaysVisible(true)
					+ SScrollBox::Slot().HAlign(HAlign_Left)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(1.0f)
						[
					SNew(SBorder).BorderImage(FEditorStyle::GetBrush("Scalability.RowBackground")).Padding(5.f).BorderBackgroundColor(FColor::Black)
					[
						SNew(STextBlock).Text(FText::FromString("THUMBNAILER SETTINGS")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
					[
						DetailsView->AsShared()
					]
				+ SVerticalBox::Slot().AutoHeight().Padding(1.0f)
					[
						SNew(SBorder).BorderImage(FEditorStyle::GetBrush("Scalability.RowBackground")).Padding(5.f).BorderBackgroundColor(FColor::Black)
						[
							SNew(STextBlock).Text(FText::FromString("SCENE SETTINGS")).Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
						]
					]
				+ SVerticalBox::Slot().AutoHeight()
					[
						AdvancedPreviewDetailsTab->AsShared()
					]
				]
			]
		]
    ];
}
