#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include "Arduino.h"

extern const String MainMenuName;
extern const String ModeSelectMenuName;
extern const String StandAloneMenuName;
extern const String ControllerMenuName;
extern const String SettingsMenuName;
extern const String ProjectionZoneMenuName;
extern const String ControllerIpMenuName;
extern const String ExitMenuName;

enum LaserMode {
  NotSelected = 0, // The showlaser will not go into a mode
  Standalone = 1,  // The showlaser is not connected to a controller and is working standalone
  Network = 2,     // The laser is ready to receive and process commands that are received
};

extern LaserMode CurrentLaserMode;

#endif