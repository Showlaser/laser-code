#include "SDCardMenu.h"
#include "../GlobalConfig.h"
#include "../SDCard.h"
#include <vector>

void SDCardMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  SDCard sdCard;
  bool initSuccess = sdCard.init();

  std::vector<String> menuItemsVector = sdCard.getJsonFiles();
  const int menuItemsLength = menuItemsVector.size() + 1;

  String menuItems[menuItemsLength];
  for (int i = 0; i < menuItemsLength - 1; i++)
  {
    menuItems[i] = menuItemsVector[i];
  }

  menuItems[menuItemsLength - 1] = ExitMenuName;

  if (rotaryValue < menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = menuItems[rotaryValue];
    oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

    if (buttonPressed) {
      Serial.println(String(itemToShowCursorAt));
      if (itemToShowCursorAt == ExitMenuName) {
        currentSelectedMenu = MainMenuName;
        return;
      }

      currentSelectedMenu = PlaySDFileMenuName;
      SelectedSDCardFile = itemToShowCursorAt;
    }
  } else {
    oledModule.resetRotaryValue();
  }

  oledModule.displayChanges();
}

String SDCardMenu::getMenuName() {
  return SDCardMenuName;
}