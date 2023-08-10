#ifndef OledModule_H
#define OledModule_H

#include "Arduino.h"
#include "Settings.h"

class OledModule {
public:
  void init();
  void checkForInput();
  void setBottomMessage(String message);

private:
  unsigned short _currentSelectedMenuId = 0;
  unsigned short _currentSelectedMenuItemId = 0;
  int _currentSelectedMenuItemValue = -999;
  unsigned short _menuItemsCount = 0;
  String _bottomMessage = "";

  int _buttonState;
  int _lastButtonState = 0;
  unsigned long _lastDebounceTime = 0;
  unsigned long _debounceDelay = 50;

  const byte _outputA = 2;
  const byte _outputB = 3;
  const byte _switch = 4;

  void showMenuOnOledDisplay(byte menuId);
  void drawTopAndBottomOfMenu(String menuName);
  void showMenu(bool buttonPressed);
  void showMainMenu(bool buttonPressed);
  void showControllerIpMenu(bool buttonPressed);
  void showMaxLaserPowerMenu(bool buttonPressed);
  void showStatusMenu(bool buttonPressed);
  void exitSubMenu();
  void setCurrentMenuIdByRotaryInput();
  bool checkForButtonPress();
};

#endif