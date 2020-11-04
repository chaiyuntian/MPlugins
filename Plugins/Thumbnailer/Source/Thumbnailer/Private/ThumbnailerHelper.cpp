/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "ThumbnailerHelper.h"
#include "ThumbnailerModule.h"
#include "ThumbnailerCommands.h"
#include "ThumbnailerSettings.h"
#include "Actors/ThumbnailActor.h"
#include "Editor/ThumbnailerViewportClient.h"
#include "Editor/ThumbnailerPreviewScene.h"
#include "ThumbnailerPluginPrivatePCH.h"
#include "Components/ThumbnailChildActorComponent.h"
#include "Misc/MessageDialog.h"

FOnStaticMeshPropertyChanged FThumbnailerCore::OnStaticMeshPropertyChanged;
FOnSkeletalMeshPropertyChanged FThumbnailerCore::OnSkeletalMeshPropertyChanged;
FOnChildActorCompPropertyChanged FThumbnailerCore::OnChildActorCompPropertyChanged;

EAssetType FThumbnailerCore::CurrentAssetType = EAssetType::None;
FOnAssetTypeChanged FThumbnailerCore::OnAssetTypeChanged;
FOnSettingsUpdated FThumbnailerCore::OnSettingsUpdated;

FSetStaticMesh FThumbnailerCore::OnSetStaticMesh;
FSetSkeletalMesh FThumbnailerCore::OnSetSkeletalMesh;
FSetChildActorClass FThumbnailerCore::OnSetChildActorClass;

FOnCollpaseCategories FThumbnailerCore::OnCollpaseCategories;
FOnResetCameraButtonClicked FThumbnailerCore::OnResetCameraButtonClicked;
FOnFocusCameraButtonClicked FThumbnailerCore::OnFocusCameraButtonClicked;
FOnResetMeshSelectionsButtonClicked FThumbnailerCore::OnResetMeshSelectionsButtonClicked;
FOnCaptureThumbnailButtonClicked FThumbnailerCore::OnCaptureThumbnailButtonClicked;

FOnSaveCameraPositionButtonClicked FThumbnailerCore::OnSaveCameraPositionButtonClicked;

FThumbnailerModule* FThumbnailerHelper::GetThumbnailer()
{
	FThumbnailerModule* m_ThumbnailerModule = FModuleManager::GetModulePtr<FThumbnailerModule>("Thumbnailer");

	return m_ThumbnailerModule;
}

void FThumbnailerHelper::NotifyThumbnailCreated(FString name, UTexture2D* obj)
{
	FSlateNotificationManager::Get().SetAllowNotifications(true);

	FNotificationInfo Info(NSLOCTEXT("UnrealEd", "ThumbnailCreatedAs", "Thumbnail created as"));
	Info.ExpireDuration = 5.0f;

	TSharedPtr<SNotificationItem> SaveMessagePtr = FSlateNotificationManager::Get().AddNotification(Info);
	SaveMessagePtr->SetHyperlink(FSimpleDelegate::CreateLambda([=]()
		{
			HighlightThumbnail(obj);
		}), FText::FromString(name));

	SaveMessagePtr->SetCompletionState(SNotificationItem::CS_Success);
}

void FThumbnailerHelper::HighlightThumbnail(UObject* obj)
{
	TArray<UObject*> BrowseObjects;
	BrowseObjects.Add(obj);
	GEditor->SyncBrowserToObjects(BrowseObjects);
}

TSharedPtr<FUICommandList> FThumbnailerHelper::CreateThumbnailerCommands()
{
	FThumbnailerModule* m_ThumbnailerModule = GetThumbnailer();

	FThumbnailerCommands::Register();
	TSharedPtr<FUICommandList> m_ThumbnailerCommands = MakeShareable(new FUICommandList);
	m_ThumbnailerCommands->MapAction(FThumbnailerCommands::Get().CreateThumbnailerWindow, FExecuteAction::CreateLambda([m_ThumbnailerModule]()
		{
			FGlobalTabmanager::Get()->InvokeTab(FName("Thumbnailer"));
		}), FCanExecuteAction());

	m_ThumbnailerCommands->MapAction(
		FThumbnailerCommands::Get().DonateAction,
		FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(TEXT("https://paypal.me/eeldev"), NULL, NULL);
	}), FCanExecuteAction());

	m_ThumbnailerCommands->MapAction(
		FThumbnailerCommands::Get().HelpAction,
		FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(TEXT("https://eeldev.com/index.php/thumbnailer-plugin/"), NULL, NULL);
	}), FCanExecuteAction());

	m_ThumbnailerCommands->MapAction(
		FThumbnailerCommands::Get().DiscordAction,
		FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(TEXT("https://discord.gg/7AGWewB"), NULL, NULL);
	}), FCanExecuteAction());

	m_ThumbnailerCommands->MapAction(
		FThumbnailerCommands::Get().MarketplaceAction,
		FExecuteAction::CreateLambda([]()
	{
		FPlatformProcess::LaunchURL(TEXT("https://www.unrealengine.com/marketplace/en-US/product/thumbnailer"), NULL, NULL);
	}), FCanExecuteAction());

	return m_ThumbnailerCommands;
}

