/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Slate/ThumbnailerCustomization.h"
#include "ThumbnailerPluginPrivatePCH.h"

#define LOCTEXT_NAMESPACE "FThumbnailerModule"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		FThumbnailerStaticMeshDetails
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
TSharedRef<IDetailCustomization> FThumbnailerStaticMeshDetails::MakeInstance()
{
	return MakeShareable(new FThumbnailerStaticMeshDetails);
}

void FThumbnailerStaticMeshDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory(FName("Physics"));
	DetailBuilder.HideCategory(FName("Collision"));
	DetailBuilder.HideCategory(FName("VirtualTexture"));
	DetailBuilder.HideCategory(FName("Tags"));
	DetailBuilder.HideCategory(FName("Cooking"));
	DetailBuilder.HideCategory(FName("MaterialParameters"));
	DetailBuilder.HideCategory(FName("Mobile"));
	DetailBuilder.HideCategory(FName("HLOD"));
	DetailBuilder.HideCategory(FName("AssetUserData"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		FThumbnailerSkeletalMeshDetails
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
TSharedRef<IDetailCustomization> FThumbnailerSkeletalMeshDetails::MakeInstance()
{
	return MakeShareable(new FThumbnailerSkeletalMeshDetails);
}

void FThumbnailerSkeletalMeshDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory(FName("Physics"));
	DetailBuilder.HideCategory(FName("Collision"));
	DetailBuilder.HideCategory(FName("Clothing"));
	DetailBuilder.HideCategory(FName("ClothingSimulation"));
	DetailBuilder.HideCategory(FName("SkinWeights"));
	DetailBuilder.HideCategory(FName("VirtualTexture"));
	DetailBuilder.HideCategory(FName("MasterPoseComponent"));
	DetailBuilder.HideCategory(FName("Tags"));
	DetailBuilder.HideCategory(FName("Activation"));
	DetailBuilder.HideCategory(FName("Cooking"));
	DetailBuilder.HideCategory(FName("MaterialParameters"));
	DetailBuilder.HideCategory(FName("Mobile"));
	DetailBuilder.HideCategory(FName("HLOD"));
	DetailBuilder.HideCategory(FName("AssetUserData"));
	DetailBuilder.HideCategory(FName("SkeletalMesh"));
	DetailBuilder.HideCategory(FName("Optimization"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		FThumbnailerActorDetails
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
TSharedRef<IDetailCustomization> FThumbnailerActorDetails::MakeInstance()
{
	return MakeShareable(new FThumbnailerActorDetails);
}

void FThumbnailerActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.HideCategory(FName("Rendering"));
	DetailBuilder.HideCategory(FName("Tags"));
	DetailBuilder.HideCategory(FName("Cooking"));
	DetailBuilder.HideCategory(FName("AssetUserData"));
	DetailBuilder.GetProperty(FName("ChildActorClass"))->SetPropertyDisplayName(LOCTEXT("ActorClassText", "Actor Class"));
	DetailBuilder.EditCategory(FName("ChildActorComponent"), LOCTEXT("SelectActorClassText", "Select Actor"), ECategoryPriority::Default);
}

#undef LOCTEXT_NAMESPACE