#include "ProjectionZoneMenu.h"
#include "GlobalConfig.h"
#include "Settings.h"

void ProjectionZoneMenu::onUpdate(OledModule &oledModule, int &rotaryValue, int minValue, int maxValue, std::function<void()> callback) {
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

void ProjectionZoneMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed) {
  const int menuItemsLength = 6;
  settingsModel settings = Settings::getSettings();
  String _laserPower = "Power p laser: " + String(settings.maxPowerPerlaserInPercentage) + "%";
  String _projectionTopInPercentage = "Top distance:" + String(settings.projectionTopInPercentage) + "%";
  String _projectionBottomInPercentage = "Bottom distance:" + String(settings.projectionBottomInPercentage) + "%";
  String _projectionLeftInPercentage = "Left distance:" + String(settings.projectionLeftInPercentage) + "%";
  String _projectionRightInPercentage = "Right distance:" + String(settings.projectionRightInPercentage) + "%";

  if (_currentSelectedItemName == "") {
    if (rotaryValue >= menuItemsLength || rotaryValue < 0) {
      oledModule.resetRotaryValue();
      return;
    }
  }

  String menuItems[menuItemsLength] = { _laserPower, _projectionTopInPercentage, _projectionBottomInPercentage, _projectionLeftInPercentage, _projectionRightInPercentage, ExitMenuName };
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

      if (_currentSelectedItemName == _laserPower) {
        oledModule.overWriteRotaryValue(settings.maxPowerPerlaserInPercentage * 4);
      } else if (_currentSelectedItemName == _projectionTopInPercentage) {
        oledModule.overWriteRotaryValue(settings.projectionTopInPercentage * 4);
      } else if (_currentSelectedItemName == _projectionBottomInPercentage) {
        oledModule.overWriteRotaryValue(settings.projectionBottomInPercentage * 4);
      } else if (_currentSelectedItemName == _projectionLeftInPercentage) {
        oledModule.overWriteRotaryValue(settings.projectionLeftInPercentage * 4);
      } else if (_currentSelectedItemName == _projectionRightInPercentage) {
        oledModule.overWriteRotaryValue(settings.projectionRightInPercentage * 4);
      }
    } else {
      _currentSelectedItemName = "";
      Settings::saveSettings();
    }
  }
  if (_currentSelectedItemName != "") {
    if (_currentSelectedItemName == _laserPower) {
      auto callbackFunction = [&]() {
        settings.maxPowerPerlaserInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Power p laser: " + String(settings.maxPowerPerlaserInPercentage) + "%";
        _laserPower = _currentSelectedItemName;

        menuItems[0] = _laserPower;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _projectionTopInPercentage) {
      auto callbackFunction = [&]() {
        settings.projectionTopInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Top distance:" + String(settings.projectionTopInPercentage) + "%";
        _projectionTopInPercentage = _currentSelectedItemName;

        menuItems[1] = _projectionTopInPercentage;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _projectionBottomInPercentage) {
      auto callbackFunction = [&]() {
        settings.projectionBottomInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);
        
        _currentSelectedItemName = "Bottom distance:" + String(settings.projectionBottomInPercentage) + "%";
        _projectionBottomInPercentage = _currentSelectedItemName;

        menuItems[2] = _projectionBottomInPercentage;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _projectionLeftInPercentage) {
      auto callbackFunction = [&]() {
        settings.projectionLeftInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Left distance:" + String(settings.projectionLeftInPercentage) + "%";
        _projectionLeftInPercentage = _currentSelectedItemName;
        
        menuItems[3] = _projectionLeftInPercentage;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _projectionRightInPercentage) {
        auto callbackFunction = [&]() {
        settings.projectionRightInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Right distance:" + String(settings.projectionRightInPercentage) + "%";
        _projectionRightInPercentage = _currentSelectedItemName;

        menuItems[4] = _projectionRightInPercentage;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
      }
  }

  oledModule.displayChanges();
}

String ProjectionZoneMenu::getMenuName() {
  return ProjectionZoneMenuName;
}