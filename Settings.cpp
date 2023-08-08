#include "Settings.h"

/**
 @brief Saves the settings to the EEPROM 
*/
void setSettings(settingsModel settings) {
  EEPROM.put(0, settings);
}

/**
  @brief Gets the saved settings from the EEPROM by reference
*/
void getSettings(settingsModel &settings) {
  EEPROM.get(0, settings);
}