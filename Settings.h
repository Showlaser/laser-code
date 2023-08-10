#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h>

struct settingsModel {
  byte controllerIp[4];
  byte maxPowerRgb;
};

extern settingsModel laserSettings;

void initSettings();
void setSettings();

#endif