#ifndef SETTINGS_H
#define SETTINGS_H

#include "Arduino.h"

struct settingsModel {
  String controllerIp;
  byte maxLaserPowerRgb[];
};

class Settings {
public:
  Settings();
  void getFromEEPROM(settingsModel& settings);
  void writeToEEPROM(settingsModel& settings);

private:
};

#endif