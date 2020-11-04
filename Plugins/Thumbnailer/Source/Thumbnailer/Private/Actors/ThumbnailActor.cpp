/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Actors/ThumbnailActor.h"
#include "Components/ThumbnailChildActorComponent.h"
#include "ThumbnailerPluginPrivatePCH.h"

AThumbnailActor::AThumbnailActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ChildActorComp = CreateDefaultSubobject<UThumbnailChildActorComponent>(TEXT("ChildActorComp"));
	ChildActorComp->SetupAttachment(RootComponent);
}

AThumbnailActor::~AThumbnailActor()
{
}