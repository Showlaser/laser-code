#include "StandaloneMenu.h"
#include "../GlobalConfig.h"

const String _startProjection = "Start projection";

void StandaloneMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  const int menuItemsLength = 2;
  String menuItems[menuItemsLength] = { _startProjection, ExitMenuName };
  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      if (itemToShowCursorAt == _startProjection) {
        CurrentLaserMode = LaserMode::Standalone;
      }

      if (itemToShowCursorAt == ExitMenuName) {
        CurrentLaserMode = LaserMode::NotSelected;
        currentSelectedMenu = ModeSelectMenuName;
        oledModule.displayChanges();
        return;
      }
    }
  } else {
    oledModule.resetRotaryValue();
  }

  oledModule.displayChanges();
}

String StandaloneMenu::getMenuName() {
  return StandAloneMenuName;
}