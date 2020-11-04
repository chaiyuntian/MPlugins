/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "Components/ThumbnailChildActorComponent.h"

void UThumbnailChildActorComponent::CreateChildActor()
{
	Super::CreateChildActor();

	OnChildActorSpawned.Broadcast(GetChildActorClass());
}