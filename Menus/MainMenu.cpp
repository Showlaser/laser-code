#include "MainMenu.h"
#include "../GlobalConfig.h"

void MainMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  const int menuItemsLength = 3;
  String menuItems[menuItemsLength] = { ModeSelectMenuName, SDCardMenuName, SettingsMenuName };
  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      currentSelectedMenu = itemToShowCursorAt;
    }
  } else {
    oledModule.resetRotaryValue();
  }

  oledModule.displayChanges();
}

String MainMenu::getMenuName() {
  return MainMenuName;
}