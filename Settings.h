#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h>

struct settingsModel {
  byte controllerIp[4];
  byte maxPowerRgb;
  byte projectionWidthInPercentage;
  byte projectionHeightInPercentage;
  byte projectionOffsetXInPercentage;
  byte projectionOffsetYInPercentage;
};

class Settings {
public:
  static settingsModel setSettings(settingsModel settings);
  static settingsModel getSettings();

private:
  static settingsModel _currentSettings;
  static bool arrayIsEmpty(byte array[]);
};

#endif