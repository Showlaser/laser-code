#include "Settings.h"

settingsModel Settings::_currentSettings;
bool Settings::_settingsInitialized = false;

/**
 @brief Saves the settings to the EEPROM 
*/
void Settings::setSettings(settingsModel &settings) {
  _currentSettings = settings;
}

/**
 @brief saves the cached settings in the EEPROM
*/
void Settings::saveSettings() {
  EEPROM.put(0, _currentSettings);
}

/**
  @brief Loads the saved settings from the EEPROM by reference in the laserSettings variable
*/
settingsModel Settings::getSettings() {
  settingsModel settings;

  if (_settingsInitialized) {
    settings = _currentSettings;
  } else {
    EEPROM.get(0, settings);
    _settingsInitialized = true;
    _currentSettings = settings;
  }

  return settings;
}