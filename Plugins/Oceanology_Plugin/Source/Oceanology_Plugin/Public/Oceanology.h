// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*=================================================
* FileName: Oceanology.h
*
* Created by: Galidar
* Project name: Oceanology
* Created on: 2019/10/09
*
* =================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Oceanology.generated.h"

UCLASS()
class OCEANOLOGY_PLUGIN_API AOceanology : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOceanology();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
