/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "ThumbnailerModule.h"
#include "ThumbnailerSettings.h"
#include "ThumbnailerCommands.h"
#include "ThumbnailerMenuExtensions.h"
#include "ThumbnailerHelper.h"
#include "Slate/ThumbnailerViewport.h"
#include "Slate/ThumbnailerStyle.h"
#include "Slate/SThumbnailerSettings.h"
#include "Slate/SThumbnailerWindow.h"
#include "Editor/ThumbnailerViewportClient.h"
#include "Actors/ThumbnailActor.h"

#include "ThumbnailerPluginPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FThumbnailerModule"

IMPLEMENT_MODULE(FThumbnailerModule, Thumbnailer)

static bool bDefaultAllowMessages = false;

void FThumbnailerModule::StartupModule()
{
	InitializeSlateStyle();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName("Thumbnailer"), FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& SpawnTabArgs) { auto Tab = CreatePluginTab(SpawnTabArgs); CurrentThumbnailerTab = Tab; return Tab; }))
		.SetDisplayName(LOCTEXT("ThumbnailerTab", "Thumbnailer"))
		.SetIcon(FSlateIcon(FThumbnailerStyle::Get().GetStyleSetName(), "Thumbnailer.TabIcon"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName("ThumbnailerSettings"), FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs& SpawnTabArgs) { auto Tab = CreatePluginSettingsTab(SpawnTabArgs); CurrentSettingsTab = Tab; return Tab; }))
		.SetDisplayName(LOCTEXT("ThumbnailerSettingsTab", "Thumbnailer Settings"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	ThumbnailerCommands = FThumbnailerHelper::CreateThumbnailerCommands();
	CreateToolbarButton();
	RegisterContentBrowserAssetExtender();

	FCoreDelegates::OnPreExit.AddLambda([this]()
		{
			ThumbnailerCommands.Reset();
			CurrentSettingsTab.Reset();
			CurrentThumbnailerTab.Reset();
			AdvancedPreviewDetailsTab.Reset();

			if (SettingsObject)
			{
				SettingsObject->RemoveFromRoot();
			}

			SettingsObject = nullptr;

			if (ThumbnailViewport.IsValid())
			{
				ThumbnailViewport->PreviewScene.Reset();
				ThumbnailViewport->ThumbnailerViewportClient.Reset();
				ThumbnailViewport.Reset();
			}
		});

	FThumbnailerCore::OnResetMeshSelectionsButtonClicked.AddLambda([]()
	{
		FThumbnailerCore::SetStaticMesh(nullptr);
		FThumbnailerCore::SetSkeletalMesh(nullptr);
		FThumbnailerCore::SetChildActorClass(nullptr);
	});

	FThumbnailerCore::OnCaptureThumbnailButtonClicked.AddRaw(this, &FThumbnailerModule::HandleCaptureThumbnailButtonClicked);
	FThumbnailerCore::OnSettingsUpdated.AddLambda([this](UThumbnailerSettings* settings) { if (GetViewport() && GetViewportClient()) GetViewportClient()->UpdateSettings(settings); });
	FThumbnailerCore::OnResetCameraButtonClicked.AddLambda([this]() { if (GetViewport() && GetViewportClient()) GetViewportClient()->ResetCamera(); });
	FThumbnailerCore::OnFocusCameraButtonClicked.AddLambda([this]() { if (GetViewport() && GetViewportClient()) GetViewportClient()->FocusCamera(); });
}

void FThumbnailerModule::ShutdownModule()
{
	FThumbnailerCore::OnCaptureThumbnailButtonClicked.Clear();
	FThumbnailerCore::OnSettingsUpdated.Clear();
	FThumbnailerCore::OnResetCameraButtonClicked.Clear();
	FThumbnailerCore::OnFocusCameraButtonClicked.Clear();

	FThumbnailerStyle::Shutdown();
	FThumbnailerCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FName("Thumbnailer"));
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FName("ThumbnailerSettings"));

	UnregisterContentBrowserAssetExtender();
}

