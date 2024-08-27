#ifndef STANDALONEMODE_H
#define STANDALONEMODE_H

#include "Arduino.h"
#include "IMode.h"
#include "OledModule.h"

class StandaloneMode : public IMode {
public:
  void init(WDT_T4<WDT1> &watchdog, OledModule &oled);
  void execute();
  void forceStop();

private:
  WDT_T4<WDT1> _watchdog;
  OledModule *_oledModule;

  void drawMenu(int rotaryValue, bool buttonPressed);
  void drawMainMenu(int rotaryValue, bool buttonPressed);

  int _previousRotaryValue = -999;
  bool _previousButtonPressed = false;

  bool _executeStandaloneMode = false;

  int _cursorX = 0;
  int _cursorY = 0;
  int _currentMenu = 0;
};

#endif