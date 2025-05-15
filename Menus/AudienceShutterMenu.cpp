#include "AudienceShutterMenu.h"

int _previousRotaryValue = 0;

void AudienceShutterMenu::onUpdate(OledModule &oledModule, int &rotaryValue, int minValue, int maxValue, std::function<void()> callback) {
  if (rotaryValue < minValue) {
    oledModule.overWriteRotaryValue(minValue * 4);
    return;
  }
  if (rotaryValue > maxValue) {
    oledModule.overWriteRotaryValue(maxValue * 4);
    return;
  }

  oledModule.clearDisplay();
  callback();
}

void AudienceShutterMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  const int menuItemsLength = 2;
  String _height = "Adjust height";


  if (_currentSelectedItemName == "") {
    if (rotaryValue >= menuItemsLength || rotaryValue < -1) {
      oledModule.resetRotaryValue();
      return;
    }
  }

  String menuItems[menuItemsLength] = { _height, ExitMenuName };
  String itemToShowCursorAt = _currentSelectedItemName;
  if (_currentSelectedItemName == "") {
    itemToShowCursorAt = menuItems[rotaryValue];
  }

  oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);

  if (buttonPressed) {
    if (itemToShowCursorAt == ExitMenuName) {
      currentSelectedMenu = SettingsMenuName;
      _currentSelectedItemName = "";
      return;
    }

    if (_currentSelectedItemName != itemToShowCursorAt) {
      _currentSelectedItemName = itemToShowCursorAt;
    } else {
      _currentSelectedItemName = "";
    }
  }
  if (_currentSelectedItemName != "") {
    if (_currentSelectedItemName == _height) {
      auto callbackFunction = [&]() {
        if (rotaryValue > 0) {
          AudienceShutter::moveShutterUp();
        } else if (rotaryValue < 0) {
          AudienceShutter::moveShutterDown();
        } else if (rotaryValue == 0) {
          AudienceShutter::stopMoving();
        }

        _currentSelectedItemName = _height;
        _height = _currentSelectedItemName;

        menuItems[0] = _height;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, -1, 1, callbackFunction);
    }
  }

  oledModule.displayChanges();
  _previousRotaryValue = rotaryValue;
}

String AudienceShutterMenu::getMenuName() {
  return AudienceShutterMenuName;
}