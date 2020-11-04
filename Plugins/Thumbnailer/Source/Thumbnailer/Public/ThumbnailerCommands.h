/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Slate/ThumbnailerStyle.h"

class FThumbnailerCommands : public TCommands<FThumbnailerCommands>
{
public:
	FThumbnailerCommands()
		: TCommands<FThumbnailerCommands>(TEXT("Thumbnailer"), NSLOCTEXT("Contexts", "Thumbnailer", "Thumbnailer Plugin"), NAME_None, FThumbnailerStyle::GetStyleSetName())
	{}
public:
	virtual void RegisterCommands() override;
public:
	TSharedPtr<FUICommandInfo> CreateThumbnailerWindow;
	TSharedPtr<FUICommandInfo> DonateAction;
	TSharedPtr<FUICommandInfo> HelpAction;
	TSharedPtr<FUICommandInfo> DiscordAction;
	TSharedPtr<FUICommandInfo> MarketplaceAction;
};