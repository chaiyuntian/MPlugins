// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


struct FGridValueRange
{
	FGridValueRange() {
		MIN = 0;
		MAX = 0;
	}

	FGridValueRange(float InMin, float InMax)
	{
		MIN = InMin;
		MAX = InMax;
	}

	float MIN;
	float MAX;
};

struct FGRDHeaderInfo
{
	// The first linke should always be "DSAA"
	uint32 GridsX;
	uint32 GridsY;
	FGridValueRange XRange;
	FGridValueRange YRange;
	FGridValueRange ZRange;
};

/**
 * Grid Reander, rembember to check the first line.
 * Check the first line 
 */
class VOXELEXTENSION_API FGRDReader
{
public:
	static bool Load(FString FilePath, TArray<float>& ValueArray, FGRDHeaderInfo& HeaderInfo);

	static void ResampleData(const TArray<float>& SourceData, uint32 SrcWidth, uint32 SrcHeight, TArray<float>& DstData, uint32 DstWidth, uint32 DstHeight);

	static float SampleGrid(const float* HeightValues, int Width, int Height, float X, float Y);


};
