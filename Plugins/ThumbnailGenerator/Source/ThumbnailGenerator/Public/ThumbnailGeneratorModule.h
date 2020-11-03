// Mans Isaksson 2020

#pragma once

#include "Modules/ModuleManager.h"
#include "Delegates/DelegateCombinations.h"
#include "Logging/LogMacros.h"

class UTexture2D;

DECLARE_LOG_CATEGORY_EXTERN(LogThumbnailGenerator, Log, All);

class FThumbnailGeneratorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSaveThumbnail, UTexture2D*, const FString&, const FString&);
	THUMBNAILGENERATOR_API static FOnSaveThumbnail OnSaveThumbnail;
#endif
};
