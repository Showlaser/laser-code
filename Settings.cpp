#include "Settings.h"

settingsModel laserSettings;

/**
 @brief Saves the settings to the EEPROM 
*/
settingsModel Settings::setSettings() {
  settingsModel settings;
  EEPROM.put(0, settings);
  return settings;
}

/**
  @brief Loads the saved settings from the EEPROM by reference in the laserSettings variable
*/
settingsModel Settings::getSettings() {
  settingsModel settings;
  EEPROM.get(0, settings);
  return settings;
}