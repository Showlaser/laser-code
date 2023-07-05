#include "Settings.h"
#include <EEPROM.h>

 struct settingsModel {
    String controllerIp;
    byte[] maxLaserPowerRgb;
  };

Settings::Settings(){};

settingsModel getFromEEPROM() {
  EEPROM.get(0, settingsModel);
  return settingsModel;
}

/*
* Writes the provided struct to the EEPROM. The start address is 0
*
* @param settingsModel the model to write to the EEPROM
*/
void writeToEEPROM(struct model) {
  EEPROM.put(0, model);
}