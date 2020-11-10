// Copyright 2020 Phyronnaz

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelExtensionLibrary.generated.h"

class UVoxelHeightmapAssetUINT16;
class UVoxelHeightmapAssetFloat;

VOXELEXTENSION_API DECLARE_LOG_CATEGORY_EXTERN(LogVoxelExtension, Log, All);

UCLASS()
class VOXELEXTENSION_API UVoxelExtensionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Voxel Extension")
	static UVoxelHeightmapAssetUINT16* CreateHeightmapObjectFromPNG(FString HeightmapPath);

	UFUNCTION(BlueprintCallable, Category = "Voxel Extension")
	static UVoxelHeightmapAssetFloat* CreateHeightmapObjectFromXML(FString XmlFilePath, FVector2D LeftTop, FVector2D RightBottom, int32 SamplesPerRow, int32 SamplesPerColumm, float VoxelSize);

	UFUNCTION(BlueprintCallable, Category = "Voxel Extension")
	static UVoxelHeightmapAssetFloat* CreateHeightmapObjectFromGRD(FString GRDFilePath, int32 ResampledWidth, int32 ResampledHeight, float HeightScale = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Voxel Extension")
	static TArray<FTransform> MakeInstanceTransformsWithDensityMask(FBox Bounds, UTexture2D* MaskTexture, int32 Channel);

};
