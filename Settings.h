#ifndef Settings_H
#define Settings_H

#include <Arduino.h>
#include <EEPROM.h>

struct settingsModel {
  byte controllerIp[4];
  byte maxPowerPerlaserInPercentage;
  byte projectionTopInPercentage;
  byte projectionBottomInPercentage;
  byte projectionLeftInPercentage;
  byte projectionRightInPercentage;
};

class Settings {
public:
  static void setSettings(settingsModel &settings);
  static void saveSettings();
  static settingsModel getSettings();

private:
  static settingsModel _currentSettings;
  static bool _settingsInitialized;
};

#endif