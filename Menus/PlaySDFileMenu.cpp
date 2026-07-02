#include "PlaySDFileMenu.h"
#include "../GlobalConfig.h"
#include "../SDCard.h"

void PlaySDFileMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed)
{
  // SD is already mounted at boot (initSDCard). Re-initialising here on every
  // render would re-mount the card and could invalidate the file PlaySDFileMode
  // is streaming during playback, so only the (stateless) wrapper is needed.
  SDCard sdCard;

  const int menuItemsLength = 3;
  String menuItems[menuItemsLength] = {"Play " + SelectedSDCardFilename, "Delete " + SelectedSDCardFilename, ExitMenuName};
  if (rotaryValue < menuItemsLength && rotaryValue >= 0)
  {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed)
    {
      if (itemToShowCursorAt == ExitMenuName)
      {
        SelectedSDCardFilename = "";
        currentSelectedMenu = SDCardMenuName;
        CurrentLaserMode = LaserMode::NotSelected;
        return;
      }

      if (itemToShowCursorAt == "Play " + SelectedSDCardFilename)
      {
        Serial.println("Play");
        // The show is streamed frame-by-frame from SD by PlaySDFileMode, so we
        // no longer load the whole file into RAM here -- just select the mode.
        CurrentLaserMode = LaserMode::SDCardMode;
        return;
      }

      if (itemToShowCursorAt == "Delete " + SelectedSDCardFilename)
      {
        sdCard.deleteJsonFile(SelectedSDCardFilename);
        SelectedSDCardFilename = "";
        currentSelectedMenu = SDCardMenuName;
        CurrentLaserMode = LaserMode::NotSelected;
        return;
      }
    }
  }
  else
  {
    oledModule.resetRotaryValue();
  }

  oledModule.displayChanges();
}

String PlaySDFileMenu::getMenuName()
{
  return PlaySDFileMenuName;
}