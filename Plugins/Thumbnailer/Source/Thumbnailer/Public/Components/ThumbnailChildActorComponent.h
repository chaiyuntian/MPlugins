/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "ThumbnailChildActorComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChildActorSpawned, TSubclassOf<AActor>);

UCLASS(HideCategories = ("Transform"))
class THUMBNAILER_API UThumbnailChildActorComponent : public UChildActorComponent
{
	GENERATED_BODY()
public:
	FOnChildActorSpawned OnChildActorSpawned;
public:
	virtual void CreateChildActor() override;
};