AThumbnailActor* FThumbnailerHelper::SpawnThumbnailActor(UWorld* world)
{
	AThumbnailActor* m_Actor = nullptr;

	if (world)
	{
		FActorSpawnParameters m_Params;
		m_Actor = world->SpawnActor<AThumbnailActor>(AThumbnailActor::StaticClass(), FTransform::Identity, m_Params);
	}

	return m_Actor;
}

void FThumbnailerHelper::ProcessThumbnail(FString file)
{
	FThumbnailerModule* m_ThumbnailerModule = GetThumbnailer();
	UTextureFactory* m_TextureFactory = CreateTextureFactory();
	const UThumbnailerSettings* m_Settings = GetDefault<UThumbnailerSettings>();

	if (m_ThumbnailerModule && m_Settings)
	{
		FString m_FullPathName = FString::Printf(TEXT("%s/%s.png"), *FPaths::ScreenShotDir(), *file);

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		TArray<uint8> m_RawImageData;

		if (FFileHelper::LoadFileToArray(m_RawImageData, *m_FullPathName))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(m_RawImageData.GetData(), m_RawImageData.Num()))
			{
				const uint8* m_Memory = m_RawImageData.GetData();

				bool bLoadedOldPackage = true;

				FString PackageName = "/Game/" + m_Settings->ThumbnailSaveDir + "/" + file;

				UPackage* Package = LoadPackage(nullptr, *PackageName, 0);

				EAppReturnType::Type m_Confirmation = EAppReturnType::Yes;

				if (!Package)
				{
					Package = CreatePackage(NULL, *PackageName);

					bLoadedOldPackage = false;
				}
				else
				{
					if (!m_Settings->bSuppressOverwriteDialog)
					{
						m_Confirmation = FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(NSLOCTEXT("Thumbnailer", "OverwriteConfirmation", "Overwrite existing Thumbnail: '{0}' ?"), FText::FromString(file)));
						
						switch (m_Confirmation)
						{
						case EAppReturnType::No:
							GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(m_TextureFactory, nullptr);
							return;
							break;
						case EAppReturnType::Yes:
						break;
						default:
							GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(m_TextureFactory, nullptr);
							return;
							break;
						}
					}
				}

				if (m_TextureFactory && m_Confirmation == EAppReturnType::Yes)
				{
					UTexture2D* Texture = (UTexture2D*)m_TextureFactory->FactoryCreateBinary(UTexture2D::StaticClass(), Package, *file, RF_Standalone | RF_Public, NULL, TEXT("png"), m_Memory, m_Memory + m_RawImageData.Num(), GWarn);

					if (Texture)
					{
						Texture->LODGroup = TextureGroup::TEXTUREGROUP_Pixels2D;
						Texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
						Texture->UpdateResource();
						Texture->PostEditChange();

						Package->SetDirtyFlag(true);

						if (!bLoadedOldPackage)
						{
							FAssetRegistryModule::AssetCreated(Texture);
						}

						if (m_Settings->bShowNotifications)
						{
							FTimerHandle m_TimerHandle;
							FTimerDelegate m_TimerDelegate;

							m_TimerDelegate.BindStatic(&FThumbnailerHelper::NotifyThumbnailCreated, file, Texture);
							m_ThumbnailerModule->GetViewportClient()->GetWorld()->GetTimerManager().SetTimer(m_TimerHandle, m_TimerDelegate, 1.0f, false);
						}

						if (m_Settings->bHighlightThumbnails)
						{
							FThumbnailerHelper::HighlightThumbnail(Texture);
						}
					}
				}

				m_TextureFactory->RemoveFromRoot();
			}
		}
	}
}

