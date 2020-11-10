// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*=================================================
* FileName: Oceanology.cpp
*
* Created by: Galidar
* Project name: Oceanology
* Created on: 2019/10/09
*
* =================================================*/

#include "Oceanology.h"

// Sets default values
AOceanology::AOceanology()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOceanology::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOceanology::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

