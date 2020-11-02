// Fill out your copyright notice in the Description page of Project Settings.


#include "GRDReader.h"

DEFINE_LOG_CATEGORY_STATIC(LogGRDReader, Log, All);


// space and return
static void JumpOverWhiteSpace(const uint8*& BufferPos)
{
	while (*BufferPos)
	{
		if (*BufferPos == 13 && *(BufferPos + 1) == 10)
		{
			BufferPos += 2;
			continue;
		}
		else if (*BufferPos <= ' ')
		{
			// Skip tab, space and invisible characters
			++BufferPos;
			continue;
		}

		break;
	}
}

static void GetLineContent(const uint8*& BufferPos, char Line[256], bool bStopOnWhitespace)
{
	JumpOverWhiteSpace(BufferPos);

	char* LinePtr = Line;

	uint32 i;

	for (i = 0; i < 255; ++i)
	{
		if (*BufferPos == 0)
		{
			break;
		}
		else if (*BufferPos == '\r' && *(BufferPos + 1) == '\n')
		{
			BufferPos += 2;
			break;
		}
		else if (*BufferPos == '\n')
		{
			++BufferPos;
			break;
		}
		else if (bStopOnWhitespace && (*BufferPos <= ' '))
		{
			// tab, space, invisible characters
			++BufferPos;
			break;
		}

		*LinePtr++ = *BufferPos++;
	}

	Line[i] = 0;
}


// @return success
static bool GetFloat(const uint8*& BufferPos, float& ret)
{
	char Line[256];

	GetLineContent(BufferPos, Line, true);

	ret = FCStringAnsi::Atof(Line);
	return true;
}


static bool GetInt(const uint8*& BufferPos, int32& ret)
{
	char Line[256];

	GetLineContent(BufferPos, Line, true);

	ret = FCStringAnsi::Atoi(Line);
	return true;
}

#define PARSE_FLOAT(x) float x; if(!GetFloat(BufferPos, x)){ UE_LOG(LogGRDReader, Warning, TEXT("Value parsing error: Error reading GRD file float value!")) return false;}
#define PARSE_INT(x) int32 x; if(!GetInt(BufferPos, x)){ UE_LOG(LogGRDReader, Warning, TEXT("Value parsing error: Error reading GRD file int value!")) return false;}

bool FGRDReader::Load(FString FilePath, TArray<float>& ValuArray, FGRDHeaderInfo& HeaderInfo)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FilePath, FILEREAD_Silent))
	{
		UE_LOG(LogGRDReader, Warning, TEXT("Error reading GRD file"));
		return false;
	}

	const uint8* BufferPos = Bytes.GetData();

	char Line1[256];

	GetLineContent(BufferPos, Line1, false);

	// Check the first line
	if (FCStringAnsi::Stricmp(Line1, "DSAA") != 0)
	{
		UE_LOG(LogGRDReader, Warning, TEXT("GRD File Format Error: the first line should be DSAA!"));
		return false;
	}
	
	PARSE_INT(GridsX);
	PARSE_INT(GridsY);

	PARSE_FLOAT(XMin); PARSE_FLOAT(XMax);
	PARSE_FLOAT(YMin); PARSE_FLOAT(YMax);
	PARSE_FLOAT(ZMin); PARSE_FLOAT(ZMax);

	HeaderInfo.GridsX = GridsX;
	HeaderInfo.GridsY = GridsY;

	HeaderInfo.XRange = FGridValueRange(XMin,XMax);
	HeaderInfo.YRange = FGridValueRange(YMin, YMax);
	HeaderInfo.ZRange = FGridValueRange(ZMin, ZMax);

	ValuArray.Empty(GridsX * GridsY);

	for (uint32 y = 0; y < (uint32)GridsY; ++y)
	{
		for (uint32 x = 0; x < (uint32)GridsX; ++x)
		{
			PARSE_FLOAT(Value);
			ValuArray.Add(Value);
		}
	}

	return true;
}



void FGRDReader::ResampleData(const TArray<float>& SourceData, uint32 SrcWidth, uint32 SrcHeight, TArray<float>& DstData, uint32 DstWidth, uint32 DstHeight)
{
	DstData.Empty(DstWidth * DstHeight);
	DstData.AddUninitialized(DstWidth * DstHeight);
	
	const float* SrcData = SourceData.GetData();

	const float DestToSrcScaleX = (float)SrcWidth / (float)DstWidth;
	const float DestToSrcScaleY = (float)SrcHeight / (float)DstHeight;

	for (uint32 Y = 0;  Y < (uint32)DstHeight; ++Y)
	{
		const float SrcY = (float)Y * DestToSrcScaleY;

		for (uint32 X = 0; X < (uint32)DstWidth; ++X)
		{

			const int32 Index = X + DstWidth * Y;
			const float SrcX = (float)X * DestToSrcScaleX;
			DstData[Index] = SampleGrid(SrcData, SrcWidth, SrcHeight, SrcX, SrcY);
		}
	}
}

float FGRDReader::SampleGrid(const float* HeightValues, int Width, int Height, float X, float Y)
{
	const int64 TexelX0 = FMath::FloorToInt(X);
	const int64 TexelY0 = FMath::FloorToInt(Y);
	const int64 TexelX1 = FMath::Min<int64>(TexelX0 + 1, Width - 1);
	const int64 TexelY1 = FMath::Min<int64>(TexelY0 + 1, Height - 1);
	checkSlow(TexelX0 >= 0 && TexelX0 < Width);
	checkSlow(TexelY0 >= 0 && TexelY0 < Height);

	const float FracX1 = FMath::Frac(X);
	const float FracY1 = FMath::Frac(Y);
	const float FracX0 = 1.0f - FracX1;
	const float FracY0 = 1.0f - FracY1;
	const float Value00 = HeightValues[TexelY0 * Width + TexelX0];
	const float Value01 = HeightValues[TexelY1 * Width + TexelX0];
	const float Value10 = HeightValues[TexelY0 * Width + TexelX1];
	const float Value11 = HeightValues[TexelY1 * Width + TexelX1];
	return
		Value00 * (FracX0 * FracY0) +
		Value01 * (FracX0 * FracY1) +
		Value10 * (FracX1 * FracY0) +
		Value11 * (FracX1 * FracY1);
}