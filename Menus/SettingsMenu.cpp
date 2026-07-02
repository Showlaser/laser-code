#include "SettingsMenu.h"
#include "../GlobalConfig.h"

void SettingsMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  const int menuItemsLength = 2;
  String menuItems[menuItemsLength] = { ProjectionZoneMenuName, ExitMenuName };
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

  oledModule.displayChanges();
}

String SettingsMenu::getMenuName() {
  return SettingsMenuName;
}