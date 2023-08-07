#ifndef OledModule_H
#define OledModule_H
#include <Arduino.h>

class OledModule {
public:
  void init();
  void checkForInput();
  void displayMessage(String message);

private:
  unsigned short currentSelectedMenuId = 0;
  unsigned short currentSelectedMenuItemId = 0;
  unsigned short menuItemsCount = 0;
  String bottomMessage = "";

  int buttonState;
  int lastButtonState = 0;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;

  const byte _outputA = 2;
  const byte _outputB = 3;
  const byte _switch = 4;

  void showMenuOnOledDisplay(byte menuId);
  void setBottomMessage(String message);
  void drawTopAndBottomOfMenu(String menuName);
  void showMenu(bool buttonPressed);
  void showMainMenu();
  void setCurrentMenuIdByRotaryInput();
};

#endif