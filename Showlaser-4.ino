#include "Laser.h"
#include <ArduinoJson.h>
#include <queue>
#include "Settings.h"
#include "OledModule.h"
#include "GlobalConfig.h"
#include "NetworkController.h"
#include "SDCard.h"
#include <vector>

#include "Modes/IMode.h"
#include "Modes/PlaySDFileMode.h"
#include "Modes/PlaySDFileMode.cpp"

#include "Menus/IMenu.h"
#include "Menus/MainMenu.h"
#include "Menus/MainMenu.cpp"
#include "Menus/ModeMenu.h"
#include "Menus/ModeMenu.cpp"
#include "Menus/ProjectionZoneMenu.h"
#include "Menus/ProjectionZoneMenu.cpp"
#include "Menus/StandaloneMenu.h"
#include "Menus/StandaloneMenu.cpp"
#include "Menus/SettingsMenu.h"
#include "Menus/SettingsMenu.cpp"
#include "Menus/SDCardMenu.h"
#include "Menus/SDCardMenu.cpp"
#include "Menus/PlaySDFileMenu.h"
#include "Menus/PlaySDFileMenu.cpp"

SDCard _sdCard;
WDT_T4<WDT1> _watchdog;
Laser _laser;
OledModule _oledModule;
NetworkController _networkController;

const int _modesLength = 1;
IMode* _modes[_modesLength];

LaserMode _previousSelectedLaserMode = LaserMode::NotSelected;

const int _menusLength = 6;
IMenu* _menus[_menusLength];

String _previousSelectedMenu = "";
String _currentSelectedMenu = MainMenuName;

unsigned long _previousScreenUpdate = 0;
unsigned short _screenRefreshRate = 24;

enum laserStatus {
  Defect = 0,                      // An hardware defect has been detected and the showlaser is locked due to safety reasons
  Ready = 1,                       // The showlaser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2,  // The showlaser has no connection to the controller
  NotConfigured = 3                // The showlaser is not yet configured
};

laserStatus _currentLaserStatus;

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
    _laser.testGalvoFeedback();
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
 @brief configures the build in Teensy watchdog to monitor for software hicups
*/
void configureWatchdog() {
  WDT_timings_t config;
  config.timeout = 1;  // in seconds, 0->128
  _watchdog.begin(config);
}

void initializeMenus() {
  _menus[0] = new MainMenu();
  _menus[1] = new ModeMenu();
  _menus[2] = new SettingsMenu();
  _menus[3] = new ProjectionZoneMenu();
  _menus[4] = new SDCardMenu();
  _menus[5] = new PlaySDFileMenu();
}

void initializeModes() {
  _modes[0] = new PlaySDFileMode(_laser);
}

bool emergencyButtonIsPressedOrDisconnected() {
  const int measureAttemptsCount = 10;
  int pressedOrDisconnectedCount = 0;

  for (int i = 0; i < measureAttemptsCount; i++) {
    if (digitalRead(7) == 1) {
      pressedOrDisconnectedCount++;
    }
    if (pressedOrDisconnectedCount > measureAttemptsCount) {
      return true;
    }
  }

  return false;
}

void executeEmergencyButtonProtocol() {
  if (emergencyButtonIsPressedOrDisconnected()) {
    _laser.disableLasers();
    _oledModule.clearDisplay();
    _oledModule.println(3, 3, "Emergency button pressed or disconnected! Restart required");
    _oledModule.displayChanges();

    while (true) {
      // prevent laser from executing commands
      _watchdog.feed();
    }
  }
}

int getSelectedMenuId() {
  for (int i = 0; i < _menusLength; i++) {
    if (_menus[i]->getMenuName() == _currentSelectedMenu) {
      return i;
    }
  }

  return 0;
}

void renderOledMenu() {
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

int getSelectedModeId() {
  for (int i = 0; i < _modesLength; i++) {
    if (_modes[i]->getModeName() == CurrentLaserMode) {
      return i;
    }
  }

  return -1;
}

void executeSelectedMode() {
  if (CurrentLaserMode != LaserMode::NotSelected) {
    int selectedModeId = getSelectedModeId();
    if (selectedModeId == -1) {
      _laser.setLaserPower(0, 0, 0);
      return;
    }

    _modes[selectedModeId]->execute();
    _previousSelectedLaserMode = CurrentLaserMode;
  } else if (CurrentLaserMode == LaserMode::NotSelected && _previousSelectedLaserMode != LaserMode::NotSelected) {
    _laser.setLaserPower(0, 0, 0);
    _previousSelectedLaserMode = CurrentLaserMode;
  }
}

void initLaser() {
  _laser.init(_watchdog);
  _oledModule.println(3, 15, "Init laser");
  _oledModule.displayChanges();

  _oledModule.println(3, 25, "Enable laser");
  _oledModule.displayChanges();
  _laser.sendTo(0, 0);
  _laser.enableLasers();
}

void initSDCard() {
  bool success = _sdCard.init();
  _oledModule.println(3, 35, success ? "SD init successfull" : "SD init failed");
  _oledModule.displayChanges();
}

void setup() {
  Serial.begin(9600);
  configureWatchdog();
  _oledModule.init();
  initLaser();
  _watchdog.feed();

  if (emergencyButtonIsPressedOrDisconnected()) {
    _oledModule.clearDisplay();
    _oledModule.println(3, 3, "Emergency button pressed or disconnected!");
    _oledModule.displayChanges();

    while (true) {
      _watchdog.feed();
    }
  }

  initSDCard();
  _sdCard.createJsonFile("{\"test\": \"[0]\"}", "test.json");
  initializeMenus();
  initializeModes();

  //_networkController.sendBroadcast();

  _oledModule.println(3, 45, "Laser init success!");
  _oledModule.displayChanges();

  const unsigned int millisToWait = 5000;
  unsigned int startMillis = millis();
  while (millis() < millisToWait + startMillis) {
    _watchdog.feed();  // Allows the user to see the init results
  }

  _menus[0]->displayMenu(_oledModule, _currentSelectedMenu, 0, false);  // render main menu on startup
}

void loop() {
  _watchdog.feed();
  renderOledMenu();

  executeEmergencyButtonProtocol();
  executeSelectedMode();
}