// Fill out your copyright notice in the Description page of Project Settings.


#include "FileIOBlueprintLibrary.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"

bool UFileIOBlueprintLibrary::LoadText(FString FileNameA, TArray<FString>& SaveTextA) {
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FileNameA))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("** Could not Find File **"));
		return false;
	}
	else
	{
		FFileHelper::LoadANSITextFileToStrings(*(FileNameA), NULL, SaveTextA);
		return true;
	}

}

