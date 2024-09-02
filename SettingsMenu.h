#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "OledModule.h"

class SettingsMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif