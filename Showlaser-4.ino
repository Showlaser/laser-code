#include <NativeEthernet.h>
#include "Laser.h"
#include <ArduinoJson.h>
#include <queue>
#include "Settings.h"
#include "OledModule.h"
#include "Watchdog_t4.h"
#include "StandaloneMode.h"
#include "IMenu.h"
#include "MainMenu.h"

WDT_T4<WDT1> _watchdog;
Laser _laser;
Settings _settings;
OledModule _oledModule;

EthernetClient _client;
IPAddress _server;

IMenu* _menus[3];
int _currentSelectedMenu = 0; // The current selected menu id of the array _menus

unsigned long _previousScreenUpdate = 0;
unsigned short _screenRefreshRate = 60;  // Fps

unsigned long _previousSettingsCheck = 0;

enum laserStatus {
  Defect = 0,                      // An hardware defect has been detected and the showlaser is locked due to safety reasons
  Ready = 1,                       // The showlaser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2,  // The showlaser has no connection to the controller
  NotConfigured = 3                // The showlaser is not yet configured
};

laserStatus _currentLaserStatus;

enum laserMode {
  Standalone = 0,  // The showlaser is not connected to a controller and is working standalone
  Network = 1,     // The laser is ready to receive and process commands that are received
};

laserMode _currentLaserMode;

/**
  @brief Generate a MAC address for the Teensy

  @param mac the variable too write the mac address to
 */
void teensyMAC(uint8_t* mac) {
  for (uint8_t by = 0; by < 2; by++) {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++) {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}

/**
 @brief Sets the status of the laser and performs certain actions based on the status

 @param status the status of the laser
 */
void setLaserStatus(laserStatus status) {
  _currentLaserStatus = status;

  if (status == laserStatus::Defect) {
    while (true) {  // keep in a infinite loop so the laser is not reachable
    }
  }
  if (status == laserStatus::Ready) {
    _laser.hardwareSelfCheck();
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::ConnectionToControllerLost) {
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::NotConfigured) {
    // TODO do stuff with the led infront and show menu on OLED screen
  }
}

/**
  @brief This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function

  @param controllerIp the IP address of the controller to connect to
 */
void connectToController(String controllerIp) {
  char firstChar = controllerIp.charAt(0);
  if (firstChar == 255 || controllerIp.length() == 0) {
    setLaserStatus(laserStatus::NotConfigured);
    return;
  }

  unsigned int attempts = 0;
  while (!_client.connect(_server, 50000)) {  // keep trying to connect to controller
    attempts++;
    if (attempts > 10) {
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
bool checkIfArrayIsNotEmpty(byte array[]) {
  int emptyValueOccurrences = 0;
  const unsigned int lengthOfArray = sizeof(byte) / sizeof(array[0]);
  for (unsigned int i = 0; i < lengthOfArray; i++) {
    if (i == 255) {
      emptyValueOccurrences++;
    }
  }

  return emptyValueOccurrences != lengthOfArray;
}

/**
 @brief Checks the settings of the settings model and displays warnings on the bottom part of the menu
*/
void checkSettingsAndDisplayWarnings() {
  settingsModel settings = _settings.getSettings();
  bool controllerIpIsEmpty = !checkIfArrayIsNotEmpty(settings.controllerIp);
  if (controllerIpIsEmpty) {
    return;
  }

  bool maxLaserPowerIsLow = settings.maxPowerRgb < 20;
  if (maxLaserPowerIsLow) {
    return;
  }
}

void myCallback() {
  // TODO see if this function is needed
}

/**
 @brief configures the build in Teensy watchdog to monitor for software hicups
*/
void configureWatchdog() {
  WDT_timings_t config;
  config.timeout = 0.25; /* in seconds, 0->128 */
  config.callback = myCallback;
  _watchdog.begin(config);
}

bool emergencyButtonIsPressed() {
  return digitalRead(7) == 1;
}

void setLaserInEmergencyMode() {
  _laser.disableLasers();
}

void selectMode() {
  const unsigned short timePerFrameInMs = 1000 / _screenRefreshRate;
  if (millis() - _previousScreenUpdate > timePerFrameInMs) {
    _previousScreenUpdate = millis();
  }

  if (millis() - _previousSettingsCheck > 5000) {
    checkSettingsAndDisplayWarnings();
    _previousSettingsCheck = millis();
  }
}

void initializeMenus() {
  _menus[0] = new MainMenu();
}

void setup() {
  configureWatchdog();

  settingsModel laserSettings = _settings.getSettings();
  _laser.init(_watchdog, laserSettings);

  _oledModule.init();
  _watchdog.feed();
  _laser.hardwareSelfCheck();

  bool emergencyButtonIsNotConnected = digitalRead(7) == 1;
  while (emergencyButtonIsNotConnected) {
    // todo set protocol for this case
  }
  // TODO move to controller class
  // byte mac[6];
  // teensyMAC(mac);
  // Ethernet.begin(mac);

  initializeMenus();
  bool settingsValid = checkIfArrayIsNotEmpty(laserSettings.controllerIp);
  if (!settingsValid) {
    setLaserStatus(laserStatus::NotConfigured);
  }
}

void loop() {
  _watchdog.feed();

  if (_client.connected()) {
  } else {
    _laser.setLaserPower(0, 0, 0);
    _client.stop();
    setLaserStatus(laserStatus::ConnectionToControllerLost);
  }
}