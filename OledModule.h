#ifndef OledModule_H
#define OledModule_H
#include <Arduino.h>

class OledModule {
public:
  void checkForInput();

private:
  long currentSelectedMenuId = -999;
  int buttonState;
  int lastButtonState = 0;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;

  const byte _outputA = 2;
  const byte _outputB = 3;
  const byte _switch = 4;
  void showMenuOnOledDisplay(byte menuId);
  void setCurrentMenuIdByRotaryInput();
};

#endif