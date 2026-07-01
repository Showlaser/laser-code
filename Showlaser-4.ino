#include "Laser.h"
#include <ArduinoJson.h>
#include <queue>
#include "Settings.h"
#include "OledModule.h"
#include "GlobalConfig.h"
#include "NetworkController.h"
#include "SDCard.h"
#include "RealtimePlayer.h"
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
RealtimePlayer _player;
OledModule _oledModule;
NetworkController _networkController;

const int _modesLength = 1;
IMode *_modes[_modesLength];

LaserMode _previousSelectedLaserMode = LaserMode::NotSelected;

const int _menusLength = 6;
IMenu *_menus[_menusLength];

String _previousSelectedMenu = "";
String _currentSelectedMenu = MainMenuName;

unsigned long _previousScreenUpdate = 0;
unsigned long _previousBroadcast = 0;
const unsigned long _broadcastIntervalMs = 30000; // 30 seconds

unsigned long _previousAliveCheck = 0;
unsigned long _aliveCheckIntervalMs = 30000;

unsigned short _screenRefreshRate = 24;

enum laserStatus
{
  Emitting = 0,
  Standby = 1,
  EmergencyButtonPressed = 2,
  PendingConnection = 3,
  ConnectionLost = 4,
  Defect = 5,
  NotConfigured = 6
};

laserStatus _currentLaserStatus;

/**
 @brief Sets the status of the laser and performs certain actions based on the status

 @param status the status of the laser
 */
void setLaserStatus(laserStatus status)
{
  _currentLaserStatus = status;

  if (status == laserStatus::Defect)
  {
    _player.stop(); // halt the realtime point clock before locking up
    _laser.disableLasers();
    while (true)
    { // keep in a infinite loop so the laser is not reachable
    }
  }
  if (status == laserStatus::Standby)
  {
    _laser.testGalvoFeedback();
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::ConnectionLost)
  {
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::NotConfigured)
  {
    // TODO do stuff with the led infront and show menu on OLED screen
  }
}

/**
 @brief configures the build in Teensy watchdog to monitor for software hicups
*/
void configureWatchdog()
{
  WDT_timings_t config;
  config.timeout = 10; // in seconds, 0->128
  _watchdog.begin(config);
}

void initializeMenus()
{
  _menus[0] = new MainMenu();
  _menus[1] = new ModeMenu();
  _menus[2] = new SettingsMenu();
  _menus[3] = new ProjectionZoneMenu();
  _menus[4] = new SDCardMenu();
  _menus[5] = new PlaySDFileMenu();
}

void initializeModes()
{
  _modes[0] = new PlaySDFileMode(_laser, _player);
}

bool emergencyButtonIsPressedOrDisconnected()
{
  const int measureAttemptsCount = 3;
  int pressedOrDisconnectedCount = 0;

  for (int i = 0; i < measureAttemptsCount + 1; i++)
  {
    if (digitalRead(7) == 1)
    {
      pressedOrDisconnectedCount++;
    }
    if (pressedOrDisconnectedCount > measureAttemptsCount)
    {
      return true;
    }
  }

  return false;
}

void executeEmergencyButtonProtocol()
{
  if (emergencyButtonIsPressedOrDisconnected())
  {
    Serial.println("Emergency button pressed!");
    _player.stop(); // halt the realtime point clock so no further DAC output
    _laser.disableLasers();
    _oledModule.clearDisplay();
    _oledModule.println(3, 3, "Emergency button pressed or disconnected! Restart required");
    _oledModule.displayChanges();

    while (true)
    {
      // prevent laser from executing commands
      _watchdog.feed();
    }
  }
}

int getSelectedMenuId()
{
  for (int i = 0; i < _menusLength; i++)
  {
    if (_menus[i]->getMenuName() == _currentSelectedMenu)
    {
      return i;
    }
  }

  return 0;
}

