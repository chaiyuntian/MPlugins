// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

//#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileIOBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GEOGLUE_API UFileIOBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

		UFUNCTION(BlueprintCallable, Category = "FileIO", meta = (keywords = "LoadTxT"))
		static bool LoadText(FString FileName, TArray<FString>& OutText);
};