void FThumbnailerModule::InitializeSlateStyle()
{
	FThumbnailerStyle::Initialize();
	FThumbnailerStyle::ReloadTextures();
}

TSharedRef<SDockTab> FThumbnailerModule::CreatePluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (!SettingsObject)
		SettingsObject = FThumbnailerHelper::CreateSettings();

	ThumbnailViewport = SNew(SThumbnailerViewport);

	AThumbnailActor* m_ThumbnailActor = FThumbnailerHelper::SpawnThumbnailActor(ThumbnailViewport->GetViewportClient()->GetWorld());

	FAdvancedPreviewSceneModule& AdvancedPreviewSceneModule = FModuleManager::LoadModuleChecked<FAdvancedPreviewSceneModule>("AdvancedPreviewScene");
	AdvancedPreviewDetailsTab = AdvancedPreviewSceneModule.CreateAdvancedPreviewSceneSettingsWidget(ThumbnailViewport->PreviewScene.ToSharedRef());

	ThumbnailViewport->SetThumbnailActor(m_ThumbnailActor);

	TSharedRef<SDockTab> m_Window = SNew(SDockTab).TabRole(ETabRole::NomadTab).Icon(FThumbnailerStyle::GetBrush("Thumbnailer.Tab")).OnTabClosed_Lambda([this](TSharedRef<SDockTab> tab)
		{
			if (CurrentSettingsTab)
			{
				CurrentSettingsTab->RequestCloseTab();
				CurrentSettingsTab.Reset();
			}
			if (CurrentThumbnailerTab)
			{
				CurrentThumbnailerTab.Reset();
			}
		})
			[
				SNew(SThumbnailerWindow)
				.moduleptr(this)
				.thumbnailActor(m_ThumbnailActor)
				.thumbnailViewport(ThumbnailViewport)
			];

	return m_Window;
}

TSharedRef<SDockTab> FThumbnailerModule::CreatePluginSettingsTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).OnTabClosed_Lambda([this](TSharedRef<SDockTab> tab){CurrentSettingsTab.Reset();}).Icon(FThumbnailerStyle::GetBrush("Thumbnailer.Tab"))
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SThumbnailerSettings)
			.thumbnailViewport(ThumbnailViewport)
			.thumbnailSettings(SettingsObject)
			.advancedPreviewDetailsTab(AdvancedPreviewDetailsTab)
		];

	return SpawnedTab;
}

void FThumbnailerModule::HandleCaptureThumbnailButtonClicked()
{
	if (FThumbnailerCore::GetCurrentAssetType() == EAssetType::None)
	{
		UE_LOG(LogThumbnailer, Error, TEXT("No asset selected as screenshot source"));

		FNotificationInfo Info(NSLOCTEXT("UnrealEd", "ThumbnailFailedText", "No asset selected as screenshot source"));
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;

		TSharedPtr<SNotificationItem> SaveMessagePtr = FSlateNotificationManager::Get().AddNotification(Info);
		SaveMessagePtr->SetCompletionState(SNotificationItem::CS_Fail);

		return;
	}

	if (TSharedPtr<FThumbnailerViewportClient> m_Client = GetViewportClient())
	{
		ScreenshotRequestProcessed = FScreenshotRequest::OnScreenshotRequestProcessed().AddRaw(this, &FThumbnailerModule::OnScreenshotRequestProcessed);

		FString m_AssetName = GetSettings()->OverrideThumbnailName.Len() == 0 ? GetCurrentAssetName() : GetSettings()->OverrideThumbnailName;
		GetHighResScreenshotConfig().SetFilename(GetSettings()->FilePrefix + m_AssetName);
		GetHighResScreenshotConfig().SetMaskEnabled(GetSettings()->bUseCustomDepthMask);
		GScreenshotResolutionX = GetSettings()->ThumbnailResolution.X;
		GScreenshotResolutionY = GetSettings()->ThumbnailResolution.Y;
		bDefaultAllowMessages = FSlateNotificationManager::Get().AreNotificationsAllowed();
		FSlateNotificationManager::Get().SetAllowNotifications(false);

		m_Client->TakeHighResScreenShot();
	}
}

