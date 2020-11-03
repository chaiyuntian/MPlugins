// Mans Isaksson 2020

#include "ThumbnailGeneratorModule.h"

DEFINE_LOG_CATEGORY(LogThumbnailGenerator);

#if WITH_EDITOR
FThumbnailGeneratorModule::FOnSaveThumbnail FThumbnailGeneratorModule::OnSaveThumbnail;
#endif

void FThumbnailGeneratorModule::StartupModule()
{
}

void FThumbnailGeneratorModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FThumbnailGeneratorModule, ThumbnailGenerator)