#include "PlaySDFileMenu.h"
#include "../GlobalConfig.h"
#include "../SDCard.h"

void PlaySDFileMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed)
{
  SDCard sdCard;
  sdCard.init();

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
        String json = sdCard.readJsonFile(SelectedSDCardFilename);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
          return;
        }

        SelectedSDCardJson = doc;
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