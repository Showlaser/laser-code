#ifndef MAINMENU_H
#define MAINMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "../OledModule.h"

class MainMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif