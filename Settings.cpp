#include "Settings.h"

settingsModel Settings::_currentSettings;
bool Settings::_settingsInitialized = false;

/**
 @brief validates the settings model before storing it in the global variable
 */
bool Settings::settingsValid(settingsModel settings)
{
  return settings.name[0] != '\0' &&
                            settings.maxPowerPerlaserInPercentage <= 100 &&
                            settings.projectionTopInPercentage <= 100 &&
                            settings.projectionBottomInPercentage <= 100 &&
                            settings.projectionLeftInPercentage <= 100 &&
                            settings.projectionRightInPercentage <= 100;
}

/**
 @brief Saves the settings to the EEPROM
*/
bool Settings::setSettings(settingsModel &settings)
{
  bool settingsValid = Settings::settingsValid(settings);
  if (settingsValid)
  {
    _currentSettings = settings;
    return true;
  }

  return false;
}

/**
 @brief saves the cached settings in the EEPROM
*/
void Settings::saveSettings()
{

  EEPROM.put(0, _currentSettings);
}

/**
  @brief Loads the saved settings from the EEPROM by reference in the laserSettings variable
*/
settingsModel Settings::getSettings()
{
  settingsModel settings;

  if (_settingsInitialized)
  {
    settings = _currentSettings;
  }
  else
  {
    EEPROM.get(0, settings);
    _settingsInitialized = true;
    _currentSettings = settings;
  }

  return settings;
}