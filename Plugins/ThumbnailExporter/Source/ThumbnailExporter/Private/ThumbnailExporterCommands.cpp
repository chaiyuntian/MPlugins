// Copyright 2018 Lian Zhang,  All Rights Reserved.

#include "ThumbnailExporterCommands.h"

#define LOCTEXT_NAMESPACE "FThumbnailExporterModule"

void FThumbnailExporterCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ThumbnailExporter", "Bring up ThumbnailExporter window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
