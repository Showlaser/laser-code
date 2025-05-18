#ifndef PLAYSDFILEMENU_H
#define PLAYSDFILEMENU_H

#include "Arduino.h"
#include "IMenu.h"
#include "../OledModule.h"

class PlaySDFileMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
};

#endif