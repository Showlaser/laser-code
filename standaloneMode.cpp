#include "StandaloneMode.h"

void StandaloneMode::init(WDT_T4<WDT1> &watchdog, OledModule &oled) {
  _watchdog = watchdog;
  _oledModule = &oled;
}

void StandaloneMode::drawMainMenu(int rotaryValue, bool buttonPressed) {
  if (rotaryValue == 1) {
  }

  _oledModule->setCursor(0, 0);
}

void StandaloneMode::execute() {
  _watchdog.feed();
}

void StandaloneMode::forceStop() {
}