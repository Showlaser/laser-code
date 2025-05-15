#ifndef AUDIENCESHUTTERMENU_H
#define AUDIENCESHUTTERMENU_H

#include "Arduino.h"
#include "../IMenu.h"
#include "../OledModule.h"
#include "../GlobalConfig.h"
#include "../Settings.h"
#include "../AudienceShutter.h"

class AudienceShutterMenu : public IMenu {
public:
  void displayMenu(OledModule &oledModule, String &currentSelectedMenu, int rotaryValue, bool buttonPressed);
  String getMenuName();

private:
  int _previousRotaryValue;

  String _currentSelectedItemName;

  /**
  @brief Contains code which checks if the value is within range and execute a callback if it passes
  */
  void onUpdate(OledModule &oledModule, int &rotaryValue, int minValue, int maxValue, std::function<void()> callback);
};

#endif