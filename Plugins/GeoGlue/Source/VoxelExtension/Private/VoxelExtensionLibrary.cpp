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
// XML header 
#include "Engine.h"
#include "Runtime/XmlParser/Public/XmlParser.h"
#include "Runtime/XmlParser/Public/FastXml.h" 

VOXELEXTENSION_API DEFINE_LOG_CATEGORY(LogVoxelExtension);

struct FVector2D_Double
{
	double X;
	double Y;

	FVector2D_Double operator-(const FVector2D_Double& Other) const
	{
		return FVector2D_Double(X - Other.X, Y - Other.Y);
	}

	FVector2D_Double operator+(const FVector2D_Double& Other) const
	{
		return FVector2D_Double(X + Other.X, Y + Other.Y);
	}

	FVector2D_Double operator/(const FVector2D_Double& Other) const
	{
		return FVector2D_Double(X * Other.X, Y * Other.Y);
	}

	FVector2D_Double operator*(const float& Other) const
	{
		return FVector2D_Double(X * Other, Y * Other);
	}

	FVector2D_Double operator/(const float& Other) const
	{
		return FVector2D_Double(X / Other, Y / Other);
	}

	FVector2D_Double operator/(const FVector2D& Other) const
	{
		return FVector2D_Double(X / Other.X, Y / Other.Y);
	}

	FVector2D_Double operator*(const FVector2D& Other) const
	{
		return FVector2D_Double(X * Other.X, Y * Other.Y);
	}

	FVector2D_Double()
		:X(0.f), Y(0.f)
	{}

	FVector2D_Double(double X, double Y)
		:X(X), Y(Y)
	{}

	FVector2D_Double(const FVector2D& Vector2D)
		:X(Vector2D.X), Y(Vector2D.Y)
	{}

	double Size() const
	{
		return sqrt(X * X + Y * Y);
	}
};

// reference:https://www.cnblogs.com/monocular/p/11573697.html
// TODO:
// R&D Ref: https://github.com/LarsFlaeten/Proland_dev/blob/aea5557efa556bf3efc9d2185fa85b3508e20b1d/terrain/sources/proland/preprocess/terrain/Preprocess.h
FVector2D_Double LongitudeLatitudeToXY(FVector2D_Double LonLat)
{

	double L = 637800000.0 * PI * 2;
	double W = L;
	double H = L / 2;  
	double Mill = 2.3; 
	double x = LonLat.X * PI / 180;// longitude -> rad
	double y = LonLat.Y * PI / 180;// latitude -> rad

	y = 1.25 * log(tan(0.25 * PI + 0.4 * y));// Miller projection

	// Turn circle to real distance
	x = (W / 2) + (W / (2 * PI)) * x;
	y = (H / 2) - (H / (2 * Mill)) * y;

	return FVector2D_Double(x, y);
}

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

// TODO: Resample Height Data using the most closest 4 points.(not necessarily a quad, because data samples are unevenly distributed, this is the tricky part.)
UVoxelHeightmapAssetFloat* UVoxelExtensionLibrary::CreateHeightmapObjectFromXML(FString XmlFilePath, FVector2D LeftTop, FVector2D RightBottom, int32 SamplesPerRow, int32 SamplesPerColumm,float VoxelSize)
{
	// Create Heighmap Asset Data
	auto* HeightmapAsset = NewObject<UVoxelHeightmapAssetFloat>();
	auto& HeightmapData = HeightmapAsset->GetData();

	bool bSuccess = false;

	FVector2D_Double LeftTopWS_Double = LongitudeLatitudeToXY(FVector2D_Double(LeftTop));
	FVector2D_Double RightBottomWS_Double = LongitudeLatitudeToXY(FVector2D_Double(RightBottom));
	FVector2D_Double Size2DWS_Double = RightBottomWS_Double - LeftTopWS_Double;

	// Calculate Resampled Width and Height
	int32 ResampledWidth = FMath::Abs(Size2DWS_Double.X) / VoxelSize;
	int32 ResampledHeight = FMath::Abs(Size2DWS_Double.Y) / VoxelSize;

	FXmlFile* XmlFile = new FXmlFile(XmlFilePath, EConstructMethod::ConstructFromFile);

	if (XmlFile->IsValid())
	{
		FXmlNode* XmlRootNode = XmlFile->GetRootNode();
		if (XmlRootNode == nullptr)
		{
			bSuccess = false;
		}
		else
		{
			// Init the Height Grid
			TArray<float> HeightGrid;
			HeightGrid.Init(SamplesPerColumm * SamplesPerRow, 0.f);

			const TArray<FXmlNode*>& Nodes_LV1 = XmlFile->GetRootNode()->GetChildrenNodes();
			const TArray<FXmlNode*>& Nodes_LV2 = Nodes_LV1[0]->GetChildrenNodes();
			const TArray<FXmlNode*>& Nodes = Nodes_LV2[0]->GetChildrenNodes();

			if (Nodes.Num() != SamplesPerColumm * SamplesPerRow)
			{
				UE_LOG(LogTemp, Warning, TEXT("Data corrupted! Nodes.Num() != SamplesPerColumm * SamplesPerRow"));
			}
			
			int32 Length = FMath::Min(Nodes.Num(), SamplesPerColumm * SamplesPerRow);

			for (int i = 0; i < Length; i++)
			{
				double HeightValue = 0;
				FVector2D_Double LonLat = FVector2D_Double();

				const TArray<FXmlNode*>& NodeLine = Nodes[i]->GetChildrenNodes();

				if (NodeLine[0]->GetTag().Compare("altitude", ESearchCase::IgnoreCase) == 0)
				{
					HeightValue = FCString::Atod(*NodeLine[0]->GetContent());
				}
				if (Nodes[1]->GetTag().Compare("latitude", ESearchCase::IgnoreCase) == 0)
				{
					LonLat.X = FCString::Atod(*NodeLine[0]->GetContent());
				}
				if (Nodes[2]->GetTag().Compare("longitude", ESearchCase::IgnoreCase) == 0)
				{
					LonLat.Y = FCString::Atod(*NodeLine[0]->GetContent());
				}

				HeightGrid.Add(HeightValue);
			}

			HeightmapData.SetSize(ResampledWidth, ResampledHeight, true, EVoxelMaterialConfig::MultiIndex);

			TArray<float> ResampledData;
			FGRDReader::ResampleData(HeightGrid, SamplesPerRow, SamplesPerColumm, ResampledData, ResampledWidth, ResampledHeight);

			for (int32 X = 0; X < ResampledWidth; X++)
			{
				for (int32 Y = 0; Y < ResampledHeight; Y++)
				{
					const int32 Index = X + ResampledHeight * Y;
					HeightmapData.SetHeight(X, Y, ResampledData[Index]);
					HeightmapData.SetMaterial_MultiIndex (X, Y, FVoxelMaterial());
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot Read xml file!"));

		bSuccess = false;
	}

	delete XmlFile;

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

TArray<FTransform> UVoxelExtensionLibrary::MakeInstanceTransformsWithDensityMask(FBox Bounds, UTexture2D* MaskTexture, int32 Channel)
{
	TArray<FTransform> Results;

	return Results;
}