void renderOledMenu()
{
  const unsigned short timePerFrameInMs = 1000 / _screenRefreshRate;
  if (millis() - _previousScreenUpdate > timePerFrameInMs)
  {
    int selectedMenuId = getSelectedMenuId();

    int previousReading = 0;
    int currentReading = 0;
    bool rotaryButtonPressed = _oledModule.checkForButtonPress();
    _oledModule.getRotaryEncoderRotation(previousReading, currentReading);

    if (rotaryButtonPressed || currentReading != previousReading || _previousSelectedMenu != _currentSelectedMenu)
    {
      _oledModule.clearDisplay();
      _menus[selectedMenuId]->displayMenu(_oledModule, _currentSelectedMenu, currentReading, rotaryButtonPressed);
    }

    if (_previousSelectedMenu != _currentSelectedMenu)
    {
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

int getSelectedModeId()
{
  for (int i = 0; i < _modesLength; i++)
  {
    if (_modes[i]->getModeName() == CurrentLaserMode)
    {
      return i;
    }
  }

  return -1;
}

void executeSelectedMode()
{
  if (CurrentLaserMode != LaserMode::NotSelected)
  {
    int selectedModeId = getSelectedModeId();
    if (selectedModeId == -1)
    {
      _laser.setLaserPower(0, 0, 0);
      return;
    }

    _modes[selectedModeId]->execute();
    _previousSelectedLaserMode = (LaserMode)selectedModeId;
  }
  else if (CurrentLaserMode == LaserMode::NotSelected && _previousSelectedLaserMode != LaserMode::NotSelected)
  {
    _laser.setLaserPower(0, 0, 0);
    _modes[_previousSelectedLaserMode]
        ->stop();
    _previousSelectedLaserMode = LaserMode::NotSelected;
  }
}

void initLaser()
{
  _laser.init(_watchdog);
  _oledModule.println(3, 15, "Init laser");
  _oledModule.displayChanges();

  _oledModule.println(3, 25, "Enable laser");
  _oledModule.displayChanges();
  _laser.sendTo(0, 0);
  _laser.enableLasers();
}

void initSDCard()
{
  _oledModule.clearDisplay();

  bool success = _sdCard.init();
  _oledModule.println(3, 35, success ? "SD init successfull" : "SD init failed");
  _oledModule.displayChanges();
}

String connectionStatusToString(ConnectionStatus status)
{
  switch (status)
  {
  case ConnectionStatus::Connected:
    return "Connected";
  case ConnectionStatus::ConnectionPending:
    return "ConnectionPending";
  case ConnectionStatus::NotConnected:
    return "NotConnected";
  case ConnectionStatus::NoNetworkCableConnected:
    return "NoNetworkCableConnected";
  default:
    return "Unknown";
  }
}

void initNetworkController()
{
  // Show feedback before init(): Ethernet.begin() can block for a few seconds
  // during DHCP, and without this the OLED would still read "SD init
  // successfull", making the wait look like a hang.
  _oledModule.clearDisplay();
  _oledModule.println(3, 5, "Connecting to network");
  _oledModule.displayChanges();

  _networkController.init(_watchdog, _sdCard, _laser);
  settingsModel settings = Settings::getSettings();
  _oledModule.println(3, 20, "Saved Controller IP: " + String(settings.controllerIp[0]) + "." + String(settings.controllerIp[1]) + "." + String(settings.controllerIp[2]) + "." + String(settings.controllerIp[3]));
  _oledModule.displayChanges();

  _oledModule.clearDisplay();
  _oledModule.displayChanges();
  delay(1000);

  _oledModule.println(3, 5, "Sending broadcast");
  _oledModule.displayChanges();

  _networkController.sendBroadcast();
  if (settings.controllerIp[0] != 0 || settings.controllerIp[1] != 0 || settings.controllerIp[2] != 0 || settings.controllerIp[3] != 0)
  {
    _networkController.connectToController(settings.controllerIp);
    Serial.println("Connection status: " + connectionStatusToString(_networkController.getConnectionStatus()));
    if (_networkController.getConnectionStatus() == ConnectionStatus::Connected)
    {
      _oledModule.println(3, 25, "Connected to controller using IP: " + String(settings.controllerIp[0]) + "." + String(settings.controllerIp[1]) + "." + String(settings.controllerIp[2]) + "." + String(settings.controllerIp[3]));
    }
    else
    {
      _oledModule.println(3, 25, "Could not connected to controller");
    }
  }

  _oledModule.displayChanges();
  delay(1000);
}

void setup()
{
  settingsModel settings = Settings::getSettings();
  if (settings.uuid[0] == '\0')
  {
    strncpy(settings.uuid, _networkController.generateUuid().c_str(), sizeof(settings.uuid) - 1);
    settings.uuid[sizeof(settings.uuid) - 1] = '\0';
    bool settingsStored = Settings::setSettings(settings);
    if (!settingsStored)
    {
      Serial.println("Settings not stored");
    }

    Settings::saveSettings();
    _networkController.sendSettingsToController(settings);
  }

  Serial.begin(9600);

  configureWatchdog();
  _oledModule.init();
  initLaser();
  _player.begin(_laser);
  _watchdog.feed();

  if (emergencyButtonIsPressedOrDisconnected())
  {
    _oledModule.clearDisplay();
    _oledModule.println(3, 3, "Emergency button pressed or disconnected!");
    _oledModule.displayChanges();

    while (true)
    {
      _watchdog.feed();
    }
  }

  initSDCard();
  initializeMenus();
  initializeModes();
  initNetworkController();

  Serial.println("Setup started");
  Serial.println("UUID: " + String(settings.uuid));
  Serial.println("ModelType: " + String(settings.modelType));
  Serial.println("name: " + String(settings.name));
  Serial.println("Controller IP: " + String(settings.controllerIp[0]) + "." + String(settings.controllerIp[1]) + "." + String(settings.controllerIp[2]) + "." + String(settings.controllerIp[3]));
  Serial.println("ConnectionStatus: " + String(settings.connectionStatus));
  Serial.println("Assigned IP: " + _networkController.getAssignedIP());

  _menus[0]
      ->displayMenu(_oledModule, _currentSelectedMenu, 0, false); // render main menu on startup
}

void loop()
{
  _watchdog.feed();
  renderOledMenu();

  executeEmergencyButtonProtocol();
  executeSelectedMode();
  _networkController.listenToApiCalls();

  if (millis() - _previousAliveCheck > _aliveCheckIntervalMs)
  {
    _networkController.laserControllerAliveCheck();
    _previousAliveCheck = millis();
  }

  ConnectionStatus connectionStatus = _networkController.getConnectionStatus();
  if (connectionStatus == ConnectionStatus::NotConnected &&
      connectionStatus != ConnectionStatus::NoNetworkCableConnected &&
      millis() - _previousBroadcast > _broadcastIntervalMs)
  {
    _networkController.sendBroadcast();
    _previousBroadcast = millis();
  }
}