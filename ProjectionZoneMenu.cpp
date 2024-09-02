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
  String _laserPower = "Laser power: " + String(settings.maxPowerRgb);
  String _width = "Width:" + String(settings.projectionWidthInPercentage) + "%";
  String _height = "Height:" + String(settings.projectionHeightInPercentage) + "%";
  String _offsetX = "Offset x:" + String(settings.projectionOffsetXInPercentage) + "%";
  String _offsetY = "Offset y:" + String(settings.projectionOffsetYInPercentage) + "%";

  if (_currentSelectedItemName == "") {
    if (rotaryValue >= menuItemsLength || rotaryValue < 0) {
      oledModule.resetRotaryValue();
      return;
    }
  }

  String menuItems[menuItemsLength] = { _laserPower, _width, _height, _offsetX, _offsetY, ExitMenuName };
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
        oledModule.overWriteRotaryValue(settings.maxPowerRgb * 4);
      } else if (_currentSelectedItemName == _width) {
        oledModule.overWriteRotaryValue(settings.projectionWidthInPercentage * 4);
      } else if (_currentSelectedItemName == _height) {
        oledModule.overWriteRotaryValue(settings.projectionHeightInPercentage * 4);
      } else if (_currentSelectedItemName == _offsetX) {
        oledModule.overWriteRotaryValue(settings.projectionOffsetXInPercentage * 4);
      } else if (_currentSelectedItemName == _offsetY) {
        oledModule.overWriteRotaryValue(settings.projectionOffsetYInPercentage * 4);
      }
    } else {
      _currentSelectedItemName = "";
      Settings::saveSettings();
    }
  }
  if (_currentSelectedItemName != "") {
    if (_currentSelectedItemName == _laserPower) {
      auto callbackFunction = [&]() {
        settings.maxPowerRgb = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Laser power: " + String(settings.maxPowerRgb);
        _laserPower = _currentSelectedItemName;

        menuItems[0] = _laserPower;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 255, callbackFunction);
    } else if (_currentSelectedItemName == _width) {
      auto callbackFunction = [&]() {
        settings.projectionWidthInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Width:" + String(settings.projectionWidthInPercentage) + "%";
        _width = _currentSelectedItemName;

        menuItems[1] = _width;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _height) {
      auto callbackFunction = [&]() {
        settings.projectionHeightInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);
        
        _currentSelectedItemName = "Height:" + String(settings.projectionHeightInPercentage) + "%";
        _height = _currentSelectedItemName;

        menuItems[2] = _height;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _offsetX) {
      auto callbackFunction = [&]() {
        settings.projectionOffsetXInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Offset x:" + String(settings.projectionOffsetXInPercentage) + "%";
        _offsetX = _currentSelectedItemName;
        
        menuItems[3] = _offsetX;
        oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
      };

      onUpdate(oledModule, rotaryValue, 0, 100, callbackFunction);
    } else if (_currentSelectedItemName == _offsetY) {
        auto callbackFunction = [&]() {
        settings.projectionOffsetYInPercentage = (byte)rotaryValue;
        Settings::setSettings(settings);

        _currentSelectedItemName = "Offset y:" + String(settings.projectionOffsetYInPercentage) + "%";
        _offsetY = _currentSelectedItemName;

        menuItems[4] = _offsetY;
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