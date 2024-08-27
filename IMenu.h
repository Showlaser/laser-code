#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include "OledModule.h"

class IMenu {
public:
  /**
  @brief displays the menu
  @param rotaryValue the current value of the rotary encoder
  @param buttonPressed if the button is pressed on the rotary encoder
  */
  virtual void displayMenu(OledModule &oledModule, int rotaryValue, bool buttonPressed);

  /**
  @brief returns the name of the menu
  @return the name of the menu
  */
  virtual String getMenuName();

  virtual ~IMenu() {}

private:
};

#endif