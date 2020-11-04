/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Slate/ThumbnailerStyle.h"
#include "ThumbnailerPluginPrivatePCH.h"

TSharedPtr<FSlateStyleSet> FThumbnailerStyle::StyleInstance = NULL;

void FThumbnailerStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FThumbnailerStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FThumbnailerStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ThumbnailerStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FThumbnailerStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("ThumbnailerStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("Thumbnailer")->GetBaseDir() / TEXT("Resources"));

	FButtonStyle CaptureScreenshotButtonStyle = FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton.Success");
	FButtonStyle ResetCameraButtonStyle = FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton.Success");
	FButtonStyle CategoryButtonStyle = FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton.Success");
	FButtonStyle ResetOptionsButtonStyle = FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton.Success");

	ResetCameraButtonStyle.Normal.TintColor = FLinearColor::White;
	CategoryButtonStyle.Normal.TintColor = FLinearColor::White;
	ResetOptionsButtonStyle.Normal.TintColor = FLinearColor::Red;

	Style->Set("Thumbnailer.CaptureScreenshotButtonStyle", CaptureScreenshotButtonStyle);
	Style->Set("Thumbnailer.ResetCameraButtonStyle", ResetCameraButtonStyle);
	Style->Set("Thumbnailer.CategoryButtonStyle", CategoryButtonStyle);
	Style->Set("Thumbnailer.ResetOptionsButtonStyle", ResetOptionsButtonStyle);
	Style->Set("Thumbnailer.CreateThumbnailerWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("Thumbnailer.Tab", new IMAGE_BRUSH(TEXT("ButtonIcon_16x"), Icon16x16));
	Style->Set("Thumbnailer.TabIcon", new IMAGE_BRUSH(TEXT("ButtonIcon_16x"), Icon16x16));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FThumbnailerStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FThumbnailerStyle::Get()
{
	return *StyleInstance;
}
