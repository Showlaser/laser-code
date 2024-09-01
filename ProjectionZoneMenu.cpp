#include "ProjectionZoneMenu.h"
#include "GlobalConfig.h"
#include "Settings.h"

void ProjectionZoneMenu::displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, int previousRotaryValue, bool buttonPressed) {
  const int menuItemsLength = 6;
  settingsModel settings = Settings::getSettings();

  String laserPower = "Laser power: " + String(settings.maxPowerRgb);
  String width = "Width: " + String(settings.projectionWidthInPercentage);
  String height = "Height: " + String(settings.projectionHeightInPercentage);
  String offsetX = "Offset x: " + String(settings.projectionOffsetXInPercentage);
  String offsetY = "Offset y: " + String(settings.projectionOffsetYInPercentage);

  String menuItems[menuItemsLength] = { laserPower, width, height, offsetX, offsetY, ExitMenuName };
  // redo this since it does not work
  if (rotaryValue <= menuItemsLength && rotaryValue >= 0) {
    String itemToShowCursorAt = _selectedItemName == "" ? menuItems[rotaryValue] : _selectedItemName;

    if (buttonPressed && _selectedItemName == "") {
      _selectedItemName = itemToShowCursorAt;
    }

    if (buttonPressed && _selectedItemName != "") {
      if (itemToShowCursorAt == laserPower) {

      } else if (itemToShowCursorAt == width) {

      } else if (itemToShowCursorAt == height) {

      } else if (itemToShowCursorAt == offsetX) {

      } else if (itemToShowCursorAt == offsetY) {

      } else if (itemToShowCursorAt == ExitMenuName) {
        CurrentLaserMode = LaserMode::NotSelected;
        currentSelectedMenu = itemToShowCursorAt;
      }
    }
  } else {
    oledModule.resetRotaryValue();
  }

  // oledModule.displaySelectableMenuItems(menuItems, menuItemsLength, itemToShowCursorAt);
}

String ProjectionZoneMenu::getMenuName() {
  return ProjectionZoneMenuName;
}