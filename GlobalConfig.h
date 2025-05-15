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
extern const String AudienceShutterMenuName;
extern const String ExitMenuName;

extern const String RandomDotsAnimationName;
extern const String LineAnimationName;
extern const String CircleAnimationName;
extern const String WideningLinesAnimationName;
extern const String RotatingPointsAnimationName;

  enum LaserMode {
    NotSelected = 0,  // The showlaser will not go into a mode
    Standalone = 1,   // The showlaser is not connected to a controller and is working standalone
    Network = 2,      // The laser is ready to receive and process commands that are received
  };

  enum ConnectionStatus {
    Connected = 0,
    ConnectionPending = 1,
    NotConnected = 2,
  };

extern LaserMode CurrentLaserMode;
extern ConnectionStatus CurrentConnectionStatus;

#endif