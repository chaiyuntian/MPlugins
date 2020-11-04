/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "ThumbnailerTypes.h"

class FThumbnailerModule;
class AThumbnailActor;
class SThumbnailerViewport;

class SThumbnailerWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SThumbnailerWindow) 
		: _moduleptr(nullptr) 
		, _thumbnailActor(nullptr)
		, _thumbnailViewport(nullptr)
	{}
	SLATE_ARGUMENT(FThumbnailerModule*, moduleptr)
	SLATE_ARGUMENT(TWeakObjectPtr<AThumbnailActor>, thumbnailActor)
	SLATE_ARGUMENT(TSharedPtr<SThumbnailerViewport>, thumbnailViewport)
	SLATE_END_ARGS()
	~SThumbnailerWindow();
public:
	void Construct(const FArguments& InArgs);
private:
	void OnStaticMeshPropertyChanged(UStaticMeshComponent* comp);
	void OnSkeletalMeshPropertyChanged(USkeletalMeshComponent* comp);
	void OnChildActorCompPropertyChanged(UThumbnailChildActorComponent* comp);
private:
	FReply HandleCollapseButtonClicked();
protected:
	FThumbnailerModule* ThumbnailerModulePtr;
	TWeakObjectPtr<AThumbnailActor> ThumbnailActor;
	
	TSharedPtr<SThumbnailerViewport> ThumbnailViewport;

	TSharedPtr<SWidgetSwitcher> StaticMeshSwitcher;
	TSharedPtr<SWidgetSwitcher> SkeletalMeshSwitcher;
	TSharedPtr<SWidgetSwitcher> ChildActorSwitcher;

	TSharedPtr<IDetailsView> MeshView;
	TSharedPtr<IDetailsView> SkelMeshView;
	TSharedPtr<IDetailsView> ChildActorComp;
private:
	FDelegateHandle OnStaticMeshPropertyChangedHandle;
	FDelegateHandle OnSkeletalMeshPropertyChangedHandle;
	FDelegateHandle OnChildActorCompPropertyChangedHandle;
	FDelegateHandle OnCollpaseCategoriesHandle;

};