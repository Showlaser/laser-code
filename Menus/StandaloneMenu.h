#ifndef STANDALONE_H
#define STANDALONE_H

#include "Arduino.h"
#include "../IMenu.h"
#include "../OledModule.h"

class StandaloneMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif