// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
/*=================================================
* FileName: Infinite_Compound.h
*
* Created by: Galidar
* Project name: Oceanology
* Created on: 2019/12/03
*
* =================================================*/
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Components/SceneComponent.h>
#include "Infinite_Compound.generated.h"


UENUM()
enum EInfinityCategory
{
	InfiniteOcean,
};


UCLASS(hidecategories = (Object, Mobility, LOD), ClassGroup = Physics, showcategories = Trigger, meta = (BlueprintSpawnableComponent) )
class OCEANOLOGY_PLUGIN_API UInfinite_Compound : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
		bool RealTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Editor")
		bool EnableScale;

//-----------------------------------------------------------------------------------------------------//



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
		float TimeJump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
		float VisionLimit;

//-----------------------------------------------------------------------------------------------------//

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
		float ScaleFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
		float BeginningScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
		float ScaleMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
		float ScaleMax;



private:
	UPROPERTY()
		TEnumAsByte<enum EInfinityCategory> InfinityCategory;


public:	
	// Sets default values for this component's properties
       UInfinite_Compound();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UWorld* Origin;
};
