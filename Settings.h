#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h>

struct settingsModel {
  byte controllerIp[4];
  byte maxPowerRgb;
};

void setSettings(settingsModel settings);
void getSettings(settingsModel &settings);

#endif