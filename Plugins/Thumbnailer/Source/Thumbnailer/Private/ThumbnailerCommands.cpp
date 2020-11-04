/**
* Copyright (C) 2017-2020 | Dry Eel Development
*
* Official Thumbnailer Documentation: https://eeldev.com
*/

#include "ThumbnailerCommands.h"

#define LOCTEXT_NAMESPACE "FThumbnailerModule"

void FThumbnailerCommands::RegisterCommands()
{
	UI_COMMAND(CreateThumbnailerWindow, "Thumbnailer", "Open the Thumbnailer Window", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(DonateAction, "Donate to Support eelDev!", "Support eelDev by Donating!", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(HelpAction, "Help & Documentation", "Thumbnailer Documentation", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(DiscordAction, "Discord Server", "eelDev Discord Server", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(MarketplaceAction, "Unreal Marketplace", "Marketplace Page", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE