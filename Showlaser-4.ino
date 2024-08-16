#include <NativeEthernet.h>
#include "Laser.h"
#include <ArduinoJson.h>
#include <queue>
#include "Settings.h"
#include "OledModule.h"
#include "Watchdog_t4.h"

WDT_T4<WDT1> _watchdog;
Laser _laser;
OledModule _oledModule;

EthernetClient _client;
IPAddress _server;

unsigned long _previousScreenUpdate = 0;
unsigned short _screenRefreshRate = 60; // Fps

unsigned long _previousSettingsCheck = 0;

enum laserStatus
{
  Defect = 0,                     // An hardware defect has been detected and the laser is locked due to safety reasons
  Ready = 1,                      // The laser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2, // The laser has no connection to the controller
  NotConfigured = 3               // The laser is not yet configured
};

laserStatus _currentLaserStatus;

/**
  @brief Generate a MAC address for the Teensy

  @param mac the variable too write the mac address to
 */
void teensyMAC(uint8_t *mac)
{
  for (uint8_t by = 0; by < 2; by++)
  {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++)
  {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}

/**
 @brief Sets the status of the laser and performs certain actions based on the status

 @param status the status of the laser
 */
void setLaserStatus(laserStatus status)
{
  _currentLaserStatus = status;

  if (status == laserStatus::Defect)
  {
    while (true)
    { // keep in a infinite loop so the laser is not reachable
    }
  }
  if (status == laserStatus::Ready)
  {
    _laser.hardwareSelfCheck();
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::ConnectionToControllerLost)
  {
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::NotConfigured)
  {
    // TODO do stuff with the led infront and show menu on OLED screen
  }
}

/**
  @brief This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function

  @param controllerIp the IP address of the controller to connect to
 */
void connectToController(String controllerIp)
{
  char firstChar = controllerIp.charAt(0);
  if (firstChar == 255 || controllerIp.length() == 0)
  {
    setLaserStatus(laserStatus::NotConfigured);
    return;
  }

  unsigned int attempts = 0;
  while (!_client.connect(_server, 50000))
  { // keep trying to connect to controller
    attempts++;
    if (attempts > 10)
    {
      setLaserStatus(laserStatus::ConnectionToControllerLost);
      return;
    }
  }
}

/**
 @brief Checks if the array objects in the array are not empty

 @param array the array to check
 @return true if the objects in the array do not have a default value, false if the the objects in the array have a default value
*/
bool checkIfArrayIsNotEmpty(byte array[])
{
  int emptyValueOccurrences = 0;
  const unsigned int lengthOfArray = sizeof(byte) / sizeof(array[0]);
  for (unsigned int i = 0; i < lengthOfArray; i++)
  {
    if (i == 255)
    {
      emptyValueOccurrences++;
    }
  }

  return emptyValueOccurrences != lengthOfArray;
}

/**
 @brief Checks the settings of the settings model and displays warnings on the bottom part of the menu
*/
void checkSettingsAndDisplayWarnings()
{
  bool controllerIpIsEmpty = !checkIfArrayIsNotEmpty(laserSettings.controllerIp);
  if (controllerIpIsEmpty)
  {
    _oledModule.setBottomMessage("Controller IP not set");
    return;
  }

  bool maxLaserPowerIsLow = laserSettings.maxPowerRgb < 20;
  if (maxLaserPowerIsLow)
  {
    _oledModule.setBottomMessage("Max laser power low");
    return;
  }

  _oledModule.setBottomMessage("");
}

void myCallback()
{
  // TODO see if this function is needed
}

void configureWatchdog()
{
  _watchdog_timings_t config;
  config.trigger = 5;  /* in seconds, 0->128 */
  config.timeout = 10; /* in seconds, 0->128 */
  config.callback = myCallback;
  _watchdog.begin(config);
}

bool emergencyButtonIsPressed()
{
  return digitalRead(D7) == 1;
}

void setLaserInEmergencyMode()
{
  _laser.disableLasers();
}

void setup()
{
  configureWatchdog();

  initSettings();
  _laser.init();

  _oledModule.init();
  _laser.hardwareSelfCheck();

  bool emergencyButtonIsConnected = digitalRead(D7) == 1;
  while (emergencyButtonIsConnected)
  {
    // todo set protocol for this case
  }

  checkSettingsAndDisplayWarnings();

  // int value = 4096;
  /*while (true)
  {
    const unsigned short timePerFrameInMs = 1000 / _screenRefreshRate;
    if (millis() - _previousScreenUpdate > timePerFrameInMs)
    {
      _oledModule.checkForInput();
      _previousScreenUpdate = millis();

      dac1.setVoltageA(value);
      dac1.setVoltageB(value);
      dac1.updateDAC();

      dac2.setVoltageA(value);
      dac2.setVoltageB(value);
      dac2.updateDAC();

      if (value > 4000)
      {
        value = 0;
      }

      value++;
    }

    if (millis() - _previousSettingsCheck > 5000)
    {
      checkSettingsAndDisplayWarnings();
      _previousSettingsCheck = millis();
    }

    _oledModule.setBottomMessage("Hello");
    _watchdog.feed();

    _oledModule.setBottomMessage("Hello 2");
    _watchdog.feed();
  }*/

  _watchdog.feed();
  byte mac[6];
  teensyMAC(mac);
  Ethernet.begin(mac);

  bool settingsValid = checkIfArrayIsNotEmpty(laserSettings.controllerIp);
  if (!settingsValid)
  {
    setLaserStatus(laserStatus::NotConfigured);
  }
}

void loop()
{
  _watchdog.feed();

  if (_client.connected())
  {
  }
  else
  {
    _laser.setLaserPower(0, 0, 0);
    _client.stop();
    setLaserStatus(laserStatus::ConnectionToControllerLost);
  }
}