void FThumbnailerModule::OnScreenshotRequestProcessed()
{
	FHighResScreenshotConfig& m_HighResScreenshotConfig = GetHighResScreenshotConfig();

	FThumbnailerHelper::ProcessThumbnail(m_HighResScreenshotConfig.FilenameOverride);

	FScreenshotRequest::OnScreenshotRequestProcessed().Remove(ScreenshotRequestProcessed);

	FTimerHandle m_TimerHandle;
	FTimerDelegate m_TimerDelegate;
	m_TimerDelegate.BindLambda([](){FSlateNotificationManager::Get().SetAllowNotifications(bDefaultAllowMessages);});

	ThumbnailViewport->ThumbnailerViewportClient->GetWorld()->GetTimerManager().SetTimer(m_TimerHandle, m_TimerDelegate, 1.0f, false);
}

void FThumbnailerModule::RegisterContentBrowserAssetExtender()
{
	if (FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		const auto m_ExtendContentBrowserAssetSelectionMenu = FContentBrowserMenuExtender_SelectedAssets::CreateLambda([&](const TArray<FAssetData>& SelectedAssets)->TSharedRef<FExtender>
			{
				TSharedRef<FExtender> Extender(new FExtender());

				Extender->AddMenuExtension(
					"GetAssetActions",
					EExtensionHook::Before,
					ThumbnailerCommands,
					FMenuExtensionDelegate::CreateStatic(&FThumbnailerMenuExtensions::CreateAssetContextMenu, SelectedAssets)
				);

				return Extender;
			});

		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates[CBAssetMenuExtenderDelegates.Add(m_ExtendContentBrowserAssetSelectionMenu)].GetHandle();
	}
}

void FThumbnailerModule::UnregisterContentBrowserAssetExtender()
{
	if (FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		CBAssetMenuExtenderDelegates.RemoveAll([&](const FContentBrowserMenuExtender_SelectedAssets& Delegate) { return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle; });
	}
}

void FThumbnailerModule::CreateToolbarButton()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, ThumbnailerCommands, FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& builder)
	{
		builder.AddToolBarButton(FThumbnailerCommands::Get().CreateThumbnailerWindow);
	}));
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	{
		auto static m_AddMenuExtension = [](FMenuBuilder& Builder)
		{
			Builder.BeginSection("Help", LOCTEXT("Thumbnailer_Help", "Help"));
			Builder.AddMenuEntry(FThumbnailerCommands::Get().MarketplaceAction);
			Builder.AddMenuEntry(FThumbnailerCommands::Get().HelpAction);
			Builder.AddMenuEntry(FThumbnailerCommands::Get().DiscordAction);
			Builder.EndSection();

			Builder.BeginSection("Support", LOCTEXT("Thumbnailer_Support", "Support"));
			Builder.AddMenuEntry(FThumbnailerCommands::Get().DonateAction);
			Builder.EndSection();
		};

		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());

		MenuExtender->AddMenuBarExtension("Window", EExtensionHook::After, ThumbnailerCommands, FMenuBarExtensionDelegate::CreateStatic([](FMenuBarBuilder& Builder)
		{
			Builder.AddPullDownMenu(
				LOCTEXT("ThumbnailerEditorMenu", "Thumbnailer Plugin"),
				LOCTEXT("ThumbnailerEditorMenu_Tooltip", "Thumbnailer Help and Documentation menu"),
				FNewMenuDelegate::CreateStatic(m_AddMenuExtension), "Thumbnailer Plugin");
		}));
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

TSharedPtr<FThumbnailerViewportClient> FThumbnailerModule::GetViewportClient() const
{
	return ThumbnailViewport->ThumbnailerViewportClient;
}

TSharedPtr<SThumbnailerViewport> FThumbnailerModule::GetViewport() const
{
	return ThumbnailViewport;
}

#undef LOCTEXT_NAMESPACE