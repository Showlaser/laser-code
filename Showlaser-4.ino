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
#include "Modes/ShowPlayerMode.h"
#include "Modes/ShowPlayerMode.cpp"
#include "Modes/PlaySDFileMode.h"
#include "Modes/PlaySDFileMode.cpp"
#include "Modes/NetworkPlayMode.h"
#include "Modes/NetworkPlayMode.cpp"

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

const int _modesLength = 2;
IMode *_modes[_modesLength];

// Index into _modes of the mode that ran last iteration, or -1 when none was
// running. This is an ARRAY INDEX, not a LaserMode value: a mode's position in
// _modes is unrelated to its enum value (NetworkPlayMode is _modes[1] but
// reports LaserMode::Network == 2).
int _previousModeId = -1;

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
  _modes[1] = new NetworkPlayMode(_laser, _player);
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
      // A mode with no backing object (e.g. Standalone): make sure whatever ran
      // before is fully stopped -- point clock halted, sources released --
      // instead of lingering while we blank.
      if (_previousModeId != -1)
      {
        _modes[_previousModeId]->stop();
        _previousModeId = -1;
      }
      _laser.setLaserPower(0, 0, 0);
      return;
    }

    // Switched directly from one active mode to another (e.g. a live upload
    // preempts an SD show): stop the previous one so it releases its source and
    // resets, rather than resuming stale state later.
    if (_previousModeId != -1 && _previousModeId != selectedModeId)
    {
      _modes[_previousModeId]->stop();
    }

    _modes[selectedModeId]->execute();
    _previousModeId = selectedModeId;
  }
  else if (_previousModeId != -1)
  {
    // Stop the point clock BEFORE commanding the blank. The ISR shares the DAC
    // driver objects with this code; a tick landing between our setVoltage and
    // updateDAC can interleave lit channel values into the blank write, leaving
    // a static lit point. stop() halts the clock (and blanks); the final blank
    // below then runs with no ISR to race against.
    _modes[_previousModeId]->stop();
    _previousModeId = -1;
    _laser.setLaserPower(0, 0, 0);
  }
  else
  {
    // Idle (nothing playing, nothing to stop): re-assert the blanked state
    // about once per second as defense in depth. If anything ever leaves a
    // colour DAC non-zero -- a glitched SPI transfer, a missed stop path -- it
    // is extinguished within a second instead of parking a static beam. The
    // point clock is not running here, so this write races nothing.
    static unsigned long lastIdleBlank = 0;
    if (millis() - lastIdleBlank >= 1000)
    {
      lastIdleBlank = millis();
      _laser.setLaserPower(0, 0, 0);
    }
  }
}

void initLaser()
{
  // _laser.init() itself runs as the very first line of setup() (it blanks the
  // DACs, which must never wait on the OLED/SD/network); this only does the
  // user-visible part: center the galvos and arm the output.
  _oledModule.println(3, 15, "Init laser");
  _oledModule.displayChanges();

  _oledModule.println(3, 25, "Enable laser");
  _oledModule.displayChanges();
  _laser.writeGalvoRaw(0, 0);
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
  // FIRST: arm the watchdog and zero every DAC output. The MCP4822 DACs HOLD
  // their last value across a CPU reset, so after a crash or watchdog reboot
  // they may still be commanding the last lit point of a show as a static
  // full-power beam. Nothing that can hang (OLED/I2C, SD, network, EEPROM) is
  // allowed to run before this blank.
  configureWatchdog();
  _laser.init(_watchdog); // configureDacs() writes 0 to all colour DACs
  _player.begin(_laser);

  Serial.begin(9600);
  if (CrashReport)
  {
    // The previous run ended in a fault -- the way a show freezes into a static
    // beam. Print the fault details so the incident is diagnosable.
    Serial.print(CrashReport);
  }

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

  _oledModule.init();
  initLaser();
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