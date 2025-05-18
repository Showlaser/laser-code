#include "PlaySDFileMenu.h"
#include "../GlobalConfig.h"
#include "../SDCard.h"

void PlaySDFileMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  Serial.println("Init sd");
  Serial.println("Play SD");
  SDCard sdCard;
  bool initSuccess = sdCard.init();
  Serial.println(initSuccess ? "init success" : "Init failed");

  const int menuItemsLength = 3;
  String menuItems[menuItemsLength] = { "Play " + SelectedSDCardFile, "Delete " + SelectedSDCardFile, ExitMenuName };
  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      Serial.print("show cursor ");
      Serial.println(String(itemToShowCursorAt));
      if (itemToShowCursorAt == ExitMenuName) {
        SelectedSDCardFile = "";
        currentSelectedMenu = SDCardMenuName;
        return;
      }

      if (itemToShowCursorAt == "Play " + SelectedSDCardFile) {
        Serial.println("Play " + SelectedSDCardFile);
        CurrentLaserMode = LaserMode::SDCardMode;
        return;
      }

      if ("Delete " + SelectedSDCardFile) {
        Serial.println("Delete file");
        //sdCard.deleteJsonFile(SelectedSDCardFile);
        SelectedSDCardFile = "";
        currentSelectedMenu = SDCardMenuName;
        return;
      }
    }
  } else {
    oledModule.resetRotaryValue();
  }

  oledModule.displayChanges();
}

String PlaySDFileMenu::getMenuName() {
  return PlaySDFileMenuName;
}