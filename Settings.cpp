#include "Settings.h"
#include <EEPROM.h>

Settings::Settings() {};

void Settings::getFromEEPROM(settingsModel& settings) {
  EEPROM.get(0, "");
}

/*
* Writes the provided struct to the EEPROM. The start address is 0
*
* @param settingsModel the model to write to the EEPROM
*/
void Settings::writeToEEPROM(settingsModel& settings) {
  EEPROM.put(0, "");
}