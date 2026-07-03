#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H
#include <ArduinoJson.h>
#include <vector>

#include "Arduino.h"

extern const String MainMenuName;
extern const String ModeSelectMenuName;
extern const String StandAloneMenuName;
extern const String ControllerMenuName;
extern const String SettingsMenuName;
extern const String ProjectionZoneMenuName;
extern const String ControllerIpMenuName;
extern const String SDCardMenuName;
extern const String PlaySDFileMenuName;
extern const String ExitMenuName;

enum LaserMode
{
  NotSelected = -1, // The showlaser will not go into a mode
  SDCardMode = 0,   // The laser is playing a file from the SD card
  Standalone = 1,   // The showlaser is not connected to a controller and is working standalone
  Network = 2,      // The laser is ready to receive and process commands that are received
};

extern LaserMode CurrentLaserMode;
extern String SelectedSDCardFilename;

// Live pattern/animation playback handoff: the /live-binary receiver streams the
// uploaded ".lzs" blob into LiveShowData and raises LiveShowPending; NetworkPlayMode
// moves the blob into its MemoryShow and starts looping it. Both run in the main
// loop (never the ISR), so this handoff needs no locking.
extern std::vector<uint8_t> LiveShowData;
extern volatile bool LiveShowPending;

// Playback timeline handoff (all main-loop context, no locking, like the live
// show handoff above): the playing mode keeps PlaybackPositionMs at the show
// time of the frame it is currently feeding, which the API exposes for the
// frontend's timeline. SeekRequestMs is set by the /seek endpoint (-1 = none)
// and consumed by the playing mode, which jumps to that show time.
extern volatile uint32_t PlaybackPositionMs;
extern volatile int32_t SeekRequestMs;

#endif