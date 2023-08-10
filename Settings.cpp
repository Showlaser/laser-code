#include "Settings.h"

settingsModel laserSettings;

/**
 @brief Saves the settings to the EEPROM 
*/
void setSettings() {
  EEPROM.put(0, laserSettings);
}

/**
  @brief Loads the saved settings from the EEPROM by reference in the laserSettings variable
*/
void initSettings() {
  EEPROM.get(0, laserSettings);
}