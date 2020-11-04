/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "ThumbnailerMenuExtensions.h"
#include "ThumbnailerModule.h"
#include "ThumbnailerHelper.h"
#include "ThumbnailerPluginPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FThumbnailerModule"

void FThumbnailerMenuExtensions::CreateAssetContextMenu(FMenuBuilder& menuBuilder, TArray<FAssetData> objects)
{
	auto m_ThumbnailerModule = FThumbnailerHelper::GetThumbnailer();

	if (objects.Num() == 1)
	{
		UObject* m_Obj = objects[0].GetAsset();

		if (m_Obj)
		{
			UStaticMesh* m_StaticMesh = Cast<UStaticMesh>(m_Obj);
			USkeletalMesh* m_SkeletalMesh = Cast<USkeletalMesh>(m_Obj);
			TSubclassOf<AActor> m_ChildActorClass = nullptr;

			if (UBlueprint* m_BlueprintAsset = Cast<UBlueprint>(m_Obj))
			{
				if (m_BlueprintAsset->GeneratedClass)
					m_ChildActorClass = static_cast<TSubclassOf<AActor>>(m_BlueprintAsset->GeneratedClass);
			}

			if (m_StaticMesh || m_SkeletalMesh || m_ChildActorClass)
			{
				FUIAction m_GenerateThumbnailAction;
				m_GenerateThumbnailAction.ExecuteAction.BindLambda([=]()
					{
						FGlobalTabmanager::Get()->InvokeTab(FName("Thumbnailer"));

						FThumbnailerCore::OnResetMeshSelectionsButtonClicked.Broadcast();
						FThumbnailerCore::OnCollpaseCategories.Broadcast();

						if (m_StaticMesh)
						{
							FThumbnailerCore::SetStaticMesh(m_StaticMesh);
						}
						else if (m_SkeletalMesh)
						{
							FThumbnailerCore::SetSkeletalMesh(m_SkeletalMesh);
						}
						else if (m_ChildActorClass)
						{
							FThumbnailerCore::SetChildActorClass(m_ChildActorClass);
						}
					});

				menuBuilder.AddMenuEntry(
					LOCTEXT("ThumbnailerLabelText", "Create Thumbnail"),
					LOCTEXT("ThumbnailerTooltipText", "Open the Thumbnailer window"),
					FSlateIcon(FThumbnailerStyle::Get().GetStyleSetName(), "Thumbnailer.TabIcon"),
					m_GenerateThumbnailAction
				);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE