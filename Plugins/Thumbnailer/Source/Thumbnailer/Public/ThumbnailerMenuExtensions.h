/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

class THUMBNAILER_API FThumbnailerMenuExtensions
{
public:
	static void CreateAssetContextMenu(FMenuBuilder& menuBuilder, TArray<FAssetData> objects);
};
