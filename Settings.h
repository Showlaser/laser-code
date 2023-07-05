#include "Arduino.h"

class Settings {
public:
  Settings();
  struct settingsModel {
    String controllerIp;
    byte maxLaserPowerRgb[];
  };

  struct settingsModel getFromEEPROM();
  void writeToEEPROM(struct settingsModel);

private:
};