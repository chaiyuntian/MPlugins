// Copyright 2018 Lian Zhang, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ThumbnailExporterStyle.h"

class FThumbnailExporterCommands : public TCommands<FThumbnailExporterCommands>
{
public:

	FThumbnailExporterCommands()
		: TCommands<FThumbnailExporterCommands>(TEXT("ThumbnailExporter"), NSLOCTEXT("Contexts", "ThumbnailExporter", "ThumbnailExporter Plugin"), NAME_None, FThumbnailExporterStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};