UTextureFactory* FThumbnailerHelper::CreateTextureFactory()
{
	const UThumbnailerSettings* m_Settings = GetDefault<UThumbnailerSettings>();

	UTextureFactory* m_TextureFactory = NewObject<UTextureFactory>();
	m_TextureFactory->AddToRoot();
	m_TextureFactory->SuppressImportOverwriteDialog(true);

	return m_TextureFactory;
}

UThumbnailerSettings* FThumbnailerHelper::CreateSettings()
{
	UThumbnailerSettings* m_Obj = NewObject<UThumbnailerSettings>(GetTransientPackage(), UThumbnailerSettings::StaticClass());
	m_Obj->AddToRoot();

	return m_Obj;
}

void FThumbnailerHelper::NotifyUser(FString message, SNotificationItem::ECompletionState type, float duration)
{
	FNotificationInfo Info(FText::FromString(message));
	Info.ExpireDuration = duration;

	TSharedPtr<SNotificationItem> m_UserMessage = FSlateNotificationManager::Get().AddNotification(Info);

	m_UserMessage->SetCompletionState(type);
}

void FThumbnailerCore::SetAssetType(EAssetType type)
{
	if (type == EAssetType::None)
	{
		if (GetStaticMeshComp()->GetStaticMesh())
			CurrentAssetType = EAssetType::StaticMesh;
		else if (GetSkelMeshComp()->SkeletalMesh)
			CurrentAssetType = EAssetType::SkeletalMesh;
		else if (GetChildActorComp()->GetChildActorClass())
			CurrentAssetType = EAssetType::Actor;
	}
	else
	{
		CurrentAssetType = type;
	}

	OnAssetTypeChanged.Broadcast(type);
}

FString FThumbnailerCore::GetCurrentAssetName()
{
	FString m_AssetName = "Invalid";

	switch (CurrentAssetType)
	{
	case EAssetType::StaticMesh:
		if (GetStaticMeshComp()->GetStaticMesh())
			m_AssetName = GetStaticMeshComp()->GetStaticMesh()->GetName();
		break;
	case EAssetType::SkeletalMesh:
		if (GetSkelMeshComp()->SkeletalMesh)
			m_AssetName = GetSkelMeshComp()->SkeletalMesh->GetName();
		break;
	case EAssetType::Actor:
		if (GetChildActorComp()->GetChildActor())
			m_AssetName = GetChildActorComp()->GetChildActorName().ToString();
	break;
	}

	return m_AssetName;
}

FVector FThumbnailerCore::GetMeshViewPoint()
{
	FVector m_OutLocation = FVector::ZeroVector;

	switch (CurrentAssetType)
	{
	case EAssetType::StaticMesh:
		if (GetStaticMeshComp()->GetStaticMesh())
			m_OutLocation = GetStaticMeshComp()->GetStaticMesh()->GetBounds().Origin;
		break;
	case EAssetType::SkeletalMesh:
		if (GetSkelMeshComp()->SkeletalMesh)
			m_OutLocation = GetSkelMeshComp()->SkeletalMesh->GetBounds().Origin;
		break;
	case EAssetType::Actor:
			m_OutLocation = GetChildActorComp()->Bounds.Origin;
		break;
	}

	return m_OutLocation;
}

void FThumbnailerCore::SetStaticMesh(UStaticMesh* mesh)
{
	if (mesh)
		SetAssetType(EAssetType::StaticMesh);
	else
		SetAssetType(EAssetType::None);

	OnSetStaticMesh.Broadcast(mesh);
}

void FThumbnailerCore::SetSkeletalMesh(USkeletalMesh* mesh)
{
	if (mesh)
		SetAssetType(EAssetType::SkeletalMesh);
	else
		SetAssetType(EAssetType::None);

	OnSetSkeletalMesh.Broadcast(mesh);
}

void FThumbnailerCore::SetChildActorClass(TSubclassOf<AActor> actorClass)
{
	if (actorClass)
		SetAssetType(EAssetType::Actor);
	else
		SetAssetType(EAssetType::None);

	OnSetChildActorClass.Broadcast(actorClass);
}
