#ifndef PROJECTIONZONEMENU_H
#define PROJECTIONZONEMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "OledModule.h"

class ProjectionZoneMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, int previousRotaryValue, bool buttonPressed);
  String getMenuName();

private:
  String _selectedItemName;
};

#endif