#include "ModeMenu.h"
#include "GlobalConfig.h"

void ModeMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, int previousRotaryValue, bool buttonPressed) {
  const int menuItemsLength = 2;
  String menuItems[menuItemsLength] = { StandAloneMenuName, ExitMenuName };
  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      if (itemToShowCursorAt == ExitMenuName) {
        currentSelectedMenu = MainMenuName;
        return;
      }

      currentSelectedMenu = itemToShowCursorAt;
    }
  } else {
    oledModule.resetRotaryValue();
  }
}

String ModeMenu::getMenuName() {
  return ModeSelectMenuName;
}