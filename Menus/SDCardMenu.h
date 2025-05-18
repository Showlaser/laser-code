#ifndef SDCARDMENU_H
#define SDCARDMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "../OledModule.h"

class SDCardMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif