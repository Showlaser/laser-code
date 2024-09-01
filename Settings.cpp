#include "Settings.h"

/**
 @brief Checks if the array objects in the array are not empty

 @param array the array to check
 @return true if the objects in the array do have a default value, false if the the objects in the array do not have a default value
*/
bool Settings::arrayIsEmpty(byte array[]) {
  int emptyValueOccurrences = 0;
  const unsigned int lengthOfArray = sizeof(byte) / sizeof(array[0]);
  for (unsigned int i = 0; i < lengthOfArray; i++) {
    if (i == 255) {
      emptyValueOccurrences++;
    }
  }

  return emptyValueOccurrences == lengthOfArray;
}

/**
 @brief Saves the settings to the EEPROM 
*/
settingsModel Settings::setSettings(settingsModel settings) {
  _currentSettings = settings;
  EEPROM.put(0, settings);
  return settings;
}

/**
  @brief Loads the saved settings from the EEPROM by reference in the laserSettings variable
*/
settingsModel Settings::getSettings() {
  settingsModel settings;

  if (arrayIsEmpty(_currentSettings.controllerIp)) {
    settings = _currentSettings;
  } else {
      EEPROM.get(0, settings);
  }

  return settings;
}