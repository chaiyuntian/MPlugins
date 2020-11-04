/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThumbnailActor.generated.h"

class UThumbnailChildActorComponent;

UCLASS()
class THUMBNAILER_API AThumbnailActor : public AActor
{
	GENERATED_BODY()
public:	
	AThumbnailActor();
	~AThumbnailActor();
public:
	UThumbnailChildActorComponent* GetChildActorComp() const { return ChildActorComp; }
private:
	UPROPERTY()
		UThumbnailChildActorComponent* ChildActorComp;
};
