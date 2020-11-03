// Mans Isaksson 2020

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ThumbnailGeneratorScript.generated.h"

UCLASS(abstract, Blueprintable)
class THUMBNAILGENERATOR_API UThumbnailGeneratorScript : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Thumbnail Actor")
	void PreCaptureActorThumbnail(AActor* ThumbnailActor);
};
