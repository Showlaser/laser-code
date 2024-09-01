#include "MainMenu.h"
#include "GlobalConfig.h"

void MainMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, int previousRotaryValue, bool buttonPressed) {
  const int menuItemsLength = 2;
  String menuItems[menuItemsLength] = { ModeSelectMenuName, SettingsMenuName };
  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      currentSelectedMenu = itemToShowCursorAt;
    }
  } else {
    oledModule.resetRotaryValue();
  }
}

String MainMenu::getMenuName() {
  return MainMenuName;
}