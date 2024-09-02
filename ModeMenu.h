#ifndef MODEMENU_H
#define MODEMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "OledModule.h"

class ModeMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif