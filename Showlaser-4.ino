#include <NativeEthernet.h>
#include "Laser.h"
#include <ArduinoJson.h>
#include <queue>
#include "Settings.h"
#include "OledModule.h"
#include "StandaloneMode.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "ModeMenu.h"
#include "StandaloneMenu.h"
#include "SettingsMenu.h"
#include "ProjectionZoneMenu.h"
#include "AudienceShutter.h"
#include "GlobalConfig.h"
WDT_T4<WDT1> _watchdog;
Laser _laser;
OledModule _oledModule;

EthernetClient _client;
IPAddress _server;

const int _menusLength = 5;
IMenu* _menus[_menusLength];

String _previousSelectedMenu = "";           // The previous selected menu name
String _currentSelectedMenu = MainMenuName;  // The current selected menu name

unsigned long _previousScreenUpdate = 0;
unsigned short _screenRefreshRate = 24;  // Fps

unsigned long _previousSettingsCheck = 0;

enum laserStatus {
  Defect = 0,                      // An hardware defect has been detected and the showlaser is locked due to safety reasons
  Ready = 1,                       // The showlaser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2,  // The showlaser has no connection to the controller
  NotConfigured = 3                // The showlaser is not yet configured
};

laserStatus _currentLaserStatus;

/**
  @brief Generate a MAC address for the Teensy

  @param mac the variable too write the mac address to
 */
/*void teensyMAC(uint8_t* mac) {
  for (uint8_t by = 0; by < 2; by++) {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++) {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}*/

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
/*void connectToController(String controllerIp) {
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
}*/

/**
 @brief configures the build in Teensy watchdog to monitor for software hicups
*/
void configureWatchdog() {
  WDT_timings_t config;
  config.timeout = 1;  // in seconds, 0->128
  _watchdog.begin(config);
}

bool emergencyButtonIsPressed() {
  return digitalRead(7) == 1;
}

void setLaserInEmergencyMode() {
  _laser.disableLasers();
}

int getSelectedMenuId() {
  for (int i = 0; i < _menusLength; i++) {
    if (_menus[i]->getMenuName() == _currentSelectedMenu) {
      return i;
    }
  }

  return 0;
}

void selectMode() {
  const unsigned short timePerFrameInMs = 1000 / _screenRefreshRate;
  if (millis() - _previousScreenUpdate > timePerFrameInMs) {
    int selectedMenuId = getSelectedMenuId();

    int previousReading = 0;
    int currentReading = 0;
    bool rotaryButtonPressed = _oledModule.checkForButtonPress();
    _oledModule.getRotaryEncoderRotation(previousReading, currentReading);

    if (rotaryButtonPressed || currentReading != previousReading || _previousSelectedMenu != _currentSelectedMenu) {
      _oledModule.clearDisplay();
      _menus[selectedMenuId]->displayMenu(_oledModule, _currentSelectedMenu, currentReading, rotaryButtonPressed);
    }

    if (_previousSelectedMenu != _currentSelectedMenu) {
      _previousSelectedMenu = _currentSelectedMenu;
      _oledModule.resetRotaryValue();
      _oledModule.clearDisplay();

      rotaryButtonPressed = _oledModule.checkForButtonPress();
      _oledModule.getRotaryEncoderRotation(previousReading, currentReading);

      selectedMenuId = getSelectedMenuId();
      _menus[selectedMenuId]->displayMenu(_oledModule, _currentSelectedMenu, currentReading, rotaryButtonPressed);
    }

    _previousScreenUpdate = millis();
  }
}

void initializeMenus() {
  _menus[0] = new MainMenu();
  _menus[1] = new ModeMenu();
  _menus[2] = new StandaloneMenu();
  _menus[3] = new SettingsMenu();
  _menus[4] = new ProjectionZoneMenu();
}

void setup() {
  Serial.begin(9600);
  configureWatchdog();

  //_laser.init(_watchdog);

  _oledModule.init();
  _watchdog.feed();
  //_laser.hardwareSelfCheck();

  //bool emergencyButtonIsNotConnected = digitalRead(7) == 1;
  //while (emergencyButtonIsNotConnected) {
  // todo set protocol for this case
  // }
  // TODO move to controller class
  // byte mac[6];
  // teensyMAC(mac);
  // Ethernet.begin(mac);
  initializeMenus();

  _menus[0]->displayMenu(_oledModule, _currentSelectedMenu, 0, false);  // render main menu on startup
}

void loop() {
  _watchdog.feed();
  selectMode();

  /*if (_client.connected()) {
  } else {
    _laser.setLaserPower(0, 0, 0);
    _client.stop();
    setLaserStatus(laserStatus::ConnectionToControllerLost);
  }*/
}