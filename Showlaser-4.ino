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
IMode *_modes[_modesLength];

LaserMode _previousSelectedLaserMode = LaserMode::NotSelected;

const int _menusLength = 6;
IMenu *_menus[_menusLength];

String _previousSelectedMenu = "";
String _currentSelectedMenu = MainMenuName;

unsigned long _previousScreenUpdate = 0;
unsigned short _screenRefreshRate = 24;

enum laserStatus
{
  Defect = 0,                     // An hardware defect has been detected and the showlaser is locked due to safety reasons
  Ready = 1,                      // The showlaser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2, // The showlaser has no connection to the controller
  NotConfigured = 3               // The showlaser is not yet configured
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
    while (true)
    { // keep in a infinite loop so the laser is not reachable
    }
  }
  if (status == laserStatus::Ready)
  {
    _laser.testGalvoFeedback();
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
 @brief configures the build in Teensy watchdog to monitor for software hicups
*/
void configureWatchdog()
{
  WDT_timings_t config;
  config.timeout = 1; // in seconds, 0->128
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
  _modes[0] = new PlaySDFileMode(_laser);
}

bool emergencyButtonIsPressedOrDisconnected()
{
  const int measureAttemptsCount = 10;
  int pressedOrDisconnectedCount = 0;

  for (int i = 0; i < measureAttemptsCount; i++)
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
    _previousSelectedLaserMode = selectedModeId;
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
  bool success = _sdCard.init();
  _oledModule.println(3, 35, success ? "SD init successfull" : "SD init failed");
  _oledModule.displayChanges();
}

void setup()
{
  Serial.begin(9600);
  configureWatchdog();
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

  //_networkController.sendBroadcast();

  _oledModule.println(3, 45, "Laser init success!");
  _oledModule.displayChanges();
  _sdCard.createJsonFile("{\"kpps\":40000,\"duration\":160,\"laserCommands\":[{\"timeMs\":0,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2000,\"y\":-1999},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1999,\"y\":2001},{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000}]]},{\"timeMs\":10,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":1663},{\"r\":54,\"g\":59,\"b\":206,\"x\":1663,\"y\":-2288},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2288,\"y\":-1662},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1662,\"y\":2289},{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":1663}]]},{\"timeMs\":20,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":1285},{\"r\":54,\"g\":59,\"b\":206,\"x\":1285,\"y\":-2520},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2520,\"y\":-1284},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1284,\"y\":2521},{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":1285}]]},{\"timeMs\":30,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":875},{\"r\":54,\"g\":59,\"b\":206,\"x\":875,\"y\":-2689},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2689,\"y\":-874},{\"r\":54,\"g\":59,\"b\":206,\"x\":-874,\"y\":2690},{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":875}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":0,\"y\":2000},{\"r\":193,\"g\":21,\"b\":21,\"x\":2000,\"y\":-2000},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2000,\"y\":-1999},{\"r\":193,\"g\":21,\"b\":21,\"x\":0,\"y\":2000}]]},{\"timeMs\":40,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2000,\"y\":-1999},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1999,\"y\":2001},{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":443},{\"r\":54,\"g\":59,\"b\":206,\"x\":443,\"y\":-2793},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2793,\"y\":-442},{\"r\":54,\"g\":59,\"b\":206,\"x\":-442,\"y\":2794},{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":443}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-554,\"y\":1942},{\"r\":193,\"g\":21,\"b\":21,\"x\":2234,\"y\":-1555},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1679,\"y\":-2386},{\"r\":193,\"g\":21,\"b\":21,\"x\":-554,\"y\":1942}]]},{\"timeMs\":50,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":1663},{\"r\":54,\"g\":59,\"b\":206,\"x\":1663,\"y\":-2288},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2288,\"y\":-1662},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1662,\"y\":2289},{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":1663}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2829,\"y\":0},{\"r\":54,\"g\":59,\"b\":206,\"x\":0,\"y\":-2828},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2828,\"y\":0},{\"r\":54,\"g\":59,\"b\":206,\"x\":0,\"y\":2829},{\"r\":54,\"g\":59,\"b\":206,\"x\":2829,\"y\":0}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1084,\"y\":1770},{\"r\":193,\"g\":21,\"b\":21,\"x\":2370,\"y\":-1071},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1284,\"y\":-2698},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1084,\"y\":1770}]]},{\"timeMs\":60,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":1285},{\"r\":54,\"g\":59,\"b\":206,\"x\":1285,\"y\":-2520},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2520,\"y\":-1284},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1284,\"y\":2521},{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":1285}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":-442},{\"r\":54,\"g\":59,\"b\":206,\"x\":-442,\"y\":-2793},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2793,\"y\":443},{\"r\":54,\"g\":59,\"b\":206,\"x\":443,\"y\":2794},{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":-442}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1567,\"y\":1491},{\"r\":193,\"g\":21,\"b\":21,\"x\":2402,\"y\":-569},{\"r\":193,\"g\":21,\"b\":21,\"x\":-834,\"y\":-2920},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1567,\"y\":1491}]]},{\"timeMs\":70,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":875},{\"r\":54,\"g\":59,\"b\":206,\"x\":875,\"y\":-2689},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2689,\"y\":-874},{\"r\":54,\"g\":59,\"b\":206,\"x\":-874,\"y\":2690},{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":875}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":0,\"y\":2000},{\"r\":193,\"g\":21,\"b\":21,\"x\":2000,\"y\":-2000},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2000,\"y\":-1999},{\"r\":193,\"g\":21,\"b\":21,\"x\":0,\"y\":2000}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":-874},{\"r\":54,\"g\":59,\"b\":206,\"x\":-874,\"y\":-2689},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2689,\"y\":875},{\"r\":54,\"g\":59,\"b\":206,\"x\":875,\"y\":2690},{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":-874}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1981,\"y\":1118},{\"r\":193,\"g\":21,\"b\":21,\"x\":2330,\"y\":-72},{\"r\":193,\"g\":21,\"b\":21,\"x\":-347,\"y\":-3045},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1981,\"y\":1118}]]},{\"timeMs\":80,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":443},{\"r\":54,\"g\":59,\"b\":206,\"x\":443,\"y\":-2793},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2793,\"y\":-442},{\"r\":54,\"g\":59,\"b\":206,\"x\":-442,\"y\":2794},{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":443}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-554,\"y\":1942},{\"r\":193,\"g\":21,\"b\":21,\"x\":2234,\"y\":-1555},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1679,\"y\":-2386},{\"r\":193,\"g\":21,\"b\":21,\"x\":-554,\"y\":1942}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":-1284},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1284,\"y\":-2520},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2520,\"y\":1285},{\"r\":54,\"g\":59,\"b\":206,\"x\":1285,\"y\":2521},{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":-1284}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":667},{\"r\":193,\"g\":21,\"b\":21,\"x\":2155,\"y\":399},{\"r\":193,\"g\":21,\"b\":21,\"x\":155,\"y\":-3065},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":667}]]},{\"timeMs\":90,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2829,\"y\":0},{\"r\":54,\"g\":59,\"b\":206,\"x\":0,\"y\":-2828},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2828,\"y\":0},{\"r\":54,\"g\":59,\"b\":206,\"x\":0,\"y\":2829},{\"r\":54,\"g\":59,\"b\":206,\"x\":2829,\"y\":0}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1084,\"y\":1770},{\"r\":193,\"g\":21,\"b\":21,\"x\":2370,\"y\":-1071},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1284,\"y\":-2698},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1084,\"y\":1770}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":-1662},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1662,\"y\":-2288},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2288,\"y\":1663},{\"r\":54,\"g\":59,\"b\":206,\"x\":1663,\"y\":2289},{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":-1662}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":158},{\"r\":193,\"g\":21,\"b\":21,\"x\":1887,\"y\":824},{\"r\":193,\"g\":21,\"b\":21,\"x\":651,\"y\":-2980},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":158}]]},{\"timeMs\":100,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":-442},{\"r\":54,\"g\":59,\"b\":206,\"x\":-442,\"y\":-2793},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2793,\"y\":443},{\"r\":54,\"g\":59,\"b\":206,\"x\":443,\"y\":2794},{\"r\":54,\"g\":59,\"b\":206,\"x\":2794,\"y\":-442}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1567,\"y\":1491},{\"r\":193,\"g\":21,\"b\":21,\"x\":2402,\"y\":-569},{\"r\":193,\"g\":21,\"b\":21,\"x\":-834,\"y\":-2920},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1567,\"y\":1491}],[{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2000,\"y\":-1999},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1999,\"y\":2001},{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-387},{\"r\":193,\"g\":21,\"b\":21,\"x\":1536,\"y\":1184},{\"r\":193,\"g\":21,\"b\":21,\"x\":1117,\"y\":-2795},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-387}]]},{\"timeMs\":110,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":-874},{\"r\":54,\"g\":59,\"b\":206,\"x\":-874,\"y\":-2689},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2689,\"y\":875},{\"r\":54,\"g\":59,\"b\":206,\"x\":875,\"y\":2690},{\"r\":54,\"g\":59,\"b\":206,\"x\":2690,\"y\":-874}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-1981,\"y\":1118},{\"r\":193,\"g\":21,\"b\":21,\"x\":2330,\"y\":-72},{\"r\":193,\"g\":21,\"b\":21,\"x\":-347,\"y\":-3045},{\"r\":193,\"g\":21,\"b\":21,\"x\":-1981,\"y\":1118}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-945},{\"r\":193,\"g\":21,\"b\":21,\"x\":1117,\"y\":1462},{\"r\":193,\"g\":21,\"b\":21,\"x\":1536,\"y\":-2516},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-945}]]},{\"timeMs\":120,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":-1284},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1284,\"y\":-2520},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2520,\"y\":1285},{\"r\":54,\"g\":59,\"b\":206,\"x\":1285,\"y\":2521},{\"r\":54,\"g\":59,\"b\":206,\"x\":2521,\"y\":-1284}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":667},{\"r\":193,\"g\":21,\"b\":21,\"x\":2155,\"y\":399},{\"r\":193,\"g\":21,\"b\":21,\"x\":155,\"y\":-3065},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":667}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":-1490},{\"r\":193,\"g\":21,\"b\":21,\"x\":651,\"y\":1648},{\"r\":193,\"g\":21,\"b\":21,\"x\":1887,\"y\":-2156},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":-1490}]]},{\"timeMs\":130,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":-1662},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1662,\"y\":-2288},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2288,\"y\":1663},{\"r\":54,\"g\":59,\"b\":206,\"x\":1663,\"y\":2289},{\"r\":54,\"g\":59,\"b\":206,\"x\":2289,\"y\":-1662}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":158},{\"r\":193,\"g\":21,\"b\":21,\"x\":1887,\"y\":824},{\"r\":193,\"g\":21,\"b\":21,\"x\":651,\"y\":-2980},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":158}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":-1999},{\"r\":193,\"g\":21,\"b\":21,\"x\":155,\"y\":1733},{\"r\":193,\"g\":21,\"b\":21,\"x\":2155,\"y\":-1732},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2309,\"y\":-1999}]]},{\"timeMs\":140,\"commands\":[[{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":-2000,\"y\":-1999},{\"r\":54,\"g\":59,\"b\":206,\"x\":-1999,\"y\":2001},{\"r\":54,\"g\":59,\"b\":206,\"x\":2001,\"y\":2000},{\"r\":54,\"g\":59,\"b\":206,\"x\":2000,\"y\":-2000}],[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-387},{\"r\":193,\"g\":21,\"b\":21,\"x\":1536,\"y\":1184},{\"r\":193,\"g\":21,\"b\":21,\"x\":1117,\"y\":-2795},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-387}]]},{\"timeMs\":150,\"commands\":[[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-945},{\"r\":193,\"g\":21,\"b\":21,\"x\":1117,\"y\":1462},{\"r\":193,\"g\":21,\"b\":21,\"x\":1536,\"y\":-2516},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2652,\"y\":-945}]]},{\"timeMs\":160,\"commands\":[[{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":-1490},{\"r\":193,\"g\":21,\"b\":21,\"x\":651,\"y\":1648},{\"r\":193,\"g\":21,\"b\":21,\"x\":1887,\"y\":-2156},{\"r\":193,\"g\":21,\"b\":21,\"x\":-2536,\"y\":-1490}]]}]}", "final.json");

  const unsigned int millisToWait = 5000;
  unsigned int startMillis = millis();
  while (millis() < millisToWait + startMillis)
  {
    _watchdog.feed(); // Allows the user to see the init results
  }

  _menus[0]->displayMenu(_oledModule, _currentSelectedMenu, 0, false); // render main menu on startup
}

void loop()
{
  _watchdog.feed();
  renderOledMenu();

  executeEmergencyButtonProtocol();
  executeSelectedMode();
}