// Copyright 2020 Phyronnaz

#include "VoxelExtensionLibrary.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "VoxelAssets/VoxelHeightmapAsset.h"
#include "VoxelAssets/VoxelHeightmapAssetData.h"
#include "VoxelAssets/VoxelHeightmapAssetData.inl"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Algo/Transform.h"
#include "GRDReader.h"

VOXELEXTENSION_API DEFINE_LOG_CATEGORY(LogVoxelExtension);
 

UVoxelHeightmapAssetUINT16* UVoxelExtensionLibrary::CreateHeightmapObjectFromPNG(FString HeightmapPath)
{
	TArray<uint8> TempData;
	auto* HeightmapAsset = NewObject<UVoxelHeightmapAssetUINT16>();
	auto& HeightmapData = HeightmapAsset->GetData();

	if (!FFileHelper::LoadFileToArray(TempData, *HeightmapPath, FILEREAD_Silent))
	{
		UE_LOG(LogVoxelExtension, Warning, TEXT("Error reading heightmap file"));
		return NULL;
	}
	else
	{
		
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if (!ImageWrapper->SetCompressed(TempData.GetData(), TempData.Num()))
		{
			UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file cannot be read (corrupt png?)"));
			return NULL;
		}
		/*
		else if (ImageWrapper->GetWidth() != ExpectedResolution.Width || ImageWrapper->GetHeight() != ExpectedResolution.Height)
		{
			UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file's resolution does not match the requested resolution"));
		}
		*/
		else
		{
			if (ImageWrapper->GetFormat() != ERGBFormat::Gray)
			{
				UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file appears to be a color png, grayscale is expected. The import *can* continue, but the result may not be what you expect..."));
			}
			else if (ImageWrapper->GetBitDepth() != 16)
			{
				UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file appears to be an 8-bit png, 16-bit is preferred. The import *can* continue, but the result may be lower quality than desired."));
			}

			TArray64<uint8> RawData;
			TArray<uint16> ResultData;
			uint32 NumPixels = ImageWrapper->GetWidth() * ImageWrapper->GetHeight();
			// Set the size of the heightmap
			HeightmapData.SetSize(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), true, EVoxelMaterialConfig::RGB);

			if (ImageWrapper->GetBitDepth() <= 8)
			{
				if (!ImageWrapper->GetRaw(ERGBFormat::Gray, 8, RawData))
				{
					UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file cannot be read (corrupt png?)"));
					return NULL;
				}
				else
				{
					ResultData.Empty(ImageWrapper->GetWidth() * ImageWrapper->GetHeight());
					Algo::Transform(RawData, ResultData, [](uint8 Value) { return Value * 0x101; }); // Expand to 16-bit
				}
			}
			else
			{
				if (!ImageWrapper->GetRaw(ERGBFormat::Gray, 16, RawData))
				{
					UE_LOG(LogVoxelExtension, Warning, TEXT("The heightmap file cannot be read (corrupt png?)"));
				}
				else
				{
					
					ResultData.Empty(NumPixels);
					ResultData.AddUninitialized(NumPixels);
					FMemory::Memcpy(ResultData.GetData(), RawData.GetData(), NumPixels * 2);
				}
			}

			int32 Width = ImageWrapper->GetWidth();
			int32 Height = ImageWrapper->GetHeight();
			for (int32 X = 0; X < Width; X++)
			{
				for (int32 Y = 0; Y < Height; Y++)
				{
					const int32 Index = X + Width * Y;
					HeightmapData.SetHeight(X, Y, ResultData[Index]);
					HeightmapData.SetMaterial_RGB(X, Y, FColor::White);
				}
			}

		}
	}

	return HeightmapAsset;
}

UVoxelHeightmapAssetFloat* UVoxelExtensionLibrary::CreateHeightmapObjectFromGRD(FString GRDFilePath, int32 ResampledWidth, int32 ResampledHeight, float HeightScale)
{
	TArray<float> HeightGrid;
	FGRDHeaderInfo HeaderInfo;

	auto* HeightmapAsset = NewObject<UVoxelHeightmapAssetFloat>();
	auto& HeightmapData = HeightmapAsset->GetData();

	if (!FGRDReader::Load(GRDFilePath, HeightGrid, HeaderInfo))
	{
		UE_LOG(LogVoxelExtension, Warning, TEXT("Error reading heightmap"));
	}

	HeightmapData.SetSize(ResampledWidth, ResampledHeight, true,EVoxelMaterialConfig::RGB);

	TArray<float> ResampledData;
	FGRDReader::ResampleData(HeightGrid, HeaderInfo.GridsX, HeaderInfo.GridsY, ResampledData, ResampledWidth, ResampledHeight);

	for (int32 X = 0; X < ResampledWidth; X++)
	{
		for (int32 Y = 0; Y < ResampledHeight; Y++)
		{
			const int32 Index = X + ResampledHeight * Y;
			HeightmapData.SetHeight(X, Y, ResampledData[Index] * HeightScale);
			HeightmapData.SetMaterial_RGB(X, Y, FColor::White);
		}
	}

	return HeightmapAsset;
}
