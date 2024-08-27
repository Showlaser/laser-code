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

void StandaloneMode::drawMenu(int rotaryValue, bool buttonPressed) {
  switch (_currentMenu) {
    case 0:
      drawMainMenu(rotaryValue, buttonPressed);
      break;
    default:
      break;
  }
}

void StandaloneMode::execute() {
  _watchdog.feed();
  int rotation = _oledModule->getRotaryEncoderRotation();
  bool buttonPressed = _oledModule->checkForButtonPress();

  if (rotation != _previousRotaryValue || buttonPressed != _previousButtonPressed) {
    drawMenu(rotation, buttonPressed);
  }
}

void StandaloneMode::forceStop() {